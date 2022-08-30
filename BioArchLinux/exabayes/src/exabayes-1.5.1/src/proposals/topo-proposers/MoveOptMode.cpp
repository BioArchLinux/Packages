#include "MoveOptMode.hpp"

#include <iostream>
#include <cassert>

std::ostream& operator<<(std::ostream& s, const MoveOptMode& c) 
{
  switch(c)
    {
    case MoveOptMode::NONE : 
      s << "NONE" ; 
      break; 
    case MoveOptMode::ONLY_SWITCHING: 
      s << "ONLY_SWITCHING" ; 
      break; 
    case MoveOptMode::ALL_INTERNAL: 
      s << "ALL_INTERNAL" ; 
      break; 
    case MoveOptMode::ALL_IN_MOVE: 
      s << "ALL_IN_MOVE" ; 
      break; 
    case MoveOptMode::ALL_SURROUNDING  : 
      s << "ALL_SURROUNDING" ; 
      break; 
    default : 
      assert(0); 
    }
  return s;
}
