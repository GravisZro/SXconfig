#ifndef CONFIGSERVER_H
#define CONFIGSERVER_H

// PDTK
#include <socket.h>
#include <cxxutils/posix_helpers.h>

class ConfigServer : public ServerSocket
{
public:
  ConfigServer(const char* const username, const char* const filename) noexcept;

  bool configUpdated (const posix::fd_t socket) noexcept { return incant(socket, "configUpdated", posix::invalid_descriptor); }
  bool setValueReturn(const posix::fd_t socket, const int errcode) noexcept { return incant(socket, "setValueReturn", posix::invalid_descriptor, errcode); }
  bool getValueReturn(const posix::fd_t socket, const std::string& value) noexcept { return incant(socket, "getValueReturn", posix::invalid_descriptor, value); }
  bool getAllReturn  (const posix::fd_t socket, const std::unordered_map<std::string, std::string>& values) noexcept { return incant(socket, "getAllReturn", posix::invalid_descriptor, values); }

  signal<posix::fd_t, std::string, std::string> setValueCall;
  signal<posix::fd_t, std::string> getValueCall;
  signal<posix::fd_t> getAllCall;

private:
  void allowDeny(posix::fd_t socket, posix::sockaddr_t addr, proccred_t cred) noexcept;
  void removeEndpoint(posix::fd_t socket) noexcept;
  void receive(posix::fd_t socket, vfifo buffer, posix::fd_t fd) noexcept;

  template<typename... ArgTypes>
  bool incant(posix::fd_t socket, const char* func_name, posix::fd_t fd, ArgTypes&... args) noexcept
  {
    vfifo data;
    data.serialize("RPC", func_name, args...);
    return write(socket, data, fd);
  }

  std::unordered_map<pid_t, posix::fd_t> m_endpoints;
  std::string m_path;
};

#endif // CONFIGSERVER_H
