#include "AbstractProposal.hpp"
#include "ParameterList.hpp"


AbstractProposal::AbstractProposal( Category cat, std::string name,double weight, double minTuning, double maxTuning, bool needsFullTraversal )  
  : _name(name)
  , _sctr{}
  , _category(cat)
  , _primParamIds{}
  , _secParamIds{}
  , _relativeWeight(weight)
  , _needsFullTraversal(needsFullTraversal)
  , _inSetExecution(false)
  , _preparedBranch(0,0)
  , _preparedOtherBranch(0,0)
  , _id{0}
  , _forProteinOnly(false)
  , _minTuning{minTuning}
  , _maxTuning{maxTuning}
  , _numTaxNeeded{4}
  , _usingOptimizedBranches(false)
  , _allParams( nullptr ) 	// do not like 
  // the proper way to initialize everything is probably a factory... 
{
} 


std::ostream& AbstractProposal::printShort(std::ostream &out)  const 
{
  out << _name << "( " ;  
    
  bool isFirst = true; 
  for(auto &v : _primParamIds)
    {
      // tout << "trying to print "  << v << std::endl; 
      if(not isFirst)
	out << ","; 
      else 
	isFirst = false; 
      auto p = _allParams->at(v); 
      p->printShort(out); 
    }

  if(_secParamIds.size() > 0)
    {
      out << ";"; 
      isFirst = true; 
      for(auto &v : _secParamIds)
	{
	  if(not isFirst)
	    out << ","; 
	  else 
	    isFirst = false; 
	  _allParams->at(v)->printShort(out); 
	}
    }

  printParams(out);

  out << " )"; 
  return out; 
}


std::ostream& AbstractProposal::printNamePartitions(std::ostream &out)
{
  out << _name  << "(" ; 
  assert(_primParamIds.size() == 1); 
  bool isFirst= true; 
  for (auto v : _allParams->at(_primParamIds[0])->getPartitions()) 
    {
      if( not isFirst)
	out << ","; 
      else 
	isFirst = false; 
      out << v ; 
    }
  out << ")" ; 
  return out; 
}


std::ostream&  operator<< ( std::ostream& out , const AbstractProposal& rhs) 
{
  out << rhs._name  << std::endl
      << "\tintegrating:\t"; 
  for(auto &r : rhs._primParamIds)
    out << rhs._allParams->at(r) << ", "  ; 
  
  if(not rhs._secParamIds.empty() )
    {
      out << std::endl << "\talso modifying:\t" ; 
      for(auto &r : rhs._secParamIds ) 
	out << rhs._allParams->at(r) << ",\t" ; 
    }
  return out; 
}


void AbstractProposal::serialize( std::ostream &out)   const
{
  _sctr.serialize(out) ; 
  writeToCheckpointCore(out); 
}


void AbstractProposal::deserialize( std::istream &in )
{
  _sctr.deserialize(in); 
  readFromCheckpointCore(in); 
}


std::vector<AbstractParameter*> AbstractProposal::getPrimaryParameterView() const
{
  auto result = std::vector<AbstractParameter*> {}; 
  for(auto &v : _primParamIds)
    result.push_back( _allParams->at(v)); 
  return result; 
}

 
std::vector<AbstractParameter*> AbstractProposal::getSecondaryParameterView() const 
{
  auto result = std::vector<AbstractParameter*>{}; 
  for(auto &v : _secParamIds)
    result.push_back( _allParams->at(v)); 
  return result; 
}


std::vector<AbstractParameter*> AbstractProposal::getBranchLengthsParameterView() const 
{
  auto result = std::vector<AbstractParameter*> {}; 
  for(auto &p : _primParamIds)
    if(_allParams->at(p)->getCategory() == Category::BRANCH_LENGTHS)
      result.push_back(_allParams->at(p)); 

  for(auto &p : _secParamIds ) 
    if(_allParams->at(p)->getCategory() == Category::BRANCH_LENGTHS)
      result.push_back(_allParams->at(p));
  return result; 
} 


std::vector<nat> AbstractProposal::getAffectedPartitions() const 
{
  assert( _primParamIds.size() == 1); 
  return _allParams->at(_primParamIds[0])->getPartitions();
} 


std::array<bool,3>
AbstractProposal::getBranchProposalMode() const 
{
  bool outer = false; 
  bool multiply = false; 
  bool sequential = false; 

  auto branchOpt = std::getenv("PROPOSE_BRANCHES"); 
  if(branchOpt != NULL )
    {
      auto &&iss =  std::istringstream(branchOpt); 
      int mode = 0; 
      iss >> mode; 
      if(mode == 0)
	{
	}
      else if(mode == 1)
	{
	  multiply = true ; 
	}
      else if(mode == 2)
	{
	  multiply = true; 
	  outer = true; 
	}
      else if(mode == 3 )
	{
	  multiply = true; 
	  sequential = true; 
	}
      else if(mode == 4 )
	{
	  multiply = true; 
	  outer = true; 
	  sequential = true; 
	}
      else 
	assert(0);
    }

  return {{multiply, outer, sequential }}; 
}


/**
   @brief log tuning for a parameter 
   
   @return tuned parameter    
 */
double AbstractProposal::tuneParameter(int batch, double accRatio, double parameter, bool inverse )
{  
  double delta = 1.0 / sqrt(batch + 1 );
  // delta = 0.1 < delta ? 0.1 : delta;

  assert(  FLOAT_IS_INITIALIZED(_minTuning)  || FLOAT_IS_INITIALIZED( _maxTuning) ); 

  double logTuning = log(parameter);
  
  if(inverse)
    logTuning += (accRatio > TARGET_RATIO)  ? -delta : +delta ;
  else 
    logTuning += (accRatio > TARGET_RATIO)  ? +delta : -delta ;
  
  double newTuning = exp(logTuning);

  if (_minTuning <  newTuning && newTuning < _maxTuning)
    return newTuning; 
  else 
    return parameter; 
}
