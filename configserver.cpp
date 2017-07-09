#include "configserver.h"

// PDTK
#include <cxxutils/vfifo.h>
#include <cxxutils/hashing.h>
#include <cxxutils/configmanip.h>
#include <cxxutils/syslogstream.h>


ConfigServer::ConfigServer(const char* const username, const char* const filename) noexcept
  : ServerSocket(EDomain::local, EType::seqpacket, EProtocol::unspec, 0)
{
  m_path.assign("/mc/").append(username).append("/").append(filename);
  if(bind(m_path.c_str()))
    posix::syslog << posix::priority::info << "daemon bound to " << m_path << posix::eom;
  else
    posix::syslog << posix::priority::error << "unable to bind daemon to " << m_path << posix::eom;

  Object::connect(newPeerRequest  , this, &ConfigServer::allowDeny);
  Object::connect(disconnectedPeer, this, &ConfigServer::removeEndpoint);
  Object::connect(newPeerMessage  , this, &ConfigServer::receive);
}


void ConfigServer::allowDeny(posix::fd_t fd, posix::sockaddr_t addr, proccred_t cred) noexcept
{
  (void)addr;
  auto endpoint = m_endpoints.find(cred.pid);
  if(endpoint == m_endpoints.end() ||           // if no connection exists OR
     !ServerSocket::peerData(endpoint->second)) // if old connection is mysteriously gone
  {
    m_endpoints.at(cred.pid) = fd; // insert or assign new value
    acceptPeerRequest(fd);
  }
  else // reject multiple connections from one endpoint
    rejectPeerRequest(fd);
}

void ConfigServer::removeEndpoint(posix::fd_t fd) noexcept
{
  for(auto endpoint : m_endpoints)
    if(fd == endpoint.second)
    { m_endpoints.erase(endpoint.first); break; }
}

void ConfigServer::receive(posix::fd_t server, vfifo buffer, posix::fd_t fd) noexcept
{
  (void)server;
  (void)fd;
  std::string str;
  if(!(buffer >> str).hadError() && str == "RPC")
  {
    buffer >> str;
    switch(hash(str))
    {
      case "setValueCall"_hash:
      {
        struct { std::string key; std::string value; } val;
        buffer >> val.key >> val.value;
        if(!buffer.hadError())
          Object::enqueue(setValueCall, val.key, val.value);
      }
      break;

      case "getValueCall"_hash:
      {
        struct { std::string key; } val;
        buffer >> val.key;
        if(!buffer.hadError())
          Object::enqueue(getValueCall, val.key);
      }
      break;

      case "getAllCall"_hash:
        Object::enqueue(getAllCall);
        break;
    }
  }
}