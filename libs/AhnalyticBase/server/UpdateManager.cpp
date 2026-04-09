#include "UpdateManager.hpp"

#include "AhnalyticBase/database/FileDatabase.hpp"
#include "AhnalyticBase/database/StackExchangeExtractDatabase.hpp"
#include "AhnalyticBase/helper/ArchiveHelper.hpp"
#include "AhnalyticBase/helper/DataHelper.hpp"
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

  setUpdateAmountFinished(0);
  setUpdateAmountMax(repoDatas.size());

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
        ret = updateGitHub(repoData, lastSha);
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

      incUpdateAmountFinished();

      // Check if we need to update or add;
      auto iter =
          std::find_if(installedUpdates.infos.begin(), installedUpdates.infos.end(), [repoData](const UpdateInfo& info) { return info.name == repoData.name; });

      if (iter != installedUpdates.infos.end())
        iter->sha = repoData.tags[repoData.tags.size() - 1].sha;
      else
      {
        UpdateInfo info;
        info.name = DataHelperC::cleanFileName(repoData.name) + "_" + repoData.language;
        info.sha = repoData.tags[repoData.tags.size() - 1].sha;
        info.type = repoData.type;
        info.language = repoData.language;
        info.version = repoData.version;
        installedUpdates.infos.push_back(info);
        installedUpdates.write((env->dbFolder / "status.json").string());
      }
    }
  }
}

bool UpdateManager::updateGitHub(const UpdateRepoData& repoData, const std::string& lastSha)
{
  httplib::Client cli("http://www.ahnalytic.org");

  std::error_code ec;

  std::string repoFolderName = DataHelperC::cleanFileName(repoData.name) + "_CPP";

  std::filesystem::path dbPath = env->dbFolder / "CPP" / "github" / (repoFolderName + ".db");
  std::filesystem::create_directories(dbPath.parent_path());
  FileDatabase db(DBType::SQLite, dbPath.string());

  if (lastSha == "")
    db.createRepoData(repoData.name, repoData.url, repoData.licence);

  std::filesystem::path pathesPath = env->workFolder / "db" / "CPP" / repoFolderName / "pathes.dat";
  std::filesystem::create_directories(pathesPath.parent_path());
  downloadFile(&cli, "/data/db/CPP/github/" + repoFolderName + "/pathes.dat", pathesPath);

  if (!std::filesystem::exists(pathesPath))
    return false;

  db.importPathesData(pathesPath);

  ahn::set<uint32_t> hashes;
  std::filesystem::path filePath = (env->dbFolder / "CPP" / "github" / (repoFolderName + "." + std::to_string(env->windowSize))).string();

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

  bool shaFound = lastSha == "";
  for (const UpdateRepoTagData& tag : repoData.tags)
  {
    if (shaFound)
    {
      std::filesystem::path tarPath = env->workFolder / "db" / "CPP" / repoFolderName / (tag.sha + ".tar");
      std::filesystem::path sigPath = env->workFolder / "db" / "CPP" / repoFolderName / (tag.sha + ".tar.sig");

      std::filesystem::create_directories(tarPath.parent_path());

      downloadFile(&cli, "/data/db/CPP/github/" + repoFolderName + "/" + tag.sha + ".tar", tarPath);
      downloadFile(&cli, "/data/db/CPP/github/" + repoFolderName + "/" + tag.sha + ".tar.sig", sigPath);

      if (SignHelper::verifyFile(tarPath.string(), env->publicPath.string(), sigPath.string()))
      {
        db.importData(tag.tagName, tag.sha, tarPath, pathesPath, std::find(doneList.begin(), doneList.end(), tag.sha) != doneList.end(), *env, hashes);

        std::filesystem::remove_all(tarPath, ec);
        std::filesystem::remove_all(sigPath, ec);
      }
    }

    doneList.push_back(tag.sha);

    if (!shaFound)
    {
      if (tag.sha == lastSha || lastSha == "")
        shaFound = true;
    }
  }

  ahn::vector<uint32_t> vec;
  vec.reserve(hashes.size());
  vec.insert(vec.end(), hashes.begin(), hashes.end());

  std::ofstream fileOut(filePath.native(), std::ios::binary);
  fileOut.write((char*)vec.data(), static_cast<std::streamsize>(vec.size() * sizeof(uint32_t)));

  std::filesystem::remove_all(pathesPath, ec);
  std::filesystem::remove_all(pathesPath.parent_path(), ec);

  return true;
}

bool UpdateManager::updateStackExchange(const UpdateRepoData& repoData, const std::string& lastSha)
{
  httplib::Client cli("http://www.ahnalytic.org");

  StackOverflowHandler handler;

  std::filesystem::path dbPath = env->dbFolder / "base" / "stackexchange" / (repoData.name + ".db");
  std::filesystem::create_directories(dbPath.parent_path());

  std::error_code ec;

  {
    StackExchangeExtractDatabase db(DBType::SQLite, dbPath.string());

    bool shaFound = lastSha == "";
    // for (const UpdateRepoTagData& tag : repoData.tags)
    for (int index = 0; index < repoData.tags.size(); index++)
    {
      std::string sha = repoData.tags[index].sha;

      if (shaFound)
      {
        std::filesystem::path tarPath = env->workFolder / "db" / "base" / "stackexchange" / (sha + ".db.tar.gz");
        std::filesystem::path sigPath = env->workFolder / "db" / "base" / "stackexchange" / (sha + ".db.tar.gz.sig");

        std::filesystem::create_directories(tarPath.parent_path());

        if (!std::filesystem::exists(tarPath))
          downloadFile(&cli, "/data/db/base/stackexchange/" + sha + ".db.tar.gz", tarPath);

        if (!std::filesystem::exists(sigPath))
          downloadFile(&cli, "/data/db/base/stackexchange/" + sha + ".db.tar.gz.sig", sigPath);

        if (SignHelper::verifyFile(tarPath.string(), env->publicPath.string(), sigPath.string()))
        {
          std::filesystem::path baseDbPath = tarPath.parent_path() / (sha + ".db");

          ArchiveHelper::extractTar(tarPath, baseDbPath.parent_path());

          // handler.importData(baseDbPath.string(), (env->dbFolder / "CPP" / "stackexchange" / (repoData.name + ".db")).string());

          StackExchangeExtractDatabase segmentDb(DBType::SQLite, baseDbPath.string());
          db.mergeDatabase(segmentDb);

          // segmentDb.processSnippeds([&db](const SnippedData& data) { db.addSnipped(data.id, data.date, data.licence, data.code); });
        }
      }

      if (!shaFound)
      {
        if (sha == lastSha || lastSha == "")
          shaFound = true;
      }
    }
  }

  // TODO: do this fo each language

  std::filesystem::path tarPath = env->workFolder / "db" / "CPP" / "stackexchange" / (repoData.name + "_CPP.tar.gz");
  std::filesystem::path sigPath = env->workFolder / "db" / "CPP" / "stackexchange" / (repoData.name + "_CPP.tar.gz.sig");

  std::filesystem::create_directories(tarPath.parent_path(), ec);

  downloadFile(&cli, "/data/db/CPP/stackexchange/" + repoData.name + "_CPP.tar.gz", tarPath);
  downloadFile(&cli, "/data/db/CPP/stackexchange/" + repoData.name + "_CPP.tar.gz.sig", sigPath);
  if (SignHelper::verifyFile(tarPath.string(), env->publicPath.string(), sigPath.string()))
  {
    std::filesystem::path dbPath = env->dbFolder / "CPP" / "stackexchange" / (repoData.name + ".db");
    ArchiveHelper::extractTar(tarPath, dbPath.parent_path());
  }

  std::filesystem::remove_all((env->workFolder / "db" / "base" / "stackexchange"), ec);
  std::filesystem::remove_all((env->workFolder / "db" / "CPP" / "stackexchange"), ec);

  return true;
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

int UpdateManager::getUpdateAmountFinished() const
{
  std::lock_guard<std::recursive_mutex> lock(mutex);
  return updateAmountFinished;
}

int UpdateManager::getUpdateAmountMax() const
{
  std::lock_guard<std::recursive_mutex> lock(mutex);
  return updateAmountMax;
}

void UpdateManager::setUpdateAmountFinished(int amount)
{
  std::lock_guard<std::recursive_mutex> lock(mutex);
  updateAmountFinished = amount;
}

void UpdateManager::incUpdateAmountFinished()
{
  std::lock_guard<std::recursive_mutex> lock(mutex);
  updateAmountFinished++;
}

void UpdateManager::setUpdateAmountMax(int amount)
{
  std::lock_guard<std::recursive_mutex> lock(mutex);
  updateAmountMax = amount;
}
