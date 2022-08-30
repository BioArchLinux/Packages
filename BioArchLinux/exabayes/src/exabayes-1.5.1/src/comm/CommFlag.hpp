#ifndef COMM_FLAG_HPP
#define COMM_FLAG_HPP

/** 
    @brief indicates how much communication is necessary     
 */ 
enum class CommFlag : int
{
  NOTHING = 0, 
    PRINT_STAT    =  1 , 		// stats for printing  
    PROPOSALS     =  1 << 1  , 		// all proposal data  
    TREE          =  1 << 2  ,		// the tree state 
    SWAP          =  1 << 3 ,     		// swap matrix 
    RAND          = 1 << 4  
}; 

#endif
