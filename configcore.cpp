#include "configcore.h"

// PDTK
#include <cxxutils/syslogstream.h>
#include <specialized/mountpoints.h>

#ifndef CONFIG_USERNAME
#define CONFIG_USERNAME         "config"
#endif

#ifndef CONFIG_IO_SOCKET
#define CONFIG_IO_SOCKET        "io"
#endif

#ifndef CONFIG_DIRECTOR_SOCKET
#define CONFIG_DIRECTOR_SOCKET  "director"
#endif

ConfigCore::ConfigCore(void)
{
  if(scfs_path == nullptr)
    reinitialize_paths();
  if(scfs_path == nullptr)
  {
    posix::syslog << posix::priority::error
                  << "SCFS is not mounted! Unable to bind Config and Config Director sockets."
                  << posix::eom;
  }
  else
  {
    std::string base = scfs_path;
    base.push_back('/');
    base.append(CONFIG_USERNAME);
    base.push_back('/');

    if(m_config_server.bind((base + CONFIG_IO_SOCKET).c_str()))
      posix::syslog << posix::priority::info
                    << "Config provider bound to socket file %1"
                    << base + CONFIG_IO_SOCKET
                    << posix::eom;
    else
      posix::syslog << posix::priority::error
                    << "Unable to bind Config provider to %1"
                    << base + CONFIG_IO_SOCKET
                    << posix::eom;

    if(m_director_server.bind((base + CONFIG_DIRECTOR_SOCKET).c_str()))
      posix::syslog << posix::priority::info
                    << "Config Director provider bound to %1"
                    << base + CONFIG_DIRECTOR_SOCKET
                    << posix::eom;
    else
      posix::syslog << posix::priority::error
                    << "Unable to bind Config Director provider to %1"
                    << base + CONFIG_DIRECTOR_SOCKET
                    << posix::eom;
  }
}
