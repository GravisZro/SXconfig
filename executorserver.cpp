#include "executorserver.h"

// POSIX
#include <fcntl.h>

// POSIX++
#include <cstdio>
#include <climits>

// PDTK
#include <cxxutils/syslogstream.h>
#include <specialized/procstat.h>

#define FILENAME_PATTERN "/etc/sxexecutor/%s.conf"
#define REQUIRED_USERNAME "executor"

ExecutorServer::ExecutorServer(void) noexcept
{
  Object::connect(newPeerRequest  , this, &ExecutorServer::request);
  Object::connect(newPeerMessage  , this, &ExecutorServer::receive);
  Object::connect(disconnectedPeer, this, &ExecutorServer::removePeer);
}

void ExecutorServer::setCall(posix::fd_t socket, std::string& key, std::string& value) noexcept
{
  int errcode = posix::success_response;
  std::string::size_type slashpos = key.find('/');
  if(slashpos == std::string::npos || // if not found OR
     !slashpos || // if first character OR
     slashpos == key.length()) // if last character
    errcode = int(std::errc::invalid_argument); // not a valid key!
  else
  {
    auto configfile = m_configfiles.find(key.substr(0, slashpos - 1));
    if(configfile == m_configfiles.end())
      errcode = int(std::errc::invalid_argument); // not a valid config file name
    else
      configfile->second.config.getNode(key.substr(slashpos, std::string::npos))->value = value;
  }
  setReturn(socket, errcode);
}

void ExecutorServer::getCall(posix::fd_t socket, std::string& key) noexcept
{
  std::string value;
  int errcode = posix::success_response;
  std::string::size_type slashpos = key.find('/');
  if(slashpos == std::string::npos || // if not found OR
     !slashpos || // if first character OR
     slashpos == key.length()) // if last character
    errcode = int(std::errc::invalid_argument); // not a valid key!
  else
  {
    auto configfile = m_configfiles.find(key.substr(0, slashpos - 1));
    if(configfile == m_configfiles.end())
      errcode = int(std::errc::invalid_argument); // not a valid config file name
    else
    {
      auto node = configfile->second.config.findNode(key);
      if(node == nullptr)
        errcode = int(std::errc::invalid_argument); // doesn't exist
      else
        value = node->value;
    }
  }
  getReturn(socket, errcode, value);
}

void ExecutorServer::unsetCall(posix::fd_t socket, std::string& key) noexcept
{
  int errcode = posix::success_response;
  std::string::size_type slashpos = key.find('/');
  if(slashpos == std::string::npos || // if not found OR
     !slashpos || // if first character OR
     slashpos == key.length()) // if last character
    errcode = int(std::errc::invalid_argument); // not a valid key!
  else
  {
    auto configfile = m_configfiles.find(key.substr(0, slashpos - 1));
    if(configfile == m_configfiles.end())
      errcode = int(std::errc::invalid_argument); // not a valid config file name
    else
    {
      std::string::size_type offset = key.rfind('/');
      auto node = configfile->second.config.findNode(key.substr(slashpos, offset - slashpos)); // look for parent node
      if(node == nullptr)
        errcode = int(std::errc::invalid_argument); // doesn't exist
      else
        node->values.erase(key.substr(offset + 1)); // erase child node if it exists
    }
  }
  unsetReturn(socket, errcode);
}

bool ExecutorServer::peerChooser(posix::fd_t socket, const proccred_t& cred) noexcept
{
  if(std::strcmp(REQUIRED_USERNAME, posix::getusername(cred.uid))) // username must be "executor"
    return false; // didn't match, reject connection

  auto endpoint = m_endpoints.find(cred.pid);
  if(endpoint == m_endpoints.end() || // if no connection exists OR
     !peerData(endpoint->second))     // if old connection is mysteriously gone (can this happen?)
  {
    m_endpoints[cred.pid] = socket; // insert or assign new value
    return true;
  }
  return false; // reject multiple connections from one endpoint
}

void ExecutorServer::removePeer(posix::fd_t socket) noexcept
{
  for(auto endpoint : m_endpoints)
    if(socket == endpoint.second)
    { m_endpoints.erase(endpoint.first); break; }
}

void ExecutorServer::request(posix::fd_t socket, posix::sockaddr_t addr, proccred_t cred) noexcept
{
  (void)addr;
  if(peerChooser(socket, cred))
    acceptPeerRequest(socket);
  else
    rejectPeerRequest(socket);
}

void ExecutorServer::receive(posix::fd_t socket, vfifo buffer, posix::fd_t fd) noexcept
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
      case "unsetCall"_hash:
        buffer >> key;
        if(!buffer.hadError())
          unsetCall(socket, key);
        break;
    }
  }
}


bool ExecutorServer::readconfig(const char* daemon)
{
  // construct config filename
  char name[PATH_MAX] = { 0 };

  if(snprintf(name, PATH_MAX, FILENAME_PATTERN, daemon) == posix::error_response) // I don't how this could fail
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

  posix::chown(name, ::getuid(), posix::getgroupid(daemon)); // reset ownership
  posix::chmod(name, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP); // reset permissions

  m_configfiles[name].fd = EventBackend::watch(name, EventFlags::FileMod);
  m_configfiles[name].config.write(buffer);
  return true;
}
