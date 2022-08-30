#ifndef TOPOLOGY_FILE
#define TOPOLOGY_FILE

#include <sstream>
#include <string>
#include <iostream> 

#include "common.h"
#include "TreeAln.hpp"
#include "OutputFile.hpp"

class TopologyFile : public OutputFile
{
public: 
  TopologyFile(std::string workdir, std::string runname, nat runid, nat couplingId, nat paramNum, bool hasManyTopoloFiles); 
  void initialize(const TreeAln& traln, nat someId, bool isDryRun)  ;     
  void sample(const TreeAln &traln, uint64_t gen, AbstractParameter* blParams)  ; 
  void regenerate(std::string workdir, std::string prevId, uint64_t gen) ; 
  void verifyNonExistance();

private: 			// METHODS
  std::streamoff getPosBeforeEnd() const ; 

private: 			// ATTRIBUTES
  nat runid; 
  nat couplingId; 
  nat paramNum; 
  bool hasManyTopoloFiles; 
}; 

#endif
