#pragma warning(disable : 4251)

#include <mimalloc.h>
#include <new>
#include <vector>

template<typename T>
using MyAllocator = mi_stl_allocator<T>;

namespace ahn
{
  template<typename T>
  using vector = std::vector<T, MyAllocator<T>>;
}

#include <cstdint>

#if defined(_WIN32) || defined(_WIN64)
#ifdef AHNALYTICBASE_EXPORTS
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT __declspec(dllimport)
#endif
#else
#define DLLEXPORT __attribute__((visibility("default")))
#endif