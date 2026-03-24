#include "UpdateManager.hpp"

#include "AhnalyticBase/database/FileDatabase.hpp"
#include "AhnalyticBase/helper/Enviroment.hpp"
#include "AhnalyticBase/helper/SignHelper.hpp"

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