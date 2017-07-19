// PDTK
#include <application.h>
#include <object.h>
#include <cxxutils/syslogstream.h>

// project
#include "configserver.h"

constexpr const char* const appname = "SXconfigd";
constexpr const char* const username  = "config";
constexpr const char* const groupname = "config";

void exiting(void)
{
  posix::syslog << posix::priority::notice << "daemon has exited." << posix::eom;
}

#include <iostream>
int main(int argc, char *argv[]) noexcept
{
  (void)argc;
  (void)argv;
  ::atexit(exiting);
  ::signal(SIGPIPE, SIG_IGN);

  posix::syslog.open(appname, posix::facility::daemon);

  if((std::strcmp(posix::getgroupname(::getgid()), groupname) && // if current username is NOT what we want AND
      ::setgid(posix::getgroupid(groupname)) == posix::error_response) || // unable to change user id
     (std::strcmp(posix::getusername(::getuid()), username) && // if current username is NOT what we want AND
      ::setuid(posix::getuserid (username)) == posix::error_response)) // unable to change user id
  {
    posix::syslog << posix::priority::critical
                  << "daemon must be launched as username "
                  << '"' << username << '"'
                  << " or have permissions to setuid/setgid"
                  << posix::eom;
    ::exit(-1);
  }
  Application app;
  ConfigServer server("file_monitor");

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
