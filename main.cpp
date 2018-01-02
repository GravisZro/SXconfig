// POSIX
#include <unistd.h>

// POSIX++
#include <cstdlib>
#include <csignal>

// PDTK
#include <application.h>
#include <object.h>
#include <cxxutils/syslogstream.h>

// Project
#include "configcore.h"


#ifndef CONFIG_APP_NAME
#define CONFIG_APP_NAME         "SXconfig"
#endif

#ifndef CONFIG_USERNAME
#define CONFIG_USERNAME         "config"
#endif

#ifndef CONFIG_GROUPNAME
#define CONFIG_GROUPNAME        CONFIG_USERNAME
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

  posix::syslog.open(CONFIG_APP_NAME, posix::facility::daemon);
/*
  if((std::strcmp(posix::getgroupname(posix::getgid()), CONFIG_GROUPNAME) && // if current username is NOT what we want AND
      !posix::setgid(posix::getgroupid(CONFIG_GROUPNAME))) || // unable to change user id
     (std::strcmp(posix::getusername(posix::getuid()), CONFIG_USERNAME) && // if current username is NOT what we want AND
      !posix::setuid(posix::getuserid (CONFIG_USERNAME)))) // unable to change user id
  {
    posix::syslog << posix::priority::critical
                  << "daemon must be launched as username "
                  << '"' << CONFIG_USERNAME << '"'
                  << " or have permissions to setuid/setgid"
                  << posix::eom;
    std::exit(errno);
  }
*/
  Application app;
  ConfigCore core;
  (void)core;
  return app.exec();
}
