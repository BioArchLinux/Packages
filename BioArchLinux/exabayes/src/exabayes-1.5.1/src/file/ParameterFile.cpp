#include "ParameterFile.hpp"
#include "GlobalVariables.hpp"
#include "Category.hpp"
#include "extensions.hpp"

#include <cassert>


ParameterFile::ParameterFile(std::string workdir, std::string runname, nat ru )
  : runid(ru)
{
  auto&& ss = std::stringstream{}; 

  ss << OutputFile::getFileBaseName(workdir) << "_parameters.run-"<< runid << "." << runname   ; 
  fullFileName = ss.str();
}


void ParameterFile::initialize(const TreeAln& traln, const ParameterList& parameters,  nat someId, bool isDryRun )  
{        
  rejectIfExists(fullFileName); 

  if(isDryRun)
    return; 

  auto&& fh =  std::ofstream{fullFileName,std::fstream::out}; 

  fh << "[ID: " << someId << "]\n"; 

  fh << "Gen";
  fh << "\tLnPr"; 
  fh << "\tLnL" ; 

  auto blParams = std::vector<AbstractParameter*> {}; 
  for(auto &param : parameters)
    if(param->getCategory() == Category::BRANCH_LENGTHS
       && param->getPrior()->needsIntegration() )
      blParams.push_back(param); 

  for(auto &param : blParams)    
    {
      fh << "\tTL{" ; 
      formatRange(fh, param->getPartitions()); 
      fh << "}"; 
    }

  for(auto &p : parameters)
    {
      if(p->isPrintToParamFile())
	{
	  fh << "\t" ; 
	  p->printAllComponentNames(fh, traln); 
	}
    }

  fh << std::endl; 

  fh.close(); 
}


void ParameterFile::sample(const TreeAln &traln, const ParameterList& parameters, uint64_t gen, log_double lnPr)  
{
  auto&& fh = std::ofstream(fullFileName, std::fstream::app);  
    
  fh << gen << "\t"; 
  fh << MAX_SCI_PRECISION; 
  fh << lnPr << "\t"; 
  fh << traln.getTrHandle().likelihood << "\t" ; 

  std::vector<AbstractParameter*> blParams ; 
  for(auto &p : parameters)
    {
      if(p->getCategory() == Category::BRANCH_LENGTHS)
	blParams.push_back(p);
    }

  // print tree lengths 
  for(auto &param :  blParams)
    {
      double tl = 0; 
      for(auto &b : traln.extractBranches(param))
	{
	  // tl += b.getInterpretedLength( param);
	  tl += b.toMeanSubstitutions(param->getMeanSubstitutionRate()); 
	}
      fh << tl << "\t"; 
    }

  bool isFirst = true; 
  for(auto &p : parameters)
    {
      if(p->isPrintToParamFile())
	{
	  if(isFirst)
	    isFirst = false; 
	  else 
	    fh << "\t"; 
	  p->printSample(fh,traln);
	}
    }
  fh << std::endl; 
}


void ParameterFile::regenerate(std::string workdir, std::string prevId, uint64_t gen) 
{
  std::ofstream fh(fullFileName, std::fstream::out); 

  std::stringstream ss; 
  ss << OutputFile::getFileBaseName(workdir) << "_parameters.run-"<< runid  << "." << prevId; 
  std::ifstream prevFile(ss.str()); 
  rejectIfNonExistant(ss.str()); 

  nat genFound = 0; 
  nat lineCtr = 0; 
  while(genFound < gen && not prevFile.eof())
    {
      std::string line; 
      getline(prevFile, line); 
      
      // hack =/ 
      bool lineIsEmpty = line.compare("") == 0 ; 
      
      if(lineCtr > 1 && not lineIsEmpty)
	{
	  std::stringstream ss1; 
	  ss1.str(line); 
	  std::string part; 
	  getline( ss1 ,part, '\t'  );       
	  genFound = std::stoi(part); 
	}

      if(genFound < gen && not lineIsEmpty)
	fh << line << std::endl; 
      ++lineCtr; 
    }

  fh.close();
} 
