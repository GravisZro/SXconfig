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

#ifndef APP_NAME
#define APP_NAME                "SXconfig"
#endif

#ifndef MCFS_PATH
#define MCFS_PATH               "/mc"
#endif

#ifndef CONFIG_USERNAME
#define CONFIG_USERNAME         "config"
#endif

#ifndef CONFIG_GROUPNAME
#define CONFIG_GROUPNAME        "config"
#endif

#ifndef CONFIG_IO_SOCKET
#define CONFIG_IO_SOCKET        "/" CONFIG_USERNAME "/io"
#endif

#ifndef CONFIG_EXECUTOR_SOCKET
#define CONFIG_EXECUTOR_SOCKET  "/" CONFIG_USERNAME "/executor"
#endif

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

  posix::syslog.open(APP_NAME, posix::facility::daemon);

  if((std::strcmp(posix::getgroupname(::getgid()), CONFIG_GROUPNAME) && // if current username is NOT what we want AND
      ::setgid(posix::getgroupid(CONFIG_GROUPNAME)) == posix::error_response) || // unable to change user id
     (std::strcmp(posix::getusername(::getuid()), CONFIG_USERNAME) && // if current username is NOT what we want AND
      ::setuid(posix::getuserid (CONFIG_USERNAME)) == posix::error_response)) // unable to change user id
  {
    posix::syslog << posix::priority::critical
                  << "daemon must be launched as username "
                  << '"' << CONFIG_USERNAME << '"'
                  << " or have permissions to setuid/setgid"
                  << posix::eom;
    std::exit(errno);
  }

  Application app;
  ConfigServer config_server;
  ExecutorConfigServer executor_server;

  if(config_server.bind(MCFS_PATH CONFIG_IO_SOCKET))
    posix::syslog << posix::priority::info << "daemon bound to " << MCFS_PATH CONFIG_IO_SOCKET << posix::eom;
  else
    posix::syslog << posix::priority::error << "unable to bind daemon to " << MCFS_PATH CONFIG_IO_SOCKET << posix::eom;


  if(executor_server.bind(MCFS_PATH CONFIG_EXECUTOR_SOCKET))
    posix::syslog << posix::priority::info << "daemon bound to " << MCFS_PATH CONFIG_EXECUTOR_SOCKET << posix::eom;
  else
    posix::syslog << posix::priority::error << "unable to bind daemon to " << MCFS_PATH CONFIG_EXECUTOR_SOCKET << posix::eom;

  return app.exec();
}
