#include "GithubCrawler.hpp"

#include "RateLimit.hpp"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <regex>
#include <sstream>
#include <thread>

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
    personalAccessToken = DEFAULT_TOKEN_ENV;
  }

  // SECURITY: enable certificate verification (was false)
  client.enable_server_certificate_verification(true);

  // sensible timeouts (connection, read)
  client.set_connection_timeout(5, 0); // 5s connect
  client.set_read_timeout(30, 0);      // 30s read

  // Remove again after in an normal network again
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

  // We'll fetch pages and follow Link: rel="next" if present.
  std::string nextPath = "/repositories?since=" + std::to_string(since);

  while (!nextPath.empty())
  {
    json repos;
    auto res = requestJson(nextPath, repos);
    if (!res)
    {
      std::cout << "Stopping crawlNewRepos because request failed.\n";
      break;
    }

    if (!repos.is_array() || repos.empty())
    {
      std::cout << "crawlNewRepos: reached end of public repositories or empty page.\n";
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

      // fetch refs + details (REST-backed)
      fetchRefsAndHead(info);

      // upsert into db
      db->upsertRepo(info);
    }

    // persist progress for this page
    db->setLastSinceId(maxIdInPage);
    since = maxIdInPage;

    // Find Link header rel="next" to continue.
    std::string linkHeader = res->get_header_value("Link");
    std::string nextUrl;
    if (!linkHeader.empty())
    {
      std::regex re("<([^>]+)>;\\s*rel=\"next\"");
      std::smatch m;
      if (std::regex_search(linkHeader, m, re) && m.size() > 1)
      {
        nextUrl = m[1];
      }
    }

    if (nextUrl.empty())
    {
      // no more pages
      break;
    }

    // GitHub Link URLs are absolute; convert to path for httplib if necessary
    const std::string apiPrefix = "https://api.github.com";
    if (nextUrl.rfind(apiPrefix, 0) == 0)
      nextUrl = nextUrl.substr(apiPrefix.length());

    nextPath = nextUrl;

    // Small throttle to be polite
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
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
  int page = 1;
  const int perPage = 100;

  while (true)
  {
    // Build search query: updated:>lastTs
    std::ostringstream oss;
    oss << "/search/repositories?q=updated:>" << lastTs << "&sort=updated&order=asc&per_page=" << perPage << "&page=" << page;

    std::string url = oss.str();

    // We'll retry a few times if "incomplete_results" is true or transient errors occur.
    int tries = 0;
    const int maxTries = 4;
    json root;
    bool gotGood = false;
    while (tries < maxTries)
    {
      tries++;
      auto res = requestJson(url, root);
      if (!res)
      {
        std::cout << "crawlUpdatedRepos: search request failed on try " << tries << ".\n";
        std::this_thread::sleep_for(std::chrono::seconds(1 << std::min(tries, 6)));
        continue;
      }

      if (!root.is_object())
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        continue;
      }

      // If incomplete_results is true, wait and retry a couple times
      if (root.contains("incomplete_results") && root["incomplete_results"].is_boolean() && root["incomplete_results"].get<bool>())
      {
        std::cout << "crawlUpdatedRepos: search returned incomplete_results=true; retrying (" << tries << ")\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(500 * tries));
        continue;
      }

      gotGood = true;
      break;
    }

    if (!gotGood)
    {
      std::cout << "Stopping crawlUpdatedRepos because search request failed repeatedly.\n";
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

    // Rate-limit friendly pause between pages
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
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
    long now = (long)time(nullptr);
    long waitSeconds = std::max(1L, resetTs - now);

    std::cout << "Rate limit reached. Sleeping for " << waitSeconds << " seconds...\n";
    std::this_thread::sleep_for(std::chrono::seconds(waitSeconds));
  }
}

// -----------------------------------------------------------------------------
// JSON GET wrapper (robust, with simple retry/backoff & rate-limit handling)
// -----------------------------------------------------------------------------

httplib::Result GitHubCrawler::requestJson(const std::string& path, json& out)
{
  // We'll attempt a few transient retries for 5xx & connection issues and handle 403 rate-limit.
  const int maxAttempts = 4;
  int attempt = 0;
  std::string lastErr;

  while (attempt < maxAttempts)
  {
    attempt++;

    httplib::Headers headers = {{"User-Agent", "cpp-crawler"}, {"Accept", "application/vnd.github+json"}};

    if (!personalAccessToken.empty())
      headers.insert({{"Authorization", "Bearer " + personalAccessToken}});

    rateLimiter.waitIfNeeded();
    auto res = client.Get(path.c_str(), headers);

    // NULL check (was crashing before)
    if (!res)
    {
      lastErr = "Request failed (no response): " + path;
      std::cerr << lastErr << "\n";
      // exponential backoff
      std::this_thread::sleep_for(std::chrono::milliseconds(200 * (1 << attempt)));
      continue;
    }

    // Let rate limiter inspect headers
    rateLimiter.onResponse(res->headers);

    // If rate-limited (403) then sleep until reset and retry
    if (res->status == 403)
    {
      std::string rem = res->get_header_value("X-RateLimit-Remaining");
      if (!rem.empty() && std::stoi(rem) == 0)
      {
        handleRateLimit(res);
        // retry after sleeping
        continue;
      }
      // otherwise treat as unrecoverable 403
      std::cerr << "HTTP 403 on " << path << "\n";
      return res;
    }

    // 5xx transient errors -> retry with backoff
    if (res->status >= 500 && res->status < 600)
    {
      std::cerr << "HTTP " << res->status << " on " << path << " (attempt " << attempt << ")\n";
      std::this_thread::sleep_for(std::chrono::milliseconds(250 * (1 << attempt)));
      continue;
    }

    if (res->status != 200)
    {
      std::cerr << "HTTP " << res->status << " on " << path << "\n";
      return res;
    }

    // parse JSON safely (catch)
    try
    {
      out = json::parse(res->body);
    }
    catch (...)
    {
      std::cerr << "JSON parse error for: " << path << "\n";
      // treat as transient (rare), retry
      std::this_thread::sleep_for(std::chrono::milliseconds(200 * attempt));
      continue;
    }

    // success
    return res;
  }

  std::cerr << "requestJson: exhausted attempts for " << path << "\n";
  return {};
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

bool GitHubCrawler::passesFilters(json& r)
{
  // skip forks
  if (r.value("fork", false))
    return false;

  // skip templates
  if (r.value("is_template", false))
    return false;

  // Optional: topic-based template detection
  if (r.contains("topics") && r["topics"].is_array())
  {
    for (const auto& t : r["topics"])
      if (t.is_string() && t.get<std::string>() == "template")
        return false;
  }

  // skip archived
  if (r.value("archived", false))
    return false;

  // skip disabled repos (either field)
  if (r.value("disabled", false) || r.value("is_disabled", false))
    return false;

  // skip mirrors (boolean field - most reliable)
  if (r.value("mirror", false))
    return false;

  // skip mirrors by mirror_url (second most reliable)
  if (r.contains("mirror_url") && r["mirror_url"].is_string() && !r["mirror_url"].get<std::string>().empty())
  {
    return false;
  }

  // --- Heuristic mirror detection (GitHub does NOT flag these) ---

  // common mirror org names
  static const std::vector<std::string> mirrorOwners = {"aosp-mirror",   "llvm-mirror",  "gcc-mirror", "linux-mirror",
                                                        "webkit-mirror", "gnome-mirror", "kde-mirror"};

  // owner/name based detection
  if (r.contains("owner") && r["owner"].is_object())
  {
    std::string owner = r["owner"].value("login", "");

    for (const auto& m : mirrorOwners)
    {
      if (owner == m)
        return false;
    }
  }

  // name contains mirror markers
  std::string name = r.value("name", "");
  std::string full = r.value("full_name", "");

  for (const auto& m : mirrorOwners)
  {
    if (full.starts_with(m))
      return false;
  }

  auto containsMirror = [](const std::string& s)
  {
    return s.find("mirror") != std::string::npos || s.find("-mirror") != std::string::npos || s.find("_mirror") != std::string::npos;
  };

  if (containsMirror(name) || containsMirror(full))
    return false;

  // only public visibility (if field exists)
  if (r.contains("visibility") && r["visibility"].is_string())
  {
    if (r["visibility"].get<std::string>() != "public")
      return false;
  }

  // skip empty repos where size truly indicates no content
  if (r.contains("size") && r["size"].is_number_integer() && r["size"].get<int>() == 0 && r.value("stargazers_count", 0) == 0 && r.value("forks_count", 0) == 0)
  {
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

void GitHubCrawler::parseRefsArray(const json& refsArray, RepoInfo& info, std::vector<pair<std::string, std::string>>& unresolvedTagObjects)
{
  if (!refsArray.is_array())
    return;

  for (const auto& r : refsArray)
  {
    if (!r.contains("ref") || r["ref"].is_null())
      continue;

    std::string refStr = r["ref"].get<std::string>();

    std::string objectSha;
    std::string objectType;
    if (r.contains("object") && r["object"].is_object())
    {
      if (r["object"].contains("sha") && !r["object"]["sha"].is_null())
        objectSha = r["object"]["sha"].get<std::string>();
      if (r["object"].contains("type") && !r["object"]["type"].is_null())
        objectType = r["object"]["type"].get<std::string>();
    }

    const std::string headsPrefix = "refs/heads/";
    const std::string tagsPrefix = "refs/tags/";

    if (refStr.rfind(headsPrefix, 0) == 0)
    {
      BranchInfo bi;
      bi.name = refStr.substr(headsPrefix.size());
      bi.commitSha = objectSha; // should be commit
      info.branches.push_back(bi);
    }
    else if (refStr.rfind(tagsPrefix, 0) == 0)
    {
      TagInfo ti;
      ti.name = refStr.substr(tagsPrefix.size());
      // If objectType == "commit" this is a lightweight tag (commit SHA).
      // If objectType == "tag" this is an annotated tag; we need to resolve the tag object to the commit SHA.
      if (objectType == "tag")
      {
        // push unresolved pair (tag object sha, tag name) to be resolved later
        unresolvedTagObjects.emplace_back(objectSha, ti.name);
        ti.commitSha.clear(); // placeholder
      }
      else
      {
        // commit or unknown -> use as-is
        ti.commitSha = objectSha;
      }
      info.tags.push_back(ti);
    }
  }
}

// -----------------------------------------------------------------------------
// Resolve an annotated tag object (git tag object) to its target commit SHA
// Uses GET /repos/:owner/:repo/git/tags/:tag_sha
// -----------------------------------------------------------------------------
static std::string resolveAnnotatedTagCommitSha(GitHubCrawler* crawler, const std::string& owner, const std::string& repo, const std::string& tagObjectSha)
{
  if (tagObjectSha.empty())
    return "";

  std::string path = "/repos/" + owner + "/" + repo + "/git/tags/" + tagObjectSha;
  json tagObj;
  auto res = crawler->requestJson(path, tagObj);
  if (!res)
    return "";

  // tagObj should contain "object": { "sha": "...", "type": "commit" }
  if (tagObj.contains("object") && tagObj["object"].is_object())
  {
    if (tagObj["object"].contains("sha") && !tagObj["object"]["sha"].is_null())
      return tagObj["object"]["sha"].get<std::string>();
  }

  return "";
}

// -----------------------------------------------------------------------------
// Fetch refs (branches + tags) and full details (REST-based) - Option A
// -----------------------------------------------------------------------------

void GitHubCrawler::fetchRefsAndHead(RepoInfo& info)
{
  if (info.fullName.empty())
    return;

  // split "owner/repo"
  auto pos = info.fullName.find('/');
  if (pos == std::string::npos)
    return;
  std::string owner = info.fullName.substr(0, pos);
  std::string name = info.fullName.substr(pos + 1);

  info.branches.clear();
  info.tags.clear();

  // 1) GET /repos/<owner>/<repo>/git/refs?per_page=100 (returns array) and page through Link:
  std::string path = "/repos/" + owner + "/" + name + "/git/refs?per_page=100";

  // We'll collect annotated tag objects that need resolution: pair(tagObjectSha, tagName)
  std::vector<std::pair<std::string, std::string>> unresolvedTagObjects;

  while (!path.empty())
  {
    json refsJson;
    auto res = requestJson(path, refsJson);
    if (!res)
      break; // bail out but keep whatever refs we already collected

    // parse refs array, add any annotated tag objects to unresolvedTagObjects
    parseRefsArray(refsJson, info, unresolvedTagObjects);

    // parse Link: header for next
    std::string linkHeader = res->get_header_value("Link");
    std::string nextUrl;
    if (!linkHeader.empty())
    {
      std::regex re("<([^>]+)>;\\s*rel=\"next\"");
      std::smatch m;
      if (std::regex_search(linkHeader, m, re) && m.size() > 1)
      {
        nextUrl = m[1];
      }
    }

    if (nextUrl.empty())
      break;

    const std::string apiPrefix = "https://api.github.com";
    if (nextUrl.rfind(apiPrefix, 0) == 0)
      nextUrl = nextUrl.substr(apiPrefix.length());

    path = nextUrl;

    // small pause between pages
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
  }

  // 2) Resolve annotated tag objects (if any)
  if (!unresolvedTagObjects.empty())
  {
    for (const auto& p : unresolvedTagObjects)
    {
      const std::string& tagObjSha = p.first;
      const std::string& tagName = p.second;

      std::string commitSha = resolveAnnotatedTagCommitSha(this, owner, name, tagObjSha);
      if (commitSha.empty())
      {
        // resolution failed; leave tag's commitSha empty
        continue;
      }

      // find tag in info.tags and set commitSha
      for (auto& t : info.tags)
      {
        if (t.name == tagName && t.commitSha.empty())
        {
          t.commitSha = commitSha;
          break;
        }
      }
    }
  }

  // 3) Fetch full details (license + default_branch) - one REST call
  json detailJson;
  if (fetchRepoDetails(info.fullName, detailJson))
  {
    fillBasicRepoInfoMetadataOnly(detailJson, info);
  }

  // 4) set headSha by matching headBranch with branches
  info.headSha.clear();
  for (const auto& b : info.branches)
  {
    if (b.name == info.headBranch)
    {
      info.headSha = b.commitSha;
      break;
    }
  }

  // fallback: prefer main then master
  if (info.headSha.empty() && !info.branches.empty())
  {
    for (const auto& b : info.branches)
    {
      if (b.name == "main")
      {
        info.headSha = b.commitSha;
        break;
      }
    }

    if (info.headSha.empty())
    {
      for (const auto& b : info.branches)
      {
        if (b.name == "master")
        {
          info.headSha = b.commitSha;
          break;
        }
      }
    }
  }
}