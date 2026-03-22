#ifndef gitclihelper_hpp__
#define gitclihelper_hpp__

#include "AhnalyticBase/Export.hpp"
#include "AhnalyticBase/helper/CliHelper.hpp"

#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>

class DLLEXPORT GitCliHelperC
{
public:
  static std::vector<TagData> getGitTagData(const std::string& url, const std::string& tempPath);
  static std::string getHeadSha(const std::string& url, const std::string& tempPath);

  static std::string getGitCloneShallow(const std::string& name, const std::string& url, const std::string& tempPath);
  static std::string getGitCloneShallow(const std::filesystem::path& repoPath, const std::string& url, const std::string& tempPath);

  static std::string getGitClone(const std::string& name, const std::string& url, const std::string& tempPath);
  static std::string getGitClone(const std::filesystem::path& repoPath, const std::string& url, const std::string& tempPath);

  static void fetchTag(const std::string& repoPath, const std::string& sha, const std::string& tempPath);

  static std::vector<std::string> getGitFiles(const std::string& name, const std::string& url, const std::string& tempPath);
  static std::vector<std::string> getGitFiles(const std::string& name, const std::string& url, const std::string& sha, const std::string& tempPath);
  static std::vector<std::string> getGitFiles(const std::string& name, const std::string& url, const std::string& sha, const std::string& lastSha,
                                              const std::string& tempPath);

  static std::unordered_map<std::string, std::string> getFilesWithContent(const std::string& repoPath, const std::string& sha,
                                                                          const std::vector<std::string>& files);

  static std::string getCreationDate(const std::string& url, const std::string& tempPath);

private:
protected:
  static std::vector<TagData> parseTags(const std::string& lines, bool isHead);
};

#endif