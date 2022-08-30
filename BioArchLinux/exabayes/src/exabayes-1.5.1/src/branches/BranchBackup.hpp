#ifndef BRANCH_BACKUP_HPP
#define  BRANCH_BACKUP_HPP

#include "BranchPlain.hpp"
#include "BranchLength.hpp"

#include <vector>

class AbstractParameter;
class TreeAln;

class BranchBackup 
{
public: 
  BranchBackup(): _backup{}
  {}
  BranchBackup(TreeAln &traln, std::vector<BranchPlain> bs, std::vector<AbstractParameter*> params); 

  void extend(TreeAln &traln, AbstractParameter* param, const BranchLength &length); 
  void extend(TreeAln &traln, AbstractParameter* param, const BranchPlain &length); 
  void resetFromBackup(TreeAln& traln) const ; 
  
  // meh 
  std::tuple<bool, BranchLength> find(const BranchPlain &branch, AbstractParameter *param) const ; 
  friend std::ostream& operator<<(std::ostream& out, const BranchBackup& rhs); 

private: 
  std::vector<std::pair<AbstractParameter*,BranchLength>> _backup; 
}; 


#endif
