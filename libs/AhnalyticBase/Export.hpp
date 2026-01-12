#pragma warning(disable : 4251)

#if defined(_WIN32) || defined(_WIN64)
#ifdef AHNALYTICBASE_EXPORTS
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT __declspec(dllimport)
#endif
#else
#define DLLEXPORT __attribute__((visibility("default")))
#endif