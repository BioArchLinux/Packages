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
//

// This code is based on code developed by Mark Holder for the CIPRES project
// Much of this file comes from Andrei Alexandrescu "Modern C++ Design"

#if !defined NXS_UTIL_COPY_H
#define NXS_UTIL_COPY_H
#include <algorithm>
#include <cstring>

#if defined(_MSC_VER)
#	undef	HAVE_COMPILE_TIME_DISPATCH
#else
#	define HAVE_COMPILE_TIME_DISPATCH
#endif

////////////////////////////////////////////////////////////////////////////////
///	Int2Type<compile time constant integer> defines a unique (and stateless)
///		class associated with a given integer.  Used for compile time dispatching
///		of function calls or creation of appropriate templated classes.
///
///	defines an unnamed enum "value" that is equal to the integer used to
///		define the class.
///	\author	Andrei Alexandrescu "Modern C++ Design"
//////////

template<int v>
class Int2Type
	{
		public:
			enum {value = v};
	};

typedef Int2Type<true>  TrueAsAType;
typedef Int2Type<false> FalseAsAType;

////////////////////////////////////////////////////////////////////////////////
///	Type2Type<typename> defines a unique (and stateless) class for each type
///		that is specified as the template argument.
///	This is useful in controlling the return type of templated functions in
///		lieu of partial template specialization of templated functions (which is
///		not allowed by the C++ standard)
///	Defines the typedef OriginalType which corresponds to the template argument
///	\author	Andrei Alexandrescu "Modern C++ Design"
//////////

template<typename T>
class Type2Type
	{
		public:
			typedef T OriginalType;
	};


namespace ncl
{
namespace hidden
{
// used by #COMPILE_TIME_ASSERT
template<bool> struct CompileTimeChecker
	{
	CompileTimeChecker(...); //default
	};
// used by #COMPILE_TIME_ASSERT
template<> struct CompileTimeChecker<false>{};
}
}

////////////////////////////////////////////////////////////////////////////////
///	\def COMPILE_TIME_ASSERT(condition, msg)
///	\brief A error-detection macro for asserting that conditions are true at compile time.
///
///	Usage:
///
///		COMPILE_TIME_ASSERT(condition to test, error_clue)
///
///	\note	The error_clue must be one alphanumeric word (no spaces of punctaution).
///	The condition to test must be known at compile time (if not a cryptic message such as "illegal non-type template argument"
///	error will be generated.
///	If the compile time assertion evaluates to false, a message such as "Illegal conversion
///		from ERROR_error_clue to CompileTimeChecker<false> ..." will be generated.
///
///	Implementation Details:
///
///		ncl::hidden::CompileTimeChecker is a boolean-templated class.
///		The constructor of ncl::hidden::CompileTimeChecker<true> accepts any type
///		The the only constructor for ncl::hidden::CompileTimeChecker<false> is the default constructor
///		The macro COMPILE_TIME_ASSERT(condition, msg):
///			1	Declares a dummy class ERROR_msg
///			2	checks if it can instantiate CompileTimeChecker<condition> from an ERROR_msg type object.
///		If the condition is true, the construction will succeed, if not the error message will be generated
///		Note that while the  CompileTimeChecker exists in ncl::hidden:: namespace.  The macro is visible
///		by any file that includes compile_assert.h and the class such as ERROR_msg will be added to the global
///		namespace.
///		The resultign code is not affected by the insertion of COMPILE_TIME_ASSERT because the entire construction
///		is inside a sizeof() so no objects are really instantiated.
///
///	\author	Andrei Alexandrescu "Modern C++ Design"
///
//////////
#define COMPILE_TIME_ASSERT(condition, msg) {class ERROR_##msg {}; (void)sizeof(ncl::hidden::CompileTimeChecker<(condition)>(ERROR_##msg()));}

#if defined(HAVE_COMPILE_TIME_DISPATCH)
template<typename T> struct SupportsBitwiseCopy { enum {kResult = false};	};
template<typename T> struct SupportsBitwiseCopy<T*> {	enum {kResult = true}; 	};
template<> struct SupportsBitwiseCopy<short int> {	enum {kResult = true}; 	};
template<> struct SupportsBitwiseCopy<int> {	enum {kResult = true}; 	};
template<> struct SupportsBitwiseCopy<char> {	enum {kResult = true}; 	};
template<> struct SupportsBitwiseCopy<long int> {	enum {kResult = true}; 	};
template<> struct SupportsBitwiseCopy<double> {	enum {kResult = true}; 	};
template<> struct SupportsBitwiseCopy<unsigned short int> {	enum {kResult = true}; 	};
template<> struct SupportsBitwiseCopy<unsigned int> {	enum {kResult = true}; 	};
template<> struct SupportsBitwiseCopy<unsigned char> {	enum {kResult = true}; 	};
template<> struct SupportsBitwiseCopy<unsigned long int> {	enum {kResult = true}; 	};
template<> struct SupportsBitwiseCopy<bool> {	enum {kResult = true}; 	};
template<> struct SupportsBitwiseCopy<wchar_t> {	enum {kResult = true}; 	};
template<> struct SupportsBitwiseCopy<float> {	enum {kResult = true}; 	};
template<> struct SupportsBitwiseCopy<long double> {	enum {kResult = true}; 	};


//	This file uses tricks discussed in Andrei Alexandrescu's book to
//	implement a call to memcpy for primitive types or any class which
//  has the statement template<> struct SupportsBitwiseCopy<CLASS> {	enum {kResult = true}; 	};
//	because of potential portability issues with TypeList, primitive types are
//	have SupportsBitwiseCopy specialized here by brute force enumeration


class NullType {};

template <typename T>
class TypeTraits
	{
	private :
		template <class U> struct PointerTraits
			{
				enum {kResult = false};
				enum {kCopyWithMemCopy = false};	// only allowing memcpy on bare pointers
				enum {kSizeOfPointee = 0};	// only allowing memcpy on bare pointers
			};
		template <class U> struct PointerTraits<U*>
			{
				enum {kResult = true};
				enum {kCopyWithMemCopy = SupportsBitwiseCopy<U>::kResult};
				enum {kSizeOfPointee = sizeof(U)};
			};
		template <class U> struct PointerTraits<const U*>
			{
				enum {kResult = true};
				enum {kCopyWithMemCopy = SupportsBitwiseCopy<U>::kResult};
				enum {kSizeOfPointee = sizeof(U)};
			};
	public:
		enum {kIsPointer = PointerTraits<T>::kResult};
		enum {kCanUseMemCpyOnPointee = PointerTraits<T>::kCopyWithMemCopy};
		enum {kPointeeSize = PointerTraits<T>::kSizeOfPointee}; //only valid if kIsPointer !!
	//	typedef PointerTraits<T>::PointeeType  PointeeType;
	};

template<class T, class U>
class Conversion
	{
	public:
		enum {kSameType = false};
	};

template<class T>
class Conversion<T,T>
	{
	public:
		enum {kSameType = true};
	};

template<class T>
class Conversion<const T*,T*>
	{
	public:
		enum {kSameType = true};
	};

template<class T>
class Conversion<T*, const T*>
	{
	public:
		enum {kSameType = true};
	};

enum CopyAlgoSeclector
	{
		kConservative,
		kFast
	};

template <typename InIt, typename OutIt>
inline OutIt CopyImpl(InIt first, InIt last, OutIt resultP, Int2Type<kConservative>)
	{
	return std::copy(first, last, resultP);
	}

template <typename InIt, typename OutIt>
inline OutIt CopyImpl(InIt first, InIt last, OutIt resultP, Int2Type<kFast>)
	{
	return (OutIt) std::memcpy(resultP, first,  ((std::size_t) (last - first)) * sizeof(*first));
	}

template <typename InIt, typename OutIt>
OutIt ncl_copy(InIt first, InIt last, OutIt resultP)
	{
		enum { kUseMemCpy =(TypeTraits<InIt>::kIsPointer &&
							TypeTraits<OutIt>::kIsPointer &&
							TypeTraits<InIt>::kCanUseMemCpyOnPointee &&
							TypeTraits<OutIt>::kCanUseMemCpyOnPointee &&
							int(TypeTraits<InIt>::kPointeeSize) == int(TypeTraits<OutIt>::kPointeeSize)) ? kFast : kConservative};
		return CopyImpl(first, last, resultP, Int2Type<kUseMemCpy>());
	}

#else //HAVE_COMPILE_TIME_DISPATCH

template <typename InIt, typename OutIt>
inline OutIt ncl_copy(InIt first, InIt last, OutIt resultP)
	{
	return std::copy(first, last, resultP);
	}

#endif //HAVE_COMPILE_TIME_DISPATCH


//adds an element from the first -> last array to the corresponding element in the result array
template <typename InIt, typename OutIt>
inline OutIt ncl_iadd(InIt first, InIt last, OutIt resultP)
	{
	for (; first != last; ++first, ++resultP)
		*resultP += *first;
	return resultP;
	}

//adds each element in resultP array with the correcpsonding element from the first -> last array
template <typename InIt, typename OutIt>
inline OutIt ncl_imult(InIt first, InIt last, OutIt resultP)
	{
	for (; first != last; ++first, ++resultP)
		*resultP *= *first;
	return resultP;
	}


#endif

