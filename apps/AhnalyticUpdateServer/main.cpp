#include "AhnalyticBase/server/UpdateServer.hpp"

#include "Service.h"

#include <iostream>

int main(int argc, char* argv[])
{
  SrvParam svParam;
#if defined(_WIN32) || defined(_WIN64)
  svParam.szDspName = L"Ahnalytic Update Service";
  svParam.szDescribe = L"hnalytic Update Service";
#endif
  svParam.szSrvName = L"Ahnalytic Update Service";

  UpdateServer server;
  svParam.fnStartCallBack = [&server]()
  {
    server.start("127.0.0.1", 80);
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