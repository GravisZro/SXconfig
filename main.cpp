// POSIX
#include <unistd.h>

// POSIX++
#include <cstdlib>
#include <csignal>

// PDTK
#include <application.h>
#include <object.h>
#include <cxxutils/syslogstream.h>

// project
#include "configserver.h"
#include "executorconfigserver.h"

constexpr const char* const appname = "SXconfigd";
constexpr const char* const username  = "config";
constexpr const char* const groupname = "config";
constexpr const char* const configs_socket_path = "/mc/config/configs";
constexpr const char* const executor_socket_path = "/mc/config/executor";

void exiting(void)
{
  posix::syslog << posix::priority::notice << "daemon has exited." << posix::eom;
}

int main(int argc, char *argv[]) noexcept
{
  (void)argc;
  (void)argv;
  std::atexit(exiting);
  std::signal(SIGPIPE, SIG_IGN); // needed for OSX

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
    std::exit(-1);
  }

  Application app;
  ConfigServer config_server;
  ExecutorConfigServer executor_server;

  if(config_server.bind(configs_socket_path))
    posix::syslog << posix::priority::info << "daemon bound to " << configs_socket_path << posix::eom;
  else
    posix::syslog << posix::priority::error << "unable to bind daemon to " << configs_socket_path << posix::eom;


  if(executor_server.bind(executor_socket_path))
    posix::syslog << posix::priority::info << "daemon bound to " << executor_socket_path << posix::eom;
  else
    posix::syslog << posix::priority::error << "unable to bind daemon to " << executor_socket_path << posix::eom;

  return app.exec();
}
