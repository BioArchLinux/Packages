#include "ParameterList.hpp"


ParameterList::ParameterList(std::vector< std::unique_ptr<AbstractParameter> > params) 
  : _params{std::move(params)}
  , _paramView{}
  {
    for(auto &p : _params)
      _paramView.push_back(p.get()); 
  }


ParameterList::ParameterList(const ParameterList& rhs)  
  : _params{}
  , _paramView{}
  {
    for(auto &p : rhs._params)
      _params.emplace_back(p->clone());
    for(auto &p : _params)
      _paramView.push_back(p.get());
    
  } 
  
ParameterList& ParameterList::operator=(ParameterList rhs)
{
  swap(*this, rhs); 
  return *this; 
}

void swap(ParameterList& lhs, ParameterList& rhs)
{
  std::swap(lhs._params, rhs._params);
  std::swap(lhs._paramView, rhs._paramView); 
}



std::ostream& operator<<(std::ostream& s, const ParameterList& c) 
{
  for(auto &p : c._paramView)
    {
      if(dynamic_cast<BranchLengthsParameter*>(p) != nullptr)
	{
	  auto pc = dynamic_cast<BranchLengthsParameter*>(p); 
	  auto cont = pc->getAttributes(); 
	  s << SOME_FIXED_PRECISION << cont._convTuner.getParameter() ;
	  s  << "," << cont._nonConvTuner.getParameter();
	}
    }
  return s;
}



std::vector<AbstractParameter*> ParameterList::getViewByCategory(Category cat) const 
{
  auto result = std::vector<AbstractParameter*>{}; 

  for(auto &p : _paramView)
    {
      if(p->getCategory() == cat)
	result.push_back(p);
    }

  return result; 
} 
