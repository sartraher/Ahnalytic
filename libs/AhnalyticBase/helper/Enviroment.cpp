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

    dbFolder = searchPath / reader.getValue("dbPath", "pathes", "");
    workFolder = searchPath / reader.getValue("workPath", "pathes", "");
    dataFolder = searchPath / reader.getValue("dataPath", "pathes", "");
    scanFolder = searchPath / reader.getValue("scanPath", "pathes", "");
    webFolder = searchPath / reader.getValue("webPath", "pathes", "");

    dbFolder = dbFolder.lexically_normal().native();
    workFolder = workFolder.lexically_normal().native();
    dataFolder = dataFolder.lexically_normal().native();
    scanFolder = scanFolder.lexically_normal().native();
    webFolder = webFolder.lexically_normal().native();
  }
}

EnviromentC::~EnviromentC()
{
}