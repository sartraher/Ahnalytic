#ifndef scanserver_hpp__
#define scanserver_hpp__

#include "AhnalyticBase/Export.hpp"

#include <filesystem>
#include <string>

class ScanServerPrivate;

class DLLEXPORT ScanServer
{
public:
  ScanServer();
  ~ScanServer();

  void init();
  void start();
  void stop();

  void updateScans();

private:
  ScanServerPrivate* priv = nullptr;

protected:
  void checkoutGitRevision(const std::string& gitUrl, const std::string& sha, const std::filesystem::path& outputPath);
  void checkoutSvnRevision(const std::string& svnUrl, const std::string& revision, const std::filesystem::path& outputPath);
  void extractArchive(const std::filesystem::path& archivePath, const std::filesystem::path& outputPath);
};

#endif