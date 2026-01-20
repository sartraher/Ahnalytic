#ifndef enviroment_hpp__
#define enviroment_hpp__

#include <filesystem>
#include <string>

class EnviromentC
{
public:
  EnviromentC();
  ~EnviromentC();

  std::filesystem::path binFolder;
  std::filesystem::path dbFolder;
  std::filesystem::path workFolder;
  std::filesystem::path dataFolder;
  std::filesystem::path scanFolder;
  std::filesystem::path webFolder;

  int windowSize = 64;

  std::string scanServerAddr = "127.0.0.1";
  int scanServerPort = 9080;

  std::string updateServerAddr = "127.0.0.1";
  int updateServerPort = 9081;

private:
protected:
};

#endif