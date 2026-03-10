#include "Github.hpp"

#include "AhnalyticBase/database/FileDatabase.hpp"
#include "AhnalyticBase/github/GithubCrawler.hpp"
#include "AhnalyticBase/helper/GitCliHelper.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <system_error>
#include <thread>

using namespace std::chrono_literals;

static std::atomic<uint64_t> g_exec_counter{0};

// Debug helper, remove later
static volatile bool exitGracefull = false;

GitHubHandler::GitHubHandler(const std::string& basePath, const std::string& tempPath) : basePath(basePath), tempPath(tempPath)
{
}

void GitHubHandler::scanRepo(const RepoInfo& info) const
{
  if (exitGracefull)
    return;

  std::unordered_map<std::string, FileDatabase*> dbs;

  // std::unordered_map<std::string, std::string> lastFileData;
  std::string lastSha;
  std::unordered_map<std::string, ScanTreeData> lastFiles;

  bool hasData = false;
  if (info.tags.size() > 0)
  {
    for (const TagInfo& tag : info.tags)
    {
      if (!exitGracefull)
      {
        hasData |= scanTag(dbs, info, tag.name, tag.commitSha, lastSha, lastFiles /*, lastFileData*/);
        lastSha = tag.commitSha;
      }
    }
  }
  else
    hasData |= scanTag(dbs, info, "HEAD", info.headSha, lastSha, lastFiles /*, lastFileData*/);

  std::filesystem::path repoPath = std::filesystem::path(tempPath) / cleanFileName(extractOwnerRepo(info.htmlUrl));

  if (!hasData)
  {
    SourceScanner scanner;
    std::list<std::string> groups = scanner.getFileGroups();
    for (const std::string& group : groups)
    {
      std::filesystem::path resPath = basePath;
      resPath = resPath.concat("/" + group).concat("/github/").concat(cleanFileName(info.fullName) + "_" + group + ".db");
      if (std::filesystem::exists(resPath))
      {
        std::error_code ec;
        std::filesystem::remove_all(resPath, ec);
      }

      std::filesystem::path emptyPath = basePath;
      emptyPath = emptyPath.concat("/CPP").concat("/github/").concat(cleanFileName(info.fullName) + "_CPP.empty");
      std::ofstream(emptyPath.string());
    }
  }

  for (auto iter = lastFiles.begin(); iter != lastFiles.end(); iter++)
    delete iter->second.tree;

  for (auto iter = dbs.begin(); iter != dbs.end(); iter++)
    delete iter->second;

  if (std::filesystem::exists(repoPath))
  {
    std::error_code ec;
    std::filesystem::remove_all(repoPath, ec);
  }
}

bool GitHubHandler::scanTag(std::unordered_map<std::string, FileDatabase*>& dbs, const RepoInfo& info, const std::string& tagName, const std::string& sha,
                            const std::string& lastSha,
                            std::unordered_map<std::string, ScanTreeData>& lastFiles /*, std::unordered_map<std::string, std::string>& lastFileData*/) const
{
  bool ret = false;
  std::string url = info.htmlUrl;

  auto getDb = [&dbs, info, url](const std::filesystem::path& resPath)
  {
    FileDatabase* db = nullptr;
    auto dbIter = dbs.find(resPath.string());
    if (dbIter == dbs.end())
    {
      bool updateMode = std::filesystem::exists(resPath);
      db = new FileDatabase(DBType::SQLite, resPath.string());
      dbs[resPath.string()] = db;

      if (!updateMode)
        db->createRepoData(info.fullName, url, info.license);
    }
    else
      db = dbIter->second;

    return db;
  };

  SourceScanner scanner;
  std::list<std::string> supportedExt = scanner.getFileTypes();
  std::list<std::string> groups = scanner.getFileGroups();

  std::unordered_map<std::string, uint32_t> tagIds;
  for (const std::string& group : groups)
  {
    std::filesystem::path resPath = basePath;
    resPath = resPath.concat("/" + group).concat("/github/").concat(cleanFileName(info.fullName) + "_" + group + ".db");

    FileDatabase* db = getDb(resPath);
    uint32_t tagId = db->createTag(tagName, sha);
    tagIds[group] = tagId;
  }

  std::unordered_map<std::string, std::string> fileData = getGitFiles(supportedExt, url, sha, lastSha);

  if (fileData.size() == 0)
    return ret;

  ret = true;

  std::unordered_map<std::string, std::vector<ScanTreeData>> buffers = scanner.scanBuffer(fileData);

  std::unordered_map<std::string, uint32_t> sizes;
  std::unordered_map<std::string, std::vector<ScanTreeData>> datas;

  auto processData = [&sizes, &datas, &dbs, tagName, sha, &tagIds](const std::string& type, FileDatabase* db)
  {
    std::vector<std::string> names;
    SourceStructureTree* root = new SourceStructureTree();
    for (const ScanTreeData& data : datas[type])
    {
      root->children.push_back(data.tree);
      names.push_back(data.path.string());
    }

    std::vector<FlatNodeDeDupData> deduped;
    std::vector<uint32_t> indexList;
    reduceTree(root, deduped, indexList);

    std::vector<char> result;
    SourceStructureTree::serialize(deduped, indexList, result, nullptr);

    // TEST
    /*
    std::vector<FlatNodeDeDupData> nodeListTestOut;
    std::vector<uint32_t> indexListTestOut;
    SourceStructureTree::deserialize(result, nodeListTestOut, indexListTestOut, nullptr);
    SourceStructureTree* rootTest = (SourceStructureTree*)rebuildTree(nodeListTestOut, indexListTestOut);
    */
    // TEST

    // FileDatabase* db = dbs[type];
    uint32_t dataId = db->createSourceTreeData(result);
    std::unordered_map<std::string, uint32_t> pathIds = db->insertNames(names);

    std::vector<uint32_t> vecPathId;
    std::vector<uint32_t> indices;
    vecPathId.reserve(datas[type].size());
    int index = 0;
    for (const ScanTreeData& data : datas[type])
    {
      vecPathId.push_back(pathIds[data.path.string()]);
      indices.push_back(index);
      index++;
    }

    db->createFiles(dataId, indices, vecPathId, tagIds[type]);

    // delete root;
    sizes[type] = 0;
    datas[type].clear();
  };

  for (auto iter = buffers.begin(); iter != buffers.end(); iter++)
  {
    std::filesystem::path resPath = basePath;
    resPath = resPath.concat("/" + iter->first).concat("/github/").concat(cleanFileName(info.fullName) + "_" + iter->first + ".db");

    FileDatabase* db = getDb(resPath);

    for (const ScanTreeData& data : iter->second)
    {
      auto lastIter = lastFiles.find(data.path.string());
      if (lastIter != lastFiles.end())
        if (*lastIter->second.tree == *data.tree)
        {
          delete data.tree;
          continue;
        }

      delete lastFiles[data.path.string()].tree;
      lastFiles[data.path.string()] = data;

      if (!sizes.contains(iter->first))
        sizes[iter->first] = data.resSize;
      else
        sizes[iter->first] += data.resSize;

      datas[iter->first].push_back(data);

      if (sizes[iter->first] > 1024 * 1024 * 10)
        processData(iter->first, db);
    }
  }

  for (auto iter = datas.begin(); iter != datas.end(); iter++)
    if (iter->second.size() > 0)
    {
      std::filesystem::path resPath = basePath;
      resPath = resPath.concat("/" + iter->first).concat("/github/").concat(cleanFileName(info.fullName) + "_" + iter->first + ".db");
      FileDatabase* db = getDb(resPath);
      processData(iter->first, db);
    }

  std::unordered_map<std::string, ScanTreeData> resFiles;
  for (auto iter = buffers.begin(); iter != buffers.end(); iter++)
    for (const ScanTreeData& data : iter->second)
      resFiles[data.path.string()] = lastFiles[data.path.string()];

  for (auto iter = lastFiles.begin(); iter != lastFiles.end(); iter++)
    if (!resFiles.contains(iter->first))
      delete iter->second.tree;

  lastFiles = resFiles;
  // lastFileData = fileData;

  return ret;
}

void GitHubHandler::replaceAll(std::string& s, const std::string& from, const std::string& to)
{
  if (from.empty())
    return;
  size_t pos = 0;
  while ((pos = s.find(from, pos)) != std::string::npos)
  {
    s.replace(pos, from.length(), to);
    pos += to.length();
  }
}

void GitHubHandler::safeDelete(const std::filesystem::path& p)
{
  std::error_code ec;
  if (std::filesystem::exists(p, ec))
  {
    try
    {
      std::filesystem::remove_all(p, ec);
    }
    catch (...)
    {
    }
  }
}

// -----------------------------
// path helpers & utilities
// -----------------------------
bool GitHubHandler::hasSupportedExtension(const std::string& path, const std::list<std::string>& exts)
{
  for (const auto& ext : exts)
  {
    if (path.size() >= ext.size() && path.compare(path.size() - ext.size(), ext.size(), ext) == 0)
      return true;
  }
  return false;
}

std::string GitHubHandler::extractOwnerRepo(const std::string& url) const
{
  std::regex rgx(R"(github\.com/([^/]+)/([^/]+)(?:\.git)?)", std::regex::icase);
  std::smatch match;
  if (!std::regex_search(url, match, rgx) || match.size() < 3)
    return "";
  return match[1].str() + "/" + match[2].str();
}

std::string GitHubHandler::cleanFileName(const std::string& name)
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

std::unordered_map<std::string, std::string> GitHubHandler::getGitFiles(const std::list<std::string>& supportedExt, const std::string& repoUrl,
                                                                        const std::string& sha, const std::string& lastSha) const
{
  std::unordered_map<std::string, std::string> result;

  std::string repoKey = cleanFileName(extractOwnerRepo(repoUrl));
  if (repoKey.empty())
    return result;

  std::filesystem::path repoPath = std::filesystem::path(tempPath) / repoKey;
  std::filesystem::path workPath = repoPath / "work";

  if (!std::filesystem::exists(repoPath / ".git"))
  {
    GitCliHelperC::getGitClone(repoPath, repoUrl, tempPath);

    // std::string cloneCmd = "git clone --no-tags --quiet \"" + repoUrl + "\" \"" + repoPath.string() + "\"";
    // ExecResult r = execAndCapture(cloneCmd);
    // if (r.exitCode != 0)
    // return result;
  }

  if (lastSha != "")
    GitCliHelperC::fetchTag(repoPath.string(), lastSha, tempPath);
  GitCliHelperC::fetchTag(repoPath.string(), sha, tempPath);

  safeDelete(repoPath / ".git" / "index.lock");
  // execAndCapture("git -C \"" + repoPath.string() + "\" remote prune origin");

  // ExecResult fetch = execAndCapture("git -C \"" + repoPath.string() + "\" fetch origin --depth=1 " + sha);
  // if (fetch.exitCode != 0)
  //{
  //   ExecResult full = execAndCapture("git -C \"" + repoPath.string() + "\" fetch origin " + sha);
  //   if (full.exitCode != 0)
  //     return result;
  // }

  safeDelete(workPath);
  std::error_code ec;
  std::filesystem::create_directories(workPath, ec);

  std::vector<std::string> files;
  if (lastSha.size() == 0)
    files = GitCliHelperC::getGitFiles(repoKey, repoUrl, sha, tempPath);
  else
    files = GitCliHelperC::getGitFiles(repoKey, repoUrl, sha, lastSha, tempPath);

  static const std::vector<std::string> denyDirs = {"third_party", "3rdparty", "vendor", "vendors",      "external",
                                                    "externals",   "deps",     "dep",    "node_modules", ".git"};

  std::vector<std::string> filesFilteres;
  for (const std::string& file : files)
  {
    if (hasSupportedExtension(file, supportedExt))
    {
      bool skip = false;
      for (const std::string& denyPath : denyDirs)
      {
        if (file.find(denyPath) != std::string_view::npos)
        {
          skip = true;
          break;
        }
      }

      if (!skip)
        filesFilteres.push_back(file);
    }
  }

  result = GitCliHelperC::getFilesWithContent(repoPath.string(), sha, filesFilteres);

  // ExecResult checkout = execAndCapture("git -C \"" + repoPath.string() + "\" --work-tree=\"" + workPath.string() + "\" checkout --force " + sha);
  // if (checkout.exitCode != 0)
  // return result;

  /*
  std::unordered_set<std::string> submodules;
  {
    std::filesystem::path gm = repoPath / ".gitmodules";
    if (std::filesystem::exists(gm))
    {
      std::ifstream in(gm);
      std::string line;
      while (std::getline(in, line))
      {
        auto pos = line.find("path =");
        if (pos != std::string::npos)
        {
          std::string p = line.substr(pos + 6);
          p.erase(std::remove_if(p.begin(), p.end(), ::isspace), p.end());
          if (!p.empty())
            submodules.insert(p);
        }
      }
    }
  }

  static const std::vector<std::string> denyDirs = {"third_party", "3rdparty", "vendor", "vendors",      "external",
                                                    "externals",   "deps",     "dep",    "node_modules", ".git"};

  auto shouldSkipPath = [&](const std::filesystem::path& rel) -> bool
  {
    for (const auto& part : rel)
    {
      std::string s = part.string();
      std::transform(s.begin(), s.end(), s.begin(), ::tolower);

      for (const auto& bad : denyDirs)
        if (s == bad)
          return true;
    }

    std::string relStr = rel.string();
    for (const auto& sm : submodules)
      if (relStr == sm || relStr.starts_with(sm + "/"))
        return true;

    return false;
  };

  for (auto it = std::filesystem::recursive_directory_iterator(workPath.native(), std::filesystem::directory_options::skip_permission_denied);
       it != std::filesystem::recursive_directory_iterator(); ++it)
  {
    const auto& entry = *it;

    if (entry.is_symlink())
      continue;

    if (!entry.is_regular_file())
      continue;

    std::filesystem::path relPath;
    try
    {
      //relPath = std::filesystem::relative(entry.path(), workPath);
      relPath = entry.path().lexically_relative(workPath);
    }
    catch (...)
    {
      continue;
    }

    if (shouldSkipPath(relPath))
      continue;

    if (!hasSupportedExtension(relPath.string(), supportedExt))
      continue;

    std::ifstream in(entry.path(), std::ios::binary);
    if (!in.is_open())
      continue;

    std::ostringstream ss;
    ss << in.rdbuf();
    result[relPath.string()] = ss.str();
  }
  */

  safeDelete(workPath);

  return result;
}