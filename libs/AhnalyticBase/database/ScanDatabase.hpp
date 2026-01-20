#ifndef ScanDatabase_hpp__
#define ScanDatabase_hpp__

#include "AhnalyticBase/database/Database.hpp"
#include "AhnalyticBase/tree/TreeSearch.hpp"

#include <filesystem>
#include <unordered_map>
#include <variant>

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
    const std::lock_guard<std::recursive_mutex> lock(mutex);
    status = ScanDataStatusE::Running;
  }

  void getData(std::string& path, std::string& rev, ScanDataTypeE& dataType)
  {
    const std::lock_guard<std::recursive_mutex> lock(mutex);
    path = dataPath;
    rev = revision;
    dataType = type;
  }
  // Interface from TreeResultInterface
  virtual bool isAborted()
  {
    const std::lock_guard<std::recursive_mutex> lock(mutex);
    return status == ScanDataStatusE::Aborted;
  }

  virtual void addResult(const TreeSearchResult& result)
  {
    const std::lock_guard<std::recursive_mutex> lock(mutex);
    results.push_back(result);
  }

  virtual std::vector<TreeSearchResult> getResult()
  {
    const std::lock_guard<std::recursive_mutex> lock(mutex);
    return results;
  }

  virtual void addDeepResult(const TreeSearchResult& result)
  {
    const std::lock_guard<std::recursive_mutex> lock(mutex);
    deepResults.push_back(result);
  }

  virtual std::vector<TreeSearchResult> getDeepResult()
  {
    const std::lock_guard<std::recursive_mutex> lock(mutex);
    return deepResults;
  }

  virtual void setMaxCount(int count)
  {
    const std::lock_guard<std::recursive_mutex> lock(mutex);
    maxCount = count;
  }

  virtual void incFinishedCount(int count)
  {
    const std::lock_guard<std::recursive_mutex> lock(mutex);
    finishedCount += count;
  }

  virtual int getMaxCount()
  {
    const std::lock_guard<std::recursive_mutex> lock(mutex);
    return maxCount;
  }

  virtual int getFinishedCount()
  {
    const std::lock_guard<std::recursive_mutex> lock(mutex);
    return finishedCount;
  }

  mutable std::recursive_mutex mutex;

  ScanDataTypeE type;
  std::string dataPath;
  std::string revision;

  ScanDataStatusE status;

  std::vector<TreeSearchResult> results;
  std::vector<TreeSearchResult> deepResults;

  int maxCount = 0;
  int finishedCount = 0;

private:
protected:
};

class DLLEXPORT VersionData : public BaseData
{
public:
  std::unordered_map<size_t, std::shared_ptr<ScanData>> scans;

private:
protected:
};

class DLLEXPORT ProjectData : public BaseData
{
public:
  std::unordered_map<size_t, VersionData> versions;

private:
protected:
};

class DLLEXPORT GroupData : public BaseData
{
public:
  std::unordered_map<size_t, ProjectData> projects;

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
  std::unordered_map<size_t, std::string> getGroups();

  // Projects
  size_t createProject(const std::string& name, size_t groupId);
  void editProject(size_t id, size_t groupId, const std::string& name);
  void removeProject(size_t id, size_t groupId);
  std::unordered_map<size_t, std::string> getProjects(size_t groupId);

  // Versions
  size_t createVersion(const std::string& name, size_t groupId, size_t projectId);
  void editVersion(size_t id, size_t groupId, size_t projectId, const std::string& name);
  void removeVersion(size_t id, size_t groupId, size_t projectId);
  std::unordered_map<size_t, std::string> getVersions(size_t groupId, size_t projectId);

  // Scans
  size_t createScan(const std::string& name, size_t groupId, size_t projectId, size_t versionId);
  void editScan(size_t id, size_t groupId, size_t projectId, size_t versionId, const std::string& name);
  void removeScan(size_t id, size_t groupId, size_t projectId, size_t versionId);
  std::unordered_map<size_t, std::string> getScans(size_t groupId, size_t projectId, size_t versionId);

  void addZipData(size_t id, size_t groupId, size_t projectId, size_t versionId, size_t scanId, const std::vector<char>& data);
  void addGitData(size_t id, size_t groupId, size_t projectId, size_t versionId, size_t scanId, const std::string& url, const std::string& sha);
  void addSvnData(size_t id, size_t groupId, size_t projectId, size_t versionId, size_t scanId, const std::string& url, const std::string& revision);

  void startScan(size_t id, size_t groupId, size_t projectId, size_t versionId, size_t scanId);
  void abortScan(size_t id, size_t groupId, size_t projectId, size_t versionId, size_t scanId);
  std::shared_ptr<ScanData> getScan(size_t id, size_t groupId, size_t projectId, size_t versionId, size_t scanId);

  std::shared_ptr<ScanData> getNextScan();

private:
  ScanDatabasePrivate* priv;

protected:
};

#endif