#ifndef FiledDatabase_hpp__
#define FiledDatabase_hpp__

#include "AhnalyticBase/database/Database.hpp"

#include <functional>

struct SourceStructureTree;

class DLLEXPORT FileDatabase : public Database
{
public:
  FileDatabase(DBType type, std::string connectionString);

  uint32_t createFile(uint32_t dataId, uint32_t index, uint32_t pathId, uint32_t tagId);
  void createFiles(uint32_t dataId, std::vector<uint32_t> indices, std::vector<uint32_t> pathIds, uint32_t tagId);
  uint32_t createRepoData(const std::string& name, const std::string& url, const std::string& license);
  uint32_t createTag(const std::string& tagName, const std::string& sha);

  std::unordered_map<std::string, std::string> getTags() const;

  void iterateFiles(std::function<void(uint32_t, const std::string&, const std::string&, SourceStructureTree*)> callback);

private:
protected:
  void initTables();
};

#endif