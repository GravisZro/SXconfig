#ifndef CONFIGSERVER_H
#define CONFIGSERVER_H

// PDTK
#include <socket.h>
#include <cxxutils/posix_helpers.h>
#include <cxxutils/vfifo.h>
#include <cxxutils/hashing.h>
#include <cxxutils/configmanip.h>

/*
  server out   void configUpdated(void);
  server inout {int errcode} unset(std::string key);
  server inout {int errcode} set(std::string key, std::string value);
  server inout {int errcode, std::string value} get(std::string key);
*/

class ConfigServer : public ServerSocket
{
public:
  ConfigServer(void) noexcept;

private:
  bool configUpdated(const posix::fd_t socket                                             ) const noexcept { return write(socket, vfifo("RPC", "configUpdated"              ), posix::invalid_descriptor); }
  bool setReturn    (const posix::fd_t socket, const int errcode                          ) const noexcept { return write(socket, vfifo("RPC", "setReturn"  , errcode       ), posix::invalid_descriptor); }
  bool unsetReturn  (const posix::fd_t socket, const int errcode                          ) const noexcept { return write(socket, vfifo("RPC", "unsetReturn", errcode       ), posix::invalid_descriptor); }
  bool getReturn    (const posix::fd_t socket, const int errcode, const std::string& value) const noexcept { return write(socket, vfifo("RPC", "getReturn"  , errcode, value), posix::invalid_descriptor); }

  void setCall  (posix::fd_t socket, std::string& key, std::string& value) noexcept;
  void getCall  (posix::fd_t socket, std::string& key) noexcept;
  void unsetCall(posix::fd_t socket, std::string& key) noexcept;

  bool peerChooser(posix::fd_t socket, const proccred_t& cred) noexcept;
  void receive(posix::fd_t socket, vfifo buffer, posix::fd_t fd) noexcept;
  void request(posix::fd_t socket, posix::sockaddr_t addr, proccred_t cred) noexcept;

  void removePeer(posix::fd_t socket) noexcept;

  struct configfile_t
  {
    posix::fd_t fd; // watches for changes
    ConfigManip config;
  };
  std::unordered_map<pid_t, posix::fd_t> m_endpoints;
  std::unordered_map<posix::fd_t, configfile_t> m_configfiles;
};

#endif // CONFIGSERVER_H
