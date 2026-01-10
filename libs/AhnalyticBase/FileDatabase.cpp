#include "FileDatabase.hpp"

#include "soci/soci.h"
#include "soci/sqlite3/soci-sqlite3.h"
// #include "soci/mysql/soci-mysql.h"
// #include "soci/postgresql/soci-postgresql.h"

#include "AhnalyticBase\SourceStructureTree.hpp"

#include <algorithm>

FileDatabase::FileDatabase(DBType type, std::string connectionString) : Database(type, connectionString)
{
  if (sql != nullptr && sql->is_connected())
  {
    initTables();
  }
}

void FileDatabase::initTables()
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);

  (*sql) << "CREATE TABLE IF NOT EXISTS \"Repo\" ("
            "\"ID\" INTEGER,"
            "\"Name\" TEXT,"
            "\"Url\" TEXT,"
            "\"Licence\" TEXT,"
            "PRIMARY KEY(\"ID\")"
            ")";

  (*sql) << "CREATE TABLE IF NOT EXISTS \"Name\" ("
            "\"ID\" INTEGER,"
            "\"Name\" TEXT UNIQUE,"
            "PRIMARY KEY(\"ID\")"
            ")";

  (*sql) << "CREATE TABLE IF NOT EXISTS \"File\" ("
            "\"ID\" INTEGER,"
            "\"DataID\" INTEGER,"
            "\"FileIndex\" INTEGER,"
            "\"PathID\" INTEGER,"
            "\"TagID\" INTEGER,"
            "PRIMARY KEY(\"ID\")"
            ")";

  (*sql) << "CREATE TABLE IF NOT EXISTS \"Tag\" ("
            "\"ID\" INTEGER,"
            "\"TagName\" TEXT,"
            "\"Sha\" TEXT,"
            "PRIMARY KEY(\"ID\")"
            ")";

  (*sql) << "CREATE TABLE IF NOT EXISTS \"SourceTreeData\" ("
            "\"ID\" INTEGER,"
            "\"Data\" BLOB,"
            "PRIMARY KEY(\"ID\")"
            ")";
}

uint32_t FileDatabase::createFile(uint32_t dataId, uint32_t index, uint32_t pathId, uint32_t tagId)
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);

  soci::rowset<int> rs = (sql->prepare << "INSERT INTO File (DataID,FileIndex,PathID,TagID) VALUES (:dataId,:fileIndex,:pathId,:tagId) RETURNING ID",
                          soci::use(dataId, "dataId"), soci::use(index, "fileIndex"), soci::use(pathId, "pathId"), soci::use(tagId, "tagId"));

  return *rs.begin();
}

void FileDatabase::createFiles(uint32_t dataId, std::vector<uint32_t> indices, std::vector<uint32_t> pathIds, uint32_t tagId)
{
  std::vector<uint32_t> dataIds(pathIds.size());
  std::vector<uint32_t> tagIds(pathIds.size());

  std::fill(dataIds.begin(), dataIds.end(), dataId);
  std::fill(tagIds.begin(), tagIds.end(), tagId);

  const std::lock_guard<std::recursive_mutex> lock(mutex);

  sql->begin();
  soci::statement statement = (sql->prepare << "INSERT INTO File (DataID,FileIndex,PathID,TagID) VALUES (:dataId,:fileIndex,:pathId,:tagId)",
                               soci::use(dataIds, "dataId"), soci::use(indices, "fileIndex"), soci::use(pathIds, "pathId"), soci::use(tagIds, "tagId"));

  statement.execute(true);
  sql->commit();

  /*
  soci::rowset<int> rs = (sql->prepare << "INSERT INTO File (DataID,FileIndex,PathID,TagID) VALUES (:dataId,:fileIndex,:pathId,:tagId) RETURNING ID",
                          soci::use(dataId, "dataId"), soci::use(index, "fileIndex"), soci::use(pathId, "pathId"), soci::use(tagId, "tagId"));

  return *rs.begin();
  */
}

uint32_t FileDatabase::createRepoData(const std::string& name, const std::string& url, const std::string& license)
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);

  soci::rowset<int> rs = (sql->prepare << "INSERT INTO Repo (Name,Url,Licence) VALUES (:name,:url,:license) RETURNING ID", soci::use(name, "name"),
                          soci::use(url, "url"), soci::use(license, "license"));

  return *rs.begin();
}

uint32_t FileDatabase::createTag(const std::string& tagName, const std::string& sha)
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);

  soci::rowset<int> rs =
      (sql->prepare << "INSERT INTO Tag (TagName,Sha) VALUES (:tagName,:sha) RETURNING ID", soci::use(tagName, "tagName"), soci::use(sha, "sha"));

  return *rs.begin();
}

std::unordered_map<std::string, std::string> FileDatabase::getTags() const
{
  std::unordered_map<std::string, std::string> ret;

  soci::rowset<soci::row> rows = (sql->prepare << "SELECT TagName, Sha FROM Tag");

  for (const soci::row& r : rows)
    ret[r.get<std::string>("TagName")] = r.get<std::string>("Sha");

  return ret;
}

void FileDatabase::iterateFiles(std::function<void(uint32_t, const std::string&, const std::string&, SourceStructureTree*)> callback)
{
  soci::rowset<soci::row> rowSet = (sql->prepare << "SELECT DataID,FileIndex,PathID,TagID FROM File");

  uint32_t lastSourceTreeId = std::numeric_limits<uint32_t>::max();
  SourceStructureTree* root = nullptr;

  for (const soci::row& r : rowSet)
  {
    uint32_t sourceTreeDataID = r.get<uint32_t>("DataID");
    uint32_t fileIndex = r.get<uint32_t>("FileIndex");
    uint32_t pathId = r.get<uint32_t>("PathID");
    uint32_t tagId = r.get<uint32_t>("TagID");

    std::string licence;
    std::string sha;

    if (lastSourceTreeId != sourceTreeDataID)
    {
      std::vector<char> sourceTreeData;
      lastSourceTreeId = sourceTreeDataID;
      getSourceTreeData(sourceTreeDataID, sourceTreeData);

      std::vector<FlatNodeDeDupData> nodeListTestOut;
      std::vector<uint32_t> indexListTestOut;
      SourceStructureTree::deserialize(sourceTreeData, nodeListTestOut, indexListTestOut, nullptr);
      root = (SourceStructureTree*)rebuildTree(nodeListTestOut, indexListTestOut);
    }

    if (root != nullptr && root->children.size() > fileIndex)
      callback(fileIndex, sha, licence, (SourceStructureTree*)root->children.at(fileIndex));
  }
}
