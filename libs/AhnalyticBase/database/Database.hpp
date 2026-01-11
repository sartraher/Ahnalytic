#ifndef database_hpp__
#define database_hpp__

#include "AhnalyticBase/compression/CompressionManager.hpp"

#include <filesystem>
#include <mutex>
#include <string>

namespace soci
{
class session;
};

enum class DBType
{
  SQLite,
  MySql,
  Postgres
};

struct VectorHash
{
  std::size_t operator()(const std::vector<uint32_t>& v) const
  {
    std::size_t seed = v.size();
    for (uint32_t i : v)
    {
      seed ^= std::hash<uint32_t>()(i) + 0x9e3779b9 + (seed << 6) + (seed >> 2); // from boost::hash_combine
    }
    return seed;
  }
};

struct VectorEqual
{
  bool operator()(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b) const
  {
    return a == b;
  }
};

class DLLEXPORT Database
{
public:
  ~Database();

  uint32_t createSourceTreeData(const std::vector<char>& data);
  void getSourceTreeData(uint32_t id, std::vector<char>& data);

  std::unordered_map<std::string, uint32_t> getNames();
  std::unordered_map<std::string, uint32_t> insertNames(std::vector<std::string> types);

  std::unordered_map<std::string, uint32_t> getTypes();
  std::unordered_map<std::string, uint32_t> insertTypes(std::vector<std::string> types);

  std::unordered_map<std::string, uint32_t> getSourceTypes();
  std::unordered_map<std::string, uint32_t> insertSourceTypes(std::vector<std::string> types);

private:
protected:
  Database(DBType type, std::string connectionString);

  CompressionManager compressionManager;
  soci::session* sql = nullptr;
  std::recursive_mutex mutex;
};

#endif