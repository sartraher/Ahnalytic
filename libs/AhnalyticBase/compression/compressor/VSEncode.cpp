#include "VSEncode.hpp"

/*

#include <cstdint>
#include <iostream>
#include <tmmintrin.h> // SSSE3
#include <vector>

inline void getByteSizesSimd(const uint32_t* in, char* sizes)
{
  __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(in));
  __m128i zero = _mm_setzero_si128();

  __m128i mask1 = _mm_set1_epi32(0xFFFFFF00);
  __m128i mask2 = _mm_set1_epi32(0xFFFF0000);
  __m128i mask3 = _mm_set1_epi32(0xFF000000);

  __m128i b1 = _mm_cmpeq_epi32(_mm_and_si128(v, mask1), zero);
  __m128i b2 = _mm_cmpeq_epi32(_mm_and_si128(v, mask2), zero);
  __m128i b3 = _mm_cmpeq_epi32(_mm_and_si128(v, mask3), zero);

  __m128i bytes = _mm_set1_epi32(4);
  bytes = _mm_sub_epi32(bytes, _mm_and_si128(b3, _mm_set1_epi32(1)));
  bytes = _mm_sub_epi32(bytes, _mm_and_si128(b2, _mm_set1_epi32(1)));
  bytes = _mm_sub_epi32(bytes, _mm_and_si128(b1, _mm_set1_epi32(1)));

  // Safe store
  alignas(16) uint32_t temp[4];
  _mm_storeu_si128(reinterpret_cast<__m128i*>(temp), bytes);

  // Downcast to byte sizes
  for (int i = 0; i < 4; ++i)
    sizes[i] = static_cast<char>(temp[i]);
}

// Encoder with auto-growing vector and SIMD size detection
void vsencodeSimdGrow(const std::vector<uint32_t>& in, std::vector<char>& out)
{
  out.clear();
  size_t len = in.size();
  size_t i = 0;

  while (i + 4 <= len)
  {
    char sizes[4];
    getByteSizesSimd(in.data() + i, sizes);

    char ctrl = 0;
    for (int j = 0; j < 4; ++j)
    {
      ctrl |= ((sizes[j] - 1) & 0x03) << (2 * j);
    }
    out.push_back(ctrl);

    for (int j = 0; j < 4; ++j)
    {
      uint32_t v = in[i + j];
      for (int b = 0; b < sizes[j]; ++b)
      {
        out.push_back(static_cast<char>((v >> (8 * b)) & 0xFF));
      }
    }
    i += 4;
  }

  // Tail: scalar fallback for the remaining <4 items
  if (i < len)
  {
    char sizes[4] = {1, 1, 1, 1};
    char ctrl = 0;
    for (int j = 0; j < (len - i); ++j)
    {
      uint32_t v = in[i + j];
      if (v < (1U << 8))
        sizes[j] = 1;
      else if (v < (1U << 16))
        sizes[j] = 2;
      else if (v < (1U << 24))
        sizes[j] = 3;
      else
        sizes[j] = 4;

      ctrl |= ((sizes[j] - 1) & 0x03) << (2 * j);
    }

    out.push_back(ctrl);
    for (int j = 0; j < (len - i); ++j)
    {
      uint32_t v = in[i + j];
      for (int b = 0; b < sizes[j]; ++b)
      {
        out.push_back(static_cast<char>((v >> (8 * b)) & 0xFF));
      }
    }
  }
}

// Decoder remains scalar (works seamlessly)
void vsdecodeSimdGrow(const std::vector<char>& in, std::vector<uint32_t>& out, size_t expected_count)
{
  out.clear();
  out.reserve(expected_count);

  size_t inpos = 0;
  size_t outpos = 0;

  while (outpos + 4 <= expected_count)
  {
    char ctrl = in[inpos++];
    for (int j = 0; j < 4; ++j)
    {
      char size = ((ctrl >> (2 * j)) & 0x03) + 1;
      uint32_t val = 0;
      for (int b = 0; b < size; ++b)
        val |= static_cast<uint32_t>(in[inpos++]) << (8 * b);

      out.push_back(val);
      ++outpos;
    }
  }

  if (outpos < expected_count)
  {
    char ctrl = in[inpos++];
    for (int j = 0; j < (expected_count - outpos); ++j)
    {
      char size = ((ctrl >> (2 * j)) & 0x03) + 1;
      uint32_t val = 0;
      for (int b = 0; b < size; ++b)
        val |= static_cast<uint32_t>(in[inpos++]) << (8 * b);
      out.push_back(val);
      ++outpos;
    }
  }
}

VSEncodeCompressor::VSEncodeCompressor()
{
}

CompressData VSEncodeCompressor::compress(const CompressData& data)
{
  std::vector<char> out;
  vsencodeSimdGrow(data.getUint32Data(), out);
  return out;
}

CompressData VSEncodeCompressor::decompress(const CompressData& data)
{
  std::vector<uint32_t> out;
  vsdecodeSimdGrow(data.getCharData(), out, data.getHeader().originalSize / sizeof(uint32_t));
  return out;
}

std::string VSEncodeCompressor::getId()
{
  return "vsencode";
}
*/