#ifndef github_hpp__
#define github_hpp__

#include "AhnalyticBase/export.hpp"
#include "SourceScanner.hpp"

#include "RateLimit.hpp"

#define CPPHTTPLIB_NO_MULTIPART_FORM_DATA
#define CPPHTTPLIB_OPENSSL_SUPPORT
#define CPPHTTPLIB_ZLIB_SUPPORT
#include <httplib.h>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

#include <list>
#include <string>
#include <unordered_map>

struct RepoInfo;
class FileDatabase;

struct ExecResult
{
  int exitCode = -1;
  std::string stdoutText;
  std::string stderrText;
};

class DLLEXPORT GitHubHandler
{
public:
  GitHubHandler(const std::string& basePath, const std::string& tempPath);

  void scanRepo(const RepoInfo& info) const;
  bool scanTag(std::unordered_map<std::string, FileDatabase*>& dbs, const RepoInfo& info, const std::string& tagName, const std::string& sha,
               std::unordered_map<std::string, ScanTreeData>& lastFiles, std::unordered_map<std::string, std::string>& lastFileData) const;

  std::string cleanFileName(const std::string& name) const;

private:
  std::string basePath;
  std::string tempPath;

protected:
  std::unordered_map<std::string, std::string> getGitFiles(const std::list<std::string>& supportedExt, const std::string& repoUrl,
                                                           const std::string& sha) const;

  static bool hasSupportedExtension(const std::string& path, const std::list<std::string>& exts);
  std::string extractOwnerRepo(const std::string& url) const;

  static std::string threadIdString();
  static void safeDelete(const std::filesystem::path& p);
  ExecResult execAndCapture(const std::string& cmdBase) const;
  static void replaceAll(std::string& s, const std::string& from, const std::string& to);
  static std::string uniqueTempName(const std::string& prefix);
};

#endif