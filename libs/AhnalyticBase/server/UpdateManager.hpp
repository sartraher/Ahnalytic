#ifndef updatemanager_hpp__
#define updatemanager_hpp__

#include <string>
#include <vector>

#include "AhnalyticBase/file/UpdateStatusFile.hpp"
#include "AhnalyticBase/helper/ThreadSafeQueue.hpp"
#include "AhnalyticBase/file/UpdateRepoFile.hpp"

class EnviromentC;

namespace httplib
{
class Client;
};

struct UpdateDiffInfo
{
  std::string visibleName;
  std::string name;
  std::string baseName;
  ahn::vector<std::pair<std::string, std::string>> existingShas;
  ahn::vector<std::pair<std::string, std::string>> missingShas;
  std::string type;
  std::string language;
  std::string licence;
  std::string url;
};

class UpdateManager
{
public:
  UpdateManager(EnviromentC* enviroment);
  ~UpdateManager();

  ahn::vector<UpdateInfo> checkUpdates() const;
  void startUpdates(const ahn::vector<std::string>& filter);

  ahn::vector<UpdateRepoData> getUpdateRepoData(const ahn::vector<std::string>& filter);

  bool updateGitHub(const UpdateRepoData& repoData, const std::string& lastSha);
  bool updateStackExchange(const UpdateRepoData& repoData, const std::string& lastSha);
  bool updateSourceForge(const UpdateRepoData& repoData, const std::string& lastSha);

  // void checkUpdateDiff(ThreadSafeQueue<UpdateDiffInfo>& queue) const;

private:
  EnviromentC* env;
  UpdateStatusFile installedUpdates;

protected:
  bool downloadFile(httplib::Client* cli, const std::string& path, const std::filesystem::path& outPath);
};

#endif
