#ifndef ScanDatabase_hpp__
#define ScanDatabase_hpp__

#include "AhnalyticBase/database/Database.hpp"
#include "AhnalyticBase/tree/TreeSearch.hpp"
#include "AhnalyticBase/file/AhnalyticFile.hpp"

#include <filesystem>
#include <shared_mutex>
#include <unordered_map>
#include <variant>

#include <nlohmann/json.hpp>

class ScanDatabasePrivate;

class DLLEXPORT BaseData
{
public:
  size_t id;
  std::string name;

private:
protected:
};

enum class ScanDataTypeE
{
  Git,
  Svn,
  Archive
};

enum class ScanDataStatusE
{
  Idle = 0,
  Started = 1,
  Running = 2,
  Aborted = 3,
  Finished = 4
};

enum class ScanFileTreeTypeE
{
  File,
  Directory
};

struct DLLEXPORT ScanFileTree
{
  ScanFileTreeTypeE type;
  std::string name;
  std::list<ScanFileTree> children;

  std::list<AhnalyticFile> files;

  nlohmann::json getJson() const;
};

class DLLEXPORT ScanData : public BaseData, public TreeResultInterface
{
public:
  ScanData()
  {
    type = ScanDataTypeE::Archive;
    status = ScanDataStatusE::Idle;
  }

  void setStatus(ScanDataStatusE nextStatus)
  {
    std::unique_lock lock(sharedMutex);
    status = nextStatus;
  }

  void getData(std::string& path, std::string& rev, ScanDataTypeE& dataType)
  {
    std::shared_lock lock(sharedMutex);
    path = dataPath;
    rev = revision;
    dataType = type;
  }

  void getBaseData(ScanDataStatusE& statusRev, size_t& idRev, std::string& nameRev)
  {
    std::shared_lock lock(sharedMutex);
    statusRev = status;
    idRev = id;
    nameRev = name;
  }

  // Interface from TreeResultInterface
  virtual bool isAborted()
  {
    std::shared_lock lock(sharedMutex);
    return status == ScanDataStatusE::Aborted;
  }

  virtual void addResult(const TreeSearchResult& result)
  {
    std::unique_lock lock(sharedMutex);
    results.push_back(result);
  }

  virtual ahn::vector<TreeSearchResult> getResult()
  {
    std::shared_lock lock(sharedMutex);
    return results;
  }

  virtual void addDeepResult(const TreeSearchResult& result)
  {
    std::unique_lock lock(sharedMutex);
    deepResults.push_back(result);
  }

  virtual ahn::vector<TreeSearchResult> getDeepResult()
  {
    std::shared_lock lock(sharedMutex);
    return deepResults;
  }

  virtual void setMaxCount(int count)
  {
    std::unique_lock lock(sharedMutex);
    maxCount = count;
  }

  virtual void incFinishedCount(int count)
  {
    std::unique_lock lock(sharedMutex);
    finishedCount += count;
  }

  virtual void setDeepMaxCount(int count)
  {
    std::unique_lock lock(sharedMutex);
    deepMaxCount = count;
  }

  virtual void incDeepFinishedCount(int count)
  {
    std::unique_lock lock(sharedMutex);
    deepFinishedCount += count;
  }

  virtual int getMaxCount()
  {
    std::shared_lock lock(sharedMutex);
    return maxCount;
  }

  virtual int getFinishedCount()
  {
    std::shared_lock lock(sharedMutex);
    return finishedCount;
  }

  virtual int getDeepMaxCount()
  {
    std::shared_lock lock(sharedMutex);
    return deepMaxCount;
  }

  virtual int getDeepFinishedCount()
  {
    std::shared_lock lock(sharedMutex);
    return deepFinishedCount;
  }

  // mutable std::recursive_mutex mutex;
  mutable std::shared_mutex sharedMutex;

  ScanDataTypeE type;
  std::string dataPath;
  std::string revision;

  ScanDataStatusE status;

  ahn::vector<TreeSearchResult> results;
  ahn::vector<TreeSearchResult> deepResults;

  int maxCount = 0;
  int finishedCount = 0;

  int deepMaxCount = 0;
  int deepFinishedCount = 0;

  ScanFileTree fileTree;

private:
protected:
};

class DLLEXPORT VersionData : public BaseData
{
public:
  ahn::map<size_t, std::shared_ptr<ScanData>> scans;

private:
protected:
};

class DLLEXPORT ProjectData : public BaseData
{
public:
  ahn::map<size_t, VersionData> versions;

private:
protected:
};

class DLLEXPORT GroupData : public BaseData
{
public:
  ahn::map<size_t, ProjectData> projects;

private:
protected:
};

// Not Really a database by now but i don't rename it for now
class DLLEXPORT ScanDatabase
{
public:
  ScanDatabase(const std::filesystem::path& scanFolder);
  ~ScanDatabase();

  void load();
  void save();

  // Groups
  size_t createGroup(const std::string& name);
  void editGroup(size_t id, const std::string& name);
  void removeGroup(size_t id);
  ahn::map<size_t, std::string> getGroups();

  // Projects
  size_t createProject(const std::string& name, size_t groupId);
  void editProject(size_t id, size_t groupId, const std::string& name);
  void removeProject(size_t id, size_t groupId);
  ahn::map<size_t, std::string> getProjects(size_t groupId);

  // Versions
  size_t createVersion(const std::string& name, size_t groupId, size_t projectId);
  void editVersion(size_t id, size_t groupId, size_t projectId, const std::string& name);
  void removeVersion(size_t id, size_t groupId, size_t projectId);
  ahn::map<size_t, std::string> getVersions(size_t groupId, size_t projectId);

  // Scans
  size_t createScan(const std::string& name, size_t groupId, size_t projectId, size_t versionId);
  void editScan(size_t id, size_t groupId, size_t projectId, size_t versionId, const std::string& name);
  void removeScan(size_t id, size_t groupId, size_t projectId, size_t versionId);
  ahn::map<size_t, std::string> getScans(size_t groupId, size_t projectId, size_t versionId);

  void addZipData(size_t id, size_t groupId, size_t projectId, size_t versionId, size_t scanId, const ahn::vector<char>& data);
  void addGitData(size_t id, size_t groupId, size_t projectId, size_t versionId, size_t scanId, const std::string& url, const std::string& sha);
  void addSvnData(size_t id, size_t groupId, size_t projectId, size_t versionId, size_t scanId, const std::string& url, const std::string& revision);

  void preScan(size_t id, size_t groupId, size_t projectId, size_t versionId, size_t scanId);
  void startScan(size_t id, size_t groupId, size_t projectId, size_t versionId, size_t scanId);
  void abortScan(size_t id, size_t groupId, size_t projectId, size_t versionId, size_t scanId);
  std::shared_ptr<ScanData> getScan(size_t id, size_t groupId, size_t projectId, size_t versionId, size_t scanId);

  std::shared_ptr<ScanData> getNextScan();

private:
  ScanDatabasePrivate* priv;

protected:
};

#endif