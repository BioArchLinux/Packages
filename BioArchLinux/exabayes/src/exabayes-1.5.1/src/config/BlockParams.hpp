#ifndef _BLOCK_PARTITION_H
#define _BLOCK_PARTITION_H

#include "ExaBlock.hpp"

#include "GlobalVariables.hpp"
#include "AbstractParameter.hpp" 
#include "TreeAln.hpp"
#include "Category.hpp"

class BlockParams : public ExaBlock
{
public: 
  BlockParams()
    : parameters{}
    , tralnPtr{nullptr}
  {
    NCL_BLOCKTYPE_ATTR_NAME = "PARAMS";    
  }
  
  BlockParams(const BlockParams &rhs) = default; 
  BlockParams& operator=(const BlockParams &rhs)  = default; 

  void setTree(const TreeAln* _traln){ tralnPtr = _traln; }
  vector<unique_ptr<AbstractParameter> > getParameters() const; 
  virtual void Read(NxsToken &token); 

private:   			// METHODS
  void partitionError(nat partition, size_t totalPart) const ; 
  void parseScheme(NxsToken& token, Category cat, nat &idCtr); 
  nat getNumSeen(Category cat) ; 
  
private: 			// ATTRIBUTES
  vector<unique_ptr<AbstractParameter> > parameters; 
  const TreeAln* tralnPtr;  	// NON-owning
}; 


#endif
