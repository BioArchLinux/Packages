#include "ProtModelParameter.hpp" 
#include "ProtModel.hpp"

#include <sstream>

using std::stringstream; 

void ProtModelParameter::applyParameter(TreeAln& traln,  const ParameterContent &content)
{
  for(auto &m : _partitions)
    {
      assert(content.protModel.size() == 1); 
      // tout << "aa model " << content.protModel[0] << std::endl;
      traln.setProteinModel(m, content.protModel[0]);
    }
}
 
ParameterContent ProtModelParameter::extractParameter(const TreeAln &traln)  const  
{

  auto& partition = traln.getPartition(_partitions.at(0)); 
  auto model = ProtModel(partition.getProtModels()); 
  auto result =   ParameterContent{} ; 
  result.protModel = {model}; 
  return result; 
}
   
void ProtModelParameter::printSample(std::ostream& fileHandle, const TreeAln &traln ) const 
{
  auto& partition = traln.getPartition(_partitions.at(0)); 
  auto name = ProtModelFun::getName(ProtModel(partition.getProtModels() )); 
  fileHandle << name; 
}
 
void ProtModelParameter::printAllComponentNames(std::ostream &fileHandle, const TreeAln &traln) const  
{
  fileHandle << "aaModel{" ; 
  bool isFirst = true; 
  for(auto &p : _partitions)
    {
      fileHandle << (isFirst ? "" : ",") << p ; 
      isFirst = false; 
    }
  fileHandle << "}"; 
}

 
void ProtModelParameter::verifyContent(const TreeAln &traln, const ParameterContent &content) const  
{
  if(content.protModel.size() != 1)
    {
      stringstream ss;
      bool first = true; 
      for (auto& partition : _partitions)
        ss << (first ?  "" : ",") << partition; 

      tout << "Incorrect number of models in parameter content. "
        "If you specified a prior for partition " <<  ss.str() <<  ", "
        "you most probably misspelled the name. If you are sure, that this is"
        " not the case, please report this as a programming error."
           << std::endl; 
      assert(0); 
    }
} 


void ProtModelParameter::checkSanityPartitionsAndPrior(const TreeAln &traln) const 
{
  checkSanityPartitionsAndPrior_FreqRevMat(traln);
  
  if(traln.getPartition(_partitions.at(0)).getStates() != 20)
    {
      std::cerr << "Error: in the config file you specified partition " << _partitions.at(0) << " to have an amino acid model. However, previously this partition was declared to have a different data type." << std::endl; 
      exitFunction(-1, true); 
    }
}



bool ProtModelParameter::fitsToPartition(Partition& p) const 
{
  return p.getDataType() == PLL_AA_DATA; 
} 



