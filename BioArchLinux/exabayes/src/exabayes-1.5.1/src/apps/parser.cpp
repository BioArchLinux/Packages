#include <fstream>
#include <stdexcept>
#include <iostream>
#include <unistd.h>
#include <cstring>

#include "AlignmentPLL.hpp"

#include "GlobalVariables.hpp"

static void helpMessage()
{
  std::cout << "\nparser produces a binary output file, that can be fed into\n"
	    << "ExaBayes/Yggdrasil. This is recommendable for large runs with hundreds\n"
	    << "of processes.\n\n" ; 

  std::cout << "./parser -s alnFile -q modelFile -n outputFile\n"
	    << "./parser -s alnFile -m {DNA|PROT} -n outputFile\n\n\n" 
	    << "     -s alnFile             a phylip-style alignment file\n\n"
	    << "     -q modelFile           a RAxML-style model file\n\n"
	    << "     -m dataType            specifies a datatype (either DNA or PROT) for a\n"
	    << "                              single partition. Not needed, when a\n"
	    << "                              model file is given.\n\n"
	    << "     -n outputfile          name of the output file\n\n"
	    << std::endl; 
}

static void myExit(int code, bool waitForAll)
{
  exit(code); 
}


int main(int argc, char **argv)
{
  exitFunction = myExit; 

  if(argc < 2)
    {
      helpMessage(); 
      exitFunction(-1, false); 
    }
  
  auto alignmentFile = std::string(""); 
  auto modelFile = std::string(""); 
  auto singlePartitionModel = std::string(""); 
  auto outputFileName = std::string(""); 
  
  int c = 0 ; 
  while(  ( c  = getopt( argc, argv, "s:q:m:n:h") )  != EOF ) 
    {
      try
	{
	  switch(c)
	    {
	    case 's': 
	      alignmentFile = std::string(optarg); 
	      break; 
	    case 'q': 
	      modelFile = std::string(optarg); 
	      break; 
	    case 'm':
	      singlePartitionModel = std::string(optarg); 
	      break; 
            case 'h': 
              helpMessage(); 
              exitFunction(0, false); 
              break; 
	    case 'n': 
	      outputFileName = std::string(optarg); 
	      break; 
	    default:
	      {
		std::cerr << "Encountered unknown command line option " <<  c 
			  << "\n\nFor an overview of program options, please use -h" << std::endl ; 
		exitFunction(-1, false); 
	      }
	    }
	}
      catch(const std::invalid_argument& ia )
	{
	  std::cerr << "Invalid argument >" << optarg << "< to option >" << reinterpret_cast<char*>(&c) << "<" << std::endl; 
	  exitFunction(-1, false); 
	}
    }

  // some validation
  if(outputFileName.compare("") == 0 )
    {
      std::cout << "Please specify an output file name via -n." << std::endl; 
      exitFunction(-1, false); 
    }

  if(outputFileName.compare("") != 0 && std::ifstream(outputFileName + ".binary"))
    {
      std::cout << "Error: output file "  <<  outputFileName  << ".binary already exists." << std::endl; 
      exitFunction(-1, false); 
    }

  if(modelFile.compare("") == 0 && singlePartitionModel.compare("") == 0)
    {
      std::cout << "Please specify either a model file via -q or a data type for a single\n"
		<< "partition (either DNA or PROT) via -m." << std::endl; 
      exitFunction(-1, false); 
    }

  if(modelFile.compare("") != 0 && not std::ifstream{modelFile})
    {
      std::cout << "Error: model file provided, but could not open model file >"  << modelFile << "<" << std::endl; 
      exitFunction(-1, false); 
    }

  
  if(not std::ifstream{alignmentFile})
    {
      std::cout << "Error: could not open alignment file >" << alignmentFile << "<" << std::endl; 
      exitFunction(-1, false); 
    }

  
  bool useSinglePartition =  modelFile.compare("") == 0; 

  auto format = AlignmentPLL::guessFormat(alignmentFile);
  auto &&phyAln = AlignmentPLL{} ; 
  phyAln.initAln(alignmentFile, format);

  if(useSinglePartition)
    phyAln.createDummyPartition(getTypeFromString(singlePartitionModel)); 
  else 
    phyAln.initPartitions(modelFile); 

  phyAln.writeToFile(outputFileName + ".binary"); 

  std::cout << "wrote binary alignment file to " << outputFileName << ".binary" << std::endl; 
  
  return 0 ;
}

