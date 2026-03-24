#ifndef SvnCliHelper_hpp__
#define SvnCliHelper_hpp__

#include "AhnalyticBase/Export.hpp"
#include "AhnalyticBase/cli/CliHelper.hpp"

#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>

class DLLEXPORT SvnCliHelperC
{
public:
  static std::string getHeadRevision(const std::string& url, const std::string& tempPath);
  static ahn::vector<TagData> getSvnTagData(const std::string& url, const std::string& tempPath);

  static std::string getSvnCloneShallow(const std::string& name, const std::string& url, const std::string& tempPath);
  static std::string getSvnCloneShallow(const std::filesystem::path& repoPath, const std::string& url, const std::string& tempPath);

  static ahn::vector<std::string> getSvnFiles(const std::string& name, const std::string& url, const std::string& tempPath);
  static ahn::vector<std::string> getSvnFiles(const std::string& name, const std::string& url, const std::string& sha, const std::string& tempPath);
  static ahn::vector<std::string> getSvnFiles(const std::string& name, const std::string& url, const std::string& sha, const std::string& lastSha,
                                                    const std::string& tempPath);

  static std::string getCreationDate(const std::string& url, const std::string& tempPath);

  static ahn::map<std::string, std::string> getFilesWithContent(const std::string& repoPath, const std::string& sha, const ahn::vector<std::string>& files,
                                                                const std::string& tempPath);

private:
protected:
};

#endif