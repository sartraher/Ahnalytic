#include "ArchiveHelper.hpp"

#include <archive.h>
#include <archive_entry.h>


#include <fstream>
#include <iostream>

void ArchiveHelper::addFile(struct archive* a, const std::filesystem::path& file_path, const std::filesystem::path& base_path)
{
  struct archive_entry* entry = archive_entry_new();

  // Path inside archive (relative path)
  auto relative_path = std::filesystem::relative(file_path, base_path);
  archive_entry_set_pathname(entry, relative_path.string().c_str());

  archive_entry_set_size(entry, std::filesystem::is_regular_file(file_path) ? std::filesystem::file_size(file_path) : 0);

  archive_entry_set_filetype(entry, std::filesystem::is_directory(file_path) ? AE_IFDIR : AE_IFREG);

  archive_entry_set_perm(entry, 0644);

  archive_write_header(a, entry);

  if (std::filesystem::is_regular_file(file_path))
  {
    std::ifstream ifs(file_path, std::ios::binary);
    char buffer[8192];

    while (ifs.read(buffer, sizeof(buffer)))
      archive_write_data(a, buffer, ifs.gcount());

    if (ifs.gcount() > 0)
      archive_write_data(a, buffer, ifs.gcount());
  }

  archive_entry_free(entry);
}

void ArchiveHelper::createTar(const std::filesystem::path& folder, const std::filesystem::path& tar_name)
{
  struct archive* a = archive_write_new();
  archive_write_set_format_pax_restricted(a); // portable tar
  archive_write_open_filename(a, tar_name.string().c_str());

  for (const auto& entry : std::filesystem::recursive_directory_iterator(folder))
  {
    addFile(a, entry.path(), folder);
  }

  archive_write_close(a);
  archive_write_free(a);
}