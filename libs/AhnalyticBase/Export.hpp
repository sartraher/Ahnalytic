#pragma warning(disable : 4251)

#ifdef AHNALYTICBASE_EXPORTS
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT __declspec(dllimport)
#endif