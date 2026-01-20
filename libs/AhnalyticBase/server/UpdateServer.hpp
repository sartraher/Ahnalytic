#ifndef updateserver_hpp__
#define updateserver_hpp__

#include "AhnalyticBase/Export.hpp"

#include <string>

class UpdateServerPrivate;

class DLLEXPORT UpdateServer
{
public:
  UpdateServer();
  ~UpdateServer();

  void init();
  void start();
  void stop();

private:
  UpdateServerPrivate* priv = nullptr;

protected:
  void updateGitHubBaseDatabase();
  void scanGitHubRepos();
};

#endif