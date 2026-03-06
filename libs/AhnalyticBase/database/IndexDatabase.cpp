#include "IndexDatabase.hpp"

#include "soci/soci.h"
#include "soci/sqlite3/soci-sqlite3.h"
// #include "soci/mysql/soci-mysql.h"
#include "soci/postgresql/soci-postgresql.h"

IndexDatabase::IndexDatabase(DBType type, std::string connectionString) : Database(type, connectionString)
{
  if (sql != nullptr && sql->is_connected())
  {
    initTables();
  }
}

void IndexDatabase::initTables()
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);

  (*sql) << "CREATE TABLE IF NOT EXISTS \"Dbs\" ("
            "\"ID\" INTEGER,"
            "\"Path\" TEXT,"
            "PRIMARY KEY(\"ID\")"
            ")";

  (*sql) << "CREATE TABLE IF NOT EXISTS \"Hashes\" ("
            "\"ID\" INTEGER PRIMARY KEY,"
            "\"Hash\" INTEGER,"
            "\"DbId\" INTEGER,"
            "UNIQUE(\"Hash\", \"DbId\")"
            ")";
}

uint32_t IndexDatabase::addDatabase(std::string& dbPath)
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);

  soci::rowset<int> rs = (sql->prepare << "INSERT INTO Dbs (Path) VALUES (:dbPath) RETURNING ID", soci::use(dbPath, "dbPath"));

  return *rs.begin();
}

void IndexDatabase::addHashList(uint32_t databaseId, std::vector<uint32_t> hashes)
{
  std::vector<uint32_t> dbList(hashes.size());
  std::fill(hashes.begin(), hashes.end(), databaseId);

  const std::lock_guard<std::recursive_mutex> lock(mutex);

  sql->begin();
  soci::statement statement =
      (sql->prepare << "INSERT INTO Hashes (Hash,DbId) VALUES (:dbList,:hashes)", soci::use(dbList, "dbList"), soci::use(hashes, "hashes"));

  statement.execute(true);
  sql->commit();
}

std::vector<uint32_t> IndexDatabase::getHashes()
{
  std::vector<uint32_t> ret;

  return ret;
}

std::vector<std::string> IndexDatabase::getDatabasesForHash(uint32_t hash)
{
  std::vector<std::string> ret;

  return ret;
}