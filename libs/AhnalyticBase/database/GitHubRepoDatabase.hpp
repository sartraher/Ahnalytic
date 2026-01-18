#pragma once

#include "Database.hpp"

#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

struct TagInfo
{
  std::string name;
  std::string commitSha;
};

struct BranchInfo
{
  std::string name;
  std::string commitSha;
};

struct DLLEXPORT RepoInfo
{
  std::string name;
  std::string fullName;
  std::string htmlUrl;
  std::string license;
  std::string language;
  std::string lastPushed;
  std::string headBranch;
  std::string headSha;

  std::vector<TagInfo> tags;
  std::vector<BranchInfo> branches;

  json serialize() const;
  void deserialize(const json& data);
};

class DLLEXPORT GitHubRepoDatabase : public Database
{
public:
  GitHubRepoDatabase(DBType type, std::string connectionString);

  // repository insert / upsert
  uint32_t addRepo(const RepoInfo& repo);
  uint32_t upsertRepo(const RepoInfo& repo); // insert or update and replace child rows

  // query helpers
  bool hasRepo(const std::string& fullName);
  std::optional<std::string> getRepoPushedAt(const std::string& fullName);
  std::optional<uint32_t> getRepoId(const std::string& fullName);

  // crawling state
  unsigned long long getLastSinceId();
  void setLastSinceId(unsigned long long sinceId);

  std::string getLastUpdateTimestamp();
  void setLastUpdateTimestamp(const std::string& isoTs);

  // iterate all repos (existing API)
  void processRepos(std::vector<std::string> langFilter, bool needTag, bool needBranches, std::function<void(RepoInfo info)> callback);

private:
  void initTables();

  void insertTags(uint32_t repoId, const std::vector<TagInfo>& tags);
  void insertBranches(uint32_t repoId, const std::vector<BranchInfo>& branches);

  std::vector<TagInfo> loadTags(uint32_t repoId);
  std::vector<BranchInfo> loadBranches(uint32_t repoId);
};