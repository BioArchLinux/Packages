#ifndef AMINO_ACID_ALPHABET
#define AMINO_ACID_ALPHABET

#include "AbstractAlphabet.hpp"

class AminoAcidAlphabet : public AbstractAlphabet
{
public : 
  virtual std::vector<std::string> getStates()  
  {
    return {
      "A","R","N","D","C","Q","E","G","H",
	"I","L","K","M","F","P","S","T","W","Y","V"
	}; 
  }

}; 

#endif
