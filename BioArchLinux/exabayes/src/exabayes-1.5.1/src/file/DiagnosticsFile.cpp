#include "DiagnosticsFile.hpp"

#include <sstream>
#include <unordered_map>


std::string DiagnosticsFile::createName(std::string runname, std::string workdir)
{
  std::stringstream ss; 
  ss << workdir << (workdir.compare("") == 0 ? "" : "/") 
     << PROGRAM_NAME << "_diagnostics." << runname ; 
  
  return ss.str();
}


void DiagnosticsFile::initialize(std::string workdir, std::string name, const std::vector<CoupledChains> &runs) 
{
  assert(not _initialized); 
  _initialized = true; 

  fullFileName = createName(name, workdir);     
  rejectIfExists(fullFileName); 

  auto&& fh = std::ofstream(fullFileName); 

  fh << "GEN" ;

  if(runs.size() > 1)
    fh << "\tasdsf"; 
  
  for(auto& run : runs)
    {
      auto numElem = run.getChains().size(); 
      auto myInfo = run.getSwapInfo(); 
      for(nat i = 0; i < numElem; ++i)
	{
	  for(nat j = i+1; j < numElem; ++j )
	    {
	      fh << "\tsw("  << i << "," << j << ")$run" << run.getRunid(); 
	    }
	}
    }
  
  for(auto& run : runs)
    {
      auto &coldChain = run.getChains()[0]; 

      auto ps = coldChain.getProposalView(); 
      auto &&ss = std::stringstream{} ; 
      for(auto &p : ps)
	{
	  ss.str(""); 
	  p->printShort(ss); 
	  ss << "$run" << run.getRunid();  
	  _names.push_back(ss.str()); 
	  fh << "\t" << ss.str(); 
	}        

      for(auto &propset : coldChain.getProposalSets())
	{
	  for(auto &p : propset.getProposalView())
	    {
	      ss.str(""); 
	      p->printShort(ss);
	      ss << "$run" << run.getRunid();
	      _names.push_back(ss.str()); 
	      fh << "\t" << ss.str();
	    }
	}
    }

  fh << std::endl; 
  fh.close(); 
}


void DiagnosticsFile::printDiagnostics(uint64_t gen, double asdsf, const std::vector<CoupledChains> &runs ) 
{
  auto &&fh = std::ofstream(fullFileName, std::fstream::app); 

  fh << gen ; 
  
  if(runs.size() > 1)
    fh << "\t" << asdsf;   

  // print swapping info (if applicable)
  for(auto &run : runs)
    {
      auto m = run.getSwapInfo().getMatrix();
      for(auto &elem  : m)
	{
	  fh << "\t"
	     << elem.getRatioOverall() 
	     << "," << elem.getTotalSeen();
	}
    }

  // print acceptance rates  
  auto name2proposal = std::unordered_map<std::string,AbstractProposal*>{}; 
  for(auto &run : runs)
    {
      for(auto &chain: run.getChains())
	{
	  if(chain.getCouplingId() != 0 ) // no hot chains
	    continue; 

	  for(auto &p : chain.getProposalView())
	    {
	      auto &&ss = std::stringstream{} ; 
	      p->printShort(ss);
	      ss << "$run"  << run.getRunid(); 

	      assert(name2proposal.find(ss.str()) == name2proposal.end()); 
	      name2proposal[ss.str()] = p; 
	    }
	  
	  for(auto &ps  :chain.getProposalSets())
	    {
	      for(auto &p : ps.getProposalView())
		{
		  auto &&oss = std::ostringstream{}; 
		  p->printShort(oss); 
		  oss << "$run" << run.getRunid();

		  assert(name2proposal.find(oss.str()) == name2proposal.end()); 
		  name2proposal[oss.str()] = p; 
		}
	    }
	}
    }
  
  for(auto &name : _names) 
    {
      if(name2proposal.find(name) == end(name2proposal))
	{
	  tout << "could not find proposal >" << name << "<" << std::endl; 
	  assert(0); 
	}
      auto& p = name2proposal[name]; 
      auto &sctr = p->getSCtr();
      fh << "\t" << sctr.getRatioOverall() << "," << sctr.getTotalSeen() ; 
    }	  

  fh << std::endl; 
}



static bool isProposal(std::string token)
{
  auto result = true;  

  result &= token.compare("GEN") != 0 ;  
  result &= token.compare("asdsf") != 0 ; 

  result &= not (token[0] == 's'  && token[1] == 'w' && token[2] == '(') ; 

  return result; 
}


static std::vector<std::string> getProposalNames(std::string line)  
{
  auto result = std::vector<std::string> {}; 
  auto&& iss = std::istringstream {line}; 

  while(iss)
    {
      auto token = std::string {}; 
      
      getline(iss, token, '\t');

      if(iss && isProposal(token))
	result.push_back(token);
    }

  return result; 
}


void DiagnosticsFile::regenerate(std::string workdir, std::string nowId, std::string prevId, uint64_t gen)
{    
  assert(not _initialized); 
  _initialized = true; 

  fullFileName = createName (nowId, workdir); 
  rejectIfExists(fullFileName); 
  auto &&fh = std::ofstream(fullFileName);   
  auto prevFileName = createName(prevId, workdir); 
  rejectIfNonExistant(prevFileName); 
  auto&& ifh = std::ifstream (prevFileName); 

  nat genFound = 0; 
  nat lineCtr = 0; 
  bool firstLine = true;  

  while(genFound < gen && not ifh.eof())
    {
      auto line = std::string{}  ; 
      getline(ifh, line); 

      // special treatment
      if(firstLine)
	{
	  firstLine = false; 
	  
	  _names = getProposalNames(line);
	}

      // TODO: all this comparing against empty lines only is needed,
      // because we could not have any data lines (and only the
      // header). Come up with a better solution. 
      if( ( lineCtr > 0 )  &&  (  line.compare("") != 0 )  )
      	{
	  auto &&ss = std::stringstream{} ; 
	  ss.str(line); 
	  auto part = std::string{} ; 
	  getline(ss, part, '\t'); 
	  genFound = std::stoi(part); 
	}

      if(( genFound < gen )  && ( line.compare("") != 0 ) )
	fh << line << std::endl; 
      ++lineCtr;
    }
} 

