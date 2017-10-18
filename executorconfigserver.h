#ifndef EXECUTORCONFIGSERVER_H
#define EXECUTORCONFIGSERVER_H

// STL
#include <list>
#include <string>
#include <unordered_map>

// PDTK
#include <socket.h>
#include <cxxutils/posix_helpers.h>
#include <cxxutils/vfifo.h>
#include <cxxutils/hashing.h>
#include <cxxutils/configmanip.h>
#include <specialized/FileEvent.h>

class ExecutorConfigServer : public ServerSocket
{
public:
  ExecutorConfigServer(void) noexcept;
 ~ExecutorConfigServer(void) noexcept;

  bool configUpdated(const posix::fd_t socket, const std::string& name) const noexcept;

private:
  bool listConfigsReturn(const posix::fd_t socket, const std::list<std::string>& names) const noexcept;
  bool setReturn        (const posix::fd_t socket, const int errcode) const noexcept;
  bool unsetReturn      (const posix::fd_t socket, const int errcode) const noexcept;
  bool getReturn        (const posix::fd_t socket, const int errcode,
                         const std::string& value, const std::list<std::string>& children) const noexcept;

  void listConfigsCall(posix::fd_t socket) noexcept;
  void setCall        (posix::fd_t socket, std::string& key, std::string& value) noexcept;
  void unsetCall      (posix::fd_t socket, std::string& key) noexcept;
  void getCall        (posix::fd_t socket, std::string& key) noexcept;

  bool peerChooser    (posix::fd_t socket, const proccred_t& cred) noexcept;
  void removePeer     (posix::fd_t socket) noexcept;
  void receive        (posix::fd_t socket, vfifo buffer, posix::fd_t fd) noexcept;
  void request        (posix::fd_t socket, posix::sockaddr_t addr, proccred_t cred) noexcept;

  void fileUpdated(const char* filename, FileEvent::Flags_t flags) noexcept;
  void dirUpdated (const char* dirname , FileEvent::Flags_t flags) noexcept;

  struct configfile_t
  {
    std::unique_ptr<FileEvent> fevent;
    ConfigManip config;
  };
  std::unordered_map<pid_t, posix::fd_t> m_endpoints;
  std::unordered_map<std::string, configfile_t> m_configfiles;
  std::unique_ptr<FileEvent> m_dir;
};

inline bool ExecutorConfigServer::listConfigsReturn(const posix::fd_t socket, const std::list<std::string>& names) const noexcept
  { return write(socket, vfifo("RPC", "listConfigsReturn", names), posix::invalid_descriptor); }

inline bool ExecutorConfigServer::configUpdated(const posix::fd_t socket, const std::string& name) const noexcept
  { return write(socket, vfifo("RPC", "configUpdated", name), posix::invalid_descriptor); }

inline bool ExecutorConfigServer::setReturn(const posix::fd_t socket, const int errcode) const noexcept
  { return write(socket, vfifo("RPC", "setReturn"  , errcode), posix::invalid_descriptor); }

inline bool ExecutorConfigServer::unsetReturn  (const posix::fd_t socket, const int errcode) const noexcept
  { return write(socket, vfifo("RPC", "unsetReturn", errcode), posix::invalid_descriptor); }

inline bool ExecutorConfigServer::getReturn(const posix::fd_t socket, const int errcode,
                                            const std::string& value, const std::list<std::string>& children) const noexcept
  { return write(socket, vfifo("RPC", "getReturn", errcode, value, children), posix::invalid_descriptor); }


#endif
