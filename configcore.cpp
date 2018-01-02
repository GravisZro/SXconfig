#include "configcore.h"

// PDTK
#include <cxxutils/syslogstream.h>

#ifndef MCFS_PATH
#define MCFS_PATH               "/mc"
#endif

#ifndef CONFIG_USERNAME
#define CONFIG_USERNAME         "config"
#endif

#ifdef ANONYMOUS_SOCKET
#undef ANONYMOUS_SOCKET
#endif
#define ANONYMOUS_SOCKET        "\0"

#ifndef CONFIG_IO_SOCKET
#define CONFIG_IO_SOCKET        MCFS_PATH "/" CONFIG_USERNAME "/io"
#endif

#ifndef CONFIG_EXECUTOR_SOCKET
#define CONFIG_EXECUTOR_SOCKET  MCFS_PATH "/" CONFIG_USERNAME "/executor"
#endif

ConfigCore::ConfigCore(void)
{
  if(m_config_server.bind(CONFIG_IO_SOCKET))
    posix::syslog << posix::priority::info << "Config daemon bound to socket file " << CONFIG_IO_SOCKET << posix::eom;
  else if(m_config_server.bind(ANONYMOUS_SOCKET CONFIG_IO_SOCKET))
    posix::syslog << posix::priority::info << "Config daemon bound to anonymous socket " << CONFIG_IO_SOCKET << posix::eom;
  else
    posix::syslog << posix::priority::error << "Unable to bind Config daemon to " << CONFIG_IO_SOCKET << posix::eom;

  if(m_executor_server.bind(CONFIG_EXECUTOR_SOCKET))
    posix::syslog << posix::priority::info << "Config daemon bound to " << CONFIG_EXECUTOR_SOCKET << posix::eom;
  else if(m_executor_server.bind(ANONYMOUS_SOCKET CONFIG_EXECUTOR_SOCKET))
    posix::syslog << posix::priority::info << "Config daemon bound to anonymous socket " << CONFIG_EXECUTOR_SOCKET << posix::eom;
  else
    posix::syslog << posix::priority::error << "Unable to bind Config daemon to " << CONFIG_EXECUTOR_SOCKET << posix::eom;
}
