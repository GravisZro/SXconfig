#include <iostream>
#include <cassert>
//#include <process.h>
#include <cxxutils/configparser.h>

int main(int argc, char *argv[]) noexcept
{
  (void)argc;
  (void)argv;

  std::string data = "[config]\nkey=original value\n # ORLY?\nKey = Value; yeah rly\n/config/key=\"overwritten\"; stuff\n";
  ConfigParser conf;
  assert(conf.parse(data));

  auto node = conf.findNode("/config/key");
  if(node != nullptr)
    std::cout << node->value << std::endl;
  else
    std::cout << "not found!" << std::endl;
  return 0;
}
