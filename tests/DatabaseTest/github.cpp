#include "gtest/gtest.h"

#include "AhnalyticBase/SnippedDatabase.hpp"
#include "AhnalyticBase/SourceScanner.hpp"
#include "AhnalyticBase/datadump.hpp"

#include "AhnalyticBase/github.hpp"
#include "AhnalyticBase/githubcrawler.hpp"

TEST(TestCaseGitHub, CrawlGitHub)
{
  GitHubCrawler crawler;
  crawler.crawlIncremental("D:/source/Ahnalytic/db/github.db");

  EXPECT_EQ(1, 1);
  EXPECT_TRUE(true);
}

/*
TEST(TestCaseGitHub, ScanGitHub)
{
  GitHubHandler handler("D:/source/Ahnalytic/db", "D:/source/Ahnalytic/work");

  GitHubRepoDatabase db(DBType::SQLite, "D:/source/Ahnalytic/db/base/github/github.db");
  db.processRepos([&handler](const RepoInfo& info) {
    if (info.language == "C" || info.language == "C++")
      handler.scanRepo(info);
    });

  EXPECT_EQ(1, 1);
  EXPECT_TRUE(true);
}
*/