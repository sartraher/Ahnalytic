#include "Database.hpp"

#include "soci/soci.h"
#include "soci/sqlite3/soci-sqlite3.h"
// #include "soci/mysql/soci-mysql.h"
#include "soci/postgresql/soci-postgresql.h"

Database::Database(DBType type, std::string connectionString)
{
  switch (type)
  {
  case DBType::SQLite:
    sql = new soci::session(*soci::factory_sqlite3(), connectionString);
    break;
  case DBType::MySql:
    // sql = new soci::session(soci::mysql, connectionString);
    break;
  case DBType::Postgres:
    // sql = new soci::session(soci::postgresql, connectionString);
    break;
  default:
    break;
  }
}

Database::~Database()
{
}

uint32_t Database::createSourceTreeData(const ahn::vector<char>& data)
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);

  soci::blob dataBlob = soci::blob(*sql);
  dataBlob.append(data.data(), data.size());

  soci::rowset<int> rs = (sql->prepare << "INSERT OR IGNORE INTO SourceTreeData (Data) VALUES (:data) RETURNING ID", soci::use(dataBlob, "data"));
  return *rs.begin();
}

uint32_t Database::createSourceTreeData(uint32_t dataId, const ahn::vector<char>& data)
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);

  soci::blob dataBlob(*sql);
  dataBlob.append(data.data(), data.size());

  soci::rowset<uint32_t> rs = (sql->prepare << "INSERT OR IGNORE INTO SourceTreeData (ID,Data) VALUES (:dataId,:data) RETURNING ID",
                               soci::use(dataId, "dataId"), soci::use(dataBlob, "data"));

  auto it = rs.begin();
  if (it != rs.end())
    return *it;

  return dataId;
}

void Database::getSourceTreeData(uint32_t id, ahn::vector<char>& data)
{
  soci::blob dataBlob(*sql);

  soci::statement st = (sql->prepare << "SELECT Data FROM SourceTreeData WHERE ID = :id", soci::use(id), soci::into(dataBlob));

  st.execute();

  if (st.fetch()) // fetch one row
  {
    std::size_t size = dataBlob.get_len();
    data.resize(size);
    size_t readAmount = dataBlob.read_from_start(data.data(), size);
    data.resize(readAmount);
  }
  else
  {
    data.clear(); // no row found
  }
}

std::string Database::getName(uint32_t id)
{
  std::string ret;

  *sql << "SELECT Name FROM Name WHERE ID = :id", soci::use(id), soci::into(ret);

  return ret;
}

std::unordered_map<std::string, uint32_t> Database::getNames()
{
  soci::rowset<soci::row> rowSet = (sql->prepare << "SELECT ID,Name FROM Name");

  std::unordered_map<std::string, uint32_t> ret;
  for (const soci::row& r : rowSet)
    ret[r.get<std::string>("Name")] = r.get<uint32_t>("ID");

  return ret;
}

std::unordered_map<std::string, uint32_t> Database::insertNames(std::vector<std::string> names)
{
  sql->begin();
  soci::statement statement = (sql->prepare << "INSERT OR IGNORE INTO Name (Name) VALUES (:name)", soci::use(names, "name"));

  statement.execute(true);
  sql->commit();

  return getNames();
}

std::unordered_map<std::string, uint32_t> Database::getTypes()
{
  soci::rowset<soci::row> rowSet = (sql->prepare << "SELECT ID,Type FROM Type");

  std::unordered_map<std::string, uint32_t> ret;
  for (const soci::row& r : rowSet)
    ret[r.get<std::string>("Type")] = r.get<uint32_t>("ID");

  return ret;
}

std::unordered_map<std::string, uint32_t> Database::insertTypes(std::vector<std::string> types)
{
  sql->begin();
  soci::statement statement = (sql->prepare << "INSERT OR IGNORE INTO Type (Type) VALUES (:type)", soci::use(types, "type"));

  statement.execute(true);
  sql->commit();

  return getTypes();
}

std::unordered_map<std::string, uint32_t> Database::getSourceTypes()
{
  soci::rowset<soci::row> rowSet = (sql->prepare << "SELECT ID,SourceType FROM SourceType");

  std::unordered_map<std::string, uint32_t> ret;
  for (const soci::row& r : rowSet)
    ret[r.get<std::string>("SourceType")] = r.get<uint32_t>("ID");

  return ret;
}

std::unordered_map<std::string, uint32_t> Database::insertSourceTypes(std::vector<std::string> sourceTypes)
{
  sql->begin();
  soci::statement statement = (sql->prepare << "INSERT OR IGNORE INTO SourceType (SourceType) VALUES (:sourceType)", soci::use(sourceTypes, "sourceType"));

  statement.execute(true);
  sql->commit();

  return getSourceTypes();
}