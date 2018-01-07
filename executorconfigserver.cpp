#include "executorconfigserver.h"

// POSIX
#include <fcntl.h>
#include <dirent.h>

// POSIX++
#include <cstdio>
#include <climits>

// STL
#include <memory>
#include <algorithm>

// PDTK
#include <cxxutils/syslogstream.h>
#include <specialized/procstat.h>


#ifndef EXECUTOR_CONFIG_PATH
#define EXECUTOR_CONFIG_PATH  "/etc/executor"
#endif

#ifndef EXECUTOR_USERNAME
#define EXECUTOR_USERNAME     "executor"
#endif

static const char* executor_configfilename(const char* base)
{
  // construct config filename
  static char name[PATH_MAX];
  std::memset(name, 0, PATH_MAX);
  if(std::snprintf(name, PATH_MAX, "%s/%s.conf", EXECUTOR_CONFIG_PATH, base) == posix::error_response) // I don't how this could fail
    return nullptr; // unable to build config filename
  return name;
}

static bool readconfig(const char* name, std::string& buffer)
{
  std::FILE* file = std::fopen(name, "a+b");

  if(file == nullptr)
  {
    posix::syslog << posix::priority::warning << "Unable to open file: " << name << " : " << std::strerror(errno) << posix::eom;
    return false;
  }

  buffer.clear();
  buffer.resize(posix::size_t(std::ftell(file)), '\n');
  if(buffer.size())
  {
    std::rewind(file);
    std::fread(const_cast<char*>(buffer.data()), sizeof(std::string::value_type), buffer.size(), file);
  }
  std::fclose(file);
  return true;
}


ExecutorConfigServer::ExecutorConfigServer(void) noexcept
{
  std::string buffer;
  DIR* dir = ::opendir(EXECUTOR_CONFIG_PATH);
  dirent* entry = nullptr;
  char base[NAME_MAX];
  if(dir != nullptr)
  {
    while((entry = ::readdir(dir)) != nullptr)
    {
      std::memset(base, 0, NAME_MAX);
      if(std::sscanf(entry->d_name, "%s.conf", base) == posix::success_response && // if filename base extracted properly AND
         readconfig(executor_configfilename(base), buffer)) // able to read config file
      {
        auto& conffile = m_configfiles[base];
        conffile.fevent = std::make_unique<FileEvent>(entry->d_name, FileEvent::WriteEvent);
        conffile.config.clear(); // erase any existing data
        conffile.config.importText(buffer);
        Object::connect(conffile.fevent->activated, this, &ExecutorConfigServer::fileUpdated);
      }
    }
    ::closedir(dir);
  }
/*
  m_dir = EventBackend::watch(EXECUTOR_CONFIG_PATH, EventFlags::DirEvent);
  if(m_dir > 0)
    Object::connect(m_dir, this, &ExecutorConfigServer::dirUpdated);
*/
  Object::connect(newPeerRequest  , this, &ExecutorConfigServer::request);
  Object::connect(newPeerMessage  , this, &ExecutorConfigServer::receive);
  Object::connect(disconnectedPeer, this, &ExecutorConfigServer::removePeer);
}

ExecutorConfigServer::~ExecutorConfigServer(void) noexcept
{
}

void ExecutorConfigServer::fileUpdated(const char* filename, FileEvent::Flags_t flags) noexcept
{
  if(flags.WriteEvent)
    for(auto& confpair : m_configfiles)
      if(!std::strcmp(confpair.second.fevent->file(), filename))
      {
        std::string tmp_buffer;
        std::unordered_map<std::string, std::string> old_config, new_config;

        confpair.second.config.exportKeyPairs(old_config); // export data
        confpair.second.config.clear(); // wipe config

        if(readconfig(filename, tmp_buffer) &&
           confpair.second.config.importText(tmp_buffer))
        {
          confpair.second.config.exportKeyPairs(new_config);

          for(auto& old_pair : old_config) // find removed and updated values
          {
            auto iter = new_config.find(old_pair.first);
            if(iter == new_config.end())
              for(auto& endpoint : m_endpoints)
                valueUnset(endpoint.second, confpair.first, old_pair.first); // invoke value deletion
            else if(iter->second != old_pair.second)
              for(auto& endpoint : m_endpoints)
                valueUpdate(endpoint.second, confpair.first, iter->first, iter->second); // invoke value update
          }

          for(auto& new_pair : new_config) // find completely new values
            if(old_config.find(new_pair.first) == old_config.end()) // if old config doesn't have a new config key
              for(auto& endpoint : m_endpoints)
                valueUpdate(endpoint.second, confpair.first, new_pair.first, new_pair.second); // invoke value update
        }
        else
          posix::syslog << posix::priority::warning << "Failed to read/parse config file: " << filename << posix::eom;
      }
}

void ExecutorConfigServer::dirUpdated(const char* dirname, FileEvent::Flags_t flags) noexcept
{
  std::printf("dir updated: %s - 0x%02x\n", dirname, uint8_t(flags));
}

void ExecutorConfigServer::listConfigsCall(posix::fd_t socket) noexcept
{
  std::vector<std::string> names;
  for(const auto& confpair : m_configfiles)
    names.push_back(confpair.first);
  listConfigsReturn(socket, names);
}

void ExecutorConfigServer::fullUpdateCall(posix::fd_t socket) noexcept
{
  for(const auto& confpair : m_configfiles) // for each parsed config file
  {
    std::unordered_map<std::string, std::string> data;
    confpair.second.config.exportKeyPairs(data); // export config data
    for(auto& pair : data) // for each key pair
      valueUpdate(socket, confpair.first, pair.first, pair.second); // send value
  }
  fullUpdateReturn(socket, posix::success_response); // send call response
}

void ExecutorConfigServer::setCall(posix::fd_t socket, const std::string& config, const std::string& key, const std::string& value) noexcept
{
  posix::error_t errcode = posix::success_response;

  auto configfile = m_configfiles.find(config);
  if(configfile == m_configfiles.end())
    errcode = posix::error_t(std::errc::invalid_argument); // not a valid config file name
  else
    configfile->second.config.getNode(key)->value = value;

  setReturn(socket, errcode, config, key);
}

void ExecutorConfigServer::getCall(posix::fd_t socket, const std::string& config, const std::string& key) noexcept
{
  std::vector<std::string> children;
  std::string value;
  posix::error_t errcode = posix::success_response;

  auto configfile = m_configfiles.find(config); // look up config by name
  if(configfile == m_configfiles.end()) // if not found
    errcode = posix::error_t(std::errc::invalid_argument); // not a valid config file name
  else
  {
    auto node = configfile->second.config.findNode(key); // find node in config file
    if(node == nullptr)
      errcode = posix::error_t(std::errc::invalid_argument); // doesn't exist
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
  getReturn(socket, errcode, config, key, value, children);
}

void ExecutorConfigServer::unsetCall(posix::fd_t socket, const std::string& config, const std::string& key) noexcept
{
  posix::error_t errcode = posix::success_response;
  auto configfile = m_configfiles.find(config); // look up config by name

  if(configfile == m_configfiles.end())
    errcode = posix::error_t(std::errc::io_error); // no such config file!
  else if(!configfile->second.config.deleteNode(key))
    errcode = posix::error_t(std::errc::invalid_argument); // doesn't exist

  unsetReturn(socket, errcode, config, key);
}

bool ExecutorConfigServer::peerChooser(posix::fd_t socket, const proccred_t& cred) noexcept
{
  if(std::strcmp(EXECUTOR_USERNAME, posix::getusername(cred.uid))) // username must be "executor"
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

void ExecutorConfigServer::removePeer(posix::fd_t socket) noexcept
{
  for(auto endpoint : m_endpoints)
    if(socket == endpoint.second)
      { m_endpoints.erase(endpoint.first); break; }
}

void ExecutorConfigServer::request(posix::fd_t socket, posix::sockaddr_t addr, proccred_t cred) noexcept
{
  (void)addr;
  if(peerChooser(socket, cred))
    acceptPeerRequest(socket);
  else
    rejectPeerRequest(socket);
}

void ExecutorConfigServer::receive(posix::fd_t socket, vfifo buffer, posix::fd_t fd) noexcept
{
  (void)fd;
  std::string config, key, value;
  if(!(buffer >> value).hadError() && value == "RPC" &&
     !(buffer >> value).hadError())
  {
    switch(hash(value))
    {
      case "listConfigsCall"_hash:
        listConfigsCall(socket);
        break;
      case "fullUpdateCall"_hash:
        fullUpdateCall(socket);
        break;
      case "setCall"_hash:
        buffer >> config >> key >> value;
        if(!buffer.hadError())
          setCall(socket, config, key, value);
        break;
      case "getCall"_hash:
        buffer >> config >> key;
        if(!buffer.hadError())
          getCall(socket, config, key);
        break;
      case "unsetCall"_hash:
        buffer >> config >> key;
        if(!buffer.hadError())
          unsetCall(socket, config, key);
        break;
    }
  }
}
