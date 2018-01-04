#include "configserver.h"

// POSIX
#include <fcntl.h>

// POSIX++
#include <cstdio>
#include <climits>

// STL
#include <memory>
#include <algorithm>

// PDTK
#include <cxxutils/syslogstream.h>
#include <specialized/procstat.h>

#ifndef CONFIG_CONFIG_PATH
#define CONFIG_CONFIG_PATH    "/etc/config"
#endif

#ifndef CONFIG_GROUPNAME
#define CONFIG_GROUPNAME      "config"
#endif

static const char* configfilename(const char* base)
{
  // construct config filename
  static char name[PATH_MAX];
  std::memset(name, 0, PATH_MAX);
  if(std::snprintf(name, PATH_MAX, "%s/%s.conf", CONFIG_CONFIG_PATH, base) == posix::error_response) // I don't how this could fail
    return nullptr; // unable to build config filename
  return name;
}

static bool readconfig(const char* base, std::string& buffer)
{
  const char* name = configfilename(base);
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
  return true;
}


ConfigServer::ConfigServer(void) noexcept
{
  Object::connect(newPeerRequest  , this, &ConfigServer::request);
  Object::connect(newPeerMessage  , this, &ConfigServer::receive);
  Object::connect(disconnectedPeer, this, &ConfigServer::removePeer);
}

ConfigServer::~ConfigServer(void) noexcept
{
}

void ConfigServer::setCall(posix::fd_t socket, const std::string& key, const std::string& value) noexcept
{
  posix::error_t errcode = posix::success_response;
  auto configfile = m_configfiles.find(socket);
  if(configfile == m_configfiles.end())
    errcode = posix::error_t(std::errc::io_error); // not a valid key!
  else
    configfile->second.config.getNode(key)->value = value;
  setReturn(socket, errcode);
}

void ConfigServer::getCall(posix::fd_t socket, const std::string& key) noexcept
{
  posix::error_t errcode = posix::success_response;
  std::list<std::string> children;
  std::string value;

  auto configfile = m_configfiles.find(socket);

  if(configfile != m_configfiles.end())
    errcode = posix::error_t(std::errc::io_error); // no config file for socket
  else
  {
    auto node = configfile->second.config.findNode(key);
    if(node == nullptr)
      errcode = posix::error_t(std::errc::invalid_argument); // node doesn't exist
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

void ConfigServer::unsetCall(posix::fd_t socket, const std::string& key) noexcept
{
  posix::error_t errcode = posix::success_response;
  auto configfile = m_configfiles.find(socket);

  if(configfile == m_configfiles.end())
    errcode = posix::error_t(std::errc::io_error); // no such config file!
  else
  {
    posix::size_t offset = key.find_last_of('/');
    auto node = configfile->second.config.findNode(key.substr(0, offset)); // look for parent node
    if(node == nullptr)
      errcode = posix::error_t(std::errc::invalid_argument);
    else
      node->children.erase(key.substr(offset + 1)); // erase child node if it exists
  }

  unsetReturn(socket, errcode);
}

void ConfigServer::fullUpdateCall(posix::fd_t socket) noexcept
{
  const auto& confpair = m_configfiles.find(socket); // find parsed config file
  if(confpair != m_configfiles.end())
  {
    std::list<std::pair<std::string, std::string>> data;
    confpair->second.config.exportKeyPairs(data); // export config data
    for(const auto& pair : data) // for each key pair
      valueUpdate(socket, pair.first, pair.second); // send value
  }
  fullUpdateReturn(socket, posix::success_response); // send call response
}

bool ConfigServer::peerChooser(posix::fd_t socket, const proccred_t& cred) noexcept
{
  if(!posix::useringroup(CONFIG_GROUPNAME, posix::getusername(cred.uid)))
    return false;

  process_state_t state;
  if(::procstat(cred.pid, &state) == posix::error_response) // get state information about the connecting process
    return false; // unable to get state

  auto endpoint = m_endpoints.find(cred.pid);
  if(endpoint == m_endpoints.end() || // if no connection exists OR
     !peerData(endpoint->second))     // if old connection is mysteriously gone (can this happen?)
  {
    std::string buffer;
    const char* filename = configfilename(state.name.c_str());

    posix::chown(filename, ::getuid(), cred.gid); // reset ownership
    posix::chmod(filename, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP); // reset permissions

    readconfig(filename, buffer);

    auto& conffile = m_configfiles[socket];
    conffile.fevent = std::make_unique<FileEvent>(filename, FileEvent::WriteEvent); // monitor file for write event
    conffile.config.clear(); // erase any existing data
    conffile.config.importText(buffer);
    Object::connect(conffile.fevent->activated, this, &ConfigServer::fileUpdated);

    m_endpoints[cred.pid] = socket; // insert or assign new value
    return true;
  }
  return false; // reject multiple connections from one endpoint
}

void ConfigServer::fileUpdated(const char* filename, FileEvent::Flags_t flags) noexcept
{
  posix::fd_t socket = posix::invalid_descriptor;
  if(flags.WriteEvent)
    for(auto& confpair : m_configfiles)
      if(!std::strcmp(confpair.second.fevent->file(), filename))
      {
        std::string tmp_buffer;
        std::list<std::pair<std::string, std::string>> old_config, new_values, removed_values;

        confpair.second.config.exportKeyPairs(old_config); // export data
        confpair.second.config.clear(); // wipe config

        socket = confpair.first;
        removed_values = old_config;

        if(readconfig(filename, tmp_buffer) &&
           confpair.second.config.importText(tmp_buffer))
        {
          confpair.second.config.exportKeyPairs(new_values);

          for(auto& pair : new_values)
            std::remove_if(removed_values.begin(), removed_values.end(), // remove matching keys (leaving deleted key/value pairs)
                           [pair](const auto& value) { return value.first == pair.first; });

          for(auto& pair : old_config)
            new_values.remove(pair); // remove identical values (leaving only new key/value pairs)

          for(auto& pair : removed_values)
            valueUnset(socket, pair.first); // invoke value deletion

          for(auto& pair : new_values)
            valueUpdate(socket, pair.first, pair.second); // invoke value update
        }
        else
          posix::syslog << posix::priority::warning << "Failed to read/parse config file: " << filename << posix::eom;
      }
}

void ConfigServer::removePeer(posix::fd_t socket) noexcept
{
  auto configfile = m_configfiles.find(socket);
  if(configfile != m_configfiles.end())
  {
    m_configfiles.erase(configfile);

    for(auto endpoint : m_endpoints)
      if(socket == endpoint.second)
      { m_endpoints.erase(endpoint.first); break; }
  }
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
  if(!(buffer >> value).hadError() && value == "RPC" &&
     !(buffer >> value).hadError())
  {
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
