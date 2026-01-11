#pragma once

#include "AhnalyticBase/Export.hpp"

#define CPPHTTPLIB_NO_MULTIPART_FORM_DATA
#define CPPHTTPLIB_OPENSSL_SUPPORT
#define CPPHTTPLIB_ZLIB_SUPPORT
#include <httplib.h>

#include "AhnalyticBase/database/GitHubRepoDatabase.hpp"
#include "AhnalyticBase/github/RateLimit.hpp"

#include <nlohmann/json.hpp>

#include <string>

#define DEFAULT_TOKEN_ENV "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"

using json = nlohmann::json;

class DLLEXPORT GitHubCrawler
{
public:
  explicit GitHubCrawler(const std::string& token = "");

  // High level incremental crawl: new repos + updates
  void crawlIncremental(const std::string& dbPath);

  httplib::Result requestJson(const std::string& path, json& out);

private:
  // low-level loops
  void crawlNewRepos(GitHubRepoDatabase* db);
  void crawlUpdatedRepos(GitHubRepoDatabase* db);

  // existing helpers
  void handleRateLimit(const httplib::Result& res);

  bool fetchRepoDetails(const std::string& fullName, json& out);
  void fillBasicRepoInfo(const json& r, RepoInfo& info);
  void fillBasicRepoInfoMetadataOnly(const json& r, RepoInfo& info);

  void parseRefsArray(const json& refsArray, RepoInfo& info, std::vector<std::pair<std::string, std::string>>& unresolvedTagObjects);
  void fetchRefsAndHead(RepoInfo& info);

  // fast filter
  bool passesFilters(json& repoSummary);

private:
  RateLimiter rateLimiter;
  httplib::SSLClient client;
  std::string personalAccessToken;
};