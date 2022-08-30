#ifndef _MY_123_MPI_TYPE_HPP
#define _MY_123_MPI_TYPE_HPP

#pragma GCC system_header
#include <mpi.h>

template<typename T>  
struct mpiType 
{
  static MPI_Datatype value ;
}; 


enum class MPI_OP_TYPE : int 
{
  SUM = 1 , 
    LAND = 2 
} ; 


template<MPI_OP_TYPE op>
struct mpiOp
{
  static MPI_Op value; 
}; 

#endif






