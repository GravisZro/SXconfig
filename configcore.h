#ifndef CONFIGCORE_H
#define CONFIGCORE_H

// PDTK
#include <object.h>
#include <cxxutils/configmanip.h>

// project
#include "configserver.h"
#include "directorconfigserver.h"

class ConfigCore : public Object
{
public:
  ConfigCore(void);

private:
  ConfigServer m_config_server;
  DirectorConfigServer m_director_server;
};

#endif // CONFIGCORE_H
