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

    if (std::filesystem::exists(outPath))
      return;

    if (!dbs.contains(id))
      dbs[id] = new StackExchangeExtractDatabase(DBType::SQLite, outPath.string());

    dbs[id]->addSnipped(data.id, data.date, data.licence, data.code);
  });

  for (auto iter = dbs.begin(); iter != dbs.end(); iter++)
    delete iter->second;
}

void StackExchangeExtractDatabase::mergeDatabase(const StackExchangeExtractDatabase& db)
{
  // SQLite performance tuning for bulk insert
  *sql << "PRAGMA synchronous = OFF;";
  *sql << "PRAGMA journal_mode = WAL;";
  *sql << "PRAGMA temp_store = MEMORY;";
  *sql << "PRAGMA cache_size = -100000;";

  soci::transaction tr(*sql);

  int stackId;
  std::string date;
  std::string licence;
  std::string code;

  soci::statement insertStmt = (sql->prepare << "INSERT INTO StackExSnipped (StackExId, Date, Licence, Code) "
                                                "VALUES (:stackExId, :date, :licence, :code)",
                                soci::use(stackId, "stackExId"), soci::use(date, "date"), soci::use(licence, "licence"), soci::use(code, "code"));

  soci::rowset<soci::row> rowSet = (db.sql->prepare << "SELECT StackExId, Date, Licence, Code FROM StackExSnipped");

  std::size_t inserted = 0;
  const std::size_t logStep = 100000;

  for (const soci::row& r : rowSet)
  {
    stackId = r.get<int>(0);
    date = r.get<std::string>(1);
    licence = r.get<std::string>(2);
    code = r.get<std::string>(3);

    insertStmt.execute(true);

    ++inserted;

    if (inserted % logStep == 0)
    {
      // optional logging
    }
  }

  tr.commit();

  // force WAL -> main DB flush
  *sql << "PRAGMA wal_checkpoint(FULL);";

  // restore safer settings
  *sql << "PRAGMA synchronous = FULL;";
  *sql << "PRAGMA journal_mode = DELETE;";
}