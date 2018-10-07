#include "configcore.h"

// PDTK
#include <cxxutils/syslogstream.h>

#ifndef SCFS_PATH
#define SCFS_PATH               "/svc"
#endif

#ifndef CONFIG_USERNAME
#define CONFIG_USERNAME         "config"
#endif

#ifdef ANONYMOUS_SOCKET
#undef ANONYMOUS_SOCKET
#endif
#define ANONYMOUS_SOCKET        "\0"

#ifndef CONFIG_IO_SOCKET
#define CONFIG_IO_SOCKET        SCFS_PATH "/" CONFIG_USERNAME "/io"
#endif

#ifndef CONFIG_DIRECTOR_SOCKET
#define CONFIG_DIRECTOR_SOCKET  SCFS_PATH "/" CONFIG_USERNAME "/director"
#endif

ConfigCore::ConfigCore(void)
{
  if(m_config_server.bind(CONFIG_IO_SOCKET))
    posix::syslog << posix::priority::info
                  << "Config provider bound to socket file %1"
                  << CONFIG_IO_SOCKET
                  << posix::eom;
  else if(m_config_server.bind(ANONYMOUS_SOCKET CONFIG_IO_SOCKET))
    posix::syslog << posix::priority::info
                  << "Config provider bound to anonymous socket %1"
                  << CONFIG_IO_SOCKET
                  << posix::eom;
  else
    posix::syslog << posix::priority::error
                  << "Unable to bind Config provider to %1"
                  << CONFIG_IO_SOCKET
                  << posix::eom;

  if(m_director_server.bind(CONFIG_DIRECTOR_SOCKET))
    posix::syslog << posix::priority::info
                  << "Config provider bound to %1"
                  << CONFIG_DIRECTOR_SOCKET
                  << posix::eom;
  else if(m_director_server.bind(ANONYMOUS_SOCKET CONFIG_DIRECTOR_SOCKET))
    posix::syslog << posix::priority::info
                  << "Config provider bound to anonymous socket %1"
                  << CONFIG_DIRECTOR_SOCKET
                  << posix::eom;
  else
    posix::syslog << posix::priority::error
                  << "Unable to bind Config provider to %1"
                  << CONFIG_DIRECTOR_SOCKET
                  << posix::eom;
}
