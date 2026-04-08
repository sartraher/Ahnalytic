#include "UpdateManager.hpp"

#include "AhnalyticBase/database/FileDatabase.hpp"
#include "AhnalyticBase/database/StackExchangeExtractDatabase.hpp"
#include "AhnalyticBase/helper/ArchiveHelper.hpp"
#include "AhnalyticBase/helper/Enviroment.hpp"
#include "AhnalyticBase/helper/SignHelper.hpp"
#include "AhnalyticBase/stackexchange/StackOverflow.hpp"

#include "BS_thread_pool.hpp"

#define CPPHTTPLIB_OPENSSL_SUPPORT
#define CPPHTTPLIB_ZLIB_SUPPORT
#include <httplib.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <ankerl/unordered_dense.h>

#include <fstream>

UpdateManager::UpdateManager(EnviromentC* enviroment)
{
  env = enviroment;

  std::string dbData = (env->dbFolder / "status.json").string();
  installedUpdates.read(dbData);
}

UpdateManager::~UpdateManager()
{
}

bool UpdateManager::downloadFile(httplib::Client* cli, const std::string& path, const std::filesystem::path& outPath)
{
  bool ret = false;
  auto res = cli->Get(path);
  if (res && res->status == 200)
  {
    std::filesystem::create_directories(outPath.parent_path());
    std::ofstream out(outPath.string(), std::ios::binary);
    out.write(res->body.data(), res->body.size());
    out.close();

    ret = true;
  }

  return ret;
}

ahn::vector<UpdateInfo> UpdateManager::checkUpdates() const
{
  ahn::vector<UpdateInfo> ret;

  httplib::Client cli("http://www.ahnalytic.org");
  httplib::Result res = cli.Get("/data/db/status.json");

  if (res && res->status == 200)
  {
    UpdateStatusFile updateFile;
    updateFile.readBuffer(res->body);

    ret.reserve(updateFile.infos.size());
    for (const UpdateInfo& info : updateFile.infos)
      if (std::find(installedUpdates.infos.begin(), installedUpdates.infos.end(), info) == installedUpdates.infos.end())
        ret.push_back(info);
  }

  return ret;
}

void UpdateManager::startUpdates(const ahn::vector<std::string>& filter)
{
  ahn::vector<UpdateRepoData> repoDatas = getUpdateRepoData(filter);

  BS::thread_pool pool;

  ThreadSafeQueue<UpdateRepoData> finishedQueue;

  for (const UpdateRepoData& repoData : repoDatas)
  {
    pool.detach_task([&finishedQueue, repoData, this]()
    {
      bool ret = false;

      std::string lastSha = "";
      auto iter =
          std::find_if(installedUpdates.infos.begin(), installedUpdates.infos.end(), [repoData](const UpdateInfo& info) { return info.name == repoData.name; });

      if (iter != installedUpdates.infos.end())
        lastSha = iter->sha;

      if (repoData.type == "github")
        ;
      //ret = updateGitHub(repoData, lastSha);
      else if (repoData.type == "stackexchange")
        ret = updateStackExchange(repoData, lastSha);
      else if (repoData.type == "sourceforge")
        ret = updateSourceForge(repoData, lastSha);

      if (ret)
        finishedQueue.push(repoData);
    });
  }

  while (pool.get_tasks_total() > 0 || finishedQueue.size() > 0)
  {
    if (finishedQueue.size() == 0)
      std::this_thread::sleep_for(std::chrono::seconds{1});
    else
    {
      UpdateRepoData repoData = finishedQueue.wait_and_pop();

      // Check if we need to update or add;
      auto iter =
          std::find_if(installedUpdates.infos.begin(), installedUpdates.infos.end(), [repoData](const UpdateInfo& info) { return info.name == repoData.name; });

      if (iter == installedUpdates.infos.end())
        iter->sha = repoData.tags[repoData.tags.size() - 1].sha;
      else
      {
        UpdateInfo info;
        info.name = repoData.name;
        info.sha = repoData.tags[repoData.tags.size() - 1].sha;
        info.type = repoData.type;
        info.language = repoData.language;
        info.version = repoData.version;
        installedUpdates.infos.push_back(info);
      }
    }
  }
}

bool UpdateManager::updateGitHub(const UpdateRepoData& repoData, const std::string& lastSha)
{
  httplib::Client cli("http://www.ahnalytic.org");

  FileDatabase db(DBType::SQLite, (env->dbFolder / "CPP" / "github" / (repoData.name + ".db")).string());

  if (lastSha == "")
    db.createRepoData(repoData.name, repoData.url, repoData.licence);

  std::error_code ec;
  std::filesystem::path pathesPath = env->workFolder / "db" / "CPP" / repoData.name / "pathes.dat";
  std::filesystem::create_directories(pathesPath.parent_path());
  downloadFile(&cli, "/data/db/CPP/github/" + repoData.name + "/pathes.dat", pathesPath);

  if (!std::filesystem::exists(pathesPath))
    return false;

  db.importPathesData(pathesPath);

  ahn::set<uint32_t> hashes;
  std::filesystem::path filePath = (env->dbFolder / "CPP" / "github" / (repoData.name + "." + std::to_string(env->windowSize))).string();

  if (std::filesystem ::exists(filePath))
  {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    ahn::vector<uint32_t> data(size / sizeof(uint32_t));

    if (file.read(reinterpret_cast<char*>(data.data()), size))
    {
      hashes.reserve(data.size());
      hashes.insert(data.begin(), data.end());
    }
  }

  // We ned this one since some sha are tagged under different names
  std::vector<std::string> doneList;

  bool shaFound = false;
  for (const UpdateRepoTagData& tag : repoData.tags)
  {
    if (!shaFound)
    {
      if (tag.sha == lastSha || lastSha == "")
        shaFound = true;
    }
    else
    {
      std::filesystem::path tarPath = env->workFolder / "db" / "CPP" / repoData.name / (tag.sha + ".tar");
      std::filesystem::path sigPath = env->workFolder / "db" / "CPP" / repoData.name / (tag.sha + ".tar.sig");

      downloadFile(&cli, "/data/db/CPP/github/" + repoData.name + "/" + tag.sha + ".tar", tarPath);
      downloadFile(&cli, "/data/db/CPP/github/" + repoData.name + "/" + tag.sha + ".tar.sig", sigPath);

      if (SignHelper::verifyFile(tarPath.string(), env->publicPath.string(), sigPath.string()))
      {
        db.importData(tag.tagName, tag.sha, tarPath, pathesPath, std::find(doneList.begin(), doneList.end(), tag.sha) != doneList.end(), *env, hashes);

        std::filesystem::remove_all(tarPath, ec);
        std::filesystem::remove_all(sigPath, ec);
      }
    }

    doneList.push_back(tag.sha);
  }

  ahn::vector<uint32_t> vec;
  vec.reserve(hashes.size());
  vec.insert(vec.end(), hashes.begin(), hashes.end());

  std::ofstream fileOut(filePath.native(), std::ios::binary);
  fileOut.write((char*)vec.data(), static_cast<std::streamsize>(vec.size() * sizeof(uint32_t)));

  std::filesystem::remove_all(pathesPath, ec);
  std::filesystem::remove_all(pathesPath.parent_path(), ec);

  return false;
}

bool UpdateManager::updateStackExchange(const UpdateRepoData& repoData, const std::string& lastSha)
{
  httplib::Client cli("http://www.ahnalytic.org");

  StackOverflowHandler handler;
  StackExchangeExtractDatabase db(DBType::SQLite, (env->dbFolder / "base" / "stackexchange" / (repoData.name + ".db")).string());
  std::error_code ec;

  bool shaFound = false;
  for (const UpdateRepoTagData& tag : repoData.tags)
  {
    if (!shaFound)
    {
      if (tag.sha == lastSha || lastSha == "")
        shaFound = true;
    }
    else
    {
      std::filesystem::path tarPath = env->workFolder / "db" / "base" / "stackexchange" / (tag.sha + ".tar.gz");
      std::filesystem::path sigPath = env->workFolder / "db" / "base" / "stackexchange" / (tag.sha + ".tar.gz.sig");

      std::filesystem::create_directories(tarPath.parent_path());

      downloadFile(&cli, "/data/db/base/stackexchange/" + tag.sha + ".tar.gz", tarPath);
      downloadFile(&cli, "/data/db/base/stackexchange/" + tag.sha + ".tar.gz.sig", sigPath);

      if (SignHelper::verifyFile(tarPath.string(), env->publicPath.string(), sigPath.string()))
      {
        std::filesystem::path baseDbPath = tarPath.parent_path() / (tag.sha + ".db");

        ArchiveHelper::extractTar(tarPath, baseDbPath.parent_path());

        handler.importData(baseDbPath.string(), (env->dbFolder / "CPP" / "stackexchange" / (repoData.name + ".db")).string());

        StackExchangeExtractDatabase segmentDb(DBType::SQLite, baseDbPath.string());

        segmentDb.processSnippeds([&db](const SnippedData& data) { db.addSnipped(data.id, data.date, data.licence, data.code); });
      }
    }
  }

  std::filesystem::remove_all((env->workFolder / "db" / "base" / "stackexchange"), ec);

  return false;
}

bool UpdateManager::updateSourceForge(const UpdateRepoData& repoData, const std::string& lastSha)
{
  return updateGitHub(repoData, lastSha);
}

ahn::vector<UpdateRepoData> UpdateManager::getUpdateRepoData(const ahn::vector<std::string>& filter)
{
  ahn::vector<UpdateRepoData> ret;

  httplib::Client cli("http://www.ahnalytic.org");
  httplib::Result res = cli.Get("/data/db/statusFull.json");

  if (res && res->status == 200)
  {
    UpdateRepoFile updateFile;
    updateFile.readBuffer(res->body);

    ret.reserve(updateFile.updateRepoData.size());
    for (const UpdateRepoData& repoData : updateFile.updateRepoData)
      if (filter.size() == 0 || std::find(filter.begin(), filter.end(), repoData.name) == filter.end())
        ret.push_back(repoData);
  }

  return ret;
}

/*

ahn::vector<UpdateInfo> UpdateManager::checkUpdates() const
{
  ahn::vector<UpdateInfo> ret;

  // Create HTTP client
  httplib::Client cli("http://www.ahnalytic.org");

  httplib::Result res = cli.Get("/data/db/status.json");

  if (res && res->status == 200)
  {
    UpdateStatusFile updateFile;
    updateFile.readBuffer(res->body);

    ret.reserve(updateFile.infos.size());
    for (const UpdateInfo& info : updateFile.infos)
      if (std::find(installedUpdates.infos.begin(), installedUpdates.infos.end(), info) == installedUpdates.infos.end())
        ret.push_back(info);
  }

  return ret;
}

void UpdateManager::checkUpdateDiff(ThreadSafeQueue<UpdateDiffInfo>& queue) const
{
  httplib::Client cli("http://www.ahnalytic.org");
  cli.set_follow_location(true);

  ahn::vector<UpdateInfo> changedGithub = checkUpdates();

  for (const UpdateInfo& info : changedGithub)
  {
    UpdateDiffInfo diffInfo;

    if (info.type == "github")
    {
      diffInfo.type = info.type;
      diffInfo.language = info.language;

      auto iter = std::find_if(installedUpdates.infos.begin(), installedUpdates.infos.end(),
                               [info](const UpdateInfo& data) { return data.name == info.name && data.type == info.type && data.language == info.language; });

      std::string lastSha = "";
      if (iter != installedUpdates.infos.end())
      {
        lastSha = iter->sha;
        diffInfo.name = info.name;
      }
      else
      {
        auto repoRes = cli.Get("/data/db/CPP/github/" + info.name + "/repo.json");

        if (repoRes && repoRes->status == 200)
        {
          try
          {
            nlohmann::json repoJson = nlohmann::json::parse(repoRes->body);
            diffInfo.licence = UpdateStatusFile::getString(repoJson, "Licence");
            diffInfo.visibleName = UpdateStatusFile::getString(repoJson, "Name");
            diffInfo.name = info.name;
            diffInfo.url = UpdateStatusFile::getString(repoJson, "Url");
          }
          catch (const std::exception&)
          {
          }
        }
      }

      auto tagsRes = cli.Get("/data/db/CPP/github/" + info.name + "/tags.json");

      if (tagsRes && tagsRes->status == 200)
      {
        try
        {
          nlohmann::json tagsJson = nlohmann::json::parse(tagsRes->body);

          int index = 0;
          if (lastSha != "")
          {
            for (; index < tagsJson.size(); index++)
            {
              diffInfo.existingShas.push_back({UpdateStatusFile::getString(tagsJson[index], "TagName"), UpdateStatusFile::getString(tagsJson[index], "Sha")});
              if (lastSha == UpdateStatusFile::getString(tagsJson[index], "Tags"))
              {
                index++;
                break;
              }
            }
          }

          for (; index < tagsJson.size(); index++)
            diffInfo.missingShas.push_back({UpdateStatusFile::getString(tagsJson[index], "TagName"), UpdateStatusFile::getString(tagsJson[index], "Sha")});
        }
        catch (const std::exception&)
        {
        }

        queue.push(diffInfo);
      }
    }
    else if (info.type == "stackexchange")
    {
      diffInfo.type = info.type;
      diffInfo.language = info.language;
      diffInfo.baseName = info.baseName;
    }
  }

  // return ret;
}

void UpdateManager::startUpdates()
{
  httplib::Client cli("http://www.ahnalytic.org");

  UpdateStatusFile resUpdates = installedUpdates;

  ThreadSafeQueue<UpdateDiffInfo> queue;
  ThreadSafeQueue<UpdateDiffInfo> finishedQueue;
  bool finished = false;
  std::jthread producer([this, &queue, &finished]
  {
    checkUpdateDiff(queue);
    finished = true;
  });

  auto downloadFile = [&cli](const std::string& path, const std::filesystem::path& outPath)
  {
    auto res = cli.Get(path);
    if (res && res->status == 200)
    {
      std::filesystem::create_directories(outPath.parent_path());
      std::ofstream out(outPath.string(), std::ios::binary);
      out.write(res->body.data(), res->body.size());
      out.close();
    }
  };

  auto updateStateFile = [&finishedQueue, &resUpdates, this]()
  {
    while (!finishedQueue.empty())
    {
      UpdateDiffInfo finishedUpdate = finishedQueue.wait_and_pop();
      auto iter = std::find_if(resUpdates.infos.begin(), resUpdates.infos.end(), [finishedUpdate](const UpdateInfo& data)
      { return data.name == finishedUpdate.name && data.type == finishedUpdate.type && data.language == finishedUpdate.language; });

      if (iter != resUpdates.infos.end())
      {
        iter->sha = finishedUpdate.missingShas[finishedUpdate.missingShas.size() - 1].second;
      }
      else
      {
        UpdateInfo info;

        info.name = finishedUpdate.name;
        info.baseName = finishedUpdate.baseName;
        info.sha = finishedUpdate.missingShas[finishedUpdate.missingShas.size() - 1].second;
        info.maxVersion = "";
        info.type = finishedUpdate.type;
        info.language = finishedUpdate.language;

        resUpdates.infos.push_back(info);
      }
    }

    std::string dbData = (env->dbFolder / "status.json").string();
    resUpdates.write(dbData);
  };

  BS::thread_pool pool;

  while (!queue.empty() || !finished)
  {
    if (queue.empty() && !finished)
      std::this_thread::sleep_for(std::chrono::seconds{1});

    UpdateDiffInfo update = queue.wait_and_pop();

    pool.detach_task([update, this, &downloadFile, &finishedQueue]()
    {
      if (update.type == "github")
      {
        FileDatabase db(DBType::SQLite, (env->dbFolder / "CPP" / "github" / (update.name + ".db")).string());

        if (update.url != "")
          db.createRepoData(update.name, update.url, update.licence);

        std::error_code ec;
        std::filesystem::path pathesPath = env->workFolder / "db" / "CPP" / update.name / "pathes.dat";
        std::filesystem::create_directories(pathesPath.parent_path());
        downloadFile("/data/db/CPP/github/" + update.name + "/pathes.dat", pathesPath);

        if (!std::filesystem::exists(pathesPath))
          return;

        db.importPathesData(pathesPath);

        ahn::vector<std::string> doneList;
        for (const auto& sha : update.existingShas)
          doneList.push_back(sha.second);

        ahn::set<uint32_t> hashes;
        std::filesystem::path filePath = (env->dbFolder / "CPP" / "github" / (update.name + "." + std::to_string(env->windowSize))).string();

        if (std::filesystem ::exists(filePath))
        {
          std::ifstream file(filePath, std::ios::binary | std::ios::ate);
          std::streamsize size = file.tellg();
          file.seekg(0, std::ios::beg);
          ahn::vector<uint32_t> data(size / sizeof(uint32_t));

          if (file.read(reinterpret_cast<char*>(data.data()), size))
          {
            hashes.reserve(data.size());
            hashes.insert(data.begin(), data.end());
          }
        }

        for (const auto& sha : update.missingShas)
        {
          std::filesystem::path tarPath = env->workFolder / "db" / "CPP" / update.name / (sha.second + ".tar");
          std::filesystem::path sigPath = env->workFolder / "db" / "CPP" / update.name / (sha.second + ".tar.sig");

          downloadFile("/data/db/CPP/github/" + update.name + "/" + sha.second + ".tar", tarPath);
          downloadFile("/data/db/CPP/github/" + update.name + "/" + sha.second + ".tar.sig", sigPath);

          if (SignHelper::verifyFile(tarPath.string(), env->publicPath.string(), sigPath.string()))
          {
            db.importData(sha.first, sha.second, tarPath, pathesPath, std::find(doneList.begin(), doneList.end(), sha.second) != doneList.end(), *env, hashes);

            std::filesystem::remove_all(tarPath, ec);
            std::filesystem::remove_all(sigPath, ec);
          }
        }

        ahn::vector<uint32_t> vec;
        vec.reserve(hashes.size());
        vec.insert(vec.end(), hashes.begin(), hashes.end());

        std::ofstream fileOut(filePath.native(), std::ios::binary);
        fileOut.write((char*)vec.data(), static_cast<std::streamsize>(vec.size() * sizeof(uint32_t)));

        std::filesystem::remove_all(pathesPath, ec);
        std::filesystem::remove_all(pathesPath.parent_path(), ec);

        finishedQueue.push(update);
      }
    });

    updateStateFile();
  }

  pool.wait();
  updateStateFile();

  installedUpdates = resUpdates;
}
*/