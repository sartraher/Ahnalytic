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

void ArchiveHelper::extractTar(const std::filesystem::path& tar_path, const std::filesystem::path& output_folder)
{
  struct archive* a = archive_read_new();
  archive_read_support_format_tar(a);
  archive_read_support_filter_all(a); // supports compressed tars too

  if (archive_read_open_filename(a, tar_path.string().c_str(), 10240) != ARCHIVE_OK)
  {
    std::cerr << "Failed to open archive: " << archive_error_string(a) << std::endl;
    archive_read_free(a);
    return;
  }

  struct archive* ext = archive_write_disk_new();
  archive_write_disk_set_options(ext, ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS);

  archive_write_disk_set_standard_lookup(ext);

  struct archive_entry* entry;

  while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
  {
    // Prepend output folder to extracted path
    std::filesystem::path full_path = output_folder / archive_entry_pathname(entry);

    archive_entry_set_pathname(entry, full_path.string().c_str());

    int r = archive_write_header(ext, entry);
    if (r != ARCHIVE_OK)
    {
      std::cerr << archive_error_string(ext) << std::endl;
    }
    else
    {
      const void* buff;
      size_t size;
      la_int64_t offset;

      while (true)
      {
        r = archive_read_data_block(a, &buff, &size, &offset);
        if (r == ARCHIVE_EOF)
          break;
        if (r != ARCHIVE_OK)
        {
          std::cerr << archive_error_string(a) << std::endl;
          break;
        }

        archive_write_data_block(ext, buff, size, offset);
      }
    }

    archive_write_finish_entry(ext);
  }

  archive_write_close(ext);
  archive_write_free(ext);

  archive_read_close(a);
  archive_read_free(a);
}

void ArchiveHelper::createGz(const std::filesystem::path& input_file, const std::filesystem::path& gz_name)
{
  struct archive* a = archive_write_new();
  archive_write_set_format_gnutar(a); // format doesn’t matter much here
  archive_write_add_filter_gzip(a);   // apply gzip compression
  archive_write_open_filename(a, gz_name.string().c_str());

  struct archive_entry* entry = archive_entry_new();
  archive_entry_set_pathname(entry, input_file.filename().string().c_str());
  archive_entry_set_size(entry, std::filesystem::file_size(input_file));
  archive_entry_set_filetype(entry, AE_IFREG);
  archive_entry_set_perm(entry, 0644);
  archive_write_header(a, entry);

  std::ifstream ifs(input_file, std::ios::binary);
  char buffer[8192];
  while (ifs.read(buffer, sizeof(buffer)))
    archive_write_data(a, buffer, ifs.gcount());
  if (ifs.gcount() > 0)
    archive_write_data(a, buffer, ifs.gcount());

  archive_entry_free(entry);
  archive_write_close(a);
  archive_write_free(a);
}

void ArchiveHelper::createTarGz(const std::filesystem::path& path, const std::filesystem::path& tar_gz_name)
{
  if (!std::filesystem::exists(path))
  {
    throw std::runtime_error("Path does not exist: " + path.string());
  }

  struct archive* a = archive_write_new();
  archive_write_add_filter_gzip(a);           // gzip compression
  archive_write_set_format_pax_restricted(a); // portable tar
  archive_write_open_filename(a, tar_gz_name.string().c_str());

  if (std::filesystem::is_regular_file(path))
  {
    // single file: use the parent as base path to keep filename in archive
    addFile(a, path, path.parent_path());
  }
  else if (std::filesystem::is_directory(path))
  {
    // directory: recursively add all files
    for (const auto& entry : std::filesystem::recursive_directory_iterator(path))
    {
      addFile(a, entry.path(), path);
    }
  }
  else
  {
    archive_write_free(a);
    throw std::runtime_error("Unsupported path type: " + path.string());
  }

  archive_write_close(a);
  archive_write_free(a);
}