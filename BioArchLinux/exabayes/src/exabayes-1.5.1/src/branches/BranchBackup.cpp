#include "BranchBackup.hpp"

#include "BranchPlain.hpp"
#include "BranchLength.hpp"

#include "TreeAln.hpp"

#include <algorithm>

using std::get;



BranchBackup::BranchBackup(TreeAln &traln, std::vector<BranchPlain> bs, std::vector<AbstractParameter*> params)
  : _backup{}
{
  for(auto &b :bs )
    {
      for(auto &p : params)
	{
	  extend(traln, p,b); 
	}
    }
} 


void BranchBackup::extend(TreeAln &traln, AbstractParameter* param, const BranchLength &length)
{
  auto bl = traln.getBranch(length, param); 
  _backup.push_back(std::make_pair(param, bl));
  traln.setBranch(length, param);
}

void BranchBackup::extend(TreeAln &traln, AbstractParameter* param, const BranchPlain &length)
{
  auto bl = traln.getBranch(length, param); 
  _backup.push_back(std::make_pair(param, bl));
}

void BranchBackup::resetFromBackup(TreeAln& traln) const 
{
  for(auto & b: _backup)
    traln.setBranch(get<1>(b), get<0>(b)); 
}

// meh 
std::tuple<bool, BranchLength> BranchBackup::find(const BranchPlain &branch, AbstractParameter *param) const 
{
  auto foundIter = std::find_if(begin(_backup), end(_backup), [&](const std::pair<AbstractParameter*, BranchLength> &elem)
				{
				  return get<0>(elem) == param && 
				  (get<1>(elem) == branch || branch.getInverted() == get<1>(elem)); 
				  // branch.equalsUndirected(get<1>(elem).toPlain()); 
				}
				); 
    
  if( foundIter == end(_backup ))
    return std::make_pair(false, BranchLength());
  else 
    return std::make_tuple(true, get<1>(*foundIter)); 
}


  
  
std::ostream& operator<<(std::ostream& out, const BranchBackup& rhs)
{
  for(auto &elem : rhs._backup)
    {
      out << "[" << get<0>(elem) << "," << get<1>(elem)  << "],"; 
    }
  return out; 
}


