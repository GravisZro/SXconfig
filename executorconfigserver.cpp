#include "executorconfigserver.h"

// POSIX
#include <fcntl.h>
#include <dirent.h>

// POSIX++
#include <cstdio>
#include <climits>

// STL
#include <memory>

// PDTK
#include <cxxutils/syslogstream.h>
#include <specialized/procstat.h>


#ifndef EXECUTOR_CONFIG_PATH
#define EXECUTOR_CONFIG_PATH  "/etc/executor"
#endif

#ifndef EXECUTOR_USERNAME
#define EXECUTOR_USERNAME     "executor"
#endif

static const char* configfilename(const char* base)
{
  // construct config filename
  static char name[PATH_MAX];
  std::memset(name, 0, PATH_MAX);
  if(std::snprintf(name, PATH_MAX, "%s/%s.conf", EXECUTOR_CONFIG_PATH, base) == posix::error_response) // I don't how this could fail
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
         readconfig(base, buffer)) // able to read config file
      {
        std::printf("filename: %s\n", entry->d_name);
        auto& conffile = m_configfiles[base];
        conffile.fevent = std::make_unique<FileEvent>(entry->d_name, FileEvent::WriteEvent);
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
//  Object::disconnect(m_dir, EventFlags::DirEvent); // disconnect filesystem monitor
}

void ExecutorConfigServer::fileUpdated(const char* filename, FileEvent::Flags_t flags) noexcept
{
  std::printf("file updated: %s - 0x%02x\n", filename, uint8_t(flags));
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
    std::list<std::pair<std::string, std::string>> data;
    confpair.second.config.exportKeyPairs(data); // export config data
    for(auto& pair : data) // for each key pair
    {
      pair.first.insert(0, confpair.first); // prepend file path
      valueUpdate(socket, pair.first, pair.second); // send value
    }
  }
  fullUpdateReturn(socket, posix::success_response); // send call response
}

void ExecutorConfigServer::setCall(posix::fd_t socket, std::string& key, std::string& value) noexcept
{
  posix::error_t errcode = posix::success_response;
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

void ExecutorConfigServer::getCall(posix::fd_t socket, std::string& key) noexcept
{
  std::vector<std::string> children;
  std::string value;
  posix::error_t errcode = posix::success_response;
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
  }
  getReturn(socket, errcode, value, children);
}

void ExecutorConfigServer::unsetCall(posix::fd_t socket, std::string& key) noexcept
{
  posix::error_t errcode = posix::success_response;
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
        node->children.erase(key.substr(offset + 1)); // erase child node if it exists
    }
  }
  unsetReturn(socket, errcode);
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
  std::string key, value;
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
