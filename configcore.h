#ifndef CONFIGCORE_H
#define CONFIGCORE_H

// PUT
#include <put/object.h>
#include <put/cxxutils/configmanip.h>

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
