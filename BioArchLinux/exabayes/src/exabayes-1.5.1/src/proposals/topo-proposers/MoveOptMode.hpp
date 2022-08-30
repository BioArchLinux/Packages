#ifndef MOVEOPTMODE_H
#define MOVEOPTMODE_H

#include <iosfwd>

enum class MoveOptMode : int 
{
  NONE = 0 , 
    ONLY_SWITCHING = 1, 
    ALL_INTERNAL = 2, 
    ALL_IN_MOVE = 3, 
    ALL_SURROUNDING = 4  
    }; 	 

std::ostream& operator<<(std::ostream& s, const MoveOptMode& c) ;

#endif /* MOVEOPTMODE_H */

