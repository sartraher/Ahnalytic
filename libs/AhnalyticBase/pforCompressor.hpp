#ifndef pforCompressor_hpp__
#define pforCompressor_hpp__

#include "compressor.hpp"

class PForCompressor : public CompressorI
{
public:
  PForCompressor(std::string id);

  virtual CompressData compress(const CompressData& data);
  virtual CompressData decompress(const CompressData& data);
  virtual std::string getId();

private:
  std::string codecId;
};

#endif