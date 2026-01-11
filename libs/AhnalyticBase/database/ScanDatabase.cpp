#include "AhnalyticBase/database/ScanDatabase.hpp"

#include "soci/soci.h"
#include "soci/sqlite3/soci-sqlite3.h"
// #include "soci/mysql/soci-mysql.h"
// #include "soci/postgresql/soci-postgresql.h"

#include <fstream>

// #include "BS_thread_pool.hpp"

class ScanDatabasePrivate
{
public:
  std::unordered_map<size_t, GroupData> groups;
  std::filesystem::path scanFolder;
  // BS::thread_pool<BS::tp::none> pool;
  //std::string connectionString;
  std::recursive_mutex mutex;

  std::atomic<int> groupId = 0;
  std::atomic<int> projectId = 0;
  std::atomic<int> versionId = 0;
  std::atomic<int> scanId = 0;

private:
protected:
};

ScanDatabase::ScanDatabase(/*DBType type, std::string connectionString,*/ const std::filesystem::path& scanFolder) :
    /*Database(type, connectionString),*/ priv(new ScanDatabasePrivate())
{
  priv->scanFolder = scanFolder;
  //priv->connectionString = connectionString;

  //if (sql != nullptr && sql->is_connected())
  //{
  //  initTables();
  //}
}
/*
void ScanDatabase::initTables()
{
  (*sql) << "CREATE TABLE IF NOT EXISTS \"Group\" ("
            "\"ID\" INTEGER,"
            "\"Name\" TEXT UNIQUE,"
            "PRIMARY KEY(\"ID\")"
            ")";

  (*sql) << "CREATE TABLE IF NOT EXISTS \"Project\" ("
            "\"ID\" INTEGER,"
            "\"GroupID\" INTEGER,"
            "\"Name\" TEXT UNIQUE,"
            "PRIMARY KEY(\"ID\")"
            ")";

  (*sql) << "CREATE TABLE IF NOT EXISTS \"Version\" ("
            "\"ID\" INTEGER,"
            "\"GroupID\" INTEGER,"
            "\"ProjectID\" INTEGER,"
            "\"Name\" TEXT UNIQUE,"
            "PRIMARY KEY(\"ID\")"
            ")";

  (*sql) << "CREATE TABLE IF NOT EXISTS \"Scan\" ("
            "\"ID\" INTEGER,"
            "\"Name\" TEXT UNIQUE,"
            "\"GroupID\" INTEGER,"
            "\"ProjectID\" INTEGER,"
            "\"VersionID\" INTEGER,"
            "PRIMARY KEY(\"ID\")"
            ")";
}
*/

size_t ScanDatabase::createGroup(const std::string& name)
{
  const std::lock_guard<std::recursive_mutex> lock(priv->mutex);

  //int id;
  //*sql << "INSERT INTO \"Group\" (Name) VALUES (:name) RETURNING ID", soci::use(name), soci::into(id);

  //size_t id = *rs.begin();
  GroupData group;
  group.id = priv->groupId.fetch_add(1);
  group.name = name;

  priv->groups[group.id] = group;

  std::filesystem::create_directory(priv->scanFolder / std::to_string(group.id));

  return group.id;
}

void ScanDatabase::editGroup(size_t id, const std::string& name)
{
  const std::lock_guard<std::recursive_mutex> lock(priv->mutex);

  if (priv->groups.contains(id))
  {
    //soci::rowset<int> rs = (sql->prepare << "UPDATE Group SET Name=:name WHERE ID=:id", soci::use(name, "name"), soci::use(id, "id"));

    priv->groups[id].name = name;
  }
}

void ScanDatabase::removeGroup(size_t id)
{
  auto groupIter = priv->groups.find(id);
  if (groupIter != priv->groups.end())
  {
    //soci::rowset<int> rs = (sql->prepare << "DELETE FROM Group WHERE ID=:id", soci::use(id, "id"));

    for (auto projectIter = groupIter->second.projects.begin(); projectIter != groupIter->second.projects.end(); projectIter++)
      removeProject(projectIter->first, id);

    std::filesystem::remove_all(priv->scanFolder / std::to_string(id));

    priv->groups.erase(id);
  }
}

std::unordered_map<size_t, std::string> ScanDatabase::getGroups()
{
  std::unordered_map<size_t, std::string> ret;

  for (auto iter = priv->groups.begin(); iter != priv->groups.end(); iter++)
    ret[iter->first] = iter->second.name;

  return ret;
}

size_t ScanDatabase::createProject(const std::string& name, size_t groupId)
{
  const std::lock_guard<std::recursive_mutex> lock(priv->mutex);

  auto groupIter = priv->groups.find(groupId);

  if (groupIter != priv->groups.end())
  {
    //soci::rowset<int> rs = (sql->prepare << "INSERT INTO Project (Name, GroupID) VALUES (:name,:groupId) RETURNING ID", soci::use(name, "name"), soci::use(groupId, "groupId"));

    //size_t id = *rs.begin();

    ProjectData project;
    project.id = priv->projectId.fetch_add(1);
    project.name = name;

    groupIter->second.projects[project.id] = project;

    std::filesystem::create_directory(priv->scanFolder / std::to_string(groupId) / std::to_string(project.id));

    return project.id;
  }

  return -1;
}

void ScanDatabase::editProject(size_t id, size_t groupId, const std::string& name)
{
  const std::lock_guard<std::recursive_mutex> lock(priv->mutex);

  auto groupIter = priv->groups.find(groupId);

  if (groupIter != priv->groups.end())
  {
    if (groupIter->second.projects.contains(groupId))
    {
      //soci::rowset<int> rs = (sql->prepare << "UPDATE Project SET Name=:name WHERE ID=:id", soci::use(name, "name"), soci::use(id, "id"));

      groupIter->second.projects[id].name = name;
    }
  }
}

void ScanDatabase::removeProject(size_t id, size_t groupId)
{
  const std::lock_guard<std::recursive_mutex> lock(priv->mutex);

  auto groupIter = priv->groups.find(groupId);

  if (groupIter != priv->groups.end())
  {
    auto projectIter = groupIter->second.projects.find(groupId);
    if (projectIter != groupIter->second.projects.end())
    {
      //soci::rowset<int> rs = (sql->prepare << "DELETE FROM Project WHERE ID=:id", soci::use(id, "id"));

      for (auto versionIter = projectIter->second.versions.begin(); versionIter != projectIter->second.versions.end(); versionIter++)
        removeVersion(versionIter->first, groupId, id);

      groupIter->second.projects.erase(id);

      std::filesystem::remove_all(priv->scanFolder / std::to_string(groupId) / std::to_string(id));
    }
  }
}

std::unordered_map<size_t, std::string> ScanDatabase::getProjects(size_t groupId)
{
  std::unordered_map<size_t, std::string> ret;
  const std::lock_guard<std::recursive_mutex> lock(priv->mutex);

  auto groupIter = priv->groups.find(groupId);

  if (groupIter != priv->groups.end())
  {
    //if (groupIter->second.projects.contains(groupId))
    //{
      for (auto iter = groupIter->second.projects.begin(); iter != groupIter->second.projects.end(); iter++)
        ret[iter->first] = iter->second.name;
    //}
  }

  return ret;
}

size_t ScanDatabase::createVersion(const std::string& name, size_t groupId, size_t projectId)
{
  const std::lock_guard<std::recursive_mutex> lock(priv->mutex);

  auto groupIter = priv->groups.find(groupId);

  if (groupIter != priv->groups.end())
  {
    auto projectIter = groupIter->second.projects.find(projectId);
    if (projectIter != groupIter->second.projects.end())
    {
      //soci::rowset<int> rs = (sql->prepare << "INSERT INTO Version (Name, GroupID, ProjectID) VALUES (:name,:groupId,:projectId) RETURNING ID", soci::use(name, "name"), soci::use(groupId, "groupId"), soci::use(projectId, "projectId"));

      //size_t id = *rs.begin();

      VersionData version;
      version.id = priv->versionId.fetch_add(1);
      version.name = name;

      projectIter->second.versions[version.id] = version;

      std::filesystem::create_directory(priv->scanFolder / std::to_string(groupId) / std::to_string(projectId) / std::to_string(version.id));

      return version.id;
    }
  }

  return -1;
}

void ScanDatabase::editVersion(size_t id, size_t groupId, size_t projectId, const std::string& name)
{
  const std::lock_guard<std::recursive_mutex> lock(priv->mutex);

  auto groupIter = priv->groups.find(groupId);

  if (groupIter != priv->groups.end())
  {
    auto projectIter = groupIter->second.projects.find(projectId);
    if (projectIter != groupIter->second.projects.end())
    {
      //soci::rowset<int> rs = (sql->prepare << "UPDATE Version SET Name=:name WHERE ID=:id", soci::use(name, "name"), soci::use(id, "id"));

      projectIter->second.versions[id].name = name;
    }
  }
}

void ScanDatabase::removeVersion(size_t id, size_t groupId, size_t projectId)
{
  const std::lock_guard<std::recursive_mutex> lock(priv->mutex);

  auto groupIter = priv->groups.find(groupId);

  if (groupIter != priv->groups.end())
  {
    auto projectIter = groupIter->second.projects.find(projectId);
    if (projectIter != groupIter->second.projects.end())
    {
      auto versionIter = projectIter->second.versions.find(id);
      if (versionIter != projectIter->second.versions.end())
      {
        //soci::rowset<int> rs = (sql->prepare << "DELETE FROM Version WHERE ID=:id", soci::use(id, "id"));

        for (auto scanIter = versionIter->second.scans.begin(); scanIter != versionIter->second.scans.end(); scanIter++)
          removeScan(scanIter->first, groupId, projectId, id);

        projectIter->second.versions.erase(id);

        std::filesystem::remove_all(priv->scanFolder / std::to_string(groupId) / std::to_string(projectId) / std::to_string(id));
      }
    }
  }
}

std::unordered_map<size_t, std::string> ScanDatabase::getVersions(size_t groupId, size_t projectId)
{
  std::unordered_map<size_t, std::string> ret;
  const std::lock_guard<std::recursive_mutex> lock(priv->mutex);

  auto groupIter = priv->groups.find(groupId);

  if (groupIter != priv->groups.end())
  {
    auto projectIter = groupIter->second.projects.find(projectId);
    if (projectIter != groupIter->second.projects.end())
    {
      for (auto iter = projectIter->second.versions.begin(); iter != projectIter->second.versions.end(); iter++)
        ret[iter->first] = iter->second.name;
    }
  }

  return ret;
}

size_t ScanDatabase::createScan(const std::string& name, size_t groupId, size_t projectId, size_t versionId)
{
  const std::lock_guard<std::recursive_mutex> lock(priv->mutex);

  auto groupIter = priv->groups.find(groupId);

  if (groupIter != priv->groups.end())
  {
    auto projectIter = groupIter->second.projects.find(projectId);
    if (projectIter != groupIter->second.projects.end())
    {
      auto versionIter = projectIter->second.versions.find(projectId);
      if (versionIter != projectIter->second.versions.end())
      {
        //soci::rowset<int> rs = (sql->prepare << "INSERT INTO Scan (Name, GroupID, ProjectID, VersionID) VALUES (:name,:groupId,:projectId,:versionId) RETURNING ID", soci::use(name, "name"), soci::use(groupId, "groupId"), soci::use(projectId, "projectId"), soci::use(versionId, "versionId"));

        //size_t id = *rs.begin();

        ScanData* scanData = new ScanData();
        scanData->id = priv->scanId.fetch_add(1);
        scanData->name = name;

        versionIter->second.scans[scanData->id] = scanData;

        std::filesystem::create_directory(priv->scanFolder / std::to_string(groupId) / std::to_string(projectId) / std::to_string(versionId) /
                                          std::to_string(scanData->id));

        return scanData->id;
      }
    }
  }

  return -1;
}

void ScanDatabase::editScan(size_t id, size_t groupId, size_t projectId, size_t versionId, const std::string& name)
{
  const std::lock_guard<std::recursive_mutex> lock(priv->mutex);

  auto groupIter = priv->groups.find(groupId);

  if (groupIter != priv->groups.end())
  {
    auto projectIter = groupIter->second.projects.find(projectId);
    if (projectIter != groupIter->second.projects.end())
    {
      auto versionIter = projectIter->second.versions.find(projectId);
      if (versionIter != projectIter->second.versions.end())
      {
        //soci::rowset<int> rs = (sql->prepare << "UPDATE Scan SET Name=:name WHERE ID=:id", soci::use(name, "name"), soci::use(id, "id"));

        versionIter->second.scans[id]->name = name;
      }
    }
  }
}

void ScanDatabase::removeScan(size_t id, size_t groupId, size_t projectId, size_t versionId)
{
  const std::lock_guard<std::recursive_mutex> lock(priv->mutex);

  auto groupIter = priv->groups.find(groupId);

  if (groupIter != priv->groups.end())
  {
    auto projectIter = groupIter->second.projects.find(projectId);
    if (projectIter != groupIter->second.projects.end())
    {
      auto versionIter = projectIter->second.versions.find(projectId);
      if (versionIter != projectIter->second.versions.end())
      {
        //soci::rowset<int> rs = (sql->prepare << "DELETE FROM Scan WHERE ID=:id", soci::use(id, "id"));

        if (versionIter->second.scans.contains(id))
          delete versionIter->second.scans[id];
        versionIter->second.scans.erase(id);

        std::filesystem::remove_all(priv->scanFolder / std::to_string(groupId) / std::to_string(projectId) / std::to_string(versionId) / std::to_string(id));
      }
    }
  }
}

std::unordered_map<size_t, std::string> ScanDatabase::getScans(size_t groupId, size_t projectId, size_t versionId)
{
  std::unordered_map<size_t, std::string> ret;
  const std::lock_guard<std::recursive_mutex> lock(priv->mutex);

  auto groupIter = priv->groups.find(groupId);

  if (groupIter != priv->groups.end())
  {
    auto projectIter = groupIter->second.projects.find(projectId);
    if (projectIter != groupIter->second.projects.end())
    {
      auto versionIter = projectIter->second.versions.find(projectId);
      if (versionIter != projectIter->second.versions.end())
      {
        for (auto iter = versionIter->second.scans.begin(); iter != versionIter->second.scans.end(); iter++)
          ret[iter->first] = iter->second->name;
      }
    }
  }

  return ret;
}

void ScanDatabase::addZipData(size_t id, size_t groupId, size_t projectId, size_t versionId, size_t scanId, const std::vector<char>& data)
{
  const std::lock_guard<std::recursive_mutex> lock(priv->mutex);

  auto groupIter = priv->groups.find(groupId);

  if (groupIter != priv->groups.end())
  {
    auto projectIter = groupIter->second.projects.find(projectId);
    if (projectIter != groupIter->second.projects.end())
    {
      auto versionIter = projectIter->second.versions.find(projectId);
      if (versionIter != projectIter->second.versions.end())
      {
        auto scanIter = versionIter->second.scans.find(scanId);

        if (scanIter != versionIter->second.scans.end())
        {
          std::string filename =
              (priv->scanFolder / std::to_string(groupId) / std::to_string(projectId) / std::to_string(versionId) / std::to_string(id) / "data.zip").string();
          std::ofstream out(filename, std::ios::binary);
          out.write(data.data(), static_cast<std::streamsize>(data.size()));

          scanIter->second->dataPath = filename;
          scanIter->second->type = ScanDataTypeE::Archive;
        }
      }
    }
  }
}

void ScanDatabase::addGitData(size_t id, size_t groupId, size_t projectId, size_t versionId, size_t scanId, const std::string& url, const std::string& sha)
{
  const std::lock_guard<std::recursive_mutex> lock(priv->mutex);

  auto groupIter = priv->groups.find(groupId);

  if (groupIter != priv->groups.end())
  {
    auto projectIter = groupIter->second.projects.find(projectId);
    if (projectIter != groupIter->second.projects.end())
    {
      auto versionIter = projectIter->second.versions.find(projectId);
      if (versionIter != projectIter->second.versions.end())
      {
        auto scanIter = versionIter->second.scans.find(scanId);

        if (scanIter != versionIter->second.scans.end())
        {
          scanIter->second->dataPath = url;
          scanIter->second->revision = sha;
          scanIter->second->type = ScanDataTypeE::Git;
        }
      }
    }
  }
}

void ScanDatabase::addSvnData(size_t id, size_t groupId, size_t projectId, size_t versionId, size_t scanId, const std::string& url, const std::string& revision)
{
  const std::lock_guard<std::recursive_mutex> lock(priv->mutex);

  auto groupIter = priv->groups.find(groupId);

  if (groupIter != priv->groups.end())
  {
    auto projectIter = groupIter->second.projects.find(projectId);
    if (projectIter != groupIter->second.projects.end())
    {
      auto versionIter = projectIter->second.versions.find(projectId);
      if (versionIter != projectIter->second.versions.end())
      {
        auto scanIter = versionIter->second.scans.find(scanId);

        if (scanIter != versionIter->second.scans.end())
        {
          scanIter->second->dataPath = url;
          scanIter->second->revision = revision;
          scanIter->second->type = ScanDataTypeE::Svn;
        }
      }
    }
  }
}

void ScanDatabase::startScan(size_t id, size_t groupId, size_t projectId, size_t versionId, size_t scanId)
{
  const std::lock_guard<std::recursive_mutex> lock(priv->mutex);

  auto groupIter = priv->groups.find(groupId);

  if (groupIter != priv->groups.end())
  {
    auto projectIter = groupIter->second.projects.find(projectId);
    if (projectIter != groupIter->second.projects.end())
    {
      auto versionIter = projectIter->second.versions.find(projectId);
      if (versionIter != projectIter->second.versions.end())
      {
        auto scanIter = versionIter->second.scans.find(scanId);

        if (scanIter != versionIter->second.scans.end())
        {
          const std::lock_guard<std::recursive_mutex> lock(scanIter->second->mutex);
          scanIter->second->status = ScanDataStatusE::Started;
        }
      }
    }
  }
}

void ScanDatabase::abortScan(size_t id, size_t groupId, size_t projectId, size_t versionId, size_t scanId)
{
  const std::lock_guard<std::recursive_mutex> lock(priv->mutex);

  auto groupIter = priv->groups.find(groupId);

  if (groupIter != priv->groups.end())
  {
    auto projectIter = groupIter->second.projects.find(projectId);
    if (projectIter != groupIter->second.projects.end())
    {
      auto versionIter = projectIter->second.versions.find(projectId);
      if (versionIter != projectIter->second.versions.end())
      {
        auto scanIter = versionIter->second.scans.find(scanId);

        if (scanIter != versionIter->second.scans.end())
        {
          const std::lock_guard<std::recursive_mutex> lock(scanIter->second->mutex);
          scanIter->second->status = ScanDataStatusE::Aborted;
        }
      }
    }
  }
}

ScanData* ScanDatabase::getScan(size_t id, size_t groupId, size_t projectId, size_t versionId, size_t scanId)
{
  const std::lock_guard<std::recursive_mutex> lock(priv->mutex);

  auto groupIter = priv->groups.find(groupId);

  if (groupIter != priv->groups.end())
  {
    auto projectIter = groupIter->second.projects.find(projectId);
    if (projectIter != groupIter->second.projects.end())
    {
      auto versionIter = projectIter->second.versions.find(projectId);
      if (versionIter != projectIter->second.versions.end())
      {
        auto scanIter = versionIter->second.scans.find(scanId);

        if (scanIter != versionIter->second.scans.end())
        {
          return scanIter->second;
        }
      }
    }
  }

  return nullptr;
}

ScanData* ScanDatabase::getNextScan()
{
  const std::lock_guard<std::recursive_mutex> lock(priv->mutex);

  for (auto groupIter = priv->groups.begin(); groupIter != priv->groups.end(); groupIter++)
  {
    for (auto prjIter = groupIter->second.projects.begin(); prjIter != groupIter->second.projects.end(); prjIter++)
    {
      for (auto versionIter = prjIter->second.versions.begin(); versionIter != prjIter->second.versions.end(); versionIter++)
      {
        for (auto scanIter = versionIter->second.scans.begin(); scanIter != versionIter->second.scans.end(); scanIter++)
        {
          if (scanIter->second->status == ScanDataStatusE::Started)
          {
            return scanIter->second;
          }
        }
      }
    }
  }

  return nullptr;
}