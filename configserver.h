#ifndef CONFIGSERVER_H
#define CONFIGSERVER_H

// PDTK
#include <socket.h>
#include <cxxutils/posix_helpers.h>
#include <cxxutils/vfifo.h>
#include <cxxutils/hashing.h>

// Generated class by Incanto
class ConfigServerInterface : public ServerSocket
{
public:
  ConfigServerInterface(void) noexcept;

  virtual bool peerChooser(posix::fd_t socket, const posix::sockaddr_t& addr, const proccred_t& cred) noexcept = 0;
  void request(posix::fd_t socket, posix::sockaddr_t addr, proccred_t cred) noexcept;

// RPC calls
  bool configUpdated(const posix::fd_t socket) const noexcept { return write(socket, vfifo("RPC", "configUpdated"), posix::invalid_descriptor); }
  bool setValueReturn(const posix::fd_t socket, const int errcode) const noexcept { return write(socket, vfifo("RPC", "setValueReturn", errcode), posix::invalid_descriptor); }
  bool getValueReturn(const posix::fd_t socket, const std::string& value) const noexcept { return write(socket, vfifo("RPC", "getValueReturn", value), posix::invalid_descriptor); }
  bool getAllReturn(const posix::fd_t socket, const std::unordered_map<std::string, std::string>& values) const noexcept { return write(socket, vfifo("RPC", "getAllReturn", values), posix::invalid_descriptor); }

// RPC invocations
  signal<posix::fd_t, std::string, std::string> setValueCall;
  signal<posix::fd_t, std::string> getValueCall;
  signal<posix::fd_t> getAllCall;

private:
  void receive(posix::fd_t socket, vfifo buffer, posix::fd_t fd) noexcept;
};


class ConfigServer : public ConfigServerInterface
{
public:
  ConfigServer(const char* const username, const char* const filename) noexcept;
  bool peerChooser(posix::fd_t socket, const posix::sockaddr_t& addr, const proccred_t& cred) noexcept;

private:
  void removeEndpoint(posix::fd_t socket) noexcept;
  std::unordered_map<pid_t, posix::fd_t> m_endpoints;
  std::string m_path;
};

#endif // CONFIGSERVER_H
