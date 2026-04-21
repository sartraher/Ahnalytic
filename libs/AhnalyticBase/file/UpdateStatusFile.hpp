#ifndef UPDATESTATUSFILE_HPP__
#define UPDATESTATUSFILE_HPP__

#include "AhnalyticBase/Export.hpp"

#include <filesystem>

#include <nlohmann/json.hpp>

using json = nlohmann::json;
struct DLLEXPORT UpdateInfo
{
  std::string name;
  std::string sha;
  std::string type;
  std::string language;
  std::string version;

  bool operator==(const UpdateInfo& other) const
  {
    return sha == other.sha && name == other.name && type == other.type && language == other.language && version == other.version;
  }
};

class DLLEXPORT UpdateStatusFile
{
public:
  UpdateStatusFile();
  ~UpdateStatusFile();

  void read(const std::filesystem::path& path);
  void write(const std::filesystem::path& path);

  void readBuffer(const std::string& buffer);

  static std::string getString(const nlohmann::json& data, const std::string& name);

  ahn::vector<UpdateInfo> infos;

private:
protected:
};

#endif