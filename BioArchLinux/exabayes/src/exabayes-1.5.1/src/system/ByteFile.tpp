#include <cassert>
#include "GlobalVariables.hpp"


template<typename T> 
T ByteFile::readVar()
{
  T result ; 
  _in.read((char*)&result, sizeof(T)  ); 
  auto bytesRead = _in.gcount();
  assert(bytesRead == std::streamsize(sizeof(T))); // 
  if(bytesRead != std::streamsize(sizeof(T)))
    {
      std::cout << "read " << bytesRead << " expected "  << sizeof(T) << std::endl; 
      assert(0); 
    }

  return result; 
}

template<typename T> 
void ByteFile::readArray(size_t length, T* array)
{
  _in.read((char*)array, sizeof(T) * length); 
  auto bytesRead = _in.gcount();
  
  if(size_t(bytesRead) != size_t(sizeof(T) * length))
    {
      std::cout << "problem reading " << length * sizeof(T) << " instead got " << SHOW(bytesRead) << std::endl; 
       assert(0);
    }
} 


template<typename T>
std::vector<T> ByteFile::readArray(size_t length)
{
  auto result = std::vector<T>(length); 
  readArray(length, result.data());
  return result; 
}
