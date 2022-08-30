#ifndef __EXA_BLOCK_HPP 
#define __EXA_BLOCK_HPP

#include <ncl/ncl.h>

class ExaBlock  : public NxsBlock
{

protected:
  double parseScientificDouble(NxsToken& token) const  ; 
  bool convertToBool(NxsString &string) const ; 


}; 

#endif
