#ifndef CONFIGSERVER_H
#define CONFIGSERVER_H

// PDTK
#include <socket.h>
#include <cxxutils/posix_helpers.h>

class ConfigServer : public ServerSocket
{
public:
  ConfigServer(const char* const username, const char* const filename) noexcept;

  bool configUpdated (const posix::fd_t client) noexcept { return incant(client, "configUpdated", posix::invalid_descriptor); }
  bool setValueReturn(const posix::fd_t client, const int errcode) noexcept { return incant(client, "setValueReturn", posix::invalid_descriptor, errcode); }
  bool getValueReturn(const posix::fd_t client, const std::string& value) noexcept { return incant(client, "getValueReturn", posix::invalid_descriptor, value); }
  bool getAllReturn  (const posix::fd_t client, const std::unordered_map<std::string, std::string>& values) noexcept { return incant(client, "getAllReturn", posix::invalid_descriptor, values); }

  signal<std::string, std::string> setValueCall;
  signal<std::string> getValueCall;
  signal<> getAllCall;

private:
  void allowDeny(posix::fd_t fd, posix::sockaddr_t addr, proccred_t cred) noexcept;
  void removeEndpoint(posix::fd_t fd) noexcept;
  void receive(posix::fd_t server, vfifo buffer, posix::fd_t fd) noexcept;

  template<typename... ArgTypes>
  bool incant(posix::fd_t client, const char* func_name, posix::fd_t fd, ArgTypes&... args) noexcept
  {
    vfifo data;
    data.serialize("RPC", func_name, args...);
    return write(client, data, fd);
  }

  std::unordered_map<pid_t, posix::fd_t> m_endpoints;
  std::string m_path;
};

#endif // CONFIGSERVER_H
