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

private:
protected:
};

#endif