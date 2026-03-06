#include "UpdateManager.hpp"

#include "AhnalyticBase/database/FileDatabase.hpp"
#include "AhnalyticBase/helper/Enviroment.hpp"
#include "AhnalyticBase/helper/SignHelper.hpp"

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

  if (std::filesystem::exists(dbData))
  {
    nlohmann::json updateData = nlohmann::json::parse(std::ifstream(dbData));

    for (int index = 0; index < updateData.size(); index++)
    {
      UpdateInfo info;
      info.name = updateData[index]["name"].get<std::string>();
      info.baseName = updateData[index]["baseName"].get<std::string>();
      info.sha = updateData[index]["sha"].get<std::string>();
      info.maxVersion = updateData[index]["maxVersion"].get<std::string>();
      info.type = updateData[index]["type"].get<std::string>();
      info.language = updateData[index]["language"].get<std::string>();
      installedUpdates.push_back(info);
    }
  }
}

UpdateManager::~UpdateManager()
{
}

std::vector<UpdateInfo> UpdateManager::checkUpdates() const
{
  std::vector<UpdateInfo> ret;

  // Create HTTP client
  httplib::Client cli("http://www.ahnalytic.org");

  httplib::Result res = cli.Get("/data/db/status.json");

  if (res && res->status == 200)
  {
    try
    {
      nlohmann::json statusData = nlohmann::json::parse(res->body);

      for (int index = 0; index < statusData.size(); index++)
      {
        UpdateInfo info;
        info.name = statusData[index]["name"].get<std::string>();
        info.baseName = statusData[index]["baseName"].get<std::string>();
        info.sha = statusData[index]["sha"].get<std::string>();
        info.maxVersion = statusData[index]["maxVersion"].get<std::string>();
        info.type = statusData[index]["type"].get<std::string>();
        info.language = statusData[index]["language"].get<std::string>();

        if (std::find(installedUpdates.begin(), installedUpdates.end(), info) == installedUpdates.end())
          ret.push_back(info);
      }
    }
    catch (const std::exception& e)
    {
      std::cerr << "JSON parse error: " << e.what() << "\n";
    }
  }

  return ret;
}

std::vector<UpdateDiffInfo> UpdateManager::checkUpdateDiff() const
{
  std::vector<UpdateDiffInfo> ret;

  httplib::Client cli("http://www.ahnalytic.org");
  cli.set_follow_location(true);

  std::vector<UpdateInfo> changedGithub = checkUpdates();

  for (const UpdateInfo& info : changedGithub)
  {
    UpdateDiffInfo diffInfo;

    if (info.type == "github")
    {
      continue;

      diffInfo.type = info.type;
      diffInfo.language = info.language;

      auto iter = std::find_if(installedUpdates.begin(), installedUpdates.end(),
                               [info](const UpdateInfo& data) { return data.name == info.name && data.type == info.type && data.language == info.language; });

      std::string lastSha = "";
      if (iter != installedUpdates.end())
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
            diffInfo.licence = repoJson["Licence"].get<std::string>();
            diffInfo.visibleName = repoJson["Name"].get<std::string>();
            diffInfo.name = info.name;
            diffInfo.url = repoJson["Url"].get<std::string>();
          }
          catch (const std::exception& e)
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
              diffInfo.existingShas.push_back({tagsJson[index]["TagName"].get<std::string>(), tagsJson[index]["Sha"].get<std::string>()});
              if (lastSha == tagsJson[index]["Tags"].get<std::string>())
              {
                index++;
                break;
              }
            }
          }

          for (; index < tagsJson.size(); index++)
            diffInfo.missingShas.push_back({tagsJson[index]["TagName"].get<std::string>(), tagsJson[index]["Sha"].get<std::string>()});
        }
        catch (const std::exception& e)
        {
        }

        ret.push_back(diffInfo);
      }
    }
    else if (info.type == "stackexchange")
    {
      diffInfo.type = info.type;
      diffInfo.language = info.language;
      diffInfo.baseName = info.baseName;

    }
  }

  return ret;
}

void UpdateManager::startUpdates()
{
  httplib::Client cli("http://www.ahnalytic.org");

  std::vector<UpdateDiffInfo> updates = checkUpdateDiff();

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

  for (const UpdateDiffInfo& update : updates)
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

      db.importPathesData(pathesPath);

      std::vector<std::string> doneList;
      for (const auto& sha : update.existingShas)
        doneList.push_back(sha.second);

      ankerl::unordered_dense::set<uint32_t> hashes;
      std::filesystem::path filePath = (env->dbFolder / "CPP" / "github" / (update.name + "." + std::to_string(env->windowSize))).string();

      if (std::filesystem ::exists(filePath))
      {
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<uint32_t> data(size / sizeof(uint32_t));

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

      std::vector<uint32_t> vec;
      vec.reserve(hashes.size());
      vec.insert(vec.end(), hashes.begin(), hashes.end());

      std::ofstream fileOut(filePath.native(), std::ios::binary);
      fileOut.write((char*)vec.data(), static_cast<std::streamsize>(vec.size() * sizeof(uint32_t)));

      std::filesystem::remove_all(pathesPath, ec);
      std::filesystem::remove_all(pathesPath.parent_path(), ec);
    }
  }
}