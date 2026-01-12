#include "AhnalyticBase/database/GitHubRepoDatabase.hpp"

#include "soci/soci.h"
#include "soci/sqlite3/soci-sqlite3.h"
// #include "soci/mysql/soci-mysql.h"
#include "soci/postgresql/soci-postgresql.h"

#include <nlohmann/json.hpp>
#include <sstream>
#include <algorithm>

using json = nlohmann::json;

// -----------------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------------

GitHubRepoDatabase::GitHubRepoDatabase(DBType type, std::string connectionString) : Database(type, connectionString)
{
  if (sql != nullptr && sql->is_connected())
  {
    initTables();
  }
}

// -----------------------------------------------------------------------------
// Initialize schema
// -----------------------------------------------------------------------------

void GitHubRepoDatabase::initTables()
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);

  // main repos table
  (*sql) << "CREATE TABLE IF NOT EXISTS GitHubRepos ("
            "  ID INTEGER PRIMARY KEY,"
            "  Name TEXT,"
            "  FullName TEXT UNIQUE,"
            "  HtmlUrl TEXT,"
            "  License TEXT,"
            "  Language TEXT,"
            "  LastPushed TEXT,"
            "  HeadBranch TEXT,"
            "  HeadSha TEXT"
            ")";

  // tags
  (*sql) << "CREATE TABLE IF NOT EXISTS GitHubRepoTags ("
            "  ID INTEGER PRIMARY KEY,"
            "  RepoId INTEGER,"
            "  Name TEXT,"
            "  CommitSha TEXT,"
            "  FOREIGN KEY(RepoId) REFERENCES GitHubRepos(ID) ON DELETE CASCADE"
            ")";

  // branches
  (*sql) << "CREATE TABLE IF NOT EXISTS GitHubRepoBranches ("
            "  ID INTEGER PRIMARY KEY,"
            "  RepoId INTEGER,"
            "  Name TEXT,"
            "  CommitSha TEXT,"
            "  FOREIGN KEY(RepoId) REFERENCES GitHubRepos(ID) ON DELETE CASCADE"
            ")";

  // crawler state (single row)
  (*sql) << "CREATE TABLE IF NOT EXISTS crawler_state ("
            "  id INTEGER PRIMARY KEY CHECK (id = 1),"
            "  last_since_id INTEGER DEFAULT 0,"
            "  last_update_ts TEXT DEFAULT '1970-01-01T00:00:00Z'"
            ")";

  // ensure one row exists
  (*sql) << "INSERT OR IGNORE INTO crawler_state (id, last_since_id, last_update_ts) VALUES (1, 0, '1970-01-01T00:00:00Z')";
}

// -----------------------------------------------------------------------------
// Insert Repo (returns ID)
// -----------------------------------------------------------------------------

uint32_t GitHubRepoDatabase::addRepo(const RepoInfo& repo)
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);

  soci::rowset<int> rs = (sql->prepare << "INSERT INTO GitHubRepos (Name, FullName, HtmlUrl, License, Language, LastPushed, HeadBranch, HeadSha) "
                                          "VALUES (:name, :fullName, :htmlUrl, :license, :language, :lastPushed, :headBranch, :headSha) "
                                          "RETURNING ID",
                          soci::use(repo.name, "name"), soci::use(repo.fullName, "fullName"), soci::use(repo.htmlUrl, "htmlUrl"),
                          soci::use(repo.license, "license"), soci::use(repo.language, "language"), soci::use(repo.lastPushed, "lastPushed"),
                          soci::use(repo.headBranch, "headBranch"), soci::use(repo.headSha, "headSha"));

  uint32_t repoId = *rs.begin();

  insertTags(repoId, repo.tags);
  insertBranches(repoId, repo.branches);

  return repoId;
}

// -----------------------------------------------------------------------------
// Upsert Repo: insert or update and replace child rows
// -----------------------------------------------------------------------------

uint32_t GitHubRepoDatabase::upsertRepo(const RepoInfo& repo)
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);

  // Check if exists
  std::optional<uint32_t> existing = getRepoId(repo.fullName);
  if (!existing.has_value())
  {
    return addRepo(repo);
  }

  uint32_t repoId = existing.value();

  // Update main row
  (*sql) << "UPDATE GitHubRepos SET "
            "Name = :name, HtmlUrl = :htmlUrl, License = :license, Language = :language, "
            "LastPushed = :lastPushed, HeadBranch = :headBranch, HeadSha = :headSha "
            "WHERE ID = :id",
      soci::use(repo.name, "name"), soci::use(repo.htmlUrl, "htmlUrl"), soci::use(repo.license, "license"), soci::use(repo.language, "language"),
      soci::use(repo.lastPushed, "lastPushed"), soci::use(repo.headBranch, "headBranch"), soci::use(repo.headSha, "headSha"), soci::use(repoId, "id");

  // Replace tags & branches: delete existing then insert new
  (*sql) << "DELETE FROM GitHubRepoTags WHERE RepoId = :repoId", soci::use(repoId, "repoId");
  (*sql) << "DELETE FROM GitHubRepoBranches WHERE RepoId = :repoId", soci::use(repoId, "repoId");

  insertTags(repoId, repo.tags);
  insertBranches(repoId, repo.branches);

  return repoId;
}

// -----------------------------------------------------------------------------
// Insert tags
// -----------------------------------------------------------------------------

void GitHubRepoDatabase::insertTags(uint32_t repoId, const std::vector<TagInfo>& tags)
{
  for (const auto& t : tags)
  {
    (*sql) << "INSERT INTO GitHubRepoTags (RepoId, Name, CommitSha) VALUES (:repoId, :name, :commitSha)", soci::use(repoId, "repoId"),
        soci::use(t.name, "name"), soci::use(t.commitSha, "commitSha");
  }
}

// -----------------------------------------------------------------------------
// Insert branches
// -----------------------------------------------------------------------------

void GitHubRepoDatabase::insertBranches(uint32_t repoId, const std::vector<BranchInfo>& branches)
{
  for (const auto& b : branches)
  {
    (*sql) << "INSERT INTO GitHubRepoBranches (RepoId, Name, CommitSha) VALUES (:repoId, :name, :commitSha)", soci::use(repoId, "repoId"),
        soci::use(b.name, "name"), soci::use(b.commitSha, "commitSha");
  }
}

// -----------------------------------------------------------------------------
// Load Tags
// -----------------------------------------------------------------------------

std::vector<TagInfo> GitHubRepoDatabase::loadTags(uint32_t repoId)
{
  std::vector<TagInfo> tags;

  soci::rowset<soci::row> rs = (sql->prepare << "SELECT Name, CommitSha FROM GitHubRepoTags WHERE RepoId = :repoId", soci::use(repoId, "repoId"));

  for (auto& r : rs)
  {
    TagInfo t;
    t.name = r.get<std::string>("Name");
    t.commitSha = r.get<std::string>("CommitSha");
    tags.push_back(t);
  }

  return tags;
}

// -----------------------------------------------------------------------------
// Load Branches
// -----------------------------------------------------------------------------

std::vector<BranchInfo> GitHubRepoDatabase::loadBranches(uint32_t repoId)
{
  std::vector<BranchInfo> branches;

  soci::rowset<soci::row> rs = (sql->prepare << "SELECT Name, CommitSha FROM GitHubRepoBranches WHERE RepoId = :repoId", soci::use(repoId, "repoId"));

  for (auto& r : rs)
  {
    BranchInfo b;
    b.name = r.get<std::string>("Name");
    b.commitSha = r.get<std::string>("CommitSha");
    branches.push_back(b);
  }

  return branches;
}

// -----------------------------------------------------------------------------
// Query helpers
// -----------------------------------------------------------------------------

bool GitHubRepoDatabase::hasRepo(const std::string& fullName)
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);

  int count = 0;
  (*sql) << "SELECT COUNT(1) FROM GitHubRepos WHERE FullName = :fullName", soci::use(fullName, "fullName"), soci::into(count);
  return count > 0;
}

std::optional<uint32_t> GitHubRepoDatabase::getRepoId(const std::string& fullName)
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);

  int id = 0;
  (*sql) << "SELECT ID FROM GitHubRepos WHERE FullName = :fullName", soci::use(fullName, "fullName"), soci::into(id);

  if (id == 0)
    return std::nullopt;

  return static_cast<uint32_t>(id);
}

std::optional<std::string> GitHubRepoDatabase::getRepoPushedAt(const std::string& fullName)
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);

  soci::indicator ind;
  std::string pushed;
  (*sql) << "SELECT LastPushed FROM GitHubRepos WHERE FullName = :fullName", soci::use(fullName, "fullName"), soci::into(pushed, ind);

  if (ind == soci::i_null)
    return std::nullopt;

  return pushed;
}

// -----------------------------------------------------------------------------
// Crawler state helpers
// -----------------------------------------------------------------------------

unsigned long long GitHubRepoDatabase::getLastSinceId()
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);

  long long since = 0;
  (*sql) << "SELECT last_since_id FROM crawler_state WHERE id = 1", soci::into(since);
  return static_cast<unsigned long long>(since);
}

void GitHubRepoDatabase::setLastSinceId(unsigned long long sinceId)
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);

  int64_t id64 = static_cast<int64_t>(sinceId);
  (*sql) << "UPDATE crawler_state SET last_since_id = :since WHERE id = 1", soci::use(id64, "since");
}

std::string GitHubRepoDatabase::getLastUpdateTimestamp()
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);

  std::string ts;
  (*sql) << "SELECT last_update_ts FROM crawler_state WHERE id = 1", soci::into(ts);
  return ts;
}

void GitHubRepoDatabase::setLastUpdateTimestamp(const std::string& isoTs)
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);

  (*sql) << "UPDATE crawler_state SET last_update_ts = :ts WHERE id = 1", soci::use(isoTs, "ts");
}

// -----------------------------------------------------------------------------
// Enumerate All Repos
// -----------------------------------------------------------------------------

void GitHubRepoDatabase::processRepos(std::vector<std::string> langFilter, bool needTag, bool needBranches, std::function<void(RepoInfo info)> callback)
{
  soci::rowset<soci::row> rows = (sql->prepare << "SELECT ID, Name, FullName, HtmlUrl, License, Language, LastPushed, HeadBranch, HeadSha FROM GitHubRepos");

  for (const soci::row& r : rows)
  {
    RepoInfo repo;

    repo.language = r.get<std::string>("Language");

    if (langFilter.size() > 0 && std::find(langFilter.begin(), langFilter.end(), repo.language) == langFilter.end())
      continue;

    int id = r.get<int>("ID");
    repo.name = r.get<std::string>("Name");
    repo.fullName = r.get<std::string>("FullName");
    repo.htmlUrl = r.get<std::string>("HtmlUrl");
    repo.license = r.get<std::string>("License");
    repo.lastPushed = r.get<std::string>("LastPushed");
    repo.headBranch = r.get<std::string>("HeadBranch");
    repo.headSha = r.get<std::string>("HeadSha");

    if (needTag)
      repo.tags = loadTags(id);

    if (needBranches)
      repo.branches = loadBranches(id);

    callback(repo);
  }
}