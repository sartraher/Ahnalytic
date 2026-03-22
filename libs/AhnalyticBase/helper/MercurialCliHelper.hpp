#ifndef MercurialCliHelper_hpp__
#define MercurialCliHelper_hpp__

#include "AhnalyticBase/Export.hpp"
#include "AhnalyticBase/helper/CliHelper.hpp"

#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>

class DLLEXPORT MercurialCliHelperC
{
public:
  static std::string getHeadChangeSet(const std::string& url, const std::string& tempPath);
  static std::vector<TagData> getMercurialTagData(const std::string& url, const std::string& tempPath);

  static std::string getMercurialCloneShallow(const std::string& name, const std::string& url, const std::string& tempPath);
  static std::string getMercurialCloneShallow(const std::filesystem::path& repoPath, const std::string& url, const std::string& tempPath);

  static std::vector<std::string> getMercurialFiles(const std::string& name, const std::string& url, const std::string& tempPath);
  static std::vector<std::string> getMercurialFiles(const std::string& name, const std::string& url, const std::string& sha, const std::string& tempPath);
  static std::vector<std::string> getMercurialFiles(const std::string& name, const std::string& url, const std::string& sha, const std::string& lastSha,
                                              const std::string& tempPath);

  static std::string getCreationDate(const std::string& url, const std::string& tempPath);

  static std::unordered_map<std::string, std::string> getFilesWithContent(const std::string& repoPath, const std::string& sha,
                                                                          const std::vector<std::string>& files);

private:
protected:
};

#endif