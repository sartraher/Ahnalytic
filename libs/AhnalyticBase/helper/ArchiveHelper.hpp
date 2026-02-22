#ifndef ArchiveHelper_hpp__
#define ArchiveHelper_hpp__

#include "AhnalyticBase/Export.hpp"

#include <filesystem>

class DLLEXPORT ArchiveHelper
{
public:
  static void addFile(struct archive* a, const std::filesystem::path& file_path, const std::filesystem::path& base_path);
  static void createTar(const std::filesystem::path& folder, const std::filesystem::path& tar_name);

private:
protected:
};

#endif