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

  ahn::map<std::string, uint32_t> ret;
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

void StackExchangeExtractDatabase::splitDatabase(const std::string& outFolder, const std::string& prefix)
{
  ahn::map<std::string, StackExchangeExtractDatabase*> dbs;

  processSnippeds([&dbs, prefix, outFolder](const SnippedData& data)
  {
    std::tm tm = {};
    int milliseconds;

    char dot;

    std::istringstream ss(data.date);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S") >> dot >> milliseconds;

    // Convert to time_point
    auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm)) + std::chrono::milliseconds(milliseconds);

    // Format YYYY_MM manually
    int year = tm.tm_year + 1900;
    int month = tm.tm_mon + 1;

    std::ostringstream idStream;
    idStream << std::setfill('0') << std::setw(4) << year << "_" << std::setw(2) << month;
    std::string id = idStream.str();

    std::filesystem::path outPath = std::filesystem::path(outFolder) / (prefix + "_" + id + ".db");

    if (!dbs.contains(id))
      dbs[id] = new StackExchangeExtractDatabase(DBType::SQLite, outPath.string());

    dbs[id]->addSnipped(data.id, data.date, data.licence, data.code);
  });

  for (auto iter = dbs.begin(); iter != dbs.end(); iter++)
    delete iter->second;
}