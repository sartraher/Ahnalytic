#ifndef Logger_hpp__
#define Logger_hpp__

#include <string>

class LoggerPrivateC;
class EnviromentC;

class LoggerC
{
public:
  LoggerC(const EnviromentC& env);
  ~LoggerC();

  static void LogDebug(const std::string& msg);
  static void LogInfo(const std::string& msg);
  static void LogWarn(const std::string& msg);
  static void LogError(const std::string& msg);

private:
  LoggerPrivateC* priv = nullptr;

protected:
};

#endif