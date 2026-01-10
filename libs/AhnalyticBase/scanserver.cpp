#include "scanserver.hpp"
#include "ScanDatabase.hpp"
#include "TreeSearch.hpp"
#include "enviroment.hpp"

#define CPPHTTPLIB_NO_MULTIPART_FORM_DATA
#define CPPHTTPLIB_OPENSSL_SUPPORT
#define CPPHTTPLIB_ZLIB_SUPPORT
#include <httplib.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "BS_thread_pool.hpp"

#include <archive.h>
#include <archive_entry.h>

class ScanServerPrivate
{
public:
  httplib::Server server;
  EnviromentC env;
  ScanDatabase* scanDatabase = nullptr;

  std::atomic<bool> inUpdate = false;
  std::jthread timerScanThread;

  BS::thread_pool<BS::tp::none> pool;
};

static void set_cors_headers(httplib::Response& res)
{
  res.set_header("Access-Control-Allow-Origin", "*");
  res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
  res.set_header("Access-Control-Max-Age", "3600");
}

static void bad_request(httplib::Response& res, const std::string& msg)
{
  res.status = 400;
  res.set_content(json{{"error", msg}}.dump(), "application/json");
  set_cors_headers(res);
}

static void ok(httplib::Response& res, const json& body)
{
  res.status = 200;
  res.set_content(body.dump(), "application/json");
  set_cors_headers(res);
}

ScanServer::ScanServer() : priv(new ScanServerPrivate())
{
  priv->scanDatabase = new ScanDatabase(/*DBType::SQLite, (priv->env.dataFolder / "scanData.db").string(),*/ priv->env.scanFolder);

  init();
}

ScanServer::~ScanServer()
{
  delete priv;
}

void ScanServer::init()
{
  /* ===================== CORS ===================== */

  priv->server.Options(R"(/.*)", [&](const httplib::Request&, httplib::Response& res)
  {
    set_cors_headers(res);
    res.status = 204;
  });

  /* ===================== GROUPS ===================== */

  priv->server.Post("/groups", [&](const httplib::Request& req, httplib::Response& res)
  {
    auto body = json::parse(req.body, nullptr, false);
    if (!body.contains("name"))
      return bad_request(res, "Missing 'name'");

    size_t id = priv->scanDatabase->createGroup(body["name"]);
    ok(res, {{"id", id}});
  });

  priv->server.Get("/groups", [&](const httplib::Request&, httplib::Response& res)
  {
    json result;
    for (auto& [id, name] : priv->scanDatabase->getGroups())
      result[std::to_string(id)] = name;
    ok(res, result);
  });

  priv->server.Put(R"(/groups/(\d+))", [&](const httplib::Request& req, httplib::Response& res)
  {
    auto body = json::parse(req.body, nullptr, false);
    if (!body.contains("name"))
      return bad_request(res, "Missing 'name'");

    priv->scanDatabase->editGroup(std::stoull(req.matches[1]), body["name"]);
    ok(res, {{"status", "updated"}});
  });

  priv->server.Delete(R"(/groups/(\d+))", [&](const httplib::Request& req, httplib::Response& res)
  {
    priv->scanDatabase->removeGroup(std::stoull(req.matches[1]));
    ok(res, {{"status", "deleted"}});
  });

  /* ===================== PROJECTS ===================== */

  priv->server.Post(R"(/groups/(\d+)/projects)", [&](const httplib::Request& req, httplib::Response& res)
  {
    auto body = json::parse(req.body, nullptr, false);
    if (!body.contains("name"))
      return bad_request(res, "Missing 'name'");

    size_t id = priv->scanDatabase->createProject(body["name"], std::stoull(req.matches[1]));

    if (id == -1)
      bad_request(res, "Error while resolving Ids");
    else
      ok(res, {{"id", id}});
  });

  priv->server.Get(R"(/groups/(\d+)/projects)", [&](const httplib::Request& req, httplib::Response& res)
  {
    json result;
    for (auto& [id, name] : priv->scanDatabase->getProjects(std::stoull(req.matches[1])))
      result[std::to_string(id)] = name;
    ok(res, result);
  });

  priv->server.Put(R"(/groups/(\d+)/projects/(\d+))", [&](const httplib::Request& req, httplib::Response& res)
  {
    auto body = json::parse(req.body, nullptr, false);
    if (!body.contains("name"))
      return bad_request(res, "Missing 'name'");

    priv->scanDatabase->editProject(std::stoull(req.matches[2]), std::stoull(req.matches[1]), body["name"]);

    ok(res, {{"status", "updated"}});
  });

  priv->server.Delete(R"(/groups/(\d+)/projects/(\d+))", [&](const httplib::Request& req, httplib::Response& res)
  {
    priv->scanDatabase->removeProject(std::stoull(req.matches[2]), std::stoull(req.matches[1]));

    ok(res, {{"status", "deleted"}});
  });

  /* ===================== VERSIONS ===================== */

  priv->server.Post(R"(/groups/(\d+)/projects/(\d+)/versions)", [&](const httplib::Request& req, httplib::Response& res)
  {
    auto body = json::parse(req.body, nullptr, false);
    if (!body.contains("name"))
      return bad_request(res, "Missing 'name'");

    size_t id = priv->scanDatabase->createVersion(body["name"], std::stoull(req.matches[1]), std::stoull(req.matches[2]));

    if (id == -1)
      bad_request(res, "Error while resolving Ids");
    else
      ok(res, {{"id", id}});
  });

  priv->server.Get(R"(/groups/(\d+)/projects/(\d+)/versions)", [&](const httplib::Request& req, httplib::Response& res)
  {
    json result;
    for (auto& [id, name] : priv->scanDatabase->getVersions(std::stoull(req.matches[1]), std::stoull(req.matches[2])))
      result[std::to_string(id)] = name;

    ok(res, result);
  });

  priv->server.Put(R"(/groups/(\d+)/projects/(\d+)/versions/(\d+))", [&](const httplib::Request& req, httplib::Response& res)
  {
    auto body = json::parse(req.body, nullptr, false);
    if (!body.contains("name"))
      return bad_request(res, "Missing 'name'");

    priv->scanDatabase->editVersion(std::stoull(req.matches[3]), std::stoull(req.matches[1]), std::stoull(req.matches[2]), body["name"]);

    ok(res, {{"status", "updated"}});
  });

  priv->server.Delete(R"(/groups/(\d+)/projects/(\d+)/versions/(\d+))", [&](const httplib::Request& req, httplib::Response& res)
  {
    priv->scanDatabase->removeVersion(std::stoull(req.matches[3]), std::stoull(req.matches[1]), std::stoull(req.matches[2]));

    ok(res, {{"status", "deleted"}});
  });

  /* ===================== SCANS ===================== */

  priv->server.Post(R"(/groups/(\d+)/projects/(\d+)/versions/(\d+)/scans)", [&](const httplib::Request& req, httplib::Response& res)
  {
    auto body = json::parse(req.body, nullptr, false);
    if (!body.contains("name"))
      return bad_request(res, "Missing 'name'");

    size_t id = priv->scanDatabase->createScan(body["name"], std::stoull(req.matches[1]), std::stoull(req.matches[2]), std::stoull(req.matches[3]));

    if (id == -1)
      bad_request(res, "Error while resolving Ids");
    else
      ok(res, {{"id", id}});
  });

  priv->server.Get(R"(/groups/(\d+)/projects/(\d+)/versions/(\d+)/scans)", [&](const httplib::Request& req, httplib::Response& res)
  {
    json result;
    for (auto& [id, name] : priv->scanDatabase->getScans(std::stoull(req.matches[1]), std::stoull(req.matches[2]), std::stoull(req.matches[3])))
      result[std::to_string(id)] = name;

    ok(res, result);
  });

  /* ===================== START / ABORT ===================== */

  priv->server.Post(R"(/groups/(\d+)/projects/(\d+)/versions/(\d+)/scans/(\d+)/start)", [&](const httplib::Request& req, httplib::Response& res)
  {
    const size_t groupId = std::stoull(req.matches[1]);
    const size_t projectId = std::stoull(req.matches[2]);
    const size_t versionId = std::stoull(req.matches[3]);
    const size_t scanId = std::stoull(req.matches[4]);

    priv->scanDatabase->startScan(scanId, groupId, projectId, versionId, scanId);

    ok(res, {{"status", "started"}});
  });

  priv->server.Post(R"(/groups/(\d+)/projects/(\d+)/versions/(\d+)/scans/(\d+)/abort)", [&](const httplib::Request& req, httplib::Response& res)
  {
    const size_t groupId = std::stoull(req.matches[1]);
    const size_t projectId = std::stoull(req.matches[2]);
    const size_t versionId = std::stoull(req.matches[3]);
    const size_t scanId = std::stoull(req.matches[4]);

    priv->scanDatabase->abortScan(scanId, groupId, projectId, versionId, scanId);

    ok(res, {{"status", "aborted"}});
  });

  priv->server.Get(R"(/groups/(\d+)/projects/(\d+)/versions/(\d+)/scans/(\d+)/info)", [&](const httplib::Request& req, httplib::Response& res)
  {
    const size_t groupId = std::stoull(req.matches[1]);
    const size_t projectId = std::stoull(req.matches[2]);
    const size_t versionId = std::stoull(req.matches[3]);
    const size_t scanId = std::stoull(req.matches[4]);

    ScanData* scanData = priv->scanDatabase->getScan(scanId, groupId, projectId, versionId, scanId);

    if (scanData != nullptr)
    {
      std::vector<TreeSearchResult> searchResults = scanData->getResult();

      // TODO: move inside of scandata
      const std::lock_guard<std::recursive_mutex> lock(scanData->mutex);

      json ret;
      ret["status"] = scanData->status;
      ret["id"] = scanData->id;
      ret["name"] = scanData->name;
      ret["id"] = scanData->id;

      json results;
      for (const TreeSearchResult& searchResult : searchResults)
      {
        if (searchResult)
        {
          json result;

          result["sourceDb"] = searchResult.sourceDb;
          result["sourceFile"] = searchResult.sourceFile;
          result["sourceRevision"] = searchResult.sourceRevision;
          result["sourceInternalId"] = searchResult.sourceInternalId;
          result["searchFile"] = searchResult.searchFile;

          for (const TreeSearchResultSet& searchResultSet : searchResult)
          {
            json resultSet;

            resultSet["baseStart"] = searchResultSet.baseStart;
            resultSet["baseEnd"] = searchResultSet.baseEnd;
            resultSet["searchStart"] = searchResultSet.searchStart;
            resultSet["searchEnd"] = searchResultSet.searchEnd;

            result["resultSets"].push_back(result);
          }

          results.push_back(result);
        }
      }

      ok(res, ret);
    }

    bad_request(res, "Scan not found");
  });

  using namespace std::chrono_literals;
  priv->timerScanThread = std::jthread([this](std::stop_token st)
  {
    while (!st.stop_requested())
    {
      // Only do it the last one is not still running
      if (!priv->inUpdate)
        updateScans();
      std::this_thread::sleep_for(1min);
    }
  });
}

void ScanServer::updateScans()
{
  priv->inUpdate = true;

  if (priv->pool.get_tasks_total() == 0)
  {
    ScanData* nextData = priv->scanDatabase->getNextScan();
    if (nextData != nullptr)
    {
      priv->pool.detach_task([this, nextData]()
      {
        nextData->setStatus(ScanDataStatusE::Running);

        // Unzip/Checkout Data
        std::string path;
        std::string revision;
        ScanDataTypeE type;
        nextData->getData(path, revision, type);

        if (nextData->isAborted())
          return;

        std::filesystem::path outPath = priv->env.scanFolder / "data";

        // Start Scan
        switch (type)
        {
        case ScanDataTypeE::Git:
          checkoutGitRevision(path, revision, outPath);
          break;
        case ScanDataTypeE::Svn:
          checkoutSvnRevision(path, revision, outPath);
          break;
        case ScanDataTypeE::Archive:
          extractArchive(priv->env.scanFolder / "data.zip", outPath);
          break;
        }

        if (nextData->isAborted())
          return;

        // First level scan
        TreeSearch treeSearch;
        treeSearch.search(outPath, priv->env, nextData);

        if (nextData->isAborted())
          return;

        // Deep scan on first level results
        treeSearch.searchDeep(outPath, priv->env, nextData);


        if (nextData->isAborted())
          return;

        // Just for consistency
        nextData->setStatus(ScanDataStatusE::Finished);
      });
    }
  }

  priv->inUpdate = false;
}

void ScanServer::checkoutGitRevision(const std::string& gitUrl, const std::string& sha, const std::filesystem::path& outputPath)
{
  if (!std::filesystem::exists(outputPath))
    std::filesystem::create_directories(outputPath);

  // Clone without checkout
  std::system(std::string("git clone --no-checkout " + gitUrl + " \"" + outputPath.string() + "\"").c_str());

  // Checkout specific revision
  std::system(std::string("cd \"" + outputPath.string() + "\" && git checkout " + sha).c_str());
}

void ScanServer::checkoutSvnRevision(const std::string& svnUrl, const std::string& revision, const std::filesystem::path& outputPath)
{
  if (!std::filesystem::exists(outputPath))
    std::filesystem::create_directories(outputPath);

  std::string cmd = "svn checkout ";
  if (!revision.empty())
    cmd += "-r " + revision + " ";

  cmd += svnUrl + " \"" + outputPath.string() + "\"";

  std::system(cmd.c_str());
}

void ScanServer::extractArchive(const std::filesystem::path& archivePath, const std::filesystem::path& outputPath)
{
  if (!std::filesystem::exists(outputPath))
    std::filesystem::create_directories(outputPath);

  struct archive* a = archive_read_new();
  struct archive* ext = archive_write_disk_new();
  struct archive_entry* entry;

  archive_read_support_format_all(a);
  archive_read_support_filter_all(a);

  archive_write_disk_set_options(ext, ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS);

  if (archive_read_open_filename(a, archivePath.string().c_str(), 10240) != ARCHIVE_OK)
    throw std::runtime_error(archive_error_string(a));

  while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
  {
    std::filesystem::path fullPath = outputPath / archive_entry_pathname(entry);
    archive_entry_set_pathname(entry, fullPath.string().c_str());

    if (archive_write_header(ext, entry) != ARCHIVE_OK)
      throw std::runtime_error(archive_error_string(ext));

    const void* buff;
    size_t size;
    la_int64_t offset;

    while (true)
    {
      int r = archive_read_data_block(a, &buff, &size, &offset);
      if (r == ARCHIVE_EOF)
        break;
      if (r != ARCHIVE_OK)
        throw std::runtime_error(archive_error_string(a));

      archive_write_data_block(ext, buff, size, offset);
    }
  }

  archive_read_close(a);
  archive_read_free(a);
  archive_write_close(ext);
  archive_write_free(ext);
}

void ScanServer::start(const std::string& addr, int port)
{
  priv->server.listen(addr, port);
}

void ScanServer::stop()
{
  priv->server.stop();
}
