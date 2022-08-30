#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_REREQ([2.67])
AC_INIT([exabayes],[1.5.1],[exabayes-at-googlegroups-dot-com])

m4_include([./m4/ax_cxx_compile_stdcxx_11.m4])
m4_include([./m4/lx_find_mpi.m4])


AC_CONFIG_SRCDIR([src/])

dnl todo-ish 
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_HEADERS([src/config.h])


AC_DEFINE([PROGRAM_NAME], ["ExaBayes"], ["name used for log files"])

AM_SILENT_RULES([yes])


# Checks for programs.

OLD_CXXFLAGS=$CXXFLAGS
OLD_CFLAGS=$CFLAGS
AC_PROG_CC( [gcc icc] )
AC_PROG_CXX( [g++ icpc] )
CXXFLAGS=$OLD_CXXFLAGS
CFLAGS=$OLD_CFLAGS

dnl TODO: could we remove this one? 
AC_C_LONG_LONG

AX_CXX_COMPILE_STDCXX_11([noext], [mandatory])

AX_CXX_COMPILE_STDCXX(14, [noext], [optional])

AC_PROG_CC_STDC
AC_PROG_CC_C99
AC_PROG_RANLIB

dnl NOTICE if you have trouble with that, then install autoconf-archive (or similar)

AX_EXT 

AX_PTHREAD


AC_ARG_VAR([MPICXX],[sets the MPI c++ compiler ])


# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h stdint.h stdlib.h string.h strings.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([clock_gettime getcwd getpagesize gettimeofday memset pow sqrt strcasecmp strchr])




dnl disallow to use sse3 
AC_ARG_ENABLE( [sse], 
	       [  --disable-sse    compiles ExaBayes without any vectorization (slower), even if available], 
 	       [ case $enableval in 
	       no ) GUESS_SSE="yes" ;; 
 	       yes ) GUESS_SSE="no" ;; 
	       * )  GUESS_SSE="no"
 	       esac ] ,
	       [GUESS_SSE="no"]
 )
AM_CONDITIONAL([NO_SSE], [ test "x$GUESS_SSE" = "xyes" ])

dnl allow to disable avx 
AC_ARG_ENABLE( [avx],
	       [  --disable-avx    compiles ExaBayes with SSE support (slower) only, altough AVX is available ], 
[ case $enableval in 
  no ) GUESSAVX="yes" ;; 
  yes ) GUESSAVX="no" ;;
  * ) GUESSAVX="no"
  esac ] , 
  [GUESSAVX="no"]
  )
AM_CONDITIONAL([NO_AVX], [ test "x$GUESSAVX" = "xyes" ])

AC_ARG_ENABLE( [mpi], 	      
	   [  --enable-mpi     compiles ExaBayes with MPI support (optionally use MPICXX, to set a non-default MPI compiler ) ], 
[  case "${enableval}" in 
   no )  MPI_CODE="no" ;; 
   yes ) MPI_CODE="yes" ;; 
   *) MPI_CODE="no" 
   esac ] ,
[MPI_CODE="no"]
)


dnl check, if we are an intel compiler 

INTEL=no
AC_TRY_COMPILE( [], 
[ 
  #ifndef __INTEL_COMPILER 
  #error "checked for intel compiler without success" 
  #endif 
], 
[ INTEL="yes" ]
)
AM_CONDITIONAL( [WITH_INTEL], [ test "x$INTEL" = "xyes" ] ) 



AC_ARG_ENABLE( [mic],
	       [  --enable-mic     compiles the PLL with support for Intel accellerators (Xeon Phi/Intel MIC)  ],
	       [  case "${enableval}" in 
	             no )  MIC_CODE="no" ;; 
		     yes ) MIC_CODE="yes" ;; 
                     *) MIC_CODE="no" 
	          esac ] ,
	       [MIC_CODE="no"]
	        )
AM_CONDITIONAL([WITH_MIC], [ test "x$MIC_CODE" = "xyes" ])



dnl enable google tests 
AC_ARG_ENABLE( [tests], 
	   [  --enable-tests   enable automated tests (developer only) ], 
	   [   case "${enableval}" in 
	       no ) CODE="no" ;; 
	       yes )  CODE="yes" ;; 
 	       *) CODE="no"
	       esac ], 
  	   [CODE="no"]
	)
AM_CONDITIONAL([WITH_TEST], [ test "x$CODE" = "xyes" ])

dnl test, if we have PLL AND force the user to provide a mpi-comiler, if we dont
if test "x$MPI_CODE" = "xyes" ; then 
   AC_LANG_PUSH([C++])	 
   LX_FIND_MPI([], AC_MSG_ERROR(could not find mpi))

   AC_LANG_POP([C++])	 
fi 

AM_CONDITIONAL([WITH_MPI], [ test "x$MPI_CODE" = "xyes" ])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
