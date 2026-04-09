#include "ScanServer.hpp"
#include "AhnalyticBase/database/ScanDatabase.hpp"
#include "AhnalyticBase/helper/Enviroment.hpp"
#include "AhnalyticBase/helper/Logger.hpp"
#include "AhnalyticBase/server/UpdateManager.hpp"
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

static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        "abcdefghijklmnopqrstuvwxyz"
                                        "0123456789+/";

std::string base64_encode(const std::string& input)
{
  std::string output;
  int val = 0;
  int valb = -6;

  for (unsigned char c : input)
  {
    val = (val << 8) + c;
    valb += 8;
    while (valb >= 0)
    {
      output.push_back(base64_chars[(val >> valb) & 0x3F]);
      valb -= 6;
    }
  }

  if (valb > -6)
  {
    output.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
  }

  while (output.size() % 4)
  {
    output.push_back('=');
  }

  return output;
}

class ScanServerPrivate
{
public:
  ScanServerPrivate() : logger(env)
  {
  }

  httplib::Server server;
  EnviromentC env;
  LoggerC logger;
  ScanDatabase* scanDatabase = nullptr;
  UpdateManager* updateManager = nullptr;

  std::atomic<bool> inUpdate = false;
  std::atomic<bool> updateDatabase = false;
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
  priv->updateManager = new UpdateManager(&priv->env);

  init();
}

ScanServer::~ScanServer()
{
  delete priv->updateManager;
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
    {
      LoggerC::LogError("Group creation failed: missing name");
      return bad_request(res, "Missing 'name'");
    }

    size_t id = priv->scanDatabase->createGroup(body["name"]);

    LoggerC::LogInfo(std::format("Group {} created", std::string(body["name"])));

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
    {
      LoggerC::LogError("Group renaming failed: missing name");
      return bad_request(res, "Missing 'name'");
    }

    priv->scanDatabase->editGroup(std::stoull(req.matches[1]), body["name"]);

    LoggerC::LogInfo(std::format("Group named to {}", std::string(body["name"])));
    ok(res, {{"status", "updated"}});
  });

  priv->server.Delete(R"(/groups/(\d+))", [&](const httplib::Request& req, httplib::Response& res)
  {
    priv->scanDatabase->removeGroup(std::stoull(req.matches[1]));
    LoggerC::LogInfo("Group deleted");
    ok(res, {{"status", "deleted"}});
  });

  /* ===================== PROJECTS ===================== */

  priv->server.Post(R"(/groups/(\d+)/projects)", [&](const httplib::Request& req, httplib::Response& res)
  {
    auto body = json::parse(req.body, nullptr, false);
    if (!body.contains("name"))
    {
      LoggerC::LogError("Project creation failed: missing name");
      return bad_request(res, "Missing 'name'");
    }

    size_t id = priv->scanDatabase->createProject(body["name"], std::stoull(req.matches[1]));

    if (id == -1)
    {
      LoggerC::LogError("Project creation failed: Error while resolving Ids");
      bad_request(res, "Error while resolving Ids");
    }
    else
    {
      LoggerC::LogInfo(std::format("Project {} created", std::string(body["name"])));
      ok(res, {{"id", id}});
    }
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
    {
      LoggerC::LogError("Project renaming failed: missing name");
      return bad_request(res, "Missing 'name'");
    }

    priv->scanDatabase->editProject(std::stoull(req.matches[2]), std::stoull(req.matches[1]), body["name"]);

    LoggerC::LogInfo(std::format("Project {} renamed", std::string(body["name"])));
    ok(res, {{"status", "updated"}});
  });

  priv->server.Delete(R"(/groups/(\d+)/projects/(\d+))", [&](const httplib::Request& req, httplib::Response& res)
  {
    priv->scanDatabase->removeProject(std::stoull(req.matches[2]), std::stoull(req.matches[1]));
    LoggerC::LogInfo("Project deleted");
    ok(res, {{"status", "deleted"}});
  });

  /* ===================== VERSIONS ===================== */

  priv->server.Post(R"(/groups/(\d+)/projects/(\d+)/versions)", [&](const httplib::Request& req, httplib::Response& res)
  {
    auto body = json::parse(req.body, nullptr, false);
    if (!body.contains("name"))
    {
      LoggerC::LogError("Version creation failed: missing name");
      return bad_request(res, "Missing 'name'");
    }

    size_t id = priv->scanDatabase->createVersion(body["name"], std::stoull(req.matches[1]), std::stoull(req.matches[2]));

    if (id == -1)
    {
      LoggerC::LogError("Version creation failed: Error while resolving Ids");
      bad_request(res, "Error while resolving Ids");
    }
    else
    {
      LoggerC::LogInfo(std::format("Version {} created", std::string(body["name"])));
      ok(res, {{"id", id}});
    }
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
    {
      LoggerC::LogError("Version renaming failed: missing name");
      return bad_request(res, "Missing 'name'");
    }

    priv->scanDatabase->editVersion(std::stoull(req.matches[3]), std::stoull(req.matches[1]), std::stoull(req.matches[2]), body["name"]);

    LoggerC::LogInfo(std::format("Version {} renamed", std::string(body["name"])));
    ok(res, {{"status", "updated"}});
  });

  priv->server.Delete(R"(/groups/(\d+)/projects/(\d+)/versions/(\d+))", [&](const httplib::Request& req, httplib::Response& res)
  {
    priv->scanDatabase->removeVersion(std::stoull(req.matches[3]), std::stoull(req.matches[1]), std::stoull(req.matches[2]));
    LoggerC::LogInfo("Version deleted");
    ok(res, {{"status", "deleted"}});
  });

  /* ===================== SCANS ===================== */

  priv->server.Post(R"(/groups/(\d+)/projects/(\d+)/versions/(\d+)/scans)", [&](const httplib::Request& req, httplib::Response& res)
  {
    auto body = json::parse(req.body, nullptr, false);
    if (!body.contains("name"))
    {
      LoggerC::LogError("Scan creation failed: missing name");
      return bad_request(res, "Missing 'name'");
    }

    size_t id = priv->scanDatabase->createScan(body["name"], std::stoull(req.matches[1]), std::stoull(req.matches[2]), std::stoull(req.matches[3]));

    if (id == -1)
    {
      LoggerC::LogError("Scan creation failed: Error while resolving Ids");
      bad_request(res, "Error while resolving Ids");
    }
    else
    {
      LoggerC::LogInfo(std::format("Scan {} created", std::string(body["name"])));
      ok(res, {{"id", id}});
    }
  });

  priv->server.Get(R"(/groups/(\d+)/projects/(\d+)/versions/(\d+)/scans)", [&](const httplib::Request& req, httplib::Response& res)
  {
    json result;
    for (auto& [id, name] : priv->scanDatabase->getScans(std::stoull(req.matches[1]), std::stoull(req.matches[2]), std::stoull(req.matches[3])))
      result[std::to_string(id)] = name;
    ok(res, result);
  });

  priv->server.Delete(R"(/groups/(\d+)/projects/(\d+)/versions/(\d+)/scans/(\d+))", [&](const httplib::Request& req, httplib::Response& res)
  {
    const size_t groupId = std::stoull(req.matches[1]);
    const size_t projectId = std::stoull(req.matches[2]);
    const size_t versionId = std::stoull(req.matches[3]);
    const size_t scanId = std::stoull(req.matches[4]);

    priv->scanDatabase->removeScan(groupId, projectId, versionId, scanId);
    LoggerC::LogInfo("Scan deleted");
    ok(res, {{"status", "deleted"}});
  });

  priv->server.Post(R"(/groups/(\d+)/projects/(\d+)/versions/(\d+)/scans/(\d+)/upload)", [&](const httplib::Request& req, httplib::Response& res)
  {
    const size_t groupId = std::stoull(req.matches[1]);
    const size_t projectId = std::stoull(req.matches[2]);
    const size_t versionId = std::stoull(req.matches[3]);
    const size_t scanId = std::stoull(req.matches[4]);

    if (!req.form.has_file("file"))
    {
      LoggerC::LogError("Error while Uploading: Missing 'file' in multipart form data");
      return bad_request(res, "Missing 'file' in multipart form data");
    }

    httplib::FormData file = req.form.get_file("file");

    if (file.content.empty())
    {
      LoggerC::LogError("Error while Uploading: File content is empty");
      return bad_request(res, "File content is empty");
    }

    try
    {
      // Convert file content to vector<char>
      ahn::vector<char> fileData(file.content.begin(), file.content.end());

      // Add the zip data to the database
      priv->scanDatabase->addZipData(scanId, groupId, projectId, versionId, scanId, fileData);
      LoggerC::LogInfo("File uploaded");
      ok(res, {{"status", "uploaded"}, {"filename", file.filename}, {"size", fileData.size()}});
    }
    catch (const std::exception& e)
    {
      LoggerC::LogError(std::format("Error while Uploading: Error processing file: {}", e.what()));
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
    LoggerC::LogInfo("Scan started");
    ok(res, {{"status", "started"}});
  });

  priv->server.Post(R"(/groups/(\d+)/projects/(\d+)/versions/(\d+)/scans/(\d+)/abort)", [&](const httplib::Request& req, httplib::Response& res)
  {
    const size_t groupId = std::stoull(req.matches[1]);
    const size_t projectId = std::stoull(req.matches[2]);
    const size_t versionId = std::stoull(req.matches[3]);
    const size_t scanId = std::stoull(req.matches[4]);

    priv->scanDatabase->abortScan(scanId, groupId, projectId, versionId, scanId);
    LoggerC::LogInfo("Scan aborted");
    ok(res, {{"status", "aborted"}});
  });

  priv->server.Get(R"(/groups/(\d+)/projects/(\d+)/versions/(\d+)/scans/(\d+)/info)", [&](const httplib::Request& req, httplib::Response& res)
  {
    const size_t groupId = std::stoull(req.matches[1]);
    const size_t projectId = std::stoull(req.matches[2]);
    const size_t versionId = std::stoull(req.matches[3]);
    const size_t scanId = std::stoull(req.matches[4]);

    auto scanData = priv->scanDatabase->getScan(scanId, groupId, projectId, versionId, scanId);

    if (scanData != nullptr)
    {
      ahn::vector<TreeSearchResult> searchResults = scanData->getDeepResult();

      json ret;

      ScanDataStatusE status;
      size_t id;
      std::string name;

      scanData->getBaseData(status, id, name);

      ret["status"] = status;
      ret["id"] = id;
      ret["name"] = name;

      ret["maxCount"] = scanData->getMaxCount();
      ret["finishedCount"] = scanData->getFinishedCount();

      ret["deepMaxCount"] = scanData->getDeepMaxCount();
      ret["deepFinishedCount"] = scanData->getDeepFinishedCount();

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

          result["elementIndex"] = searchResult.elementIndex;
          // result["searchContent"] = base64_encode(searchResult.searchContent);
          // result["sourceContent"] = base64_encode(searchResult.sourceContent);
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

  priv->server.Get(R"(/groups/(\d+)/projects/(\d+)/versions/(\d+)/scans/(\d+)/files/(\d+))", [&](const httplib::Request& req, httplib::Response& res)
  {
    const size_t groupId = std::stoull(req.matches[1]);
    const size_t projectId = std::stoull(req.matches[2]);
    const size_t versionId = std::stoull(req.matches[3]);
    const size_t scanId = std::stoull(req.matches[4]);
    const size_t elementId = std::stoull(req.matches[5]);

    auto scanData = priv->scanDatabase->getScan(scanId, groupId, projectId, versionId, scanId);

    if (scanData != nullptr)
    {
      json ret;

      std::filesystem::path outPath = std::filesystem::path(scanData->dataPath).parent_path() / "found";

      std::ifstream sourceContentStream(outPath / std::to_string(elementId) / "sourceContent");
      std::ifstream searchContentStream(outPath / std::to_string(elementId) / "searchContent");

      std::string sourceContent((std::istreambuf_iterator<char>(sourceContentStream)), std::istreambuf_iterator<char>());

      std::string searchContent((std::istreambuf_iterator<char>(searchContentStream)), std::istreambuf_iterator<char>());

      ret["searchContent"] = base64_encode(searchContent);
      ret["sourceContent"] = base64_encode(sourceContent);

      ok(res, ret);
    }
  });

  priv->server.Get(R"(/updates/check)", [&](const httplib::Request& req, httplib::Response& res)
  {
    json ret;
    ahn::vector<UpdateInfo> updates = priv->updateManager->checkUpdates();
    for (const UpdateInfo& info : updates)
    {
      json element;

      element["name"] = info.name;
      element["sha"] = info.sha;
      element["type"] = info.type;
      element["language"] = info.language;

      ret.push_back(element);
    }

    ok(res, ret);
    return;
  });

  priv->server.Get(R"(/updates/status)", [&](const httplib::Request& req, httplib::Response& res)
  {
    json ret;

    ret["inUpdate"] = (bool)priv->updateDatabase;

    ret["amountFinished"] = priv->updateManager->getUpdateAmountFinished();
    ret["amountMax"] = priv->updateManager->getUpdateAmountMax();

    ok(res, ret);
    return;
  });

  priv->server.Post(R"(/updates/startupdate)", [&](const httplib::Request& req, httplib::Response& res)
  {
    priv->updateDatabase = true;
    LoggerC::LogInfo("Database update started");
    ok(res, {{"status", "startupdate"}});
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
      std::this_thread::sleep_for(5s);
    }
  });
}

void ScanServer::updateScans()
{
  priv->inUpdate = true;

  priv->scanDatabase->save();

  if (priv->pool.get_tasks_total() == 0)
  {
    if (priv->updateDatabase)
    {
      priv->pool.detach_task([this]()
      {
        ahn::vector<std::string> filter;
        priv->updateManager->startUpdates(filter);
        priv->updateDatabase = false;
      });
    }

    auto nextData = priv->scanDatabase->getNextScan();
    if (nextData != nullptr)
    {
      priv->pool.detach_task([this, nextData]()
      {
        LoggerC::LogInfo("Scan started started");
        nextData->setStatus(ScanDataStatusE::Running);

        // Unzip/Checkout Data
        std::string path;
        std::string revision;
        ScanDataTypeE type;
        nextData->getData(path, revision, type);

        if (nextData->isAborted())
          return;

        std::filesystem::path outPath = std::filesystem::path(nextData->dataPath).parent_path() / "data";

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
        treeSearch.search(outPath, priv->env, nextData.operator->());

        if (nextData->isAborted())
          return;

        // Deep scan on first level results
        treeSearch.searchDeep(outPath, priv->env, nextData.operator->());

        if (nextData->isAborted())
          return;

        // Just for consistency
        nextData->setStatus(ScanDataStatusE::Finished);
        priv->scanDatabase->save();
        LoggerC::LogInfo("Scan started finished");
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
  std::filesystem::create_directories(outputPath);

  struct archive* a = archive_read_new();
  struct archive_entry* entry = nullptr;

  if (!a)
    throw std::runtime_error("Failed to allocate archive");

  try
  {
    archive_read_support_format_zip(a);
    archive_read_support_format_tar(a);
    archive_read_support_filter_gzip(a);

    if (archive_read_open_filename(a, archivePath.string().c_str(), 4 * 1024 * 1024) != ARCHIVE_OK)
      throw std::runtime_error(std::string("Failed to open archive: ") + archive_error_string(a));

    int flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_SECURE_NODOTDOT | ARCHIVE_EXTRACT_UNLINK;

    int r;
    while ((r = archive_read_next_header(a, &entry)) == ARCHIVE_OK)
    {
      const char* entryPath = archive_entry_pathname(entry);
      if (!entryPath)
        continue;

      std::filesystem::path entryRel = std::filesystem::path(entryPath).relative_path();
      std::filesystem::path fullPath = outputPath / entryRel;

      std::string fullPathStr = fullPath.string();

      archive_entry_set_pathname(entry, fullPathStr.c_str());

      r = archive_read_extract(a, entry, flags);

      if (r != ARCHIVE_OK && r != ARCHIVE_WARN)
      {
        // skip bad entry but continue
        archive_read_data_skip(a);
        continue;
      }
    }

    if (r != ARCHIVE_EOF)
      throw std::runtime_error(archive_error_string(a));

    archive_read_free(a);
  }
  catch (...)
  {
    archive_read_free(a);
    throw;
  }
}

void ScanServer::start()
{
  priv->server.listen(priv->env.scanServerAddr, priv->env.scanServerPort);
}

void ScanServer::stop()
{
  priv->server.stop();
}
