#ifndef CONFIGSERVER_H
#define CONFIGSERVER_H

// PDTK
#include <socket.h>
#include <cxxutils/posix_helpers.h>

class ConfigServer : public ServerSocket
{
public:
  ConfigServer(const char* const username, const char* const filename);

  bool setValueReturn(const posix::fd_t client, const int errcode) { return incant(client, "setValueReturn", posix::invalid_descriptor, errcode); }
  bool getValueReturn(const posix::fd_t client, const std::string& value) { return incant(client, "getValueReturn", posix::invalid_descriptor, value); }

  signal<std::string, std::string> setValueCall;
  signal<std::string> getValueCall;

private:
  void allowDeny(posix::fd_t fd, posix::sockaddr_t addr, proccred_t cred);
  void removeEndpoint(posix::fd_t fd);
  void receive(posix::fd_t server, vfifo buffer, posix::fd_t fd);

  template<typename... ArgTypes>
  bool incant(posix::fd_t client, const char* func_name, posix::fd_t fd, ArgTypes&... args)
  {
    vfifo data;
    data.serialize("RPC", func_name, args...);
    return write(client, data, fd);
  }

  std::unordered_map<pid_t, posix::fd_t> m_endpoints;
  std::string m_path;
};

#endif // CONFIGSERVER_H
