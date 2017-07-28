#include "configserver.h"

// POSIX
#include <fcntl.h>

// POSIX++
#include <cstdio>
#include <climits>
#include <cassert>

// PDTK
#include <cxxutils/syslogstream.h>
#include <specialized/procstat.h>

#define FILENAME_PATTERN "/etc/sxconfig/%s.conf"

ConfigServer::ConfigServer(void) noexcept
{
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
  std::list<std::string> children;
  std::string value;
  int errcode = posix::success_response;

  auto configfile = m_configfiles.find(socket);

  if(configfile != m_configfiles.end())
    errcode = int(std::errc::io_error); // no config file for socket
  else
  {
    auto node = configfile->second.config.findNode(key);
    if(node == nullptr)
      errcode = int(std::errc::invalid_argument); // node doesn't exist
    else
    {
      switch(node->type)
      {
        case node_t::type_e::array:
        case node_t::type_e::multisection:
        case node_t::type_e::section:
          for(const auto& child : node->children)
            children.push_back(child.first);
        case node_t::type_e::invalid:
        case node_t::type_e::value:
        case node_t::type_e::string:
          value = node->value;
      }
    }
  }
  getReturn(socket, errcode, value, children);
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
    node->children.erase(key.substr(offset + 1)); // erase child node if it exists

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
    char name[PATH_MAX] = { 0 };

    if(snprintf(name, PATH_MAX, FILENAME_PATTERN, state.name.c_str()) == posix::error_response) // I don't how this could fail
      return false; // unable to build config filename

    std::string buffer;
    std::FILE* file = std::fopen(name, "a+b");

    if(file == nullptr)
    {
      posix::syslog << "unable to open file: " << name << " : " << std::strerror(errno) << posix::eom;
      return false;
    }

    buffer.resize(std::ftell(file), '\n');
    if(buffer.size())
    {
      std::rewind(file);
      std::fread(const_cast<char*>(buffer.data()), sizeof(std::string::value_type), buffer.size(), file);
    }
    std::fclose(file);

    posix::chown(name, ::getuid(), cred.gid); // reset ownership
    posix::chmod(name, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP); // reset permissions

    m_configfiles[socket].fd = EventBackend::watch(name, EventFlags::FileMod);
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
