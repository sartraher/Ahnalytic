#include "gtest/gtest.h"

#include "AhnalyticBase/database/FileDatabase.hpp"
#include "AhnalyticBase/database/SnippedDatabase.hpp"
#include "AhnalyticBase/tree/SourceScanner.hpp"
#include "AhnalyticBase/stackexchange/DataDump.hpp"
#include "AhnalyticBase/github/Github.hpp"
#include "AhnalyticBase/github/GithubCrawler.hpp"

#include "BS_thread_pool.hpp"

TEST(TestCaseGitHub, ScanGitHub)
{
  GitHubHandler handler("D:/source/Ahnalytic/db", "D:/work");

  // BS::thread_pool poolTags(8);
  // BS::thread_pool poolSingle(8);
  BS::thread_pool pool;

  std::vector<std::string> langFilter{"C", "C++"};

  GitHubRepoDatabase db(DBType::SQLite, "D:/source/Ahnalytic/db/base/github/github.db");
  db.processRepos(langFilter, true, false, [&handler, /*&poolTags, &poolSingle*/ &pool](RepoInfo info)
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

      /*
      if (info.tags.size() == 0)
        poolSingle.submit_task([&handler, info]() { handler.scanRepo(info); });
      else
        poolTags.submit_task([&handler, info]() { handler.scanRepo(info); });

      while ((poolSingle.get_tasks_total() + poolTags.get_tasks_total()) > 1000)
      {
        poolSingle.wait_for(std::chrono::minutes(5));
        poolTags.wait_for(std::chrono::minutes(5));
      }
      */
    }
  });

  pool.wait();
  // poolSingle.wait();
  // poolTags.wait();

  EXPECT_EQ(1, 1);
  EXPECT_TRUE(true);
}