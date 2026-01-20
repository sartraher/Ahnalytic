#include "AhnalyticBase/server/UpdateServer.hpp"
#include "AhnalyticBase/helper/Enviroment.hpp"

#include "AhnalyticBase/database/FileDatabase.hpp"
#include "AhnalyticBase/github/Github.hpp"
#include "AhnalyticBase/github/GithubCrawler.hpp"

#define CPPHTTPLIB_NO_MULTIPART_FORM_DATA
#define CPPHTTPLIB_OPENSSL_SUPPORT
#define CPPHTTPLIB_ZLIB_SUPPORT
#include <httplib.h>

#include <nlohmann/json.hpp>

#include "BS_thread_pool.hpp"

#include <atomic>
#include <thread>

class UpdateServerPrivate
{
public:
  httplib::Server server;
  EnviromentC env;
  BS::thread_pool<BS::tp::none> pool;

  std::atomic<bool> inDbBaseUpdate = false;
  std::atomic<bool> inDbScanUpdate = false;
  std::jthread timerScanThread;
};

UpdateServer::UpdateServer() : priv(new UpdateServerPrivate())
{
}

UpdateServer::~UpdateServer()
{
  delete priv;
}

void UpdateServer::init()
{
  priv->server.Get("/updateSince", [&](const httplib::Request& req, httplib::Response& res)
  {
    if (req.has_param("since"))
    {
      auto since = req.get_param_value("since");
    }

    // auto user_id = req.path_params.at("id");
    // res.set_content(user_id, "text/plain");
  });

  using namespace std::chrono_literals;
  priv->timerScanThread = std::jthread([this](std::stop_token st)
  {
    while (!st.stop_requested())
    {
      // Only do it the last one is not still running
      if (!priv->inDbBaseUpdate)
        updateGitHubBaseDatabase();
      std::this_thread::sleep_for(30min);
    }
  });

  priv->timerScanThread = std::jthread([this](std::stop_token st)
  {
    while (!st.stop_requested())
    {
      // Only do it the last one is not still running and the current poll is running low
      if (!priv->inDbScanUpdate && priv->pool.get_tasks_total() < 100)
        scanGitHubRepos();
      std::this_thread::sleep_for(30min);
    }
  });
}

void UpdateServer::start()
{
  priv->server.listen(priv->env.updateServerAddr, priv->env.updateServerPort);
}

void UpdateServer::stop()
{
  priv->server.stop();
}

void UpdateServer::updateGitHubBaseDatabase()
{
  priv->inDbBaseUpdate = true;
  GitHubCrawler crawler;
  crawler.crawlIncremental((priv->env.dbFolder / "/github.db").string());
  priv->inDbBaseUpdate = false;
}

void UpdateServer::scanGitHubRepos()
{
  std::filesystem::path basePath = (priv->env.dbFolder / "/github.db");
  std::filesystem::path scanPath = (priv->env.dbFolder / "base/github/github.db");

  if (std::filesystem::last_write_time(basePath) == std::filesystem::last_write_time(scanPath))
    return;

  priv->inDbScanUpdate = true;

  std::filesystem::copy(basePath, scanPath);

  GitHubHandler handler(priv->env.dbFolder.string(), priv->env.workFolder.string());

  std::vector<std::string> langFilter{"C", "C++"};

  GitHubRepoDatabase db(DBType::SQLite, scanPath.string());
  db.processRepos(langFilter, true, false, [&handler, this](RepoInfo info)
  {
    std::filesystem::path resPath = priv->env.dbFolder;
    resPath = resPath.concat("/CPP").concat("/github/").concat(handler.cleanFileName(info.fullName) + "_CPP.db");

    std::filesystem::path lastPath = priv->env.dbFolder;
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

    resPath = priv->env.dbFolder;
    resPath = resPath.concat("/CPP").concat("/github/").concat(handler.cleanFileName(info.fullName) + "_CPP.empty");

    if (std::filesystem::exists(resPath))
      return;

    priv->pool.detach_task([&handler, info]() { handler.scanRepo(info); });
  });

  priv->inDbScanUpdate = false;
}