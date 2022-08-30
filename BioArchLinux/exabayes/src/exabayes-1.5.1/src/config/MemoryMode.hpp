#ifndef _MEMORY_MODE_H
#define _MEMORY_MODE_H

#include <string>

enum class MemoryMode
{
  RESTORE_ALL = 0,
    RESTORE_INNER_TIP = 1 , 	// inner-inner + inner-tip  
    RESTORE_INNER_INNER = 2 , 	// only inner-inner 
    RESTORE_NONE = 3 
}; 

std::string toString(MemoryMode mem); 

#endif
