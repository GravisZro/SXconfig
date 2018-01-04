#ifndef CONFIGSERVER_H
#define CONFIGSERVER_H

// STL
#include <list>
#include <string>
#include <unordered_map>
#include <memory>

// PDTK
#include <socket.h>
#include <cxxutils/posix_helpers.h>
#include <cxxutils/vfifo.h>
#include <cxxutils/hashing.h>
#include <cxxutils/configmanip.h>
#include <specialized/FileEvent.h>

class ConfigServer : public ServerSocket
{
public:
  ConfigServer(void) noexcept;
 ~ConfigServer(void) noexcept;

private:
  bool valueUpdate      (const posix::fd_t socket, const std::string& key, const std::string& value) const noexcept;
  bool valueUnset       (const posix::fd_t socket, const std::string& key) const noexcept;
  bool fullUpdateReturn (const posix::fd_t socket, const posix::error_t errcode) const noexcept;
  bool unsetReturn      (const posix::fd_t socket, const posix::error_t errcode) const noexcept;
  bool setReturn        (const posix::fd_t socket, const posix::error_t errcode) const noexcept;
  bool getReturn        (const posix::fd_t socket, const posix::error_t errcode, const std::string& value, const std::list<std::string>& children) const noexcept;

  void fullUpdateCall (posix::fd_t socket) noexcept;
  void unsetCall      (posix::fd_t socket, const std::string& key) noexcept;
  void setCall        (posix::fd_t socket, const std::string& key, const std::string& value) noexcept;
  void getCall        (posix::fd_t socket, const std::string& key) noexcept;

  bool peerChooser(posix::fd_t socket, const proccred_t& cred) noexcept;
  void receive(posix::fd_t socket, vfifo buffer, posix::fd_t fd) noexcept;
  void request(posix::fd_t socket, posix::sockaddr_t addr, proccred_t cred) noexcept;

  void removePeer(posix::fd_t socket) noexcept;
  void fileUpdated(const char* filename, FileEvent::Flags_t flags) noexcept;

  struct configfile_t
  {
    std::unique_ptr<FileEvent> fevent;
    ConfigManip config;
  };
  std::unordered_map<pid_t, posix::fd_t> m_endpoints;
  std::unordered_map<posix::fd_t, configfile_t> m_configfiles;
};

inline bool ConfigServer::valueUpdate(const posix::fd_t socket, const std::string& key, const std::string& value) const noexcept
  { return write(socket, vfifo("RPC", "valueUpdate", key, value), posix::invalid_descriptor); }

inline bool ConfigServer::valueUnset(const posix::fd_t socket, const std::string& key) const noexcept
  { return write(socket, vfifo("RPC", "valueUnset", key), posix::invalid_descriptor); }

inline bool ConfigServer::fullUpdateReturn(const posix::fd_t socket, const posix::error_t errcode) const noexcept
  { return write(socket, vfifo("RPC", "fullUpdateReturn", errcode), posix::invalid_descriptor); }

inline bool ConfigServer::unsetReturn(const posix::fd_t socket, const posix::error_t errcode) const noexcept
  { return write(socket, vfifo("RPC", "unsetReturn", errcode), posix::invalid_descriptor); }

inline bool ConfigServer::setReturn(const posix::fd_t socket, const posix::error_t errcode) const noexcept
  { return write(socket, vfifo("RPC", "setReturn", errcode), posix::invalid_descriptor); }

inline bool ConfigServer::getReturn(const posix::fd_t socket, const posix::error_t errcode,
                                    const std::string& value, const std::list<std::string>& children) const noexcept
  { return write(socket, vfifo("RPC", "getReturn", errcode, value, children), posix::invalid_descriptor); }

#endif // CONFIGSERVER_H
