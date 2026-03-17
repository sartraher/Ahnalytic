#ifndef UPDATESTATUSFILE_HPP__
#define UPDATESTATUSFILE_HPP__

#include "AhnalyticBase/Export.hpp"

#include <filesystem>

#include <nlohmann/json.hpp>

using json = nlohmann::json;
struct UpdateInfo
{
  std::string name;
  std::string baseName;
  std::string sha;
  std::string maxVersion;
  std::string type;
  std::string language;

  bool operator==(const UpdateInfo& other) const
  {
    return name == other.name && sha == other.sha && type == other.type && language == other.language && baseName == other.baseName &&
           maxVersion == other.maxVersion;
  }
};

class UpdateStatusFile
{
public:
  UpdateStatusFile();
  ~UpdateStatusFile();

  void read(const std::filesystem::path& path);
  void write(const std::filesystem::path& path);

  void readBuffer(const std::string& buffer);
  
  static std::string getString(const nlohmann::json& data, const std::string& name);

  std::vector<UpdateInfo> infos;

private:
protected:
};

#endif