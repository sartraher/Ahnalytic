
#include "AhnalyticBase/database/GitHubRepoDatabase.hpp"
#include "AhnalyticBase/github/RateLimit.hpp"
#include "AhnalyticBase/helper/CliHelper.hpp"
#include "AhnalyticBase/helper/DataHelper.hpp"
#include "AhnalyticBase/helper/Enviroment.hpp"
#include "AhnalyticBase/helper/ThreadSafeQueue.hpp"

#include "BS_thread_pool.hpp"

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <atomic>
#include <iostream>
#include <string>

using json = nlohmann::json;

class SourceForgeCrawler
{
public:
  SourceForgeCrawler(SourceForgeRepoDatabase& db) : client("sourceforge.net", 443), db(db)
  {
    client.enable_server_certificate_verification(true);
    client.set_connection_timeout(5, 0);
    client.set_read_timeout(30, 0);
  }

  void crawl(ThreadSafeQueue<RepoInfo>& queue)
  {
    int page = db.getLastSinceId(); // restore last progress
    if (page < 1)
      page = 1;

    while (true)
    {
      std::string path = "/directory/?page=" + std::to_string(page);

      auto res = client.Get(path.c_str());
      if (!res || res->status != 200)
      {
        std::cerr << "[SF] HTTP " << (res ? res->status : 0) << " on " << path << "\n";
        break;
      }

      std::vector<std::string> projectNames = parseProjectsFromHtml(res->body);
      if (projectNames.empty())
      {
        std::cout << "[SF] no more projects on page " << page << "\n";
        break;
      }

      for (const auto& name : projectNames)
      {
        processProject(name, queue);
      }

      // persist progress after this page
      db.setLastSinceId(page);

      page++;

      // simple backpressure
      while (queue.size() > 50)
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
  }

private:
  httplib::SSLClient client;
  SourceForgeRepoDatabase& db;

  // -------------------------
  // HTML parsing
  // -------------------------
  std::vector<std::string> parseProjectsFromHtml(const std::string& html)
  {
    std::vector<std::string> out;
    // look for href="/projects/{project}/"
    std::regex re(R"(<a[^>]+href=\"/projects/([a-zA-Z0-9_\-]+)/\")");
    auto begin = std::sregex_iterator(html.begin(), html.end(), re);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it)
    {
      std::string name = (*it)[1].str();
      if (std::find(out.begin(), out.end(), name) == out.end())
        out.push_back(name);
    }

    return out;
  }

  bool urlExists(const std::string& url)
  {
    std::string working = url;

    const std::string https = "https://";
    if (working.starts_with(https))
      working = working.substr(https.size());

    size_t slash = working.find('/');
    if (slash == std::string::npos)
      return false;

    std::string host = working.substr(0, slash);
    std::string path = working.substr(slash);

    httplib::SSLClient client(host.c_str(), 443);
    client.enable_server_certificate_verification(true);
    client.set_connection_timeout(5, 0);
    client.set_read_timeout(10, 0);

    // try HEAD first
    auto res = client.Head(path.c_str());

    if (res && (res->status == 200 || res->status == 301 || res->status == 302))
      return true;

    // fallback to lightweight GET
    res = client.Get(path.c_str());
    if (res && (res->status == 200 || res->status == 301 || res->status == 302))
      return true;

    return false;
  }

  void processProject(const std::string& name, ThreadSafeQueue<RepoInfo>& queue)
  {
    // std::string path = "/p/" + name + "/code/";

    std::string type = detectRepoType(name);
    if (type.empty())
      return;

    // auto res = client.Get(path.c_str());
    // if (!res || res->status != 200)
    // return;

    RepoInfo info;
    info.name = name;
    info.fullName = name;
    info.repoType = type;
    info.htmlUrl = "https://sourceforge.net/p/" + name + "/code/";
    info.cloneUrl = makeCloneUrl(name, type);

    queue.push(info);
  }

  std::string detectRepoType(const std::string& name)
  {
    if (urlExistsFast("git.code.sf.net", "/p/" + name + "/code"))
      return "git";

    if (urlExistsFast("svn.code.sf.net", "/p/" + name + "/code/"))
      return "svn";

    if (urlExistsFast("hg.code.sf.net", "/p/" + name + "/code"))
      return "hg";

    return "";
  }

  bool urlExistsFast(const std::string& host, const std::string& path)
  {
    httplib::SSLClient client(host.c_str(), 443);
    client.enable_server_certificate_verification(true);
    client.set_connection_timeout(3, 0);
    client.set_read_timeout(5, 0);

    auto res = client.Head(path.c_str());

    if (!res)
      return false;

    // Only accept strong positives
    if (res->status < 400)
      return true;

    return false;
  }

  std::string makeCloneUrl(const std::string& project, const std::string& type)
  {
    if (type == "git")
      return "https://git.code.sf.net/p/" + project + "/code";
    else if (type == "svn")
      return "https://svn.code.sf.net/p/" + project + "/code/";
    else if (type == "hg")
      return "https://hg.code.sf.net/p/" + project + "/code";

    return "";
  }
};

class SourceForgeProcessor
{
  EnviromentC env;

public:
  SourceForgeProcessor(EnviromentC& enviroment)
  {
    env = enviroment;
  }

  void process(RepoInfo info, ThreadSafeQueue<RepoInfo>& outQueue, CliHelperWrapper& cliHelper)
  {
    std::cout << "Start " << info.name << " Type " << info.repoType << std::endl;

    std::string headId = cliHelper.getHeadId(info.cloneUrl, env.workFolder.string());

    // Tags
    if (info.tags.size() == 0)
    {
      std::vector<TagData> tagData = cliHelper.getTagData(info.cloneUrl, env.workFolder.string());

      for (const TagData& tag : tagData)
        info.tags.push_back({tag.name, tag.sha});

      if (info.tags.size() == 0)
        info.tags.push_back({"HEAD", headId});
    }

    // std::string repoPath;
    std::vector<std::string> files;

    std::string name = DataHelperC::cleanFileName(info.name);
    files = cliHelper.getFiles(name, info.cloneUrl, env.workFolder.string());

    if (files.size() == 0)
    {
      std::cout << "No Files " << info.name << std::endl;
      return;
    }

    // Language
    if (info.language == "")
    {
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
        std::unordered_map<std::string, std::string> fileData =
            cliHelper.getFilesWithContent(cliHelper.repoPath, info.cloneUrl, headId, {license}, env.workFolder.string());
        info.license = DataHelperC::getLicenceName(fileData[license], env.workFolder.string());
      }
    }

    // if (cliHelper.repoPath != "")
    {
      std::cout << cliHelper.getCreationDate(info.cloneUrl, env.workFolder.string()) << std::endl;

      std::error_code ec;
      std::filesystem::remove_all(cliHelper.repoPath, ec);
    }

    std::cout << "End " << info.name << std::endl;

    outQueue.push(info);
  }

private:
protected:
};

int main(int argc, char* argv[])
{
  SourceForgeRepoDatabase db(DBType::SQLite, "E:/db/sourceforge.db");

  EnviromentC env;

  bool finished = false;
  ThreadSafeQueue<RepoInfo> queue;
  ThreadSafeQueue<RepoInfo> outQueue;

  BS::thread_pool pool;

  SourceForgeCrawler crawler(db);

  std::jthread producer([&]() { crawler.crawl(queue); });

  SourceForgeProcessor processor(env);

  while (!queue.empty() || !finished)
  {
    pool.detach_task([&queue, &outQueue, env, &processor]()
    {
      if (queue.empty())
      {
        std::this_thread::sleep_for(std::chrono::seconds{1});
      }
      else
      {
        RepoInfo info = queue.wait_and_pop();
        CliHelperWrapper wrapper(info.repoType);
        processor.process(info, outQueue, wrapper);
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