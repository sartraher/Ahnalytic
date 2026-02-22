#include "FileDatabase.hpp"

#include "soci/soci.h"
#include "soci/sqlite3/soci-sqlite3.h"
// #include "soci/mysql/soci-mysql.h"
// #include "soci/postgresql/soci-postgresql.h"

#include "AhnalyticBase/helper/ArchiveHelper.hpp"
#include "AhnalyticBase/helper/Diagnostic.hpp"
#include "AhnalyticBase/helper/Enviroment.hpp"
#include "AhnalyticBase/helper/SignHelper.hpp"
#include "AhnalyticBase/tree/SourceStructureTree.hpp"

#include <archive.h>
#include <archive_entry.h>

#include <algorithm>
#include <fstream>
#include <numeric>
#include <set>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

bool removeAll(const std::filesystem::path& path)
{
  std::error_code ec;

  if (!std::filesystem::exists(path))
    return true;

  for (auto& entry : std::filesystem::recursive_directory_iterator(path))
  {
    DWORD attrs = GetFileAttributesW(entry.path().wstring().c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_READONLY))
      SetFileAttributesW(entry.path().wstring().c_str(), attrs & ~FILE_ATTRIBUTE_READONLY);

    std::filesystem::remove_all(entry.path(), ec);
    if (ec)
      std::cerr << "Failed to remove " << entry.path() << ": " << ec.message() << "\n";
  }

  // retry top-level folder if it still exists
  int attempts = 0;
  while (std::filesystem::exists(path) && attempts++ < 10)
  {
    std::filesystem::remove(path, ec);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  return !std::filesystem::exists(path);
}

FileDatabase::FileDatabase(DBType type, std::string connectionString) : Database(type, connectionString)
{
  if (sql != nullptr && sql->is_connected())
  {
    initTables();
  }
}

void FileDatabase::initTables()
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);

  (*sql) << "CREATE TABLE IF NOT EXISTS \"Repo\" ("
            "\"ID\" INTEGER,"
            "\"Name\" TEXT,"
            "\"Url\" TEXT,"
            "\"Licence\" TEXT,"
            "PRIMARY KEY(\"ID\")"
            ")";

  (*sql) << "CREATE TABLE IF NOT EXISTS \"Name\" ("
            "\"ID\" INTEGER,"
            "\"Name\" TEXT UNIQUE,"
            "PRIMARY KEY(\"ID\")"
            ")";

  (*sql) << "CREATE TABLE IF NOT EXISTS \"File\" ("
            "\"ID\" INTEGER,"
            "\"DataID\" INTEGER,"
            "\"FileIndex\" INTEGER,"
            "\"PathID\" INTEGER,"
            "\"TagID\" INTEGER,"
            "PRIMARY KEY(\"ID\")"
            ")";

  (*sql) << "CREATE TABLE IF NOT EXISTS \"Tag\" ("
            "\"ID\" INTEGER,"
            "\"TagName\" TEXT,"
            "\"Sha\" TEXT,"
            "PRIMARY KEY(\"ID\")"
            ")";

  (*sql) << "CREATE TABLE IF NOT EXISTS \"SourceTreeData\" ("
            "\"ID\" INTEGER,"
            "\"Data\" BLOB,"
            "PRIMARY KEY(\"ID\")"
            ")";
}

uint32_t FileDatabase::createFile(uint32_t dataId, uint32_t index, uint32_t pathId, uint32_t tagId)
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);

  soci::rowset<int> rs = (sql->prepare << "INSERT INTO File (DataID,FileIndex,PathID,TagID) VALUES (:dataId,:fileIndex,:pathId,:tagId) RETURNING ID",
                          soci::use(dataId, "dataId"), soci::use(index, "fileIndex"), soci::use(pathId, "pathId"), soci::use(tagId, "tagId"));

  return *rs.begin();
}

void FileDatabase::createFiles(uint32_t dataId, std::vector<uint32_t> indices, std::vector<uint32_t> pathIds, uint32_t tagId)
{
  std::vector<uint32_t> dataIds(pathIds.size());
  std::vector<uint32_t> tagIds(pathIds.size());

  std::fill(dataIds.begin(), dataIds.end(), dataId);
  std::fill(tagIds.begin(), tagIds.end(), tagId);

  const std::lock_guard<std::recursive_mutex> lock(mutex);

  sql->begin();
  soci::statement statement = (sql->prepare << "INSERT INTO File (DataID,FileIndex,PathID,TagID) VALUES (:dataId,:fileIndex,:pathId,:tagId)",
                               soci::use(dataIds, "dataId"), soci::use(indices, "fileIndex"), soci::use(pathIds, "pathId"), soci::use(tagIds, "tagId"));

  statement.execute(true);
  sql->commit();

  /*
  soci::rowset<int> rs = (sql->prepare << "INSERT INTO File (DataID,FileIndex,PathID,TagID) VALUES (:dataId,:fileIndex,:pathId,:tagId) RETURNING ID",
                          soci::use(dataId, "dataId"), soci::use(index, "fileIndex"), soci::use(pathId, "pathId"), soci::use(tagId, "tagId"));

  return *rs.begin();
  */
}

uint32_t FileDatabase::createRepoData(const std::string& name, const std::string& url, const std::string& license)
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);

  soci::rowset<int> rs = (sql->prepare << "INSERT INTO Repo (Name,Url,Licence) VALUES (:name,:url,:license) RETURNING ID", soci::use(name, "name"),
                          soci::use(url, "url"), soci::use(license, "license"));

  return *rs.begin();
}

uint32_t FileDatabase::createTag(const std::string& tagName, const std::string& sha)
{
  const std::lock_guard<std::recursive_mutex> lock(mutex);

  soci::rowset<int> rs =
      (sql->prepare << "INSERT INTO Tag (TagName,Sha) VALUES (:tagName,:sha) RETURNING ID", soci::use(tagName, "tagName"), soci::use(sha, "sha"));

  return *rs.begin();
}

std::unordered_map<std::string, std::string> FileDatabase::getTags() const
{
  std::unordered_map<std::string, std::string> ret;

  soci::rowset<soci::row> rows = (sql->prepare << "SELECT TagName, Sha FROM Tag");

  for (const soci::row& r : rows)
    ret[r.get<std::string>("TagName")] = r.get<std::string>("Sha");

  return ret;
}

void FileDatabase::iterateFiles(std::function<void(uint32_t, const std::string&, const std::string&, SourceStructureTree*)> callback)
{
  soci::rowset<soci::row> rowSet = (sql->prepare << "SELECT DataID,FileIndex,PathID,TagID FROM File");

#undef max
  uint32_t lastSourceTreeId = std::numeric_limits<uint32_t>::max();
  SourceStructureTree* root = nullptr;

  for (const soci::row& r : rowSet)
  {
    uint32_t sourceTreeDataID = r.get<uint32_t>("DataID");
    uint32_t fileIndex = r.get<uint32_t>("FileIndex");
    uint32_t pathId = r.get<uint32_t>("PathID");
    uint32_t tagId = r.get<uint32_t>("TagID");

    std::string licence;
    std::string sha;

    if (lastSourceTreeId != sourceTreeDataID)
    {
      std::vector<char> sourceTreeData;
      lastSourceTreeId = sourceTreeDataID;
      getSourceTreeData(sourceTreeDataID, sourceTreeData);

      std::vector<FlatNodeDeDupData> nodeListTestOut;
      std::vector<uint32_t> indexListTestOut;
      SourceStructureTree::deserialize(sourceTreeData, nodeListTestOut, indexListTestOut, nullptr);
      root = (SourceStructureTree*)rebuildTree(nodeListTestOut, indexListTestOut);
    }

    if (root != nullptr && root->children.size() > fileIndex)
      callback(fileIndex, sha, licence, (SourceStructureTree*)root->children.at(fileIndex));
  }
}

void FileDatabase::exportData(std::filesystem::path& outPath)
{
  EnviromentC env;

  std::unordered_map<std::string, std::string> ret;

  CompressionManager compressionManager;

  std::vector<std::filesystem::path> toDelete;

  std::vector<std::pair<uint32_t, std::string>> tagIds;
  json tagsJson;

  {
    soci::rowset<soci::row> rows = (sql->prepare << "SELECT ID, TagName, Sha FROM Tag ORDER BY ID ASC");
    for (const soci::row& r : rows)
    {
      uint32_t tagId = r.get<uint32_t>("ID");
      std::string tagName = r.get<std::string>("TagName");
      std::string sha = r.get<std::string>("Sha");

      json tagJson;
      tagJson["TagName"] = tagName;
      tagJson["Sha"] = sha;
      tagsJson.push_back(tagJson);

      tagIds.push_back(std::make_pair(tagId, sha));
    }
  }

  for (std::pair<uint32_t, std::string>& tagData : tagIds)
  {
    std::filesystem::path shaPath = outPath;
    shaPath = shaPath.concat("/").concat(tagData.second);    

    soci::rowset<soci::row> rowSet = (sql->prepare << "SELECT ID, DataID, FileIndex, PathID FROM File WHERE TagID = :tagId", soci::use(tagData.first, "tagId"));

    std::vector<uint32_t> fileIds;
    std::vector<uint32_t> dataIds;
    std::vector<uint32_t> fileIndices;
    std::vector<uint32_t> pathIds;

    for (const soci::row& r : rowSet)
    {
      uint32_t fileId = r.get<uint32_t>("ID");
      uint32_t dataId = r.get<uint32_t>("DataID");
      uint32_t fileIndex = r.get<uint32_t>("FileIndex");
      uint32_t pathId = r.get<uint32_t>("PathID");

      fileIds.push_back(fileId);
      dataIds.push_back(dataId);
      fileIndices.push_back(fileIndex);
      pathIds.push_back(pathId);
    }

    if (fileIds.size() == 0)
      continue;

    std::filesystem::create_directory(shaPath);

    Diagnostic diagonstic(fileIds.size());
    Diagnostic* dia = &diagonstic;

    auto labeledDia = [dia](const std::string& label)
    {
      if (dia)
        dia->setLabel(label);
      return dia;
    };

    CompressData compressedFileIds = compressionManager.compress(fileIds, labeledDia("fileIds"), std::vector<ModAlgosE>{ModAlgosE::None, ModAlgosE::Delta},
                                                                 std::vector<CompressionAlgosE>{CompressionAlgosE::LZMA, CompressionAlgosE::BSC});
    CompressData compressedDataIds = compressionManager.compress(dataIds, labeledDia("dataIds"), std::vector<ModAlgosE>{ModAlgosE::None, ModAlgosE::Delta},
                                                                 std::vector<CompressionAlgosE>{CompressionAlgosE::LZMA, CompressionAlgosE::BSC});
    CompressData compressedFileIndices =
        compressionManager.compress(fileIndices, labeledDia("fileIndices"), std::vector<ModAlgosE>{ModAlgosE::None, ModAlgosE::Delta},
                                    std::vector<CompressionAlgosE>{CompressionAlgosE::LZMA, CompressionAlgosE::BSC});
    CompressData compressedPathIds = compressionManager.compress(pathIds, labeledDia("pathIds"), std::vector<ModAlgosE>{ModAlgosE::None, ModAlgosE::Delta},
                                                                 std::vector<CompressionAlgosE>{CompressionAlgosE::LZMA, CompressionAlgosE::BSC});

    std::filesystem::path filePath = shaPath;
    filePath = filePath.concat("/").concat("file.dat");
    std::ofstream fileOut(filePath.native(), std::ios::binary);

    auto writeCompressedData = [&fileOut](const CompressData& data)
    {
      std::vector<char> charData = data.getCharData();

      uint32_t value = static_cast<uint32_t>(charData.size());
      fileOut.write(reinterpret_cast<const char*>(&value), sizeof(value));
      fileOut.write(charData.data(), static_cast<std::streamsize>(charData.size()));
    };

    writeCompressedData(compressedFileIds);
    writeCompressedData(compressedDataIds);
    writeCompressedData(compressedFileIndices);
    writeCompressedData(compressedPathIds);

    fileOut.close();

    std::set<uint32_t> datas(dataIds.begin(), dataIds.end());

    std::filesystem::path dataPath = shaPath;
    dataPath = dataPath.concat("/").concat("data.dat");
    std::ofstream dataOut(dataPath.native(), std::ios::binary);

    auto writeData = [&dataOut](const std::vector<char>& data)
    {
      uint32_t value = static_cast<uint32_t>(data.size());
      dataOut.write(reinterpret_cast<const char*>(&value), sizeof(value));
      dataOut.write(data.data(), static_cast<std::streamsize>(data.size()));
    };

    for (const uint32_t& dataId : datas)
    {
      std::vector<char> sourceTreeData;
      getSourceTreeData(dataId, sourceTreeData);
      writeData(sourceTreeData);
    }

    dataOut.close();

    std::filesystem::path tarPath = shaPath;
    tarPath = tarPath.concat(".tar");
    ArchiveHelper::createTar(shaPath, tarPath);

    std::filesystem::path signPath = tarPath;
    signPath = signPath.concat(".sig");
    SignHelper::signFile(tarPath.string(), env.privatePath.string(), signPath.string());

    toDelete.push_back(shaPath);
  }

  // Tags
  std::filesystem::path tagsPath = outPath;
  tagsPath = tagsPath.concat("/").concat("tags.json");
  std::ofstream tagsOut(tagsPath.native());
  tagsOut << tagsJson.dump(3);
  tagsOut.close();

  // Repo
  std::string repoName;
  std::string repoUrl;
  std::string repoLicence;
  (*sql) << "SELECT Name, Url, Licence FROM Repo", soci::into(repoName), soci::into(repoUrl), soci::into(repoLicence);

  json repoJson;
  repoJson["Name"] = repoName;
  repoJson["Url"] = repoUrl;
  repoJson["Licence"] = repoLicence;
  std::filesystem::path repoPath = outPath;
  repoPath = repoPath.concat("/").concat("repo.json");
  std::ofstream repoOut(repoPath.native());
  repoOut << repoJson.dump(2);
  repoOut.close();

  // Pathes
  std::vector<std::string> vec;

  soci::rowset<soci::row> rows = (sql->prepare << "SELECT * FROM Name ORDER BY ID ASC");
  for (const soci::row& r : rows)
    vec.push_back(r.get<std::string>("Name"));

  std::string joinedData = std::accumulate(std::next(vec.begin()), vec.end(), vec[0], [](const std::string& a, const std::string& b) { return a + "|" + b; });

  Diagnostic diagonstic(joinedData.size());

  std::vector<char> data(joinedData.begin(), joinedData.end());
  CompressData compressedFileIds = compressionManager.compress(data, &diagonstic, std::vector<ModAlgosE>{ModAlgosE::None, ModAlgosE::Delta},
                                                               std::vector<CompressionAlgosE>{CompressionAlgosE::LZMA, CompressionAlgosE::BSC});

  std::filesystem::path pathesPath = outPath;
  pathesPath = pathesPath.concat("/").concat("pathes.dat");
  std::ofstream pathOut(pathesPath.native(), std::ios::binary);
  std::vector<char> charData = compressedFileIds.getCharData();
  pathOut.write(charData.data(), static_cast<std::streamsize>(charData.size()));
  pathOut.close();

  for (const std::filesystem::path& path : toDelete)
  {
    while (!removeAll(path))
      ;
  }
}
