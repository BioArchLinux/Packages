#include <cassert>
#include <algorithm>
#include <unistd.h>
#include <unordered_map>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

int NUM_BRANCHES; 

#define _INCLUDE_DEFINITIONS
#include "GlobalVariables.hpp"
#undef _INCLUDE_DEFINITIONS

#include "common.h"
#include "extensions.hpp"

#include "SplitFreqAssessor.hpp"


static void printUsage()
{
  std::cout
	    << "\nComputes the avgerage and maximum deviation of split frequencies\n"
	    << "of sets of trees.\n\n"
	    << "USAGE: ./asdsf [-m] [-b relBurnin | -r start ] [ -i ignoreFreq ]  -f file[..]\n\n"
	    << "      -i ignoreFreq    ignore splits with frequencies lower than ignoreFreq\n"
	    << "                       [Range: 0.0 <= ignoreFreq < 1.0; default: 0.1]\n\n"
	    << "      -f file[..]      two or more topology files\n\n"
	    << "      -r relBurnin     discard first relBurnin percent of tree samples \n"
	    << "                       [ 0.0 <= relBurnin < 1.0; default 0.25]\n\n"
	    << "      -b num           constant burn-in: discard the first <num> samples\n\n"
	    << "      -q               quiet run: only prints asdsf / msdsf\n"
	    << "\n\n"
    ; 

  exit(0); 
}


static void myExit(int code, bool waitForAll)
{
  exit(code); 
}



int main(int argc, char** argv)
{
  exitFunction = myExit; 

// #ifdef _USE_GOOGLE_PROFILER
//   auto myProfileFile = "profile.out"; 
//   remove(myProfileFile);
//   ProfilerStart(myProfileFile);
// #endif


  NUM_BRANCHES = 1; 

  bool constBurninWasSet = false; 
  bool relBurninWasSet = false; 
  auto quiet = false;  

  nat constBurnin = 0; 
  double relBurnin = 0.25; 
  auto files = std::vector<std::string>{}; 
  double ignoreFreq = 0.1; 

  int c = 0;   
  while( ( c = getopt(argc, argv,"r:b:f:i:hq") ) != EOF )
    {
      switch(c)
	{
	case 'h': 
	  {
	    printUsage();
	  }
	  break; 
	case 'i': 
	  {
	    auto &&iss = std::stringstream{optarg}; 
	    iss >> ignoreFreq ; 
	    assert(ignoreFreq < 1. && 0. <= ignoreFreq); 
	  }
	  break; 
	case 'b':
	  {
	    constBurninWasSet  = true; 
	    auto &&iss = std::stringstream{optarg}; 
	    iss >> constBurnin ; 
	  }
	  break; 
	case 'f':
	  {
	    int index  = optind -1; 
	    while(index < argc)
	      {
		auto next = std::string{argv[index]}; 
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
	case 'r': 
	  {
	    relBurninWasSet = true ; 
	    auto &&iss = std::istringstream {optarg}; 
	    iss >> relBurnin; 
	  }
	  break; 
	case 'q': 
	  {
	    quiet = true; 
	    break; 
	  }
	default : 
	  {
	    std::cerr << "unknown argument >" << c << "<" << std::endl; 
	    exitFunction(-1, false); 
	    }
	} 
    }
  
  if(argc == 0 || files.size() == 0 )
    printUsage(); 

  for(auto file : files)
    {
      if(not std::ifstream(file)) 
	{
	  std::cout << "error: could not open file >" << file << "<" << std::endl; 
	  exit(-1); 
	}
    }

  if(constBurninWasSet && relBurninWasSet)
    {
      std::cout << "Error: you cannot set the relative burn-in AND a range of trees to\n"
		<< "use.\n";
      exit(-1); 
    }

  if(relBurninWasSet && not (0. <= relBurnin && relBurnin < 1.) )
    {
      std::cout << "Error: please specifify a value (percentage) between (0,1) for the relative burn-in. Your value was "<< relBurnin << std::endl; 
      exit(-1 ); 
    }    

  auto sdsf = SplitFreqAssessor(files, true); 

  auto numTrees = std::vector<nat>(files.size(),0);
  std::transform(begin(files), files.end(), begin(numTrees), [&](const std::string &file)  { return sdsf.getNumTreeAvailable(file); } ); 
  
  // determine the range 
  auto start =  std::vector<nat>(files.size(),0);
  for(auto i = 0u; i < files.size(); ++i)
    {
      if(constBurninWasSet)
	{
	  if( numTrees[i] < constBurnin)
	    {
	      std::cout << "you are trying to discard " << constBurnin << " trees, but the minimum trees available in one of the files is just " << numTrees[i] << std::endl; 
	      exit(-1); 
	    }
	  
	  // std::cout << SHOW(constBurnin) << std::endl; 

	  start[i] = constBurnin; 
	}
      else 
	start[i] = nat(double(numTrees[i]) * relBurnin); 

      if(not quiet)
	std::cout << "using trees number " << start[i] <<  " to "<< numTrees[i] << " for file " << files[i] << std::endl; 
    }

  sdsf.extractBips( start  , numTrees ); 

  auto sdsfResult = sdsf.computeAsdsfNew(ignoreFreq); 

  if(not quiet)
    {
      std::cout << "average deviation of split frequencies: " <<  sdsfResult.first * 100  << "%" << std::endl; 
      std::cout  << "maximum deviation of split frequencies: " << sdsfResult.second * 100 << "%" << std::endl; 
      std::cout << "ignored splits that occurred in less than "  << ignoreFreq * 100 << "% of the trees for any of the specified files." << std::endl; 
    }
  else 
    {
      std::cout << sdsfResult.first * 100  << "\t" << sdsfResult.second * 100 << std::endl; 
    }

  return 0; 
}


