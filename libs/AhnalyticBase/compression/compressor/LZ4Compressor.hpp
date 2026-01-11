#ifndef lz4Compressor_hpp__
#define lz4Compressor_hpp__

#include "AhnalyticBase/compression/Compressor.hpp"

class Lz4Compressor : public CompressorI
{
public:
  Lz4Compressor();

  virtual CompressData compress(const CompressData& data);
  virtual CompressData decompress(const CompressData& data);
  virtual std::string getId();
};

#endif