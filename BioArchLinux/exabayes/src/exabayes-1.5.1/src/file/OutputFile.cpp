#include "OutputFile.hpp"
#include "GlobalVariables.hpp"

#include <fstream>
#include <iostream>


void OutputFile::rejectIfExists(std::string fileName)
{
  if( std::ifstream(fileName) ) 
    {
      std::cerr << std::endl <<  "File " << fileName << " already exists (probably \n"
		<< "from previous run). Please choose a new run-id or remove previous output files. " << std::endl; 
      exitFunction(-1, false);
    }
}

void OutputFile::rejectIfNonExistant(std::string fileName)
{
  if( not std::ifstream(fileName) )      
    {
      std::cerr << "Error: could not find file from previous run. \n"
		<< "The assumed name of this file was >" <<  fileName << "<. Aborting." << std::endl; 
      exitFunction(0, true); 
    }
}

std::string OutputFile::getFileBaseName(std::string workdir )
{
  auto &&ss = std::stringstream{}; 
  ss << workdir << ( workdir.compare("") == 0  ? "" : "/") << PROGRAM_NAME; 
  return ss.str(); 
}

bool OutputFile::directoryExists(std::string name)
{
  struct stat st;
  if(stat(name.c_str(),&st) == 0)
    if((st.st_mode & S_IFDIR) != 0)
      return true; 
  return false; 
}


void OutputFile::removeMe() const 
{
  if(fullFileName.compare("") != 0 )
    remove(fullFileName.c_str() ); 
}
