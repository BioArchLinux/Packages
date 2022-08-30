#ifndef ABSTRACT_ALPHABET
#define ABSTRACT_ALPHABET 

#include <vector> 
#include <string>

typedef unsigned int nat; 


enum class Alphabet  : int 
  {
    BINARY = 1 , 
      DNA = 2 , 
      PROTEIN  = 3 
  }; 


Alphabet getTypeFromString(std::string str); 
std::string getStringFromType(Alphabet type); 




class AbstractAlphabet
{
public: 
  virtual ~AbstractAlphabet(){}
  virtual std::vector<std::string> getStates() = 0 ; 
  std::vector<std::string> getCombinations() 
  {
    auto result = std::vector<std::string>{}; 
    auto names = getStates();

    for(nat i = 0; i < names.size() ; ++i)
      for(nat j = i+1 ; j < names.size()  ; ++j)
	result.push_back(names[i] + "<->"  + names[j]);
  
    return result; 
  } 

}; 

#endif
