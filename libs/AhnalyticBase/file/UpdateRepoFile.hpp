#ifndef UpdateRepoFile_hpp__
#define UpdateRepoFile_hpp__

#include "AhnalyticBase/Export.hpp"

#include <filesystem>
#include <vector>

#include <nlohmann/json.hpp>

/*
* {
    "Licence": "NO LICENSE",
    "Name": "fizwidget/binarydump",
    "Url": "https://github.com/fizwidget/binarydump",
    "language": "CPP",
    "tags": [
      {
        "Sha": "df12e79a9e333a8ee96e4f301052563a9a55b0d2",
        "TagName": "HEAD"
      }
    ],
    "type": "github",
    "version": "1"
  }
*/

struct UpdateRepoTagData
{
  std::string tagName;
  std::string sha;
};

struct UpdateRepoData
{
  std::string licence;
  std::string name;
  std::string url;
  std::string language;
  std::string type;
  std::string version;

  ahn::vector<UpdateRepoTagData> tags;
};

class DLLEXPORT UpdateRepoFile
{
public:
  UpdateRepoFile();
  ~UpdateRepoFile();

  void read(const std::filesystem::path& path);
  void write(const std::filesystem::path& path);

  void readBuffer(const std::string& buffer);

  ahn::vector<UpdateRepoData> updateRepoData;

private:

protected:
  std::string getString(const nlohmann::json& data, const std::string& name);
};

#endif