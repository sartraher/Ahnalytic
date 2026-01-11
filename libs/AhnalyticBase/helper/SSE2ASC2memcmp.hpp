#ifndef sse2asx2memcmp_hpp__
#define sse2asx2memcmp_hpp__

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <immintrin.h>

#pragma once
#include <cstddef>
#include <cstring>
#include <immintrin.h>

static inline bool memcmp_equal_avx2(const void* a, const void* b, size_t n)
{
#if defined(__AVX2__)
  const uint8_t* p1 = (const uint8_t*)a;
  const uint8_t* p2 = (const uint8_t*)b;

  size_t i = 0;
  for (; i + 32 <= n; i += 32)
  {
    __m256i v1 = _mm256_loadu_si256((const __m256i*)(p1 + i));
    __m256i v2 = _mm256_loadu_si256((const __m256i*)(p2 + i));
    __m256i cmp = _mm256_cmpeq_epi8(v1, v2);

    if (_mm256_movemask_epi8(cmp) != -1)
      return false;
  }

  for (; i < n; ++i)
    if (p1[i] != p2[i])
      return false;

  return true;
#else
  return false; // AVX2 disabled at compile time
#endif
}

static inline bool memcmp_equal_sse2(const void* a, const void* b, size_t n)
{
#if defined(__SSE2__)
  const uint8_t* p1 = (const uint8_t*)a;
  const uint8_t* p2 = (const uint8_t*)b;

  size_t i = 0;
  for (; i + 16 <= n; i += 16)
  {
    __m128i v1 = _mm_loadu_si128((const __m128i*)(p1 + i));
    __m128i v2 = _mm_loadu_si128((const __m128i*)(p2 + i));
    __m128i cmp = _mm_cmpeq_epi8(v1, v2);

    if (_mm_movemask_epi8(cmp) != 0xFFFF)
      return false;
  }

  for (; i < n; ++i)
    if (p1[i] != p2[i])
      return false;

  return true;
#else
  return false; // SSE2 disabled at compile time (rare on x86-64)
#endif
}

static inline bool memcmp_equal(const void* a, const void* b, size_t n)
{
#if defined(__AVX2__)
  return memcmp_equal_avx2(a, b, n);
#elif defined(__SSE2__)
  return memcmp_equal_sse2(a, b, n);
#else
  return std::memcmp(a, b, n) == 0;
#endif
}

#include <immintrin.h>

#include <cstdint>

#include <vector>

// Fingerprint for early rejection

inline uint32_t fingerprint32(const uint32_t* arr, int size)
{
  uint32_t hash = 0;
  for (int i = 0; i < size; i++)
    hash += arr[i] ^ (arr[i] << 5);
  return hash;
}

// Fully unrolled AVX2 comparison for any windowSize >=64

inline bool avx2_compare_unrolled_dynamic(const uint32_t* window, const uint32_t* array, int windowSize)
{
  int blocks = windowSize / 8;
  int tail = windowSize % 8;

  const __m256i FIRST = _mm256_set1_epi32(window[0]);

  for (int i = 0; i <= windowSize - 8; i++)
  {
    __m256i chunk = _mm256_loadu_si256((__m256i const*)(array + i));
    __m256i cmp = _mm256_cmpeq_epi32(chunk, FIRST);
    unsigned mask = _mm256_movemask_ps(_mm256_castsi256_ps(cmp));
    if (!mask)
      continue;

    int start = i;
    const uint32_t* tptr = array + start;

    // --- Fully unroll AVX2 blocks ---
    int offset = 0;
    for (int b = 0; b + 7 < blocks; b += 8)
    {
      __m256i v0 = _mm256_loadu_si256((__m256i const*)(window + offset + 0 * 8));
      __m256i t0 = _mm256_loadu_si256((__m256i const*)(tptr + offset + 0 * 8));
      if (_mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpeq_epi32(v0, t0))) != 0xFF)
        goto next_candidate;

      __m256i v1 = _mm256_loadu_si256((__m256i const*)(window + offset + 1 * 8));
      __m256i t1 = _mm256_loadu_si256((__m256i const*)(tptr + offset + 1 * 8));
      if (_mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpeq_epi32(v1, t1))) != 0xFF)
        goto next_candidate;

      __m256i v2 = _mm256_loadu_si256((__m256i const*)(window + offset + 2 * 8));
      __m256i t2 = _mm256_loadu_si256((__m256i const*)(tptr + offset + 2 * 8));
      if (_mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpeq_epi32(v2, t2))) != 0xFF)
        goto next_candidate;

      __m256i v3 = _mm256_loadu_si256((__m256i const*)(window + offset + 3 * 8));
      __m256i t3 = _mm256_loadu_si256((__m256i const*)(tptr + offset + 3 * 8));
      if (_mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpeq_epi32(v3, t3))) != 0xFF)
        goto next_candidate;

      __m256i v4 = _mm256_loadu_si256((__m256i const*)(window + offset + 4 * 8));
      __m256i t4 = _mm256_loadu_si256((__m256i const*)(tptr + offset + 4 * 8));
      if (_mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpeq_epi32(v4, t4))) != 0xFF)
        goto next_candidate;

      __m256i v5 = _mm256_loadu_si256((__m256i const*)(window + offset + 5 * 8));
      __m256i t5 = _mm256_loadu_si256((__m256i const*)(tptr + offset + 5 * 8));
      if (_mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpeq_epi32(v5, t5))) != 0xFF)
        goto next_candidate;

      __m256i v6 = _mm256_loadu_si256((__m256i const*)(window + offset + 6 * 8));
      __m256i t6 = _mm256_loadu_si256((__m256i const*)(tptr + offset + 6 * 8));
      if (_mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpeq_epi32(v6, t6))) != 0xFF)
        goto next_candidate;

      __m256i v7 = _mm256_loadu_si256((__m256i const*)(window + offset + 7 * 8));
      __m256i t7 = _mm256_loadu_si256((__m256i const*)(tptr + offset + 7 * 8));
      if (_mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpeq_epi32(v7, t7))) != 0xFF)
        goto next_candidate;

      offset += 8 * 8; // 8 blocks processed
    }

    // Process remaining blocks
    while (offset < blocks * 8)
    {
      __m256i v = _mm256_loadu_si256((__m256i const*)(window + offset));
      __m256i t = _mm256_loadu_si256((__m256i const*)(tptr + offset));
      if (_mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpeq_epi32(v, t))) != 0xFF)
        goto next_candidate;
      offset += 8;
    }

    // Tail comparison
    for (int t = 0; t < tail; t++)
      if (tptr[offset + t] != window[offset + t])
        goto next_candidate;

    return true;

  next_candidate:;
  }
  return false;
}

// Main search function

bool searchSubArray(const uint32_t* array1, int array1Size, const uint32_t* array2, int array2Size, int windowSize)
{
  if (windowSize < 64)
    return false;
  if (array1Size < windowSize || array2Size < windowSize)
    return false;

  // Precompute fingerprints for array1 windows
  std::vector<uint32_t> array1Hashes(array1Size - windowSize + 1);
  for (int i = 0; i <= array1Size - windowSize; i++)
    array1Hashes[i] = fingerprint32(array1 + i, windowSize);

  // Slide windows over array2
  for (int start2 = 0; start2 <= array2Size - windowSize; start2++)
  {
    const uint32_t* window2 = array2 + start2;
    uint32_t hash2 = fingerprint32(window2, windowSize);

    // Compare with array1 windows
    for (int i = 0; i <= array1Size - windowSize; i++)
    {
      if (array1Hashes[i] != hash2)
        continue; // early reject
      if (avx2_compare_unrolled_dynamic(window2, array1 + i, windowSize))
        return true;
    }
  }

  return false;
}

#endif