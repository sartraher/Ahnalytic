#ifndef diagnostic_hpp__
#define diagnostic_hpp__

#include "AhnalyticBase/Export.hpp"

#include <string>
#include <vector>

class DLLEXPORT Diagnostic
{
public:
  Diagnostic(size_t origSize);
  ~Diagnostic();

  void setDupReduce(size_t size);
  void setLabel(const std::string& label);
  void setCompression(size_t inSize, size_t cmpSize, const std::string& compression, const std::string& alternatives);
  void setResultSize(size_t size);
  void write();

private:
protected:
  size_t fileSize;
  std::string curLabel;
  size_t dupReduceSize;
  std::vector<std::string> compressions;
  size_t resultSize;
};

#endif