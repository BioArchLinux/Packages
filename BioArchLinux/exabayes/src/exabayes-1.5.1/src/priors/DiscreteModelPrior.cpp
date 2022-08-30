#include "DiscreteModelPrior.hpp"
#include "ProtModel.hpp"


DiscreteModelPrior:: DiscreteModelPrior(std::unordered_map<ProtModel,double> model)
  :_modelProbs{model}
{
} 


ParameterContent DiscreteModelPrior::getInitialValue() const 
{
  auto result = ParameterContent{}; 
  auto aModel = std::get<0>( *(_modelProbs.begin() )); 
  result.protModel.push_back( aModel);  
  return result;
} 


log_double DiscreteModelPrior::accountForMeanSubstChange( TreeAln &traln, const AbstractParameter* param , double myOld, double myNew ) const 
{
  assert(0); 
  return log_double::fromAbs(0);
}

 
// TODO bool uniform 
ParameterContent DiscreteModelPrior::drawFromPrior(Randomness &rand)  const 
{
  auto modelList = std::vector<ProtModel>{}; 
  for(auto &v : _modelProbs)
    if( FLOAT_IS_INITIALIZED(std::get<1>(v)) )
      modelList.push_back(std::get<0>(v)); 

  auto index = rand.drawIntegerOpen(int(modelList.size()))  ; 
  
  auto result = ParameterContent{}; 
  result.protModel.push_back(modelList.at(index));
  return result; 
}

log_double DiscreteModelPrior::getLogProb( const ParameterContent& content ) const 
{
  assert(content.protModel.size() == 1 ); 
  
  auto model = ProtModel(content.protModel[0]);
  // auto model = ProtModel(content.values[0]); 
  assert(_modelProbs.find(model) != _modelProbs.end()); 

  return log_double::fromAbs(_modelProbs.at(model));
}

 
void DiscreteModelPrior::print(std::ostream &out) const 
{
  if(_modelProbs.size() == 1 )
    {
      auto model = std::get<0>(* _modelProbs.begin()); 
      out << "Fixed(" << ProtModelFun::getName(model) << ")" ; 
    }
  else 
    {
      out << "Discrete(" ; 
      auto isFirst = bool{true}; 
      for(auto &modelPair : _modelProbs)
	{
	  if(isFirst)
	    isFirst = false; 
	  else 
	    out << "," ;

	  out << ProtModelFun::getName(std::get<0>(modelPair)) << "=" << SOME_FIXED_PRECISION <<  std::get<1>(modelPair) ; 
	}
      out << ")"; 
    }
}

