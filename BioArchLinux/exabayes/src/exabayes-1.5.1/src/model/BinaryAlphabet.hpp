#ifndef BINARYALPHABET_H
#define BINARYALPHABET_H


#include "AbstractAlphabet.hpp"


// this should be simplified...not everything needs to be class 


class BinaryAlphabet
{
public:
  std::vector<std::string> getStates()
  {
    return { "0", "1" };
  }
};




#endif /* BINARYALPHABET_H */
