#include "AhnalyticBase/database/ScanDatabase.hpp"

#include "soci/soci.h"
#include "soci/sqlite3/soci-sqlite3.h"

#include <atomic>
#include <fstream>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

/*************************************
 * Data serialisation/deserialisation
 *************************************/

NLOHMANN_JSON_SERIALIZE_ENUM(ScanDataTypeE, {{ScanDataTypeE::Git, "Git"}, {ScanDataTypeE::Svn, "Svn"}, {ScanDataTypeE::Archive, "Archive"}})

NLOHMANN_JSON_SERIALIZE_ENUM(ScanDataStatusE, {{ScanDataStatusE::Idle, "Idle"},
                                               {ScanDataStatusE::Started, "Started"},
                                               {ScanDataStatusE::Running, "Running"},
                                               {ScanDataStatusE::Aborted, "Aborted"},
                                               {ScanDataStatusE::Finished, "Finished"}})

NLOHMANN_JSON_SERIALIZE_ENUM(TreeSearchResult::ResultSourceTypeE, {{TreeSearchResult::Stackexchange, "Stackexchange"},
                                                                   {TreeSearchResult::Github, "Github"},
                                                                   {TreeSearchResult::SourceForge, "SourceForge"}})

inline void to_json(nlohmann::json& j, const TreeSearchResultSet& v)
{
  j = {{"baseStart", v.baseStart}, {"baseEnd", v.baseEnd}, {"searchStart", v.searchStart}, {"searchEnd", v.searchEnd}};
}

inline void from_json(const nlohmann::json& j, TreeSearchResultSet& v)
{
  j.at("baseStart").get_to(v.baseStart);
  j.at("baseEnd").get_to(v.baseEnd);
  j.at("searchStart").get_to(v.searchStart);
  j.at("searchEnd").get_to(v.searchEnd);
}
inline void to_json(nlohmann::json& j, const TreeSearchResult& r)
{
  j = {{"sets", static_cast<const std::vector<TreeSearchResultSet>&>(r)},
       {"sourceDb", r.sourceDb},
       {"sourceFile", r.sourceFile},
       {"sourceRevision", r.sourceRevision},
       {"sourceInternalId", r.sourceInternalId},
       {"searchFile", r.searchFile},
       {"type", r.type},
       {"sourceContent", r.sourceContent},
       {"searchContent", r.searchContent},
       {"licence", r.licence}};
}

inline void from_json(const nlohmann::json& j, TreeSearchResult& r)
{
  r.clear();
  for (const auto& e : j.at("sets"))
    r.push_back(e.get<TreeSearchResultSet>());

  j.at("sourceDb").get_to(r.sourceDb);
  j.at("sourceFile").get_to(r.sourceFile);
  j.at("sourceRevision").get_to(r.sourceRevision);
  j.at("sourceInternalId").get_to(r.sourceInternalId);
  j.at("searchFile").get_to(r.searchFile);
  j.at("type").get_to(r.type);
  j.at("sourceContent").get_to(r.sourceContent);
  j.at("searchContent").get_to(r.searchContent);
  j.at("licence").get_to(r.licence);
}

inline void to_json(nlohmann::json& j, const ScanData& s)
{
  const std::lock_guard<std::recursive_mutex> lock(s.mutex);

  j = {{"id", s.id},
       {"name", s.name},
       {"type", s.type},
       {"dataPath", s.dataPath},
       {"revision", s.revision},
       {"status", s.status},
       {"results", s.results},
       {"deepResults", s.deepResults},
       {"maxCount", s.maxCount},
       {"finishedCount", s.finishedCount}};
}

inline void from_json(const nlohmann::json& j, ScanData& s)
{
  const std::lock_guard<std::recursive_mutex> lock(s.mutex);

  j.at("id").get_to(s.id);
  j.at("name").get_to(s.name);
  j.at("type").get_to(s.type);
  j.at("dataPath").get_to(s.dataPath);
  j.at("revision").get_to(s.revision);
  j.at("status").get_to(s.status);

  if (s.status == ScanDataStatusE::Running)
    s.status = ScanDataStatusE::Aborted;

  j.at("results").get_to(s.results);
  j.at("deepResults").get_to(s.deepResults);
  j.at("maxCount").get_to(s.maxCount);
  j.at("finishedCount").get_to(s.finishedCount);
}

inline void to_json(nlohmann::json& j, const VersionData& v)
{
  j["id"] = v.id;
  j["name"] = v.name;

  j["scans"] = nlohmann::json::object();
  for (const auto& [id, scan] : v.scans)
    j["scans"][std::to_string(id)] = *scan;
}

inline void from_json(const nlohmann::json& j, VersionData& v)
{
  j.at("id").get_to(v.id);
  j.at("name").get_to(v.name);

  v.scans.clear();
  for (auto& [key, val] : j.at("scans").items())
  {
    auto scan = std::make_shared<ScanData>();
    val.get_to(*scan);
    v.scans[std::stoull(key)] = scan;
  }
}

inline void to_json(nlohmann::json& j, const ProjectData& p)
{
  j["id"] = p.id;
  j["name"] = p.name;
  j["versions"] = p.versions;
}

inline void from_json(const nlohmann::json& j, ProjectData& p)
{
  j.at("id").get_to(p.id);
  j.at("name").get_to(p.name);
  j.at("versions").get_to(p.versions);
}

inline void to_json(nlohmann::json& j, const GroupData& g)
{
  j["id"] = g.id;
  j["name"] = g.name;
  j["projects"] = g.projects;
}

inline void from_json(const nlohmann::json& j, GroupData& g)
{
  j.at("id").get_to(g.id);
  j.at("name").get_to(g.name);
  j.at("projects").get_to(g.projects);
}

/*************************************
 * ScanDatabase
 **************************************/

class ScanDatabasePrivate
{
public:
  std::unordered_map<size_t, GroupData> groups;
  std::filesystem::path scanFolder;
  std::recursive_mutex mutex;

  std::atomic<int> groupId = 0;
  std::atomic<int> projectId = 0;
  std::atomic<int> versionId = 0;
  std::atomic<int> scanId = 0;

private:
protected:
};

ScanDatabase::ScanDatabase(const std::filesystem::path& scanFolder) : priv(new ScanDatabasePrivate())
{
  priv->scanFolder = scanFolder;
  load();
}

ScanDatabase::~ScanDatabase()
{
  save();
}

void ScanDatabase::load()
{
  std::filesystem::path filename = priv->scanFolder / "status.json";
  if (std::filesystem::exists(filename))
  {
    std::ifstream ifs(filename);
    nlohmann::json j;
    ifs >> j;

    priv->groups.clear();
    for (const auto& [key, val] : j.items())
      priv->groups.emplace(std::stoull(key), val.get<GroupData>());
  }

  const std::lock_guard<std::recursive_mutex> lock(priv->mutex);

  priv->groupId = 0;
  priv->projectId = 0;
  priv->versionId = 0;
  priv->scanId = 0;

  auto updateMaximum = [](std::atomic<int>& curVal, int const& value)
  {
    int prevVal = curVal;
    while (prevVal < value && !curVal.compare_exchange_weak(prevVal, value))
    {
    }
  };

  for (auto groupIter = priv->groups.begin(); groupIter != priv->groups.end(); groupIter++)
  {
    updateMaximum(priv->groupId, groupIter->first);
    for (auto prjIter = groupIter->second.projects.begin(); prjIter != groupIter->second.projects.end(); prjIter++)
    {
      updateMaximum(priv->projectId, prjIter->first);
      for (auto versionIter = prjIter->second.versions.begin(); versionIter != prjIter->second.versions.end(); versionIter++)
      {
        updateMaximum(priv->versionId, versionIter->first);
        for (auto scanIter = versionIter->second.scans.begin(); scanIter != versionIter->second.scans.end(); scanIter++)
        {
          updateMaximum(priv->scanId, scanIter->first);
        }
      }
    }
  }

  priv->groupId.fetch_add(1);
  priv->projectId.fetch_add(1);
  priv->versionId.fetch_add(1);
  priv->scanId.fetch_add(1);
}

void ScanDatabase::save()
{
  nlohmann::json j = nlohmann::json::object();
  for (const auto& [id, group] : priv->groups)
    j[std::to_string(id)] = group;

  std::filesystem::path filename = priv->scanFolder / "status.json";
  std::ofstream ofs(filename.string(), std::ios::out | std::ios::trunc);
  ofs << j.dump(2);
}

size_t ScanDatabase::createGroup(const std::string& name)
{
  const std::lock_guard<std::recursive_mutex> lock(priv->mutex);

  GroupData group;
  group.id = priv->groupId.fetch_add(1);
  group.name = name;

  priv->groups[group.id] = group;

  std::filesystem::create_directory(priv->scanFolder / std::to_string(group.id));

  save();

  return group.id;
}

void ScanDatabase::editGroup(size_t id, const std::string& name)
{
  const std::lock_guard<std::recursive_mutex> lock(priv->mutex);

  if (priv->groups.contains(id))
  {
    priv->groups[id].name = name;
  }

  save();
}

void ScanDatabase::removeGroup(size_t id)
{
  auto groupIter = priv->groups.find(id);
  if (groupIter != priv->groups.end())
  {
    std::unordered_map<size_t, ProjectData> projects = groupIter->second.projects;

    for (auto projectIter = projects.begin(); projectIter != projects.end(); projectIter++)
      removeProject(projectIter->first, id);

    std::filesystem::remove_all(priv->scanFolder / std::to_string(id));

    priv->groups.erase(id);

    save();
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
    ProjectData project;
    project.id = priv->projectId.fetch_add(1);
    project.name = name;

    groupIter->second.projects[project.id] = project;

    std::filesystem::create_directory(priv->scanFolder / std::to_string(groupId) / std::to_string(project.id));

    save();

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
      groupIter->second.projects[id].name = name;

      save();
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
      std::unordered_map<size_t, VersionData> versions = projectIter->second.versions;

      for (auto versionIter = versions.begin(); versionIter != versions.end(); versionIter++)
        removeVersion(versionIter->first, groupId, id);

      groupIter->second.projects.erase(id);

      std::filesystem::remove_all(priv->scanFolder / std::to_string(groupId) / std::to_string(id));

      save();
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
    for (auto iter = groupIter->second.projects.begin(); iter != groupIter->second.projects.end(); iter++)
      ret[iter->first] = iter->second.name;
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
      VersionData version;
      version.id = priv->versionId.fetch_add(1);
      version.name = name;

      projectIter->second.versions[version.id] = version;

      std::filesystem::create_directory(priv->scanFolder / std::to_string(groupId) / std::to_string(projectId) / std::to_string(version.id));

      save();

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
      projectIter->second.versions[id].name = name;
      save();
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
        std::unordered_map<size_t, std::shared_ptr<ScanData>> scans = versionIter->second.scans;

        for (auto scanIter = scans.begin(); scanIter != scans.end(); scanIter++)
          removeScan(scanIter->first, groupId, projectId, id);

        projectIter->second.versions.erase(id);

        std::filesystem::remove_all(priv->scanFolder / std::to_string(groupId) / std::to_string(projectId) / std::to_string(id));

        save();
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
        auto scanData = std::make_shared<ScanData>();
        scanData->id = priv->scanId.fetch_add(1);
        scanData->name = name;

        versionIter->second.scans[scanData->id] = scanData;

        std::filesystem::create_directory(priv->scanFolder / std::to_string(groupId) / std::to_string(projectId) / std::to_string(versionId) /
                                          std::to_string(scanData->id));

        save();

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
        versionIter->second.scans[id]->name = name;

        save();
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
        // if (versionIter->second.scans.contains(id))
        // delete versionIter->second.scans[id];
        versionIter->second.scans.erase(id);

        std::filesystem::remove_all(priv->scanFolder / std::to_string(groupId) / std::to_string(projectId) / std::to_string(versionId) / std::to_string(id));

        save();
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

          save();
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
          scanIter->second->results.clear();
          scanIter->second->deepResults.clear();
          scanIter->second->status = ScanDataStatusE::Started;

          save();
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

          save();
        }
      }
    }
  }
}

std::shared_ptr<ScanData> ScanDatabase::getScan(size_t id, size_t groupId, size_t projectId, size_t versionId, size_t scanId)
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

std::shared_ptr<ScanData> ScanDatabase::getNextScan()
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