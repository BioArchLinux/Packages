#include <sstream>
#include <cstring>
#include <unistd.h>
#include <iosfwd>

int NUM_BRANCHES; 	

#include "ConsensusTree.hpp"
#include "GlobalVariables.hpp"


static void printUsage(std::ostream &out)
{
  out << "\nconsense computes various flavours of consensus trees from sets of trees.\n\n"; 
  out << "Usage: ./consense -n id  -f file[..] [-t threshold] [-b burnin] \n\n" ; 
  out << "        -n runid         an id for the output file\n" ; 
  out << "        -t thresh        a threshold for the consenus tree. Valid values:\n"
      << "                         values between 50 (majority rule) and 100 (strict) or MRE\n" 
      << "                         (the greedily refined MR consensus).  Default: MRE\n" ; 
  out << "        -b relBurnin     proportion of trees to discard as burn-in (from start). Default: 0.25\n"; 
  out << "        -f file[..]      one or more exabayes topology files\n\n" ;
}


static std::tuple<std::string,std::vector<std::string>,double, nat,bool>
processCommandLine(int argc, char **argv)
{
  auto id = std::string{}; 
  auto thresh = double(50); 
  bool isRefined = true; 
  auto files = std::vector<std::string>{}; 
  double burnin = 0.25; 

  int c = 0; 
  while( ( c = getopt(argc, argv, "n:t:f:b:h") ) != EOF )
    {
      switch(c)
	{
	case 'h': 
	  printUsage(std::cout);
	  exitFunction(0, false);
	  break; 
	case 'n': 
	  {
	    id = std::string{optarg, strlen(optarg)}; 
	  }
	  break; 
	case 't' : 
	  {
	    if(std::string{"MRE"}.compare(optarg) == 0 
	       || std::string{"mre"}.compare(optarg) == 0)
	      {
		thresh = 50; 
		isRefined = true; 
	      }
	    else 
	      {
		auto &&iss = std::istringstream{optarg}; 
		iss >> thresh	; 
		isRefined = false; 
	      }
	  }
	  break; 
	case 'f': 
	  {
	    int index = optind -1; 
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
	case 'b': 
	  {
	    auto &&iss = std::istringstream {optarg}; 
	    iss >> burnin; 
	  }
	  break; 
	default: 
	  {
	    std::cerr << "Unrecognized option >" <<  optarg << "<"  << std::endl; 
	    exitFunction(-1, false); 
	  }
	}
    }

  if(thresh < 50 || thresh > 100 )
    {
      std::cerr << "error: correct values for -t in [50,100] or MRE." << std::endl; 
      exitFunction(-1, false); 
    }

  if(files.size() == 0 )
    {
      std::cerr << "Please specfiy tree input files via -f" << std::endl; 
      exitFunction(-1, false); 
    }
  if(id.compare("") == 0)
    {
      std::cerr << "Please specify a runid for the output via -n. " << std::endl; 
      exitFunction(-1, false); 
    }
 
  if(not (  0 <= burnin   &&  burnin < 1. ))
    {
      std::cerr << "The relative burn-in range is [0,1)." << std::endl;
      exitFunction(-1, false); 
    }


  return std::make_tuple(id, files, burnin, thresh, isRefined);
}




static void myExit(int code, bool waitForAll)
{
  exit(code); 
}




int main(int argc, char **argv)
{
  exitFunction = myExit; 

  NUM_BRANCHES = 1;   // BAD

  if(argc < 2 ) 
    {
      printUsage(std::cout);
      exitFunction(-1, false); 
    }

  bool isMre = false;   
  auto threshold = double{0.}; 
 
  std::string id= {}; 
  double burnin = 0; 
  auto files = std::vector<std::string>{}; 

  std::tie(id, files, burnin, threshold, isMre) = processCommandLine(argc, argv);

  for(auto &file : files)
    {
      if(not std::ifstream(file))
	{
	  std::cerr << "Error: could not open file >" << file << "<" << std::endl; 
	  exitFunction(-1, false); 
	}    
    }

  assert(threshold > 1); 
  threshold /= 100.; 

  auto&& ct = ConsensusTree(files, burnin, threshold,isMre); 
  auto header = ct.getTreeHeader(); 
  auto result = ct.getConsensusTreeString(false);
  auto type = ct.getType();

  auto&& ss = std::stringstream{}; 
  ss << PROGRAM_NAME << "_" << type << "Nexus." << id; 

  auto &&ss2 = std::ostringstream{}; 
  ss2 << PROGRAM_NAME << "_" << type << "Newick." << id ; 

  if(std::ifstream(ss.str()))
    {
      std::cerr << std::endl << "File " << ss.str() << " already exists (probably \n"
		<< "left over from a previous run). Please choose a new run-id or remove\n"
		<< "previous output files." << std::endl; 
      exitFunction(-1, false); 
    }

  auto &&outfile = std::ofstream(ss.str()); 
  outfile << header  << result << std::endl; 
  outfile << "end;" << std::endl; 


  auto &&outfile2 = std::ofstream{ss2.str()}; 
  outfile2 << ct.getConsensusTreeString(true) << std::endl; 


  std::cout << "Printed consensus tree in nexus format to " << ss.str() << std::endl; 
  std::cout << "Printed consensus tree in newick format to " << ss2.str() << std::endl; 

  return 0; 
}
