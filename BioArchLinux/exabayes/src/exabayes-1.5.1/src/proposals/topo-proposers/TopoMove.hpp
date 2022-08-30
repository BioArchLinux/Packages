#ifndef _ABSTRACT_MOVE_HPP
#define _ABSTRACT_MOVE_HPP

#include <vector>
#include "TreeAln.hpp"
#include "LikelihoodEvaluator.hpp"

#include "MoveOptMode.hpp"


class AbstractParameter; 

class TopoMove 
{
public: 
  virtual ~TopoMove() = 0; 

  virtual void apply(TreeAln &traln, const std::vector<AbstractParameter*> &blParams) const = 0; 
  virtual BranchPlain getEvalBranch(const TreeAln &traln) const = 0; 
  virtual std::vector<nat> getDirtyNodes() const = 0 ; 
  virtual std::unique_ptr<TopoMove> getInverse() const = 0 ; 
  virtual TopoMove* clone() const = 0 ; 
  virtual void print(std::ostream& out) const = 0; 
  
  virtual int getNniDistance() const = 0; 

  virtual std::vector<BranchPlain> getBranchesToPropose(const TreeAln& traln, MoveOptMode mode ) = 0; 
  
  virtual void invalidateArrays(LikelihoodEvaluator& eval,  TreeAln& traln, MoveOptMode mode) const = 0; 

  // common 
  friend std::ostream& operator<<(std::ostream& out, const TopoMove& elem)
  {
    elem.print(out);
    return out; 
  } 

}; 

inline TopoMove::~TopoMove() = default; 

#endif
