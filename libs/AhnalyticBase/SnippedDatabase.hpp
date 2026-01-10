#ifndef SnippedDatabase_hpp__
#define SnippedDatabase_hpp__

#include "AhnalyticBase/Database.hpp"

#include <functional>

struct SourceStructureTree;

class DLLEXPORT SnippedDatabase : public Database
{
public:
  SnippedDatabase(DBType type, std::string connectionString);

  uint32_t createSnipped(uint32_t internalId, uint32_t licenceId, uint32_t sourceTreeDataID, uint32_t sourceTreeDataIndex);

  uint32_t getLicence(std::string licence);
  std::string getLicence(uint32_t licenceId);
  uint32_t insertLicence(std::string licence);

  void iterateSnippeds(std::function<void(uint32_t, const std::string&, SourceStructureTree* tree)> callback);

private:
protected:
  void initTables();
};

#endif