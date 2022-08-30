#include <unistd.h>
#include <iostream>
#include <cstring>
#include <iostream>

#include "CommandLine.hpp"	
#include "GlobalVariables.hpp"
#include "OutputFile.hpp"
#include "MemoryMode.hpp"
#include "FlagType.hpp"


CommandLine::CommandLine()
  : seed({{0,0}})
  , configFileName("")
  , alnFileName("")
  , runid("")
  , treeFile("")
  , workDir("")
  , runNumParallel(1)
  , chainNumParallel(1)
  , checkpointId("")
  , memoryMode(MemoryMode::RESTORE_ALL)
  , saveMemorySEV(false)
  , dryRun (false)
  , modelFile("")
  , singleModel("")
  , quiet{false}
  , readerStride(-1)
  , _cmdString("")
  , _totalThreads(-1)
  , _hasThreadPinning{true}
  , _onlyPrintHelp(false)
  , _onlyPrintVersion(false)
  {
  
    seed.v[0] = 0; 
    seed.v[1] = 0; 
  }


std::string CommandLine::getCommandLineString() const 
{
  return _cmdString; 
}

void CommandLine::initialize(  int argc, char **argv)
{
  // first copy the command line string 
  for(int i = 0; i < argc; ++i)
    {
      _cmdString += std::string(argv[i], argv[i] + strlen(argv[i])); 
      _cmdString += " "; 
    }

  int c ; 

  // TODO threads/ processes? 
  
  while( (c = getopt(argc,argv, "c:df:vhn:w:s:t:R:r:M:C:m:Sq:zxT:")) != EOF)
    {
      try
	{	  
	  switch(c)
	    {
	    case 'z': 
	      {
		quiet = true; 
	      }
	      break; 
	    case 'c': 		// config file 	  
	      {
		configFileName = std::string(optarg); 
		assertFileExists(configFileName);
	      }
	      break; 
	    case 'f': 		// aln file 
	      alnFileName = std::string(optarg); 
	      assertFileExists(alnFileName); 
	      break; 
	    case 'v':  		// version 
	      _onlyPrintVersion = true; 
	      break; 
	    case 'd': 
	      dryRun = true; 
	      break; 
	    case 'h': 		// help 
	      _onlyPrintHelp = true; 
	      break; 
	    case 'n': 		// runid 
	      runid = std::string(optarg); 	  
	      break; 
	    case 't': 		// trees -- have that in the config file? 
	      treeFile = std::string(optarg); 
	      break; 
	    case 'w':		// working dir  
	      workDir = std::string(optarg); 
	      break; 
	    case 's': 		// seed 
	      seed.v[0] = std::stoi(optarg);
	      break; 
	    case 'r': 
	      checkpointId = std::string{optarg};   
	      break; 
	    case 'q':
	      modelFile = std::string{optarg}; 
	      break; 
	    case 'm': 
	      singleModel = std::string{optarg}; 
	      break; 
	    case 'S': 
	      saveMemorySEV = true; 
	      break; 
	    case 'M': 
	      memoryMode = MemoryMode(std::stoi(optarg)); 
	      break; 
	    case 'C': 
	      chainNumParallel = std::stoi(optarg); 
	      break; 
	    case 'R': 
	      runNumParallel = std::stoi(optarg);
	      break; 
	    case 'T': 
	      _totalThreads = std::stoi(optarg); 
	      break; 
	      // case 'x': 
	      //   readerStride = std::stoi(optarg); 
	      //   break; 
	    case 'x': 
	      _hasThreadPinning = false; 
	      break;
	    default: 
	      {
		std::cerr << "Encountered unknown command line option -" <<  char(c) 
			  << "\n\nFor an overview of program options, please use -h" << std::endl ; 
		// TODO mpi-finalize stuff 
		exitFunction(-1, true); 
	      }
	    }
	}
      catch(const std::invalid_argument& ia)
	{
	  std::cerr << "Invalid argument >" << optarg << "< to option >" << reinterpret_cast<char*>(&c) << "<" << std::endl; 
	  exitFunction(-1, true);
	}
    }  
  
  if(_onlyPrintHelp || _onlyPrintVersion)
    return; 


  if(isYggdrasil
     && (_totalThreads % (runNumParallel * chainNumParallel)) != 0 )
    {
      std::cerr << "Error: for the threaded version it is currently necessary that the number of threads is a multiple of the product of the number of chains and runs that is executed in parallel." << std::endl; 
      exitFunction(-1, true); 
    }


  if(runid.compare("") == 0 )
    {
      std::cerr << "please specify a runid with -n runid" << std::endl; 
      exitFunction(-1, true); 
    }
  
  if(seed.v[0] != 0 && checkpointId.compare("") != 0 )
    {
      std::cout << std::endl << "You provided a seed and run-id for a restart from a checkpoint.\n"
		<< "Please be aware that the seed will be ignored." << std::endl; 
    }

  if(checkpointId.compare("") == 0 && seed.v[0] == 0 )
    {
      std::cerr << "please specify a seed via -s seed (must NOT be 0)"   << std::endl; 
      exitFunction(-1, true); 
    }

  if(workDir.compare("") != 0 && not OutputFile::directoryExists(workDir))
    {
      std::cout << std::endl << "Could not find the provided working directory >" << workDir << "<" << std::endl; 
      exitFunction(-1, true);
    }

  if(alnFileName.compare("") == 0 )
    {
      std::cerr << "please specify an alignment file via -f file" <<  std::endl 
		<< "You have to transform your NEWICK-style alignment into a binary file using the appropriate parser (see manual)." << std::endl; 

      exitFunction(-1, true); 
    }

  if(alnFileIsBinary())
    {
      if(singleModel.compare("") != 0 || modelFile.compare("") != 0 )
	{
	  std::cout << "Found binary alignment file. Additionally, you provided a model file\n"
	    "(-q) or specified a data type for a single partiton. This information\n"
	    "will be ignored.\n"; 
	  modelFile = ""; 
	  singleModel = ""; 
	}
    }
  else 
    {
      if(singleModel.compare("") == 0 && ( modelFile.compare("") == 0 || not std::ifstream(modelFile) )  )
	{
	  std::cout << "Found a phylip-style alignment file. However, you did not provide a\n"
                    << "model file (see -q, resp. it could not be found) or a data type specification for a single\n"
		    << "partition (-m). Cannot proceed.\n" ; 
	  exitFunction(-1, true); 
	}
    }

  if( treeFile.compare("") != 0 && not std::ifstream(treeFile))
    {
      std::cout << "Could not find tree file passed via -t >"  << treeFile << "<"<< std::endl; 
      exitFunction(-1, true); 
    }
}




void CommandLine::printVersion(std::ostream& out )
{   
  out  << "This is " << PROGRAM_NAME << ", version " << PACKAGE_VERSION << std::endl
       << "================================================================\n\n" 
       << "For bugs reports and feature inquiries, please send an email to " << PACKAGE_BUGREPORT << std::endl; 
}


void CommandLine::printHelp(std::ostream& out)
{
  printVersion(out); 

  out << std::endl << "Usage: " << (isYggdrasil ? "./yggdrasil" : "./exabayes"  ) <<  " -f alnFile [ -q modelFile ] [ -m model ] [ -s seed | -r id ]  -n id [options..] "
      << std::endl; 

  
  out << "\n\n"
      << "================================================================\n"
      << "Mandatory Arguments: \n"
      << "================================================================\n\n"
      << "    -f alnFile       a alignment file (either binary and created by parser or plain-text phylip)\n\n"
      << "    -s seed          a master seed for the MCMC\n\n"
      << "    -n ruid          a run id\n\n" 
      << "    -r id            restart from checkpoint. Just specify the id of the previous run (-n) here. \n"
      << "                       Make sure, that all files created by the previous run are in the working directory.\n"
      << "                       This option is not mandatory for the start-up, seed (via -s) will be ignored.\n\n"
      << "    -q modelfile     a RAxML-style model file (see manual) for multi-partition alignments. Not needed \n"
      << "                       with binary files.\n\n"
      << "    -t treeFile      a file containing starting trees (in Newick format) for chains. If the file provides less\n"
      << "                       starting trees than chains to be initialized, parsimony/random trees will be used for\n"
      << "                       remaining chains. If a tree contains branch lengths, these branch lengths will be used\n"
      << "                       as initial values.\n\n"
      << "    -m model         indicates the type of data for a single partition non-binary alignment file\n" 
      << "                       (valid values: BIN, DNA or PROT)\n\n"
      << std::endl;     

  out <<      "\n" 
      << "================================================================\n"
      <<  "Options:\n" 
      << "================================================================\n\n"
      << "    -v               print version and quit\n\n"
      << "    -h               print this help\n\n" 
      << "    -z               quiet mode. Substantially reduces the information printed by " << PROGRAM_NAME << ".\n"
      << "                      This option will save you some idle time, when you run " << PROGRAM_NAME << " with a\n"
      << "                      lot of processes.\n\n" 
      << "    -d               execute a dry-run. Procesess the input, but does not execute any sampling.\n\n"
      << "    -c confFile      a file configuring your " << PROGRAM_NAME << " run. For a template see the 'examples' folder.\n\n"
      << "    -w dir           specify a working directory for output files\n\n"; 

  out << "    -R num           the number of runs (i.e., independent chains) to be executed in parallel\n\n"
      << "    -C num           number of chains (i.e., coupled chains) to be executed in parallel\n\n" 
      << "    -x               disables thread pinning (which schedules a thread to a cpu core\n"
      << "                       in a fixed way). You should try this function, if you notice load imbalances with the\n"
      << "                       threaded version of the code (i.e., using -T x).\n\n" ;  
  
  out << "    -S               try to save memory using the SEV-technique for gap columns on large gappy alignments\n" 
      << "                       Please refer to  http://www.biomedcentral.com/1471-2105/12/470\n" 
      << "                       On very gappy alignments this option yields considerable runtime improvements. \n\n"
      << "    -T x             start x threads per MPI process. If you do not use MPI, simply start x threads. \n\n" 
      << "    -M mode          specifies the memory versus runtime trade-off (see manual for detailed discussion).\n"
      << "                       <mode> is a value between 0 (fastest, highest memory consumption) and 3 (slowest,\n"
      << "                       least memory consumption)\n\n"
      << std::endl; 
}


void CommandLine::assertFileExists(std::string filename)
{
  auto &&in = std::ifstream{filename}; 
  if( not in  )
    {
      std::cerr << "could not file file " << filename << ". Aborting." << std::endl; 
      exitFunction(-1, true); 
    }
}


randCtr_t CommandLine::getSeed() const
{
  return seed ; 
} 


bool CommandLine::alnFileIsBinary() const
{
  auto&& in =std::ifstream (alnFileName, std::ios::binary); 

  assert(in) ; 

  auto fileId = std::string{"BINARY"} ; 
  char firstBytes[7]; 
  memset(firstBytes, '\0', 7 * sizeof(char)); 
  in.read(firstBytes, 6 * sizeof(char));
  auto readString = std::string(firstBytes); 

  bool result = readString.compare(fileId) == 0 ;

  return result; 
}  


