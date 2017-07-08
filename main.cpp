// PDTK
#include <application.h>
#include <object.h>
#include <cxxutils/syslogstream.h>

// project
#include "configserver.h"

constexpr const char* const appname = "SXconfigd";
constexpr const char* const username = "config";

void exiting(void)
{
  posix::syslog << posix::priority::notice << "daemon has exited." << posix::eom;
}

void allfunc(void)
{
  posix::syslog << "tada!" << posix::eom;
}

int main(int argc, char *argv[]) noexcept
{
  (void)argc;
  (void)argv;
  ::atexit(exiting);

  posix::syslog.open(appname, posix::facility::daemon);
  if(posix::getusername(::getuid()) != username)
  {
    posix::syslog << posix::priority::critical << "daemon must be launched as username " << '"' << username << '"' << posix::eom;
    ::exit(-1);
  }

  Application app;
  ConfigServer server(username, "file_monitor");

  Object::connect(server.getAllCall, allfunc);

  return app.exec();
}

#if 0
// STL
#include <iostream>

// C++
#include <cassert>

// PDTK
#include <cxxutils/configmanip.h>

int main(int argc, char *argv[]) noexcept
{
  std::string data = "[config]\nkey=original value\n # ORLY?\nKey = Value; yeah rly\n/config/key=\"overwritten\"; stuff\n[config]key=\"oh my\"\n";
  ConfigManip conf;
  assert(conf.read(data));

  std::string dout;
  assert(conf.write(dout));
  std::cout << "OUTPUT" << std::endl << '"' << dout << '"' << std::endl;
  //return 0;

  auto node = conf.findNode("/config/1/key");
  if(node != nullptr)
    std::cout << "node: " << '"' << node->value << '"' << std::endl;
  else
    std::cout << "not found!" << std::endl;
  return 0;
}
#endif
