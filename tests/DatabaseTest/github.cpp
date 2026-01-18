#include "gtest/gtest.h"

#include "AhnalyticBase/database/SnippedDatabase.hpp"
#include "AhnalyticBase/tree/SourceScanner.hpp"
#include "AhnalyticBase/stackexchange/DataDump.hpp"

#include "AhnalyticBase/github/Github.hpp"
#include "AhnalyticBase/github/GithubCrawler.hpp"

TEST(TestCaseGitHub, CrawlGitHub)
{
  GitHubCrawler crawler;
  crawler.crawlIncremental("D:/source/git/Ahnalytic/db/github.db");

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