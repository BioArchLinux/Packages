#include "AbstractAlphabet.hpp"

#include <iostream>
#include <cassert>


Alphabet getTypeFromString(std::string str)
{
  auto type = Alphabet::DNA; 
  if( str.compare("DNA") == 0  )
    type = Alphabet::DNA; 
  else if(str.compare("PROT") == 0 )
    type = Alphabet::PROTEIN; 
  else if(str.compare("BIN") == 0 )
    type = Alphabet::BINARY; 
  else 
    {
      std::cout << "unknown model >" << str << "<" << std::endl;
      assert(0); 
    }
      
  return type; 
}
 
std::string getStringFromType(Alphabet type)
{
  switch(type)
    {
    case Alphabet::DNA: 
      return "DNA"; 
    case Alphabet::PROTEIN: 
      return "PROT" ; 
    case Alphabet::BINARY:
      return "BIN"; 
    default: 
      assert(0); 
    }

  return "BIN"; 
}


