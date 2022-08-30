
/**
   @file ConfigReader.hpp

   @brief specializes a nxsreader for parsing of an exabayes block.
*/

#ifndef _NCL_CONFIG_READER
#define _NCL_CONFIG_READER

#include <ncl/ncl.h>
#include <vector>

#include "PriorBelief.hpp"
#include "AbstractProposal.hpp"

#include "BlockParams.hpp" 
#include "BlockPrior.hpp"

class ConfigReader : public NxsReader
{
public: 
  ConfigReader() : NxsReader(){SetWarningOutputLevel(SUPPRESS_WARNINGS_LEVEL); }
  virtual void ExitingBlock(NxsString blockName){}
  virtual void ExecuteStopping(){}
  virtual void ExecuteStarting(){}

}; 

#endif
