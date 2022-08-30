#include "extensions.hpp"
#include <cassert>


void formatRange(std::ostream &out, const std::vector<nat> &values) 
{
  bool inRange = false; 
  for(auto iter = begin(values); iter != end(values) ; ++iter )
    {
      if(iter == begin(values))
	out << *iter; 
      else if(iter == end(values) -1 )
	{
	  if(inRange)
	    out << "-" << *iter ;  
	  else 
	    out << "," << *iter; 
	}
      else 
	{ 
	  bool haveBeenInRange = ((iter-1) ==  begin(values)) || inRange; 
	  inRange = *iter == *(iter-1) + 1;  

	  if(not haveBeenInRange )
	    {
	      out << "," << *iter ; 
	    }
	  else
	    {
	      if(not inRange)
		out << "-" << *iter; 
	    }
	}
    }
  
}


template<typename T, int ALIGN>
T* aligned_malloc( size_t size )
{
  T *ptr = NULL; 
  int res = posix_memalign((void**)&ptr, ALIGN, sizeof(T) * size); 
  assert(res == 0); 
  assert(ptr != NULL); 
  return ptr; 
}
template int* aligned_malloc<int,size_t(EXA_ALIGN)>( size_t size ) ; 
template nat* aligned_malloc<nat,size_t(EXA_ALIGN)>( size_t size ) ; 
template unsigned char* aligned_malloc<unsigned char,size_t(EXA_ALIGN)>( size_t size ) ; 
template double* aligned_malloc<double,size_t(EXA_ALIGN)>( size_t size ) ; 



std::string getEnvironmentVariable( const std::string  & key ) 
{
  char * val;                                                                        
  val = getenv( key.c_str() );                                                       
  std::string retval = "";                                                           
  
  if (val != NULL) 
    { 
      retval = val;                                                                    
    } 
  
  return retval;                                                                        
}
