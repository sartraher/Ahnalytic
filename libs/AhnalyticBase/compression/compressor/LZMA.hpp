#ifndef lzmaCompressor_hpp__
#define lzmaCompressor_hpp__

#include "AhnalyticBase/compression/Compressor.hpp"

class LZMACompressor : public CompressorI
{
public:
  LZMACompressor();

  virtual CompressData compress(const CompressData& data);
  virtual CompressData decompress(const CompressData& data);
  virtual std::string getId();
};

#endif