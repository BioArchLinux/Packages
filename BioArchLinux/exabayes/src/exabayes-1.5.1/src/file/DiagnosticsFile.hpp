#ifndef _DIAGNOSTICS_FILE_H
#define _DIAGNOSTICS_FILE_H

#include <vector>

#include "CoupledChains.hpp"
#include "OutputFile.hpp"
#include "GlobalVariables.hpp"

class DiagnosticsFile : public  OutputFile
{
public:   
  DiagnosticsFile() 
    : _names{}
    , _initialized(false) 
  {}

  void initialize(std::string workdir, std::string name, const std::vector<CoupledChains> &runs) ; 
  void regenerate(std::string workdir, std::string nowId, std::string prevId, uint64_t gen); 
  void printDiagnostics(uint64_t gen, double asdsf, const std::vector<CoupledChains> &runs );  

  bool isInitialized() const {return _initialized; }

private:   			// METHODS
  std::string createName(std::string runname, std::string workdir); 

private: 			// ATTRIBUTES 
  std::vector<std::string> _names;   
  bool _initialized; 
}; 

#endif
