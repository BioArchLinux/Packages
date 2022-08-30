#include <iosfwd>
#include <cstring>
#include <unistd.h>

int NUM_BRANCHES ; 

#include "BipartitionExtractor.hpp" 

// #define _INCLUDE_DEFINITIONS
// #include "GlobalVariables.hpp"
// #undef _INCLUDE_DEFINITIONS


static void printUsage(std::ostream &out)
{
  out << "extractBips is a utility for extracting bipartitions, branch lengths\n"
      << "associated with bipartitions and statistics about these branch lengths\n"
      << "from tree sets.\n\n"; 
  out << "USAGE: ./extractBips -n  id -f file[..] [-b burnin]\n\n" ; 
  out << "Options:\n"; 
  out << "         -n id            an id for the output file\n"  ; 
  out << "         -f file[..]      one or more topology files\n"; 
  out << "         -b relBurnin     proportion of trees to discard as burn-in (beginning at\n"
      << "                          the start of the file). Default: 0.25\n\n"; 
}

static std::tuple<std::string, std::vector<std::string>,double> processCommandLine(int argc, char **argv)
{
  double burnin = 0.25; 
  auto files = std::vector<std::string> {}; 
  auto id = std::string{}; 

  int c = 0; 
  while(   (c = getopt(argc, argv, "n:f:b:h") ) != EOF )
    {
      switch(c)
	{
	case 'h': 
	  {
	    printUsage(std::cout); 
	    exitFunction(-1, false); 
	  }
	  break; 
	case 'n': 
	  {
	    id = std::string{optarg, optarg + strlen(optarg)}; 
	  }
	  break; 
	case 'f': 
	  {
	    int index  = optind -1; 
	    while(index < argc)
	      {
		auto next = std::string{strdup(argv[index])}; 
		++index; 
		if(next[0] != '-') 
		  files.push_back(next);
		else 
		  {
		    optind =  index -1 ; 
		    break; 
		  }
	      }
	  }
	  break; 
	case 'b': 
	  {
	    auto &&iss = std::istringstream{optarg}; 
	    iss >> burnin; 
	  }
	  break; 
	default : 
	  {
	    std::cerr << "Error: unknown option >" << char(c) << "<. " << std::endl; 
	    exitFunction(-1, false); 
	  }
	}
    }

  if(not ( 0<= burnin && burnin < 1.  ) )
    {
      std::cerr << "The relative burn-in to be discarded must be in the range [0,1)." << std::endl; 
      exitFunction(-1, false); 
    }
 
  if(id.compare("") == 0)
    {
      std::cerr << "Please provide a run-id via -n " << std::endl; 
      exitFunction(-1, false); 
    }    
  
  return std::make_tuple(id, files, burnin);
}


static void myExit(int code, bool waitForAll)
{
  exit(code); 
}





int main(int argc, char** argv)
{
  exitFunction = myExit; 

  NUM_BRANCHES = 1; // BAD 

  if(argc < 2) 
    {
      printUsage(std::cout);
      exitFunction(-1, false); 
    }

  auto files = std::vector<std::string>{};
  double burnin = 0; 
  auto id = std::string{};

  std::tie(id,files,burnin) = processCommandLine(argc, argv); 

  for(auto f : files)
    std::cout  << "file: " << f << std::endl; 

  for(auto file : files)
    {
      if(not std::ifstream(file))
	{
	  std::cerr << "Error: could not open file >" <<  file  << "<" << std::endl; 
	  exitFunction(-1, false); 
	}    
    }

  assert(files.size() >  0); 

  auto&& bipEx = BipartitionExtractor(files, false, true);

  nat numTree = bipEx.getNumTreesInFile( files.at(0) );
  
  // std::cout <<  "have "<< numTree << std::endl; 
  
  nat absBurnin = nat(double(numTree) * burnin); 

  bipEx.extractBips<true>(absBurnin);
  bipEx.printBipartitions(id);
  bipEx.printBipartitionStatistics(id); 
  bipEx.printFileNames(id);
  bipEx.printBranchLengths(id);

  return 0; 
}
