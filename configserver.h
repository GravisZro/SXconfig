#ifndef CONFIGSERVER_H
#define CONFIGSERVER_H

// PDTK
#include <socket.h>
#include <cxxutils/posix_helpers.h>
#include <cxxutils/vfifo.h>
#include <cxxutils/hashing.h>
#include <cxxutils/configmanip.h>

// Generated class by Incanto
class ConfigServerInterface : public ServerSocket
{
public:
  ConfigServerInterface(void) noexcept;

  virtual bool peerChooser(posix::fd_t socket, const proccred_t& cred) noexcept = 0;
  void request(posix::fd_t socket, posix::sockaddr_t addr, proccred_t cred) noexcept;

// RPC incantations (outgoing)
  bool configUpdated(const posix::fd_t socket) const noexcept { return write(socket, vfifo("RPC", "configUpdated")); }
  bool setReturn(const posix::fd_t socket, const int errcode) const noexcept { return write(socket, vfifo("RPC", "setReturn", errcode)); }
  bool getReturn(const posix::fd_t socket, const std::string& value) const noexcept { return write(socket, vfifo("RPC", "getReturn", value)); }
  bool getAllReturn(const posix::fd_t socket, const std::unordered_map<std::string, std::string>& values) const noexcept { return write(socket, vfifo("RPC", "getAllReturn", values)); }

// RPC invocations (incoming)
  signal<posix::fd_t, std::string, std::string> setCall;
  signal<posix::fd_t, std::string> unsetCall;
  signal<posix::fd_t, std::string> getCall;
  signal<posix::fd_t> getAllCall;

private:
  void receive(posix::fd_t socket, vfifo buffer, posix::fd_t fd) noexcept;
};


class ConfigServer : public ConfigServerInterface
{
public:
  ConfigServer(const char* const username, const char* const filename) noexcept;
  bool peerChooser(posix::fd_t socket, const proccred_t& cred) noexcept;

  void set(posix::fd_t socket, std::string key, std::string value) noexcept;
  void unset(posix::fd_t socket, std::string key) noexcept;
  void get(posix::fd_t socket, std::string key) noexcept;
  void getAll(posix::fd_t socket) noexcept;

private:
  struct configfile_t
  {
    posix::fd_t fd;
    ConfigManip data;
  };
  void removePeer(posix::fd_t socket) noexcept;
  std::unordered_map<pid_t, posix::fd_t> m_endpoints;
  std::unordered_map<posix::fd_t, configfile_t> m_configfiles;
  std::string m_path;
};

#endif // CONFIGSERVER_H
