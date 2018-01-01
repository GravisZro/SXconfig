#ifndef CONFIGCORE_H
#define CONFIGCORE_H

// PDTK
#include <object.h>
#include <cxxutils/configmanip.h>

// project
#include "configserver.h"
#include "executorconfigserver.h"

class ConfigCore : public Object
{
public:
  ConfigCore(void);

private:
  ConfigServer m_config_server;
  ExecutorConfigServer m_executor_server;
};

#endif // CONFIGCORE_H
