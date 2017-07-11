#include "configserver.h"

// POSIX
#include <limits.h>

// PDTK
#include <cxxutils/syslogstream.h>

ConfigServerInterface::ConfigServerInterface(void) noexcept
{
  Object::connect(newPeerRequest, this, &ConfigServerInterface::request);
  Object::connect(newPeerMessage, this, &ConfigServerInterface::receive);
}

void ConfigServerInterface::request(posix::fd_t socket, posix::sockaddr_t addr, proccred_t cred) noexcept
{
  (void)addr;
  if(peerChooser(socket, cred))
    acceptPeerRequest(socket);
  else
    rejectPeerRequest(socket);
}

void ConfigServerInterface::receive(posix::fd_t socket, vfifo buffer, posix::fd_t fd) noexcept
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
          Object::enqueue(setCall, socket, val.key, val.value);
      }
      break;
      case "getCall"_hash:
      {
        struct { std::string key; } val;
        buffer >> val.key;
        if(!buffer.hadError())
          Object::enqueue(getCall, socket, val.key);
      }
      break;
      case "getAllCall"_hash:
      {
        Object::enqueue(getAllCall, socket);
      }
      break;
    }
  }
}

ConfigServer::ConfigServer(const char* const username, const char* const filename) noexcept
{
  m_path.assign("/mc/").append(username).append("/").append(filename);
  if(bind(m_path.c_str()))
    posix::syslog << posix::priority::info << "daemon bound to " << m_path << posix::eom;
  else
    posix::syslog << posix::priority::error << "unable to bind daemon to " << m_path << posix::eom;

  Object::connect(disconnectedPeer, this, &ConfigServer::removePeer);
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

void ConfigServer::set(posix::fd_t socket, std::string key, std::string value) noexcept
{
  int errcode = posix::success_response;


  setReturn(socket, errcode);
}

void ConfigServer::get(posix::fd_t socket, std::string key) noexcept
{
  std::string value;


  getReturn(socket, value);
}

void ConfigServer::getAll(posix::fd_t socket) noexcept
{
  std::unordered_map<std::string, std::string> values;


  getAllReturn(socket, values);
}

void ConfigServer::removePeer(posix::fd_t socket) noexcept
{
  auto configfile = m_configfiles.find(socket);
  if(configfile != m_configfiles.end())
  {
    Object::disconnect(configfile->second.fd);
    m_configfiles.erase(configfile);
  }

  for(auto endpoint : m_endpoints)
    if(socket == endpoint.second)
    { m_endpoints.erase(endpoint.first); break; }
}
