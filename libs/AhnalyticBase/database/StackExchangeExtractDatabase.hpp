#ifndef StackExchangeExtractDatabase_hpp__
#define StackExchangeExtractDatabase_hpp__

#include "AhnalyticBase/database/Database.hpp"
#include "AhnalyticBase/stackexchange/DataDump.hpp"

#include <functional>

class DLLEXPORT StackExchangeExtractDatabase : public Database
{
public:
  StackExchangeExtractDatabase(DBType type, std::string connectionString);

  uint32_t addSnipped(const std::string& stackExId, const std::string& date, const std::string& licence, const std::string& code);
  void processSnippeds(std::function<void(const SnippedData& data)> callback);
  void getSnipped(const std::string& stackExId, std::string& date, std::string& licence, std::string& code);

private:
protected:
  void initTables();
};

#endif