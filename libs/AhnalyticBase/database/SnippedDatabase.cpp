#include "AhnalyticBase/database/SnippedDatabase.hpp"

#include "soci/soci.h"
#include "soci/sqlite3/soci-sqlite3.h"
// #include "soci/mysql/soci-mysql.h"
//#include "soci/postgresql/soci-postgresql.h"

#include "AhnalyticBase/tree/SourceStructureTree.hpp"

SnippedDatabase::SnippedDatabase(DBType type, std::string connectionString) : Database(type, connectionString)
{
  if (sql != nullptr && sql->is_connected())
  {
    initTables();
  }
}

void SnippedDatabase::initTables()
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);

  (*sql) << ("CREATE TABLE IF NOT EXISTS \"Snipped\" ("
             "\"ID\" INTEGER,"
             "\"InternalID\" INTEGER,"
             "\"LicenceId\" INTEGER,"
             "\"SourceTreeDataID\" INTEGER,"
             "\"SourceTreeDataIndex\" INTEGER,"
             "PRIMARY KEY(\"ID\")"
             ")");

  (*sql) << "CREATE TABLE IF NOT EXISTS \"Licence\" ("
            "\"ID\" INTEGER,"
            "\"Licence\" TEXT UNIQUE,"
            "PRIMARY KEY(\"ID\")"
            ")";

  (*sql) << "CREATE TABLE IF NOT EXISTS \"SourceTreeData\" ("
            "\"ID\" INTEGER,"
            "\"Data\" BLOB,"
            "PRIMARY KEY(\"ID\")"
            ")";
}

uint32_t SnippedDatabase::createSnipped(uint32_t internalId, uint32_t licenceId, uint32_t sourceTreeDataID, uint32_t sourceTreeDataIndex)
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);

  soci::rowset<int> rs = (sql->prepare << "INSERT INTO Snipped (InternalID,LicenceId,SourceTreeDataID,SourceTreeDataIndex) VALUES "
                                          "(:internalId,:licenceId,:sourceTreeDataID,:sourceTreeDataIndex) RETURNING ID",
                          soci::use(internalId, "internalId"), soci::use(licenceId, "licenceId"), soci::use(sourceTreeDataID, "sourceTreeDataID"),
                          soci::use(sourceTreeDataIndex, "sourceTreeDataIndex"));
  return *rs.begin();
}

uint32_t SnippedDatabase::getLicence(std::string licence)
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);

  soci::rowset<soci::row> rowSet = (sql->prepare << "SELECT ID FROM Licence WHERE Licence=:licence", soci::use(licence, "licence"));

  uint32_t ret = 0;
  for (const soci::row& r : rowSet)
    ret = r.get<uint32_t>("ID");

  return ret;
}

std::string SnippedDatabase::getLicence(uint32_t licenceId)
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);

  soci::rowset<soci::row> rowSet = (sql->prepare << "SELECT Licence FROM Licence WHERE ID=:licenceId", soci::use(licenceId, "licenceId"));

  std::string ret;
  for (const soci::row& r : rowSet)
    ret = r.get<std::string>("Licence");

  return ret;
}

uint32_t SnippedDatabase::insertLicence(std::string licence)
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);

  sql->begin();
  soci::statement statement = (sql->prepare << "INSERT OR IGNORE INTO Licence (Licence) VALUES (:licence)", soci::use(licence, "licence"));

  statement.execute(true);
  sql->commit();

  return getLicence(licence);
}

void SnippedDatabase::iterateSnippeds(std::function<void(uint32_t, const std::string&, SourceStructureTree* tree)> callback)
{
  soci::rowset<soci::row> rowSet = (sql->prepare << "SELECT InternalID,LicenceId,SourceTreeDataID,SourceTreeDataIndex FROM Snipped");

  std::unordered_map<uint32_t, std::string> licenceLookup;

  uint32_t lastSourceTreeId = std::numeric_limits<uint32_t>::max();
  SourceStructureTree* root = nullptr;

  for (const soci::row& r : rowSet)
  {
    uint32_t internalId = r.get<uint32_t>("InternalID");
    uint32_t licenceId = r.get<uint32_t>("LicenceId");
    uint32_t sourceTreeDataID = r.get<uint32_t>("SourceTreeDataID");
    uint32_t sourceTreeDataIndex = r.get<uint32_t>("SourceTreeDataIndex");

    std::string licence;
    auto licIter = licenceLookup.find(licenceId);
    if (licIter == licenceLookup.end())
    {
      licence = getLicence(licenceId);
      licenceLookup[licenceId] = licence;
    }
    else
      licence = licIter->second;

    if (lastSourceTreeId != sourceTreeDataID)
    {
      delete root;
      root = nullptr;
      std::vector<char> sourceTreeData;
      lastSourceTreeId = sourceTreeDataID;
      getSourceTreeData(sourceTreeDataID, sourceTreeData);

      std::vector<FlatNodeDeDupData> nodeListTestOut;
      std::vector<uint32_t> indexListTestOut;
      SourceStructureTree::deserialize(sourceTreeData, nodeListTestOut, indexListTestOut, nullptr);
      root = (SourceStructureTree*)rebuildTree(nodeListTestOut, indexListTestOut);
    }

    if (root != nullptr && root->children.size() > sourceTreeDataIndex)
      callback(internalId, licence, (SourceStructureTree*)root->children.at(sourceTreeDataIndex));
  }

  delete root;
}