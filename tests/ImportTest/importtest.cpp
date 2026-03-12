#include "gtest/gtest.h"

#include "AhnalyticBase/database/FileDatabase.hpp"
#include "AhnalyticBase/database/SnippedDatabase.hpp"
#include "AhnalyticBase/github/Github.hpp"
#include "AhnalyticBase/github/GithubCrawler.hpp"
#include "AhnalyticBase/stackexchange/DataDump.hpp"
#include "AhnalyticBase/tree/SourceScanner.hpp"
#include "AhnalyticBase/tree/TreeSearch.hpp"

#include "BS_thread_pool.hpp"

#include <fstream>

TEST(TestCaseGitHub, Serialize)
{
  EnviromentC env;

  RepoInfo info;

  info.name = "Slayer";
  info.fullName = "666shashank/Slayer";
  info.htmlUrl = "https://github.com/666shashank/Slayer";
  info.language = "C++";

  info.headSha = "87a747fc5ec367c5c1514c966aff269807304485";
  info.tags.push_back({"HEAD", "87a747fc5ec367c5c1514c966aff269807304485"});

  std::string dbIn = "d:/work/CPP/github/666shashank_Slayer_CPP.db";
  std::filesystem::path workPath = "D:/work";

  GitHubHandler handler(workPath.string(), workPath.string());
  handler.scanRepo(info);

  FileDatabase db(DBType::SQLite, dbIn);
  

  db.exportData(workPath);

  FileDatabase dbout(DBType::SQLite, "D:/work/test.db");

  std::filesystem::path pathesPath = "D:/work/pathes.dat";

  std::filesystem::path tagFile = "D:/work/tags.json";
  std::ifstream tagStream(tagFile.string());
  nlohmann::json tagData;
  tagStream >> tagData;

  std::filesystem::path repoFile = "D:/work/repo.json";
  std::ifstream repoStream(repoFile.string());
  nlohmann::json repogData;
  repoStream >> repogData;

  dbout.createRepoData(repogData["Name"].get<std::string>(), repogData["Url"].get<std::string>(), repogData["Licence"].get<std::string>());

  dbout.importPathesData(pathesPath);

  std::vector<std::string> doneList;

  ankerl::unordered_dense::set<uint32_t> hashes;

  // for (const auto& entry : std::filesystem::directory_iterator(workPath))
  for (int index = 0; index < tagData.size(); index++)
  {
    std::string name = tagData.at(index)["TagName"].get<std::string>();
    std::string sha = tagData.at(index)["Sha"].get<std::string>();

    std::filesystem::path tarPath = workPath / (sha + ".tar");

    // if (entry.is_regular_file() && entry.path().extension() == ".tar")
    //{
    // std::filesystem::path tarPath = entry.path();

    dbout.importData(name, sha, tarPath, pathesPath, std::find(doneList.begin(), doneList.end(), sha) != doneList.end(), env, hashes);
    doneList.push_back(sha);
    //}
  }

  std::vector<uint32_t> vec;
  vec.reserve(hashes.size());
  vec.insert(vec.end(), hashes.begin(), hashes.end());

  // CompressData cmpData = compressionManager.compress(CompressData(vec), nullptr, std::vector<ModAlgosE>{ModAlgosE::None, ModAlgosE::Delta},
  // std::vector<CompressionAlgosE>{CompressionAlgosE::LZMA, CompressionAlgosE::BSC}, true);

  std::filesystem::path filePath = workPath;
  filePath = filePath.concat("/").concat("test." + std::to_string(env.windowSize));
  std::ofstream fileOut(filePath.native(), std::ios::binary);

  // std::vector<char> charData = cmpData.getCharData(CompressData::On);
  fileOut.write((char*)vec.data(), static_cast<std::streamsize>(vec.size() * sizeof(uint32_t)));

  EXPECT_EQ(1, 1);
  EXPECT_TRUE(true);
}

/*
TEST(TestCaseGitHub, ScanGitHub)
{
  GitHubHandler handler("D:/source/Ahnalytic/db", "D:/work");

  // BS::thread_pool poolTags(8);
  // BS::thread_pool poolSingle(8);
  BS::thread_pool pool;

  std::vector<std::string> langFilter{"C", "C++"};

  GitHubRepoDatabase db(DBType::SQLite, "D:/source/Ahnalytic/db/base/github/github.db");
  db.processRepos(langFilter, true, false, [&handler,  &pool](RepoInfo info)
  {
    // if (info.language == "C" || info.language == "C++")
    {
      std::filesystem::path resPath = "D:/source/Ahnalytic/db";
      resPath = resPath.concat("/CPP").concat("/github/").concat(handler.cleanFileName(info.fullName) + "_CPP.db");

      std::filesystem::path lastPath = "D:/source/Ahnalytic/db";
      lastPath = lastPath.concat("/CPP").concat("/github_last/").concat(handler.cleanFileName(info.fullName) + "_CPP.db");

      if (!std::filesystem::exists(resPath) && std::filesystem::exists(lastPath))
        std::filesystem::copy(lastPath, resPath);

      if (std::filesystem::exists(resPath))
      {
        FileDatabase* db = new FileDatabase(DBType::SQLite, resPath.string());

        std::unordered_map<std::string, std::string> tags = db->getTags();

        std::vector<TagInfo> resTags;
        info.tags.reserve(info.tags.size());
        for (const TagInfo& tag : info.tags)
          if (!tags.contains(tag.name))
            resTags.push_back(tag);
        info.tags = resTags;

        if (info.tags.size() == 0)
          return;
      }

      resPath = "D:/source/Ahnalytic/db";
      resPath = resPath.concat("/CPP").concat("/github/").concat(handler.cleanFileName(info.fullName) + "_CPP.empty");

      if (std::filesystem::exists(resPath))
        return;

      pool.detach_task([&handler, info]() { handler.scanRepo(info); });

      while (pool.get_tasks_total() > 1000)
        pool.wait_for(std::chrono::minutes(5));

    }
  });

  pool.wait();
  // poolSingle.wait();
  // poolTags.wait();

  EXPECT_EQ(1, 1);
  EXPECT_TRUE(true);
}

*/