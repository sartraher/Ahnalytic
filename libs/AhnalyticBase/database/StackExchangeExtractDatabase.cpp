#include "StackExchangeExtractDatabase.hpp"

#include "soci/soci.h"
#include "soci/sqlite3/soci-sqlite3.h"
// #include "soci/mysql/soci-mysql.h"
#include "soci/postgresql/soci-postgresql.h"

StackExchangeExtractDatabase::StackExchangeExtractDatabase(DBType type, std::string connectionString) : Database(type, connectionString)
{
  if (sql != nullptr && sql->is_connected())
  {
    initTables();
  }
}

void StackExchangeExtractDatabase::initTables()
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);

  (*sql) << "CREATE TABLE IF NOT EXISTS \"StackExSnipped\" ("
            "\"ID\" INTEGER,"
            "\"StackExId\" INTEGER,"
            "\"Date\" TEXT,"
            "\"Licence\" TEXT,"
            "\"Code\" TEXT,"
            "PRIMARY KEY(\"ID\")"
            ")";
}

uint32_t StackExchangeExtractDatabase::addSnipped(int stackExId, const std::string& date, const std::string& licence, const std::string& code)
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);
    
  soci::rowset<int> rs = (sql->prepare << "INSERT INTO StackExSnipped (StackExId,Date,Licence,Code) VALUES (:stackExId,:date,:licence,:code) RETURNING ID",
                          soci::use(stackExId, "stackExId"), soci::use(date, "date"), soci::use(licence, "licence"), soci::use(code, "code"));
  return *rs.begin();
}

void StackExchangeExtractDatabase::processSnippeds(std::function<void(const SnippedData& data)> callback)
{
  soci::rowset<soci::row> rowSet = (sql->prepare << "SELECT StackExId,Date,Licence,Code FROM StackExSnipped");

  std::unordered_map<std::string, uint32_t> ret;
  for (const soci::row& r : rowSet)
    callback({r.get<int>("StackExId"), r.get<std::string>("Code"), r.get<std::string>("Licence"), r.get<std::string>("Date")});
}

void StackExchangeExtractDatabase::getSnipped(const std::string& stackExId, std::string& date, std::string& licence, std::string& code)
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);

  soci::rowset<soci::row> rowSet =
      (sql->prepare << "SELECT Date,Licence,Code FROM StackExSnipped WHERE StackExId=:stackExId", soci::use(stackExId, "stackExId"));

  for (const soci::row& r : rowSet)
  {
    date = r.get<std::string>("Date");
    licence = r.get<std::string>("Licence");
    code = r.get<std::string>("Code");
    break;
  }
}
