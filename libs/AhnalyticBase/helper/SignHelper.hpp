#ifndef SignHelper_hpp__
#define SignHelper_hpp__

#include "AhnalyticBase/Export.hpp"

#include <string>
#include <vector>

class DLLEXPORT SignHelper
{
public:
  static void signFile(const std::string& tarPath, const std::string& privateKeyPath, const std::string& sigPath);
  static bool verifyFile(const std::string& tarPath, const std::string& publicKeyPath, const std::string& sigPath);

private:
protected:
  static std::vector<unsigned char> readFile(const std::string& path);
};

#endif