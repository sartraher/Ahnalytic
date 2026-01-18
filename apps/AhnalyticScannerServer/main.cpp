#include "AhnalyticBase/server/ScanServer.hpp"

#include "Service.h"

#include <iostream>

int main(int argc, char* argv[])
{
  SrvParam svParam;
#if defined(_WIN32) || defined(_WIN64)
  svParam.szDspName = L"Ahnalytic Scanner Service";
  svParam.szDescribe = L"Ahnalytic Scanner Service";
#endif
  svParam.szSrvName = L"Ahnalytic Scanner Service";

  ScanServer server;
  svParam.fnStartCallBack = [&server]()
  {
    server.start("127.0.0.1", 9080);
  };
  svParam.fnStopCallBack = [&server]()
  {
    server.stop();
  };
  svParam.fnSignalCallBack = []()
    {
    };

  return ServiceMain(argc, argv, svParam);
}