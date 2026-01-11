#ifndef bscCompressor_hpp__
#define bscCompressor_hpp__

#include "AhnalyticBase/compression/Compressor.hpp"

class BscCompressor : public CompressorI
{
public:
  BscCompressor();

  virtual CompressData compress(const CompressData& data);
  virtual CompressData decompress(const CompressData& data);
  virtual std::string getId();
};

#endif