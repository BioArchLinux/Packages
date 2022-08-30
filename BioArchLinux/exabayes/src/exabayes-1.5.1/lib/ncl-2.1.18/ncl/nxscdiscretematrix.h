//	Copyright (C) 2008 Mark Holder
//
//	This file is part of NCL (Nexus Class Library) version 2.1
//
//	NCL is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	NCL is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with NCL; if not, write to the Free Software Foundation, Inc.,
//	59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

// This code is based on code developed by Mark Holder for the CIPRES project

#if !defined(NXS_C_DISCRETE_MATRIX_H)
#define NXS_C_DISCRETE_MATRIX_H


#if defined (HAVE_CONFIG_H)
#	include <config.h>
#endif

#if defined(_MSC_VER)
#	undef	HAVE_COMPILE_TIME_DISPATCH
#else
#	define HAVE_COMPILE_TIME_DISPATCH
#endif

/* For typedefs like uint8_t */
#if HAVE_INTTYPES_H
#	include <inttypes.h>
#elif HAVE_STDINT_H
#	include <stdint.h>
#elif defined(_MSC_VER) && _MSC_VER >= 1200
#	include <basetsd.h>
	typedef   INT8 int8_t;
	typedef  UINT8 uint8_t;
	typedef  INT64 int64_t;
	typedef UINT64 uint64_t;
#elif defined(_MSC_VER)
	typedef signed char int8_t;
	typedef unsigned char uint8_t;
	typedef long long int64_t;
	typedef unsigned long long uint64_t;
#elif defined(_WIN32)
#	include <stdint.h>
#endif

	/* For size_t */
#if defined(HAVE_STDDEF_H)
#	include <stddef.h>
#endif


#ifdef __cplusplus
extern "C"
{
#endif


typedef int8_t NxsCDiscreteState_t; /** type used to enumerate possible states.
								-1 is used for gaps, other negative flags may be added later.
								This size limits the maximum number of states allowed. */
typedef int8_t NxsCDiscreteStateSet; /** type used to refer to unique combinations of states (the "fundamental" states and ambiguity codes)
								-1 is used for gaps.  To handle all possible data sets, this must be large enough to hold
								2^(nStates + 1) values if the datatype allows gaps.  Thus using int8_t limits us to 8 states */

/*
The following enum is a cropping of the NxsCharactersBlock::DataTypesEnum
which includes all of the datatypes (and only those) that can be expressed
in a NxsCDiscreteMatrix. Each of the enum facets will have the same
value as in  NxsCharactersBlock::DataTypesEnum.

This enum is also handy because it is accessible via C.
*/
typedef enum {
			  NxsAltGeneric_Datatype = 1,
			  NxsAltDNA_Datatype = 2,
			  NxsAltRNA_Datatype = 3,
			  NxsAltNuc_Datatype = 4,
			  NxsAltAA_Datatype = 5,
			  NxsAltCodon_Datatype = 6
			  } NxsAltDatatypes;
const int LowestNxsCDatatype = 1;
const int HighestNxsCDatatype = 6;

typedef struct NxsCDiscreteMatrixStruct
	{
	NxsCDiscreteState_t 	  *stateList; 		/** Flattened array of array of observed states.  If more than one state was observed, then the first element is the number of states observed.
											  Exceptions: -1 is for gaps, nStates is for missing. */
	unsigned * stateListPos;  	/** Maps a state set code (the elements of the matrix) to the index in stateList where the states are listed */
	NxsCDiscreteStateSet ** matrix;			/** taxa x characters matrix of indices of state sets */
	const char * symbolsList;	/** array of the characters used to stand for each state ("ACGT?NRY" for example) //@temp paup depends on all symbols being unique (even ambiguity codes)*/
	unsigned nStates;
	unsigned nChar;
	unsigned nTax;
	unsigned nObservedStateSets; /* the length of stateListPos */
	NxsAltDatatypes datatype;
	} NxsCDiscreteMatrix;


#ifdef __cplusplus
}
#endif

#endif /* __TREEINFER_HELPER_H */

