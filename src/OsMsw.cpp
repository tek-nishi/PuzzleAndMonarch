//
// OSごとの差異を吸収
//

#include "Defines.hpp"
#include <string>
#include "Os.hpp"


namespace ngs { namespace Os {

std::string lang()
{
  auto* l = setlocale(LC_ALL, "");

  DOUT << "locale:" << l << std::endl;

  return l;
}

} }
