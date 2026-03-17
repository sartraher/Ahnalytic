#ifndef FiledDatabase_hpp__
#define FiledDatabase_hpp__

#include "AhnalyticBase/database/Database.hpp"
#include "AhnalyticBase/helper/Enviroment.hpp"

#include <ankerl/unordered_dense.h>

#include <filesystem>
#include <functional>

struct SourceStructureTree;

class DLLEXPORT FileDatabase : public Database
{
public:
  FileDatabase(DBType type, std::string connectionString);

  uint32_t createFile(uint32_t dataId, uint32_t index, uint32_t pathId, uint32_t tagId);
  void createFiles(uint32_t dataId, ahn::vector<uint32_t> indices, ahn::vector<uint32_t> pathIds, uint32_t tagId);
  void createFiles(ahn::vector<uint32_t> ids, ahn::vector<uint32_t> dataIds, ahn::vector<uint32_t> indices, ahn::vector<uint32_t> pathIds,
                   ahn::vector<uint32_t> tagIds);
  uint32_t createRepoData(const std::string& name, const std::string& url, const std::string& license);
  uint32_t createTag(const std::string& tagName, const std::string& sha);

  std::unordered_map<std::string, std::string> getTags() const;
  std::string getTagSha(uint32_t id) const;

  void iterateSourceTrees(std::function<void(SourceStructureTree*)> callback);
  void iterateFiles(std::function<void(uint32_t, const std::string&, const std::string&, SourceStructureTree*)> callback);

  void exportData(std::filesystem::path& outPath);

  void importPathesData(std::filesystem::path& pathesPath);
  void importData(const std::string& tagName, const std::string& sha, std::filesystem::path& tarPath, std::filesystem::path& pathesPath, bool tagOnly,
                  const EnviromentC& env, ankerl::unordered_dense::set<uint32_t>& hashes);

  std::string getRepoUrl();
  std::string getRepoLicence();

private:
protected:
  void initTables();
};

#endif