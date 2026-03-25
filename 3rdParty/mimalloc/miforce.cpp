#include <mimalloc-new-delete.h>
#include <mimalloc.h>

#include <iostream>
#include <string>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <dlfcn.h>
#include <limits.h>
#include <unistd.h>
#endif

// addr = pointer to a function inside the module you want
std::string getCurrentModulePath(void* addr = nullptr)
{
#if defined(_WIN32)
  char path[MAX_PATH] = {0};
  HMODULE hModule = nullptr;

  if (addr)
  {
    // Get the module handle from the function address (DLL)
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)addr, &hModule);
  }
  else
  {
    // No address? Return main executable
    hModule = GetModuleHandleA(NULL);
  }

  GetModuleFileNameA(hModule, path, MAX_PATH);
  return std::string(path);

#elif defined(__linux__) || defined(__APPLE__)
  Dl_info info;
  if (!addr)
  {
    addr = (void*)getCurrentModulePath; // default to current library/exe
  }
  if (dladdr(addr, &info) && info.dli_fname)
  {
    return std::string(info.dli_fname);
  }
  return "";

#else
  return ""; // unsupported
#endif
}

// Example function inside a DLL or shared library
void myLibraryFunction()
{
}

class MiTest
{
public:
  MiTest()
  {
    void* p = new int;
    std::cout << "MiMalloc Active:" << mi_is_in_heap_region(p) << " For current module path: " << getCurrentModulePath((void*)myLibraryFunction) << std::endl;
  }
};

static MiTest miTest;