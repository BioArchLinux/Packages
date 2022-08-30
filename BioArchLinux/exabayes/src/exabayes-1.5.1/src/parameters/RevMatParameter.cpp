#include <algorithm>
#include <functional>

#include "RateHelper.hpp"
#include "DnaAlphabet.hpp"
#include "AminoAcidAlphabet.hpp"

#include "BoundsChecker.hpp"
#include "GlobalVariables.hpp"

#include "RevMatParameter.hpp"



void RevMatParameter::applyParameter(TreeAln& traln, const ParameterContent &content)
{
  auto tmp = content.values; 

  RateHelper::convertRelativeToLast(tmp); 
  
  // tout << "SET " << tmp << std::endl; 
  
  for(auto &m : _partitions)
    traln.setRevMat(tmp, m, false);
} 


ParameterContent RevMatParameter::extractParameter(const TreeAln &traln )  const
{
  ParameterContent result; 

  result.values = traln.getRevMat(_partitions.at(0), false); 
  
  // DO ***NOT*** REMOVE 
  // this is necessary to avoid rounding errors
  // with AA GTR matrices; but essentially it is just a hack to reduce
  // the probability of the gtr aa matrix to mess up
  RateHelper::convertRelativeToLast(result.values); 
  RateHelper::convertToSum1(result.values);

  // tout << "GET " << result.values << std::endl; 

  return result; 
}   


ParameterContent RevMatParameter::extractParameterRaw(const TreeAln& traln) const 
{
  auto result = ParameterContent{}; 
  result.values = traln.getRevMat(_partitions.at(0), true); 
  return result; 
}
 

void RevMatParameter::applyParameterRaw(TreeAln &traln, const ParameterContent & content) const 
{
  for(auto &m : _partitions)
    traln.setRevMat(content.values, m, true); 
} 

void RevMatParameter::printSample(std::ostream& fileHandle, const TreeAln &traln) const 
{
  auto content = extractParameter(traln);

  bool isFirst = true; 
  for(auto &v : content.values)
    {
      fileHandle << (isFirst ?  "" : "\t" ) << v ; 
      isFirst = false; 
    }
}
 
void RevMatParameter::printAllComponentNames(std::ostream &fileHandle, const TreeAln &traln) const 
{
  auto content = extractParameter(traln); 
  std::vector<std::string> names; 
  
  auto size = content.values.size(); 
  switch(size)
    {
    case 6 : 
      names = DnaAlphabet().getCombinations();
      break; 
    case 190: 			// really? 
      names = AminoAcidAlphabet().getCombinations(); 
      break; 
    default : 
      {
	std::cerr << "error in revMatParameter: encountered case >" <<  size << std::endl; 
	assert(0); 
      }
    }

  bool isFirstG = true; 
  for(nat i = 0; i < content.values.size() ; ++i)
    {
      fileHandle << (isFirstG ? "" : "\t" ) << "r{" ;
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


void RevMatParameter::verifyContent(const TreeAln&traln,  const ParameterContent &content) const 
{
  auto& partition = traln.getPartition(_partitions[0]);

  if (partition.getDataType() == PLL_BINARY_DATA)
  {
      tout << "\n\nError: you specfied a reversible rates parameter for"
           << " partition " << _partitions[0] << ", which consists of binary data."
           << std::endl;
      assert(0);
  }


  auto num = RateHelper::numStateToNumInTriangleMatrix(partition.getStates());

  bool ok = true;

  ok &= content.values.size( )== num ; 
  
  auto newValues = content.values; 
  RateHelper::convertRelativeToLast(newValues); 

  // for(auto &v : newValues)
  //   v /= *(newValues.rbegin()); 
  
  if(ok)
    ok &= BoundsChecker::checkRevmat(newValues); 

  if(not ok )
    {
      tout << "Wrong content " << content << " for parameter "
      << this << ". Did you mis-specify a fixed prior or are your input values to extreme?" << std::endl; 
      assert(0); 
    }
} 


void RevMatParameter::checkSanityPartitionsAndPrior(const TreeAln &traln) const 
{
  checkSanityPartitionsAndPrior_FreqRevMat(traln);
  nat numStates = traln.getPartition(_partitions[0]).getStates(); 
  
  auto initVal = _prior->getInitialValue(); 
  
  if( initVal.values.size() != RateHelper::numStateToNumInTriangleMatrix(numStates) && 
      initVal.protModel.size() == 0 )
    {
      tout << "Error while processing parsed priors: you specified prior " << _prior.get() << " for parameter "; 
      printShort(tout) << " that is not applicable." << std::endl; 
      exitFunction(-1, true); 
    }
}
