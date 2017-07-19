#include "configserver.h"

// POSIX
#include <fcntl.h>

// POSIX++
#include <cstdio>
#include <climits>

// PDTK
#include <cxxutils/syslogstream.h>
#include <specialized/procstat.h>


ConfigServer::ConfigServer(const char* const filename) noexcept
{
  char path[PATH_MAX] = { 0 };
  std::snprintf(path, PATH_MAX, "/mc/config/%s", filename);
  if(bind(path))
    posix::syslog << posix::priority::info << "daemon bound to " << path << posix::eom;
  else
    posix::syslog << posix::priority::error << "unable to bind daemon to " << path << posix::eom;

  Object::connect(newPeerRequest  , this, &ConfigServer::request);
  Object::connect(newPeerMessage  , this, &ConfigServer::receive);
  Object::connect(disconnectedPeer, this, &ConfigServer::removePeer);
}

void ConfigServer::setCall(posix::fd_t socket, std::string& key, std::string& value) noexcept
{
  auto configfile = m_configfiles.find(socket);
  assert(configfile != m_configfiles.end());
  int errcode = posix::success_response;

  configfile->second.config.getNode(key)->value = value;
  setReturn(socket, errcode);
}

void ConfigServer::getCall(posix::fd_t socket, std::string& key) noexcept
{
  auto configfile = m_configfiles.find(socket);
  assert(configfile != m_configfiles.end());
  std::string value;
  int errcode = posix::success_response;

  auto node = configfile->second.config.findNode(key);
  if(node == nullptr)
    errcode = posix::error(std::errc::invalid_argument);
  else
    value = node->value;
  getReturn(socket, errcode, value);
}

void ConfigServer::unsetCall(posix::fd_t socket, std::string& key) noexcept
{
  auto configfile = m_configfiles.find(socket);
  assert(configfile != m_configfiles.end());
  int errcode = posix::success_response;

  posix::size_t offset = key.find_last_of('/');
  auto node = configfile->second.config.findNode(key.substr(0, offset)); // look for parent node
  if(node == nullptr)
    errcode = posix::error_response;
  else
    node->values.erase(key.substr(offset + 1)); // erase child node if it exists

  unsetReturn(socket, errcode);
}

bool ConfigServer::peerChooser(posix::fd_t socket, const proccred_t& cred) noexcept
{
  if(!posix::useringroup("config", posix::getusername(cred.uid)))
    return false;

  process_state_t state;
  if(::procstat(cred.pid, &state) == posix::error_response) // get state information about the connecting process
    return false; // unable to get state

  auto endpoint = m_endpoints.find(cred.pid);
  if(endpoint == m_endpoints.end() || // if no connection exists OR
     !peerData(endpoint->second))     // if old connection is mysteriously gone (can this happen?)
  {
    // construct config filename
    char path[PATH_MAX] = { 0 };

    if(snprintf(path, PATH_MAX, "/etc/sxconfig/%s.conf", state.name.c_str()) == posix::error_response) // I don't how this could fail
      return false; // unable to build config filename

    ::chown(path, ::getuid(), ::getgid());

    std::string buffer;
    std::FILE* file = std::fopen(path, "rb");

    if(file == nullptr)
    {
      posix::syslog << "unable to open file: " << path << " : " << std::strerror(errno) << posix::eom;
      return false;
    }

    buffer.resize(std::ftell(file), '\n');
    if(buffer.size())
    {
      std::rewind(file);
      std::fread(const_cast<char*>(buffer.data()), sizeof(std::string::value_type), buffer.size(), file);
    }
    std::fclose(file);
    ::chown(path, cred.uid, ::getegid());

    m_configfiles[socket].config.write(buffer);

    m_endpoints[cred.pid] = socket; // insert or assign new value
    return true;
  }
  return false; // reject multiple connections from one endpoint
}

void ConfigServer::removePeer(posix::fd_t socket) noexcept
{
  auto configfile = m_configfiles.find(socket);
  assert(configfile != m_configfiles.end());

  Object::disconnect(configfile->second.fd);
  m_configfiles.erase(configfile);

  for(auto endpoint : m_endpoints)
    if(socket == endpoint.second)
    { m_endpoints.erase(endpoint.first); break; }
}

void ConfigServer::request(posix::fd_t socket, posix::sockaddr_t addr, proccred_t cred) noexcept
{
  (void)addr;
  if(peerChooser(socket, cred))
    acceptPeerRequest(socket);
  else
    rejectPeerRequest(socket);
}

void ConfigServer::receive(posix::fd_t socket, vfifo buffer, posix::fd_t fd) noexcept
{
  (void)fd;
  std::string key, value;
  if(!(buffer >> value).hadError() && value == "RPC")
  {
    buffer >> value;
    switch(hash(value))
    {
      case "setCall"_hash:
        buffer >> key >> value;
        if(!buffer.hadError())
          setCall(socket, key, value);
        break;
      case "getCall"_hash:
        buffer >> key;
        if(!buffer.hadError())
          getCall(socket, key);
        break;
      case "unsetCall"_hash:
        buffer >> key;
        if(!buffer.hadError())
          unsetCall(socket, key);
        break;
    }
  }
}
