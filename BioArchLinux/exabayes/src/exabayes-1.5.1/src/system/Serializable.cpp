#include "Serializable.hpp"
#include "GlobalVariables.hpp"
#include "log_double.hpp"


void Serializable::getOfstream(std::string name, std::ofstream &result)
{
  if(checkpointIsBinary)
    result.open(name, std::ios::binary); 
  else 
    result.open(name); 
}



void Serializable::getIfstream(std::string name, std::ifstream &result )
{
  if(checkpointIsBinary)
    result.open(name, std::ios::binary); 
  else 
    result.open(name); 
}



void Serializable::readDelimiter(std::istream &in)  
{
  char c; 
  in >> c ; 
  assert(c == DELIM); 
}


std::string Serializable::readString(std::istream &in )
{
  auto result = std::string{}; 

  if(checkpointIsBinary)
    {
      int length = 0; 
      in.read((char*)&length, sizeof(int)); 
      
      result.resize(length) ;
      char *aString  = new char[length]; 
      // auto aString = std::vector<char> (length, '\0');
      in.read(aString, length * sizeof(char));       
      result = std::string(aString); 
      delete [] aString; 
      // result = std::string(aString.begin(), aString.end());
    }
  else 
    {      
      getline(in, result, DELIM); 
    }
  return result; 
}

void Serializable::writeString(std::ostream &out, std::string toWrite) const 
{  
  if(checkpointIsBinary)
    {
      toWrite += '\0'; 
      int length = int(toWrite.size()); 
      out.write((char*)&length, sizeof(int)); 
      out.write(toWrite.c_str(), length * sizeof(char)); 
    }
  else  
    {
      out << toWrite << DELIM; 
    }
}




template<> log_double Serializable::cRead<log_double>(std::istream &in)  
{
  double tmp = cRead<double>(in); 
  return log_double::fromLog(tmp); 
}

template<> void Serializable::cWrite<log_double>( std::ostream &out, const log_double &val )  const 
{
  double tmp = (double)val.getRawLog(); 
  cWrite(out,  tmp); 
}





template<typename T>
void Serializable::cWrite(std::ostream &out, const T& toWrite) const 
{
  static_assert(std::is_fundamental<T>::value, "only serialization of fundamental types allowed."); 

  static_assert(not std::is_same<T, std::string>::value, "Do NOT use the cWrite funciton with strings (there is a specific function for that)");

  // std::cout << "wrote " << toWrite << std::endl; 
  
  if(checkpointIsBinary)
    {      
      out.write((const char*)&toWrite, sizeof(T)); 
      // std::cout << "WROTE " << toWrite << std::endl;
    }
  else 
    {
      out << std::scientific << MAX_SCI_PRECISION; 
      out << toWrite << DELIM; 
    }
}
template void Serializable::cWrite<int>(std::ostream &out, const int& toWrite) const ;
template void Serializable::cWrite<nat>(std::ostream &out, const nat& toWrite) const ;
template void Serializable::cWrite<long>(std::ostream &out, const long& toWrite) const ;
template void Serializable::cWrite<double>(std::ostream &out, const double& toWrite) const ;
template void Serializable::cWrite<unsigned long>(std::ostream &out, const unsigned long& toWrite) const ;
template void Serializable::cWrite<unsigned long long>(std::ostream &out, const unsigned long long& toWrite) const ;



template<typename T>
T Serializable::cRead(std::istream &in )
{
  static_assert(std::is_fundamental<T>::value, "only serialization of fundamental types allowed."); 
  
  T result; 

  static_assert(not std::is_same<T, std::string>::value, "Do NOT use the cRead funciton with strings (there is a specific function for that)");

  if(checkpointIsBinary)
    {
      in.read((char*)&result, sizeof(T)); 
      // std::cout << "READ " << result << std::endl; 
    }
  else 
    {
      in >> result; 
      // std::cout <<  "READ " << result << std::endl; 
      readDelimiter(in);
    }

  // std::cout << "read " << result << std::endl; 

  return result; 
}
template int Serializable::cRead(std::istream &in );
template nat Serializable::cRead(std::istream &in );
template long Serializable::cRead(std::istream &in );
template double Serializable::cRead(std::istream &in );
template unsigned long Serializable::cRead(std::istream &in );
template unsigned long long Serializable::cRead(std::istream &in );
