// PUT
#include <put/application.h>
#include <put/object.h>
#include <put/cxxutils/posix_helpers.h>
#include <put/cxxutils/syslogstream.h>

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
  posix::syslog << posix::priority::notice
                << "program has exited."
                << posix::eom;
}

int main(int argc, char *argv[]) noexcept
{
  (void)argc;
  (void)argv;
  posix::atexit(exiting);
  posix::signal(SIGPIPE, SIG_IGN); // needed for OSX

  posix::syslog.open(CONFIG_APP_NAME, posix::facility::provider);

  if(posix::strcmp(posix::getgroupname(posix::getgid()), CONFIG_GROUPNAME) && // if current groupname is NOT what we want AND
     !posix::setgid(posix::getgroupid(CONFIG_GROUPNAME))) // unable to change group id
  {
    posix::syslog << posix::priority::critical
                  << "provider must be launched as group name \"%1\" or have permissions to setgid"
                  << CONFIG_GROUPNAME
                  << posix::eom;
    posix::exit(errno);
  }

  if(posix::strcmp(posix::getusername(posix::getuid()), CONFIG_USERNAME) && // if current username is NOT what we want AND
     !posix::setuid(posix::getuserid(CONFIG_USERNAME))) // unable to change user id
  {
    posix::syslog << posix::priority::critical
                  << "provider must be launched as user name \"%1\" or have permissions to setuid"
                  << CONFIG_USERNAME
                  << posix::eom;
    posix::exit(errno);
  }

  Application app;
  ConfigCore core;
  (void)core;
  return app.exec();
}
