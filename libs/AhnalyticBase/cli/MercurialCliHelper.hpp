#ifndef MercurialCliHelper_hpp__
#define MercurialCliHelper_hpp__

#include "AhnalyticBase/Export.hpp"
#include "AhnalyticBase/cli/CliHelper.hpp"

#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>

class DLLEXPORT MercurialCliHelperC
{
public:
  static std::string getHeadChangeSet(const std::string& url, const std::string& tempPath);
  static ahn::vector<TagData> getMercurialTagData(const std::string& url, const std::string& tempPath);

  static std::string getMercurialCloneShallow(const std::string& name, const std::string& url, const std::string& tempPath);
  static std::string getMercurialCloneShallow(const std::filesystem::path& repoPath, const std::string& url, const std::string& tempPath);

  static ahn::vector<std::string> getMercurialFiles(const std::string& name, const std::string& url, const std::string& tempPath);
  static ahn::vector<std::string> getMercurialFiles(const std::string& name, const std::string& url, const std::string& sha, const std::string& tempPath);
  static ahn::vector<std::string> getMercurialFiles(const std::string& name, const std::string& url, const std::string& sha, const std::string& lastSha,
                                              const std::string& tempPath);

  static std::string getCreationDate(const std::string& url, const std::string& tempPath);

  static ahn::map<std::string, std::string> getFilesWithContent(const std::string& repoPath, const std::string& sha, const ahn::vector<std::string>& files);

private:
protected:
};

#endif