#ifndef DNA_ALPHABET
#define DNA_ALPHABET

#include "AbstractAlphabet.hpp"

class DnaAlphabet : public AbstractAlphabet
{
public : 
  std::vector<std::string> getStates()  
  {
    return { "A" , "C", "G", "T"} ; 
  } 
}; 



#endif
