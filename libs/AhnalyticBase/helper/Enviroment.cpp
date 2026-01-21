#include "AhnalyticBase/helper/Enviroment.hpp"
#include "AhnalyticBase/helper/IniReader.hpp"

#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

std::filesystem::path getExePath()
{
  char buffer[1024];

#ifdef _WIN32
  // Windows: Use GetModuleFileNameA
  GetModuleFileNameA(NULL, buffer, sizeof(buffer)); // NULL for the current process
#elif defined(__linux__) || defined(__APPLE__)
  // Linux/macOS: Use readlink on /proc/self/exe (Linux) or _NSGetExecutablePath (macOS)
  ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
  if (len != -1)
  {
    buffer[len] = '\0';
  }
#endif

  return std::filesystem::path(buffer);
}

EnviromentC::EnviromentC()
{
  std::filesystem::path searchPath = getExePath().parent_path();
  std::filesystem::path configPath = searchPath / "ahnalytic.cfg";

  binFolder = searchPath;

  while (!std::filesystem::exists(configPath) && searchPath != searchPath.root_path())
  {
    searchPath = searchPath.parent_path();
    configPath = searchPath / "ahnalytic.cfg";
  }

  if (std::filesystem::exists(configPath))
  {
    IniReader reader(configPath.string());

    auto readFolder = [&reader, searchPath](const std::string& name)
    {
      std::filesystem::path folder = reader.getValue(name, "pathes", "");
      if (folder.is_relative())
        folder = searchPath / folder;
      return folder.lexically_normal().native();
    };

    dbFolder = readFolder("dbPath");
    workFolder = readFolder("workPath");
    dataFolder = readFolder("dataPath");
    scanFolder = readFolder("scanPath");
    webFolder = readFolder("webPath");

    windowSize = std::stoi(reader.getValue("windowSize", "search", "64"));

    scanServerAddr = reader.getValue("addr", "ScanServer", "127.0.0.1");
    scanServerPort = std::stoi(reader.getValue("port", "ScanServer", "9080"));

    updateServerAddr = reader.getValue("addr", "UpdateServer", "127.0.0.1");
    updateServerPort = std::stoi(reader.getValue("port", "UpdateServer", "9081"));
  }
}

EnviromentC::~EnviromentC()
{
}