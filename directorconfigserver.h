#ifndef DIRECTORCONFIGSERVER_H
#define DIRECTORCONFIGSERVER_H

// STL
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

// PUT
#include <put/socket.h>
#include <put/cxxutils/posix_helpers.h>
#include <put/cxxutils/vfifo.h>
#include <put/cxxutils/hashing.h>
#include <put/cxxutils/configmanip.h>
#include <put/specialized/fileevent.h>

class DirectorConfigServer : public ServerSocket
{
public:
  DirectorConfigServer(void) noexcept;
 ~DirectorConfigServer(void) noexcept;

  bool valueSet   (const posix::fd_t socket, const std::string& config, const std::string& key, const std::string& value) const noexcept;
  bool valueUnset (const posix::fd_t socket, const std::string& config, const std::string& key) const noexcept;

private:
  bool listConfigsReturn(const posix::fd_t socket, const std::vector<std::string>& names) const noexcept;
  bool syncReturn       (const posix::fd_t socket, const posix::error_t errcode) const noexcept;

  bool setReturn        (const posix::fd_t socket, const posix::error_t errcode, const std::string& config, const std::string& key) const noexcept;
  bool unsetReturn      (const posix::fd_t socket, const posix::error_t errcode, const std::string& config, const std::string& key) const noexcept;
  bool getReturn        (const posix::fd_t socket, const posix::error_t errcode, const std::string& config, const std::string& key,
                         const std::string& value, const std::vector<std::string>& children) const noexcept;

  void listConfigsCall  (posix::fd_t socket) noexcept;
  void syncCall         (posix::fd_t socket) noexcept;
  void setCall          (posix::fd_t socket, const std::string& config, const std::string& key, const std::string& value) noexcept;
  void unsetCall        (posix::fd_t socket, const std::string& config, const std::string& key) noexcept;
  void getCall          (posix::fd_t socket, const std::string& config, const std::string& key) noexcept;

  bool peerChooser      (posix::fd_t socket, const proccred_t& cred) noexcept;
  void removePeer       (posix::fd_t socket) noexcept;
  void receive          (posix::fd_t socket, vfifo buffer, posix::fd_t fd) noexcept;
  void request          (posix::fd_t socket, posix::sockaddr_t addr, proccred_t cred) noexcept;

  void fileUpdated(std::string filename, FileEvent::Flags_t flags) noexcept;
  void dirUpdated (std::string dirname , FileEvent::Flags_t flags) noexcept;

  struct configfile_t
  {
    std::unique_ptr<FileEvent> fevent;
    ConfigManip config;
  };
  std::unordered_map<pid_t, posix::fd_t> m_endpoints;
  std::unordered_map<std::string, configfile_t> m_configfiles;
  std::unique_ptr<FileEvent> m_dir;
};

inline bool DirectorConfigServer::listConfigsReturn(const posix::fd_t socket, const std::vector<std::string>& names) const noexcept
  { return write(socket, vfifo("RPC", "listConfigsReturn", names), posix::invalid_descriptor); }

inline bool DirectorConfigServer::syncReturn(const posix::fd_t socket, const posix::error_t errcode) const noexcept
  { return write(socket, vfifo("RPC", "syncReturn", errcode), posix::invalid_descriptor); }

inline bool DirectorConfigServer::valueSet(const posix::fd_t socket, const std::string& config, const std::string& key, const std::string& value) const noexcept
  { return write(socket, vfifo("RPC", "valueSet", config, key, value), posix::invalid_descriptor); }

inline bool DirectorConfigServer::valueUnset(const posix::fd_t socket, const std::string& config, const std::string& key) const noexcept
  { return write(socket, vfifo("RPC", "valueUnset", config, key), posix::invalid_descriptor); }

inline bool DirectorConfigServer::setReturn(const posix::fd_t socket, const posix::error_t errcode, const std::string& config, const std::string& key) const noexcept
  { return write(socket, vfifo("RPC", "setReturn", errcode, config, key), posix::invalid_descriptor); }

inline bool DirectorConfigServer::unsetReturn(const posix::fd_t socket, const posix::error_t errcode, const std::string& config, const std::string& key) const noexcept
  { return write(socket, vfifo("RPC", "unsetReturn", errcode, config, key), posix::invalid_descriptor); }

inline bool DirectorConfigServer::getReturn(const posix::fd_t socket, const posix::error_t errcode, const std::string& config, const std::string& key,
                                            const std::string& value, const std::vector<std::string>& children) const noexcept
  { return write(socket, vfifo("RPC", "getReturn", errcode, config, key, value, children), posix::invalid_descriptor); }

#endif
