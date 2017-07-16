#include "configserver.h"

// POSIX
#include <limits.h>

// PDTK
#include <cxxutils/syslogstream.h>


ConfigServer::ConfigServer(const char* const username, const char* const filename) noexcept
{
  m_path.assign("/mc/").append(username).append("/").append(filename);
  if(bind(m_path.c_str()))
    posix::syslog << posix::priority::info << "daemon bound to " << m_path << posix::eom;
  else
    posix::syslog << posix::priority::error << "unable to bind daemon to " << m_path << posix::eom;

  Object::connect(newPeerRequest  , this, &ConfigServer::request);
  Object::connect(newPeerMessage  , this, &ConfigServer::receive);
  Object::connect(disconnectedPeer, this, &ConfigServer::removePeer);
}

void ConfigServer::setCall(posix::fd_t socket, std::string& key, std::string& value) noexcept
{
  auto configfile = m_configfiles.find(socket);
  assert(configfile != m_configfiles.end());
  int errcode = posix::success_response;

  configfile->second.data.getNode(key)->value = value;
  setReturn(socket, errcode);
}

void ConfigServer::getCall(posix::fd_t socket, std::string& key) noexcept
{
  auto configfile = m_configfiles.find(socket);
  assert(configfile != m_configfiles.end());
  std::string value;
  int errcode = posix::success_response;

  auto node = configfile->second.data.findNode(key);
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
  auto node = configfile->second.data.findNode(key.substr(0, offset)); // look for parent node
  if(node == nullptr)
    errcode = posix::error_response;
  else
    node->values.erase(key.substr(offset + 1)); // erase child node if it exists

  unsetReturn(socket, errcode);
}

bool ConfigServer::peerChooser(posix::fd_t socket, const proccred_t& cred) noexcept
{
  auto endpoint = m_endpoints.find(cred.pid);
  if(endpoint == m_endpoints.end() || // if no connection exists OR
     !peerData(endpoint->second))     // if old connection is mysteriously gone (can this happen?)
  {
    // construct config file name
    char filename[PATH_MAX] = {0};
    const char* username = posix::getusername(cred.uid);

    if(username == nullptr) // if UID was invalid
      return false;
    if(snprintf(filename, PATH_MAX, "/etc/sxconfig/%s.conf", username) == posix::error_response) // I don't how this could fail
      return false;

    // read and watch config file

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

// GENERATED CODE BELOW

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
  std::string str;
  if(!(buffer >> str).hadError() && str == "RPC")
  {
    buffer >> str;
    switch(hash(str))
    {
      case "setCall"_hash:
      {
        struct { std::string key; std::string value; } val;
        buffer >> val.key >> val.value;
        if(!buffer.hadError())
          setCall(socket, val.key, val.value);
      }
      break;
      case "getCall"_hash:
      {
        struct { std::string key; } val;
        buffer >> val.key;
        if(!buffer.hadError())
          getCall(socket, val.key);
      }
      break;
      case "unsetCall"_hash:
      {
        struct { std::string key; } val;
        buffer >> val.key;
        if(!buffer.hadError())
          unsetCall(socket, val.key);
      }
      break;
    }
  }
}
