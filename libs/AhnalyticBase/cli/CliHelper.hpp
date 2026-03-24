#ifndef CliHelper_hpp__
#define CliHelper_hpp__

#include "AhnalyticBase/Export.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

struct DLLEXPORT TagData
{
  std::string sha;
  std::string name;
};

class DLLEXPORT CliHelperWrapper
{
  enum TargetTypeE
  {
    Git,
    Svn,
    Mercurial
  };

  TargetTypeE type;

public:
  std::string repoPath;

  CliHelperWrapper(const std::string& typeName);
  std::string getHeadId(const std::string& url, const std::string& tempPath);
  ahn::vector<TagData> getTagData(const std::string& url, const std::string& tempPath);
  ahn::vector<std::string> getFiles(const std::string& name, const std::string& url, const std::string& tempPath);
  std::string getCreationDate(const std::string& url, const std::string& tempPath);
  ahn::map<std::string, std::string> getFilesWithContent(const std::string& repoPath, const std::string& repoUrl, const std::string& sha,
                                                         const ahn::vector<std::string>& files, const std::string& tempPath);
};


#endif
