#pragma warning(disable : 4251)

#include <ankerl/unordered_dense.h>
#include <mimalloc.h>
#include <new>
#include <unordered_map>
#include <vector>

template <typename T>
using MyAllocator = mi_stl_allocator<T>;

namespace ahn
{
template <typename T>
using vector = std::vector<T, MyAllocator<T>>;

template <typename K, typename Hash = ankerl::unordered_dense::hash<K>, typename KeyEqual = std::equal_to<K>>
using set = ankerl::unordered_dense::set<K, Hash, KeyEqual, MyAllocator<K>>;

template <typename K, typename V, typename Hash = ankerl::unordered_dense::hash<K>, typename KeyEqual = std::equal_to<K>>
using map = ankerl::unordered_dense::map<K, V, Hash, KeyEqual, MyAllocator<std::pair<K, V>>>;
} // namespace ahn

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