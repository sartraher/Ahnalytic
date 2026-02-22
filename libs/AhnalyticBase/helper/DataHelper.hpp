#ifndef DataHelper_hpp__
#define DataHelper_hpp__

#include "AhnalyticBase/Export.hpp"

#include <string>

struct ExecResult
{
  int exitCode = -1;
  std::string stdoutText;
  std::string stderrText;
};

class DLLEXPORT DataHelperC
{
public:
  static std::string getFormatName(const std::string& ext);
  static std::string getLicenceName(const std::string& licence, const std::string& tempPath);

  static ExecResult execAndCapture(const std::string& cmdBase, const std::string& tempPath);

private:
  static std::string uniqueTempName(const std::string& prefix);
  static std::string nowString();
  static std::string threadIdString();


protected:
};

#endif