#include "ScanServer.hpp"
#include "AhnalyticBase/database/ScanDatabase.hpp"
#include "AhnalyticBase/helper/Enviroment.hpp"
#include "AhnalyticBase/tree/TreeSearch.hpp"

// Remove multipart restriction to enable file uploads
// #define CPPHTTPLIB_NO_MULTIPART_FORM_DATA
#define CPPHTTPLIB_OPENSSL_SUPPORT
#define CPPHTTPLIB_ZLIB_SUPPORT
#include <httplib.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "BS_thread_pool.hpp"

#include <archive.h>
#include <archive_entry.h>
#include <fstream>

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

  priv->server.Post(R"(/groups/(\d+)/projects/(\d+)/versions/(\d+)/scans/(\d+)/upload)", [&](const httplib::Request& req, httplib::Response& res)
  {
    const size_t groupId = std::stoull(req.matches[1]);
    const size_t projectId = std::stoull(req.matches[2]);
    const size_t versionId = std::stoull(req.matches[3]);
    const size_t scanId = std::stoull(req.matches[4]);

    if (!req.form.has_file("file"))
      return bad_request(res, "Missing 'file' in multipart form data");

    httplib::FormData file = req.form.get_file("file");

    if (file.content.empty())
      return bad_request(res, "File content is empty");

    try
    {
      // Convert file content to vector<char>
      std::vector<char> fileData(file.content.begin(), file.content.end());

      // Add the zip data to the database
      priv->scanDatabase->addZipData(scanId, groupId, projectId, versionId, scanId, fileData);

      ok(res, {{"status", "uploaded"}, {"filename", file.filename}, {"size", fileData.size()}});
    }
    catch (const std::exception& e)
    {
      bad_request(res, std::string("Error processing file: ") + e.what());
    }
  });

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
      std::vector<TreeSearchResult> searchResults = scanData->getDeepResult();

      // TODO: move inside of scandata
      const std::lock_guard<std::recursive_mutex> lock(scanData->mutex);

      json ret;
      ret["status"] = scanData->status;
      ret["id"] = scanData->id;
      ret["name"] = scanData->name;

      ret["maxCount"] = scanData->getMaxCount();
      ret["finishedCount"] = scanData->getFinishedCount();

      json results = json::array();
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

          result["searchContent"] = searchResult.searchContent;
          result["sourceContent"] = searchResult.sourceContent;
          result["licence"] = searchResult.licence;          

          for (const TreeSearchResultSet& searchResultSet : searchResult)
          {
            json resultSet;

            resultSet["baseStart"] = searchResultSet.baseStart;
            resultSet["baseEnd"] = searchResultSet.baseEnd;
            resultSet["searchStart"] = searchResultSet.searchStart;
            resultSet["searchEnd"] = searchResultSet.searchEnd;

            result["resultSets"].push_back(resultSet);
          }

          results.push_back(result);
        }
      }
      ret["results"] = results;

      ok(res, ret);
      return;
    }

    bad_request(res, "Scan not found");
  });

  // Mount /public to ./www directory
  bool ret = priv->server.set_mount_point("/www", priv->env.webFolder.string());

  // Update Scans in timed thread
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
          extractArchive(nextData->dataPath, outPath);
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
  struct archive_entry* entry = nullptr;

  if (!a)
    throw std::runtime_error("Failed to allocate archive structure");

  try
  {
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);

    std::string archivePathStr = archivePath.string();
    if (archive_read_open_filename(a, archivePathStr.c_str(), 10240) != ARCHIVE_OK)
    {
      throw std::runtime_error(std::string("Failed to open archive: ") + archive_error_string(a));
    }

    int r;
    while ((r = archive_read_next_header(a, &entry)) == ARCHIVE_OK || r == ARCHIVE_WARN)
    {
      if (r != ARCHIVE_OK && r != ARCHIVE_WARN)
        break;

      const char* entryPath = archive_entry_pathname(entry);
      if (!entryPath)
        continue;

      std::filesystem::path fullPath = outputPath / entryPath;
      fullPath = fullPath.lexically_normal();

      // Skip entries that try to escape the output directory
      if (!std::filesystem::path(entryPath).is_relative())
        continue;

      // Check if path tries to escape output directory
      try
      {
        if (std::filesystem::relative(fullPath, outputPath).string().find("..") != std::string::npos)
          continue;
      }
      catch (...)
      {
        continue;
      }

      // Handle directories
      if (archive_entry_filetype(entry) == AE_IFDIR)
      {
        std::error_code ec;
        std::filesystem::create_directories(fullPath, ec);
        continue;
      }

      // Create parent directories
      std::filesystem::path parentPath = fullPath.parent_path();
      if (!parentPath.empty() && parentPath != outputPath)
      {
        std::error_code ec;
        std::filesystem::create_directories(parentPath, ec);
        if (ec && !std::filesystem::exists(parentPath))
          throw std::runtime_error("Failed to create directory: " + parentPath.string());
      }

      // Manually extract file data
      std::ofstream outFile(fullPath, std::ios::binary);
      if (!outFile)
        throw std::runtime_error("Failed to open file for writing: " + fullPath.string());

      const void* buff;
      size_t size;
      la_int64_t offset;

      while (true)
      {
        int r_data = archive_read_data_block(a, &buff, &size, &offset);
        if (r_data == ARCHIVE_EOF)
          break;
        if (r_data != ARCHIVE_OK && r_data != ARCHIVE_WARN)
          throw std::runtime_error(std::string("Failed to read archive data: ") + archive_error_string(a));

        if (size > 0)
        {
          outFile.write(static_cast<const char*>(buff), size);
          if (!outFile)
            throw std::runtime_error("Failed to write file data: " + fullPath.string());
        }
      }

      outFile.close();

      // Set file permissions if available (platform-specific)
#ifndef _WIN32
      mode_t mode = archive_entry_perm(entry);
      if (mode != 0)
      {
        std::error_code ec;
        std::filesystem::permissions(fullPath, static_cast<std::filesystem::perms>(mode), std::filesystem::perm_options::replace, ec);
      }
#endif
    }

    archive_read_close(a);
    archive_read_free(a);
  }
  catch (const std::exception&)
  {
    archive_read_close(a);
    archive_read_free(a);
    throw;
  }
}

void ScanServer::start(const std::string& addr, int port)
{
  priv->server.listen(addr, port);
}

void ScanServer::stop()
{
  priv->server.stop();
}
