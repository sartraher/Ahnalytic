#ifndef IndexDatabase_hpp__
#define IndexDatabase_hpp__

#include "AhnalyticBase/database/Database.hpp"

#include <functional>

class DLLEXPORT IndexDatabase : public Database
{
public:
  IndexDatabase(DBType type, std::string connectionString);

  uint32_t addDatabase(std::string& dbPath);
  void addHashList(uint32_t databaseId, std::vector<uint32_t> hashes);

  std::vector<uint32_t> getHashes();
  std::vector<std::string> getDatabasesForHash(uint32_t hash);

private:
protected:
  void initTables();
};

#endif