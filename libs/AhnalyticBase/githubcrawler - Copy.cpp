#include "GitHubCrawler.hpp"

#include "RateLimit.hpp"

#include <iostream>
#include <regex>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <sstream>

using namespace std;


#define DEFAULT_TOKEN_LITERAL ""

// -----------------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------------

GitHubCrawler::GitHubCrawler(const std::string& token) : client("api.github.com", 443)
{
  if (!token.empty())
  {
    personalAccessToken = token;
  }
  else
  {
    //const char* env = std::getenv(DEFAULT_TOKEN_ENV);
    personalAccessToken = DEFAULT_TOKEN_ENV;
  }

  client.enable_server_certificate_verification(false);
}


// -----------------------------------------------------------------------------
// Top-level incremental crawl
// -----------------------------------------------------------------------------

void GitHubCrawler::crawlIncremental(const std::string& dbPath)
{
  GitHubRepoDatabase db(DBType::SQLite, dbPath);

  // 1) Forward scan to discover any new repos
  crawlNewRepos(&db);

  // 2) Search-based update scan to catch changes to existing repos since last update time
  crawlUpdatedRepos(&db);
}

// -----------------------------------------------------------------------------
// Crawl repositories created after last since id
// -----------------------------------------------------------------------------

void GitHubCrawler::crawlNewRepos(GitHubRepoDatabase* db)
{
  unsigned long long since = db->getLastSinceId();

  while (true)
  {
    std::string url = "/repositories?since=" + std::to_string(since);
    json repos;
    auto response = requestJson(url, repos);
    if (!response)
    {
      std::cout << "Stopping crawlNewRepos because request failed.\n";
      break;
    }

    if (!repos.is_array() || repos.empty())
    {
      std::cout << "crawlNewRepos: reached end of public repositories.\n";
      break;
    }

    unsigned long long maxIdInPage = since;

    for (auto& repoJson : repos)
    {
      // each item has an "id"
      if (repoJson.contains("id") && !repoJson["id"].is_null())
      {
        try
        {
          unsigned long long itemId = repoJson["id"].get<unsigned long long>();
          if (itemId > maxIdInPage)
            maxIdInPage = itemId;
        }
        catch (...)
        {
          // ignore parse errors for id
        }
      }

      if (!passesFilters(repoJson))
        continue;

      RepoInfo info;
      fillBasicRepoInfo(repoJson, info);

      // fetch refs + details (2 requests)
      fetchRefsAndHead(info);

      // upsert into db
      db->upsertRepo(info);
    }

    // persist progress even if some repos failed
    db->setLastSinceId(maxIdInPage);
    since = maxIdInPage;
  }
}

// -----------------------------------------------------------------------------
// Crawl repositories updated since last_update_ts using the search API
// -----------------------------------------------------------------------------

void GitHubCrawler::crawlUpdatedRepos(GitHubRepoDatabase* db)
{
  // load last timestamp (ISO 8601)
  std::string lastTs = db->getLastUpdateTimestamp();
  if (lastTs.empty())
    lastTs = "1970-01-01T00:00:00Z";

  // We'll page the search API. Per GitHub limits, per_page max 100.
  // We'll advance page by page and stop when no more items.
  int page = 1;
  const int perPage = 100;

  while (true)
  {
    // Build search query: updated:>lastTs
    // NOTE: we URL-encode the colon and greater-than, but httplib accepts simple strings
    std::ostringstream oss;
    oss << "/search/repositories?q=updated:>" << lastTs
      << "&sort=updated&order=asc&per_page=" << perPage << "&page=" << page;

    std::string url = oss.str();
    json root;
    auto res = requestJson(url, root);
    if (!res)
    {
      std::cout << "Stopping crawlUpdatedRepos because search request failed.\n";
      break;
    }

    // Search endpoint returns { total_count, incomplete_results, items: [...] }
    if (!root.contains("items") || !root["items"].is_array())
    {
      std::cout << "crawlUpdatedRepos: no items returned.\n";
      break;
    }

    auto& items = root["items"];
    if (items.empty())
    {
      std::cout << "crawlUpdatedRepos: no more updated repos.\n";
      break;
    }

    std::string newestTs = lastTs; // track newest updated_at found in this page

    for (auto& repoJson : items)
    {
      // Quick filters (skip forks/templates/archived/disabled/mirrors)
      if (!passesFilters(repoJson))
        continue;

      // Get full_name
      std::string fullName;
      if (repoJson.contains("full_name") && !repoJson["full_name"].is_null())
        fullName = repoJson["full_name"].get<std::string>();
      else
        continue; // cannot proceed

      // Determine pushed_at or updated_at for fast skip
      std::string pushedAt;
      if (repoJson.contains("pushed_at") && !repoJson["pushed_at"].is_null())
        pushedAt = repoJson["pushed_at"].get<std::string>();

      std::optional<std::string> localPushed = db->getRepoPushedAt(fullName);
      if (localPushed.has_value() && !pushedAt.empty() && localPushed.value() == pushedAt)
      {
        // unchanged, skip
      }
      else
      {
        // changed or new -> fetch refs & details
        RepoInfo info;
        fillBasicRepoInfo(repoJson, info);

        fetchRefsAndHead(info);

        db->upsertRepo(info);
      }

      // update newestTs from repoJson.updated_at if present
      if (repoJson.contains("updated_at") && !repoJson["updated_at"].is_null())
      {
        std::string updatedAt = repoJson["updated_at"].get<std::string>();
        if (updatedAt > newestTs)
          newestTs = updatedAt;
      }
      else if (!pushedAt.empty() && pushedAt > newestTs)
      {
        newestTs = pushedAt;
      }
    }

    // Advance to next page
    page++;

    // Save newest timestamp so next run starts after this point.
    if (newestTs > lastTs)
    {
      db->setLastUpdateTimestamp(newestTs);
      lastTs = newestTs;
    }

    // If fewer than perPage items returned, done
    if ((int)items.size() < perPage)
      break;

    // NOTE: GitHub Search has rate limits; consider inserting a small sleep
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
}

// -----------------------------------------------------------------------------
// Rate limit handling
// -----------------------------------------------------------------------------

void GitHubCrawler::handleRateLimit(const httplib::Result& res)
{
  if (!res)
    return;

  std::string rem = res->get_header_value("X-RateLimit-Remaining");
  std::string reset = res->get_header_value("X-RateLimit-Reset");

  int remaining = rem.empty() ? 1 : std::stoi(rem);

  if (remaining == 0)
  {
    long resetTs = reset.empty() ? 0 : std::stol(reset);
    long now = time(nullptr);
    long waitSeconds = std::max(1L, resetTs - now);

    std::cout << "Rate limit reached. Sleeping for " << waitSeconds << " seconds...\n";
    std::this_thread::sleep_for(std::chrono::seconds(waitSeconds));
  }
}

// -----------------------------------------------------------------------------
// JSON GET wrapper
// -----------------------------------------------------------------------------

httplib::Result GitHubCrawler::requestJson(const std::string& path, json& out)
{
  httplib::Headers headers =
  {
      {"User-Agent", "cpp-crawler"},
      {"Accept", "application/vnd.github+json"}
  };

  if (!personalAccessToken.empty())
    headers.insert({ {"Authorization", "Bearer " + personalAccessToken} });

  rateLimiter.waitIfNeeded();
  auto res = client.Get(path.c_str(), headers);
  rateLimiter.onResponse(res->headers);

  if (!res)
  {
    std::cerr << "Request failed: " << path << "\n";
    return res;
  }

  handleRateLimit(res);

  if (res->status != 200)
  {
    std::cerr << "HTTP " << res->status << " on " << path << "\n";
    return {};
  }

  try
  {
    out = json::parse(res->body);
  }
  catch (...)
  {
    std::cerr << "JSON parse error for: " << path << "\n";
    return {};
  }

  return res;
}

// -----------------------------------------------------------------------------
// Fetch full repo details
// -----------------------------------------------------------------------------

bool GitHubCrawler::fetchRepoDetails(const std::string& fullName, json& out)
{
  if (fullName.empty())
    return false;

  std::string path = "/repos/" + fullName;
  auto res = requestJson(path, out);
  return (bool)res;
}

// -----------------------------------------------------------------------------
// Decide whether to include this repo (fast listing checks only)
// -----------------------------------------------------------------------------

bool GitHubCrawler::passesFilters(json& repoSummary)
{
  if (repoSummary.contains("fork") && !repoSummary["fork"].is_null())
  {
    if (repoSummary["fork"].get<bool>())
      return false;
  }

  if (repoSummary.contains("is_template") && !repoSummary["is_template"].is_null())
  {
    if (repoSummary["is_template"].get<bool>())
      return false;
  }

  if (repoSummary.contains("archived") && !repoSummary["archived"].is_null())
  {
    if (repoSummary["archived"].get<bool>())
      return false;
  }

  if (repoSummary.contains("disabled") && !repoSummary["disabled"].is_null())
  {
    if (repoSummary["disabled"].get<bool>())
      return false;
  }

  if (repoSummary.contains("mirror_url") && !repoSummary["mirror_url"].is_null())
  {
    return false;
  }

  if (repoSummary.contains("size") && !repoSummary["size"].is_null())
  {
    if (repoSummary["size"].get<int>() == 0)
      return false;
  }

  return true;
}

// -----------------------------------------------------------------------------
// Fill basic repo info (null-safe) from the listing object
// -----------------------------------------------------------------------------

void GitHubCrawler::fillBasicRepoInfo(const json& r, RepoInfo& info)
{
  if (r.contains("name") && !r["name"].is_null())
    info.name = r["name"].get<std::string>();
  else
    info.name.clear();

  if (r.contains("full_name") && !r["full_name"].is_null())
    info.fullName = r["full_name"].get<std::string>();
  else
    info.fullName.clear();

  if (r.contains("html_url") && !r["html_url"].is_null())
    info.htmlUrl = r["html_url"].get<std::string>();
  else
    info.htmlUrl.clear();

  if (r.contains("language") && !r["language"].is_null())
    info.language = r["language"].get<std::string>();
  else
    info.language.clear();

  if (r.contains("pushed_at") && !r["pushed_at"].is_null())
    info.lastPushed = r["pushed_at"].get<std::string>();
  else
    info.lastPushed.clear();

  if (r.contains("license") && r["license"].is_object())
  {
    if (r["license"].contains("spdx_id") && !r["license"]["spdx_id"].is_null())
      info.license = r["license"]["spdx_id"].get<std::string>();
    else
      info.license = "NOASSERTION";
  }
  else
  {
    info.license = "NO LICENSE";
  }

  if (r.contains("default_branch") && !r["default_branch"].is_null())
    info.headBranch = r["default_branch"].get<std::string>();
  else
    info.headBranch.clear();

  info.headSha.clear();
  info.tags.clear();
  info.branches.clear();
}

// -----------------------------------------------------------------------------
// Fill only metadata (without clearing tags/branches) - used when merging details
// -----------------------------------------------------------------------------

void GitHubCrawler::fillBasicRepoInfoMetadataOnly(const json& r, RepoInfo& info)
{
  if (r.contains("name") && !r["name"].is_null())
    info.name = r["name"].get<std::string>();

  if (r.contains("full_name") && !r["full_name"].is_null())
    info.fullName = r["full_name"].get<std::string>();

  if (r.contains("html_url") && !r["html_url"].is_null())
    info.htmlUrl = r["html_url"].get<std::string>();

  if (r.contains("language") && !r["language"].is_null())
    info.language = r["language"].get<std::string>();

  if (r.contains("pushed_at") && !r["pushed_at"].is_null())
    info.lastPushed = r["pushed_at"].get<std::string>();

  if (r.contains("license") && r["license"].is_object())
  {
    if (r["license"].contains("spdx_id") && !r["license"]["spdx_id"].is_null())
      info.license = r["license"]["spdx_id"].get<std::string>();
    else
      info.license = "NOASSERTION";
  }

  if (r.contains("default_branch") && !r["default_branch"].is_null())
    info.headBranch = r["default_branch"].get<std::string>();
}

// -----------------------------------------------------------------------------
// Parse refs array returned by /git/refs and populate info.tags/info.branches.
// -----------------------------------------------------------------------------

void GitHubCrawler::parseRefsArray(const json& refsArray, RepoInfo& info)
{
  if (!refsArray.is_array())
    return;

  for (const auto& r : refsArray)
  {
    if (!r.contains("ref") || r["ref"].is_null())
      continue;

    std::string refStr = r["ref"].get<std::string>();

    std::string objectSha;
    if (r.contains("object") && r["object"].is_object())
    {
      if (r["object"].contains("sha") && !r["object"]["sha"].is_null())
        objectSha = r["object"]["sha"].get<std::string>();
    }

    const std::string headsPrefix = "refs/heads/";
    const std::string tagsPrefix = "refs/tags/";

    if (refStr.rfind(headsPrefix, 0) == 0)
    {
      BranchInfo bi;
      bi.name = refStr.substr(headsPrefix.size());
      bi.commitSha = objectSha;
      info.branches.push_back(bi);
    }
    else if (refStr.rfind(tagsPrefix, 0) == 0)
    {
      TagInfo ti;
      ti.name = refStr.substr(tagsPrefix.size());
      ti.commitSha = objectSha;
      info.tags.push_back(ti);
    }
  }
}

// -----------------------------------------------------------------------------
// Fetch refs (branches + tags) and full details (2 requests)
// -----------------------------------------------------------------------------

void GitHubCrawler::fetchRefsAndHead(RepoInfo& info)
{
  if (info.fullName.empty())
    return;

  // -----------------------------
  // 1) Fetch ALL pages of /git/refs
  // -----------------------------
  info.tags.clear();
  info.branches.clear();

  std::string basePath = "/repos/" + info.fullName + "/git/refs";
  std::string path = basePath + "?per_page=100";

  while (true)
  {
    json refsJson;
    auto res = requestJson(path, refsJson);
    if (!res)
      break;

    // parse refs from this page
    parseRefsArray(refsJson, info);

    // detect "Link: <...>; rel=\"next\""
    std::string linkHeader = res->get_header_value("Link");
    std::string nextUrl;

    if (!linkHeader.empty())
    {
      // find rel="next"
      std::regex re("<([^>]+)>; rel=\"next\"");
      std::smatch m;
      if (std::regex_search(linkHeader, m, re) && m.size() > 1)
      {
        nextUrl = m[1];
      }
    }

    if (nextUrl.empty())
      break; // no more pages

    // GitHub returns absolute API URLs — strip domain
    const std::string apiPrefix = "https://api.github.com";
    if (nextUrl.rfind(apiPrefix, 0) == 0)
      nextUrl = nextUrl.substr(apiPrefix.length());

    path = nextUrl;
  }

  // -----------------------------
  // 2) Fetch full repo metadata
  // -----------------------------
  json detailJson;
  if (fetchRepoDetails(info.fullName, detailJson))
    fillBasicRepoInfoMetadataOnly(detailJson, info);

  // -----------------------------
  // 3) Determine head SHA
  // -----------------------------
  info.headSha.clear();
  for (const auto& b : info.branches)
  {
    if (b.name == info.headBranch)
    {
      info.headSha = b.commitSha;
      break;
    }
  }

  // fallback: main or master
  if (info.headSha.empty())
  {
    for (const auto& b : info.branches)
      if (b.name == "main") { info.headSha = b.commitSha; break; }

    if (info.headSha.empty())
    {
      for (const auto& b : info.branches)
        if (b.name == "master") { info.headSha = b.commitSha; break; }
    }
  }
}

