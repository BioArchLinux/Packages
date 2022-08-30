#include "FrequencyParameter.hpp"
#include "BoundsChecker.hpp"
#include "DnaAlphabet.hpp"
#include "BinaryAlphabet.hpp"
#include "AminoAcidAlphabet.hpp"
#include "RateHelper.hpp"


void FrequencyParameter::applyParameter(TreeAln& traln, const ParameterContent &content)
{ 
  auto tmp = content.values; 
  
  RateHelper::convertToSum1(tmp); 
    
  // tout << MAX_SCI_PRECISION << "setting " << content.values << std::endl; 

  for(auto &m : _partitions)    
    traln.setFrequencies(tmp, m); 
}


ParameterContent FrequencyParameter::extractParameter(const TreeAln &traln )  const
{
  auto result = ParameterContent{}; 
  result.values = traln.getFrequencies(_partitions[0]); 

  // tout << MAX_SCI_PRECISION << "extracting "<< result.values << std::endl; 

  // RateHelper::convertToSum1(result.values); 

  return result; 
}   


void FrequencyParameter::printSample(std::ostream& fileHandle, const TreeAln &traln) const 
{
  auto content =  extractParameter(traln); 
  bool isFirst = true; 
  for(auto &v : content.values)
    {
      fileHandle << (isFirst ? "" : "\t") << v ; 
      isFirst = false; 
    }
} 

 
void FrequencyParameter::printAllComponentNames(std::ostream &fileHandle, const TreeAln &traln)  const   
{
  auto content = extractParameter(traln); 

  std::vector<std::string> names; 
  switch(content.values.size())
    {
    case 2: 
      names = BinaryAlphabet().getStates(); 
      break; 
    case 4:       
      names = DnaAlphabet().getStates() ; 
      break; 
    case 20: 
      names = AminoAcidAlphabet().getStates(); 
      break; 
    default: 
      assert(0); 
    }

  bool isFirstG = true; 
  for(nat i = 0; i < content.values.size() ; ++i)
    {
      fileHandle << (isFirstG ? "" : "\t" ) << "pi{" ;
      isFirstG = false; 
	
      bool isFirst = true; 
      for(auto &p : _partitions)
	{
	  fileHandle  << (isFirst ? "": "," ) << p ; 
	  isFirst = false; 
	}	    
      fileHandle  << "}("  << names.at(i) << ")"; 
    }  
} 


void FrequencyParameter::verifyContent(const TreeAln &traln, const ParameterContent &content) const 
{
  auto& partition = traln.getPartition(_partitions[0]);
  bool ok = true; 
  // ok &= BoundsChecker::checkFrequencies(content.values); 
  ok &= (content.values.size() ==  nat(partition.getStates())); 
  
  auto newValues = content.values; 
  RateHelper::convertToSum1(newValues);
  
  if(ok)
    ok &= BoundsChecker::checkFrequencies(newValues);

  if(not ok)
    {
      tout << "ERROR: we had " << content << " for parameter " << this << ". Did you mis-specify a fixed prior?"  << std::endl; 
      assert(0); 
    }  
}



void FrequencyParameter::checkSanityPartitionsAndPrior(const TreeAln &traln) const 
{
  auto numStates = traln.getPartition(_partitions.at(0)).getStates();
  checkSanityPartitionsAndPrior_FreqRevMat(traln);
  auto initVal = _prior->getInitialValue(); 

  if( int(initVal.values.size()) != numStates && 
      initVal.protModel.size() == 0 )
    {
      tout << "Error while processing parsed priors: you specified prior " << _prior.get() << " for parameter "; 
      printShort(tout) << " that is not applicable." << std::endl; 
      exitFunction(-1, true); 
    }
}

