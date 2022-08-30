#include "RateHetParameter.hpp"
#include "BoundsChecker.hpp"


void RateHetParameter::applyParameter(TreeAln& traln, const ParameterContent &content)
{
  for(auto &m : _partitions)
    {
      traln.setAlpha(content.values[0], m); 
    }
}
 

ParameterContent RateHetParameter::extractParameter(const TreeAln &traln )  const
{
  ParameterContent result; 
  result.values = {traln.getAlpha(_partitions[0])} ;
  return result;  
}   


void RateHetParameter::printSample(std::ostream& fileHandle, const TreeAln &traln) const 
{
  auto content = extractParameter(traln); 
  // TODO numeric limits
  fileHandle << content.values[0] ; 
}

 
void RateHetParameter::printAllComponentNames(std::ostream &fileHandle, const TreeAln &traln) const 
{
  fileHandle << "alpha{"  ;
  bool isFirst = true; 
  for(auto &p : _partitions) 
    {
      fileHandle << (isFirst ? "" : "," ) << p ; 
      isFirst = false; 
    }
  fileHandle<< "}" ; 
} 




void RateHetParameter::verifyContent(const TreeAln&traln, const ParameterContent &content) const  
{
  if(content.values.size() > 1  || not BoundsChecker::checkAlpha(content.values[0]) ) 
    {
      tout << "Wrong content " << content << " for parameter "  << this << ". Did you mis-specify a fixed prior or are your input values to extreme?" << std::endl; 
      assert(0); 
    }
  
}
