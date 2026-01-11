#ifndef compressor_hpp__
#define compressor_hpp__

#include "AhnalyticBase/compression/CompressData.hpp"
#include "AhnalyticBase/Export.hpp"

#include <string>
#include <vector>

class DLLEXPORT CompressorI
{
public:
  virtual ~CompressorI() {};
  virtual CompressData compress(const CompressData& data) = 0;
  virtual CompressData decompress(const CompressData& data) = 0;
  virtual std::string getId() = 0;

private:
protected:
};

#endif