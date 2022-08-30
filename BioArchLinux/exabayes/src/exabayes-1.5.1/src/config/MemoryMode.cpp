#include "MemoryMode.hpp"
#include <cassert>


std::string toString(MemoryMode mem)
{
  auto result = std::string{};
  switch(mem)
    {
    case MemoryMode::RESTORE_ALL: 
      result = "restores all"; 
      break; 
    case MemoryMode::RESTORE_INNER_TIP : 
      result = "restores inner-tip"; 
      break; 
    case MemoryMode::RESTORE_INNER_INNER: 
      result = "restores inner-inner"; 
      break; 
    case MemoryMode::RESTORE_NONE: 
      result = "restores nothing"; 
      break; 
    default : 
      assert(0); 
    }
  return result ; 
}
