#include <gtest/gtest.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "system/extensions.hpp"
#include "system/GlobalVariables.hpp"

void initLogFile( )
{
  // globals.logFile = std::string{"/dev/null"}; 
  globals.logStream = make_unique<std::ofstream>(std::string{"/dev/null"}); 
  globals.teeOut =  make_unique<TeeStream>(std::cout, *globals.logStream, MY_TID);
}


// TODO 
// #ifdef _WITH_MPI
// #include <mpi.h>
// #endif

// #include "TopLevelInvocation.cpp" // 

// #include "BipartitionTest.cpp"
#include "tests/PartitionAssignmentTest.cpp"
// #include "TreeAlnTest.cpp"

#include "MessageQueueTest.cpp"
#include "LocalCommTest.cpp"
#include "brentTest.cpp"


int main (int argc, char **argv)
{
  int result = 0; 

  testing::InitGoogleTest(&argc, argv); 

  initLogFile(); 

  
// #ifdef _WITH_MPI  
//   MPI_Init(&argc, &argv); 
  
//   int myRank = 0;  
//   MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
//   if(myRank > 0 )
//     {
//       // hack to silent remaining processes  
//       int bak, newOne; 
//       fflush(stdout); 
//       bak = dup(1); 
//       newOne = open("/dev/null", O_WRONLY) ; 
//       dup2(newOne,1); 
//       close(newOne); 
//     }

//   result = RUN_ALL_TESTS(); 
//   MPI_Finalize(); 
// #else 
  result = RUN_ALL_TESTS(); 
// #endif
  
  return result; 
}
