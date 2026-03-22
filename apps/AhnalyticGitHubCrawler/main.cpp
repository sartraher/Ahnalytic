
#include "AhnalyticBase/database/GitHubRepoDatabase.hpp"
#include "AhnalyticBase/github/RateLimit.hpp"
#include "AhnalyticBase/helper/DataHelper.hpp"
#include "AhnalyticBase/helper/Enviroment.hpp"
#include "AhnalyticBase/helper/GitCliHelper.hpp"
#include "AhnalyticBase/helper/ThreadSafeQueue.hpp"

#include "BS_thread_pool.hpp"

#include "args/args.hxx"

#include <condition_variable>
#include <fstream>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <unordered_map>

class Crawler
{
public:
  Crawler(const std::string& token) : client("api.github.com", 443)
  {
    if (!token.empty())
    {
      personalAccessToken = token;
    }
    else
    {
      EnviromentC env;
      personalAccessToken = env.gitHubPAT;
    }

    // SECURITY: enable certificate verification (was false)
    client.enable_server_certificate_verification(true);

    // sensible timeouts (connection, read)
    client.set_connection_timeout(5, 0); // 5s connect
    client.set_read_timeout(30, 0);      // 30s read

    // Remove again after in an normal network again
    client.enable_server_certificate_verification(false);
  }

  httplib::Result requestJson(const std::string& path, json& out)
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

  void handleRateLimit(const httplib::Result& res)
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

  void fillBasicRepoInfo(const json& r, RepoInfo& info)
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

    if (r.contains("created_at") && !r["created_at"].is_null())
      std::cout << r["created_at"].get<std::string>();

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

  bool passesFilters(json& r)
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
    if (r.contains("size") && r["size"].is_number_integer() && r["size"].get<int>() == 0 && r.value("stargazers_count", 0) == 0 &&
        r.value("forks_count", 0) == 0)
    {
      return false;
    }

    return true;
  }

  std::string cleanFileName(const std::string& name)
  {
    static const std::string illegal = "\\/:*?\"<>|";
    std::string out;
    out.reserve(name.size());

    for (char c : name)
    {
      if (static_cast<unsigned char>(c) < 32)
        continue;
      if (illegal.find(c) != std::string::npos)
        out.push_back('_');
      else
        out.push_back(c);
    }

    auto trim = [](std::string& s)
    {
      auto is_bad = [](char c)
      {
        return c == ' ' || c == '.' || c == '\t';
      };
      s.erase(s.begin(), std::find_if(s.begin(), s.end(), [&](char c) { return !is_bad(c); }));
      if (s.empty())
        return;
      s.erase(std::find_if(s.rbegin(), s.rend(), [&](char c) { return !is_bad(c); }).base(), s.end());
    };
    trim(out);

    if (out.empty())
      out = "_";

    static const std::vector<std::string> reserved = {"CON",  "PRN",  "AUX",  "NUL",  "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7",
                                                      "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"};

    std::string upper;
    upper.reserve(out.size());
    std::transform(out.begin(), out.end(), std::back_inserter(upper), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    if (std::find(reserved.begin(), reserved.end(), upper) != reserved.end())
      out = "_" + out;

    return out;
  }

  std::string extractOwnerRepo(const std::string& url) const
  {
    std::regex rgx(R"(github\.com/([^/]+)/([^/]+)(?:\.git)?)", std::regex::icase);
    std::smatch match;
    if (!std::regex_search(url, match, rgx) || match.size() < 3)
      return "";
    return match[1].str() + "/" + match[2].str();
  }

private:
  RateLimiter rateLimiter;
  httplib::SSLClient client;
  std::string personalAccessToken;
};

int main(int argc, char* argv[])
{
  GitHubRepoDatabase db(DBType::SQLite, "E:/db/github.db");

  EnviromentC env;

  bool finished = false;
  ThreadSafeQueue<RepoInfo> queue;
  ThreadSafeQueue<RepoInfo> outQueue;
  static bool stopFlag = false;

  BS::thread_pool pool;

  Crawler crawler(env.gitHubPAT);

  std::jthread producer([&queue, &finished, &db, &crawler]
  {
    unsigned long long since = db.getLastSinceId();

    // We'll fetch pages and follow Link: rel="next" if present.
    std::string nextPath = "/repositories?since=" + std::to_string(since);

    while (!nextPath.empty())
    {
      json repos;
      auto res = crawler.requestJson(nextPath, repos);
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

        if (!crawler.passesFilters(repoJson))
          continue;

        RepoInfo info;
        crawler.fillBasicRepoInfo(repoJson, info);

        queue.push(info);

        while (queue.size() > 100)
          std::this_thread::sleep_for(std::chrono::seconds{1});
      }

      // persist progress for this page
      db.setLastSinceId(maxIdInPage);
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

      if (nextUrl.empty() || stopFlag)
      {
        // no more pages
        break;
      }

      // GitHub Link URLs are absolute; convert to path for httplib if necessary
      const std::string apiPrefix = "https://api.github.com";
      if (nextUrl.rfind(apiPrefix, 0) == 0)
        nextUrl = nextUrl.substr(apiPrefix.length());

      nextPath = nextUrl;
    }
  });

  while (!queue.empty() || !finished)
  {
    pool.detach_task([&queue, &outQueue, env, &crawler]()
    {
      if (queue.empty())
      {
        std::this_thread::sleep_for(std::chrono::seconds{1});
      }
      else
      {
        RepoInfo info = queue.wait_and_pop();
        // std::cout << info.fullName << std::endl;

        std::string headSha = GitCliHelperC::getHeadSha(info.htmlUrl, env.workFolder.string());

        // Tags
        if (info.tags.size() == 0)
        {
          std::vector<TagData> tagData = GitCliHelperC::getGitTagData(info.htmlUrl, env.workFolder.string());
          for (const TagData& tag : tagData)
            info.tags.push_back({tag.name, tag.sha});

          if (info.tags.size() == 0)
            info.tags.push_back({"HEAD", headSha});
        }

        std::string repoPath;
        std::vector<std::string> files;

        // Language
        if (info.language == "")
        {
          std::string name = crawler.cleanFileName(crawler.extractOwnerRepo(info.htmlUrl));
          repoPath = GitCliHelperC::getGitCloneShallow(name, info.htmlUrl, env.workFolder.string());
          files = GitCliHelperC::getGitFiles(name, info.htmlUrl, env.workFolder.string());

          if (files.size() == 0)
            return;

          std::unordered_map<std::string, int> langCount;
          for (const std::string& file : files)
          {
            std::string lang = DataHelperC::getFormatName(std::filesystem::path(file).extension().string());
            if (!lang.empty())
            {
              if (langCount.find(lang) == langCount.end())
                langCount[lang] = 0;

              langCount[lang]++;
            }
          }

          std::string maxLang;
          int maxCount = 0;
          for (auto iter = langCount.begin(); iter != langCount.end(); iter++)
          {
            if (iter->second > maxCount)
            {
              maxCount = iter->second;
              maxLang = iter->first;
            }
          }

          info.language = maxLang;

          // std::cout << maxLang << std::endl;
        }

        // License
        if (info.license == "" || info.license == "NO LICENSE")
        {
          std::string name = crawler.cleanFileName(crawler.extractOwnerRepo(info.htmlUrl));
          if (repoPath.size() == 0)
            repoPath = GitCliHelperC::getGitCloneShallow(name, info.htmlUrl, env.workFolder.string());

          if (files.size() == 0)
            files = GitCliHelperC::getGitFiles(name, info.htmlUrl, env.workFolder.string());

          auto toUpper = [](std::string s) -> std::string
          {
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });
            return s;
          };

          auto contains = [](std::string_view str, std::string_view sub)
          {
            return str.find(sub) != std::string_view::npos;
          };

          std::string licenseMain;
          std::string licenseSecond;
          for (const std::string& file : files)
          {
            std::string upperFile = toUpper(file);

            if (upperFile.starts_with("LICENSE") || upperFile.starts_with("LICENSE."))
              licenseMain = file;
            else if (contains(upperFile, "LICENSE"))
              licenseSecond = file;
          }

          std::string license;
          if (licenseMain.size() > 0)
            license = licenseMain;
          else if (licenseSecond.size() > 0)
            license = licenseSecond;

          if (license != "")
          {
            std::unordered_map<std::string, std::string> fileData = GitCliHelperC::getFilesWithContent(repoPath, headSha, {license});
            info.license = DataHelperC::getLicenceName(fileData[license], env.workFolder.string());
          }
        }

        if (repoPath != "")
        {
          std::cout << GitCliHelperC::getCreationDate(repoPath, env.workFolder.string());

          std::error_code ec;
          std::filesystem::remove_all(repoPath, ec);
        }

        outQueue.push(info);
      }
    });

    while (!outQueue.empty())
    {
      RepoInfo info = outQueue.wait_and_pop();
      db.upsertRepo(info);
    }

    while (pool.get_tasks_total() > 100)
    {
      std::this_thread::sleep_for(std::chrono::seconds{1});
    }
  }

  while (!outQueue.empty())
  {
    RepoInfo info = outQueue.wait_and_pop();
    db.upsertRepo(info);
  }
}