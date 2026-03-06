#ifndef ArchiveHelper_hpp__
#define ArchiveHelper_hpp__

#include "AhnalyticBase/Export.hpp"

#include <filesystem>

class DLLEXPORT ArchiveHelper
{
public:
  static void addFile(struct archive* a, const std::filesystem::path& file_path, const std::filesystem::path& base_path);
  static void createTar(const std::filesystem::path& folder, const std::filesystem::path& tar_name);
  static void createGz(const std::filesystem::path& input_file, const std::filesystem::path& gz_name);
  static void createTarGz(const std::filesystem::path& path, const std::filesystem::path& tar_gz_name);

  static void extractTar(const std::filesystem::path& tar_path, const std::filesystem::path& output_folder);

private:
protected:
};

#endif