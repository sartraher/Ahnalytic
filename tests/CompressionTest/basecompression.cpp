#include "AhnalyticBase/compressionManager.hpp"
#include "gtest/gtest.h"

template <typename T> void cmpData(T inData, ModAlgosE mod, CompressionAlgosE algo)
{
  CompressionManager manager;
  CompressData data(inData, false);

  CompressData cmpData = manager.compress(data, nullptr, std::vector<ModAlgosE>{mod}, std::vector<CompressionAlgosE>{algo}, true);
  CompressData outData = manager.decompress(cmpData);

  std::vector<char> inCharData = data.getCharData();
  std::vector<char> vecOutData = outData.getCharData();

  if (inCharData.size() == vecOutData.size())
    EXPECT_EQ(0, memcmp(inCharData.data(), vecOutData.data(), inCharData.size()));
  else
    EXPECT_TRUE(false);
}

template <typename T> void createTestData(std::vector<T>& data)
{
  T min = std::numeric_limits<T>::min();
  T max = std::numeric_limits<T>::max();

  T step = (max - min) / 100;

  data.reserve(max - min);
  for (int index = 0; index < 100; index++)
    data.push_back(step * index);
}

#define JOIN(x, y) JOIN_AGAIN(x, y)
#define JOIN_AGAIN(x, y) x##y

#define COMPRESSIONTEST(type, mod, algo)                                                                                                                       \
  TEST(BaseCompressionTest, JOIN(JOIN(algo, mod), type)##Compression)                                                                                          \
  {                                                                                                                                                            \
    std::vector<type> inCharData;                                                                                                                              \
    createTestData(inCharData);                                                                                                                                \
    cmpData(inCharData, ModAlgosE::mod, CompressionAlgosE::algo);                                                                                              \
  }

#define COMPRESSIONTESTALL(algo)                                                                                                                               \
  COMPRESSIONTEST(char, None, algo)                                                                                                                            \
  COMPRESSIONTEST(char, Delta, algo)                                                                                                                           \
  COMPRESSIONTEST(uint32_t, None, algo)                                                                                                                        \
  COMPRESSIONTEST(uint32_t, Delta, algo)

/*
*  None = 0,
  GZip = 1,
  VSEncoding = 2,
  SIMDoptpFor = 3,
  LZMA = 4,
  ZStd = 5,
  BSC = 6,
  LZ4 = 7,
  Test = 8
*/

COMPRESSIONTESTALL(None)
// COMPRESSIONTESTALL(GZip)
COMPRESSIONTESTALL(VSEncoding)
COMPRESSIONTESTALL(SIMDoptpFor)
COMPRESSIONTESTALL(LZMA)
COMPRESSIONTESTALL(ZStd)
COMPRESSIONTESTALL(BSC)
COMPRESSIONTESTALL(LZ4)
COMPRESSIONTESTALL(Test)