#ifndef updatemanager_hpp__
#define updatemanager_hpp__

#include <string>
#include <vector>

#include "AhnalyticBase/helper/ThreadSafeQueue.hpp"
#include "AhnalyticBase/file/UpdateStatusFile.hpp"

class EnviromentC;

struct UpdateDiffInfo
{
  std::string visibleName;
  std::string name;
  std::string baseName;
  std::vector<std::pair<std::string, std::string>> existingShas;
  std::vector<std::pair<std::string, std::string>> missingShas;
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

  std::vector<UpdateInfo> checkUpdates() const;
  //std::vector<UpdateDiffInfo> checkUpdateDiff() const;

  void checkUpdateDiff(ThreadSafeQueue<UpdateDiffInfo>& queue) const;
  void startUpdates();

private:
  EnviromentC* env;
  UpdateStatusFile installedUpdates;

protected:
};

#endif
