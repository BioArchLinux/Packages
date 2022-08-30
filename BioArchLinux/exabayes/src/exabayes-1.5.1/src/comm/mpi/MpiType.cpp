#include "common.h"

#include "MpiType.hpp"
#include <cstdint>


#ifndef MPI_UINT8_T 
#define MPI_UINT8_T MPI_UNSIGNED_CHAR
#endif
#ifndef MPI_UINT16_T
#define MPI_UINT16_T MPI_UNSIGNED_SHORT
#endif
#ifndef MPI_UINT32_T
#define MPI_UINT32_T MPI_UNSIGNED
#endif
#ifndef MPI_UINT64_T
#define MPI_UINT64_T MPI_UNSIGNED_LONG
#endif

#ifndef MPI_INT8_T 
#define MPI_INT8_T MPI_CHAR
#endif
#ifndef MPI_INT16_T
#define MPI_INT16_T MPI_SHORT
#endif
#ifndef MPI_INT32_T
#define MPI_INT32_T MPI_INTEGER
#endif
#ifndef MPI_INT64_T
#define MPI_INT64_T MPI_LONG
#endif


template<> MPI_Datatype mpiType<double>::value = MPI_DOUBLE; 
// template<> MPI_Datatype mpiType<unsigned long long>::value = MPI_UNSIGNED_LONG; 
template<> MPI_Datatype mpiType<char>::value = MPI_CHAR; 

template<> MPI_Datatype mpiType<uint8_t>::value = MPI_UINT8_T; 
template<> MPI_Datatype mpiType<uint16_t>::value = MPI_UINT16_T; 
template<> MPI_Datatype mpiType<uint32_t>::value = MPI_UINT32_T; 
template<> MPI_Datatype mpiType<uint64_t>::value = MPI_UINT64_T; 

template<> MPI_Datatype mpiType<int8_t>::value = MPI_INT8_T; 
template<> MPI_Datatype mpiType<int16_t>::value = MPI_INT16_T; 
template<> MPI_Datatype mpiType<int32_t>::value = MPI_INT32_T; 
template<> MPI_Datatype mpiType<int64_t>::value = MPI_INT64_T; 

// unused 
template<> MPI_Op mpiOp<MPI_OP_TYPE::SUM>::value = MPI_SUM; 
template<>MPI_Op mpiOp<MPI_OP_TYPE::LAND>::value = MPI_LAND; 
