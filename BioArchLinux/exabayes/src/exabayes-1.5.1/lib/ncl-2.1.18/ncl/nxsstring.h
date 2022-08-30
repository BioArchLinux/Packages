//	Copyright (C) 1999-2003 Paul O. Lewis and Mark T. Holder
//
//	This file is part of NCL (Nexus Class Library) version 2.0.
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

#ifndef NCL_NXSSTRING_H
#define NCL_NXSSTRING_H


#include <cstring>
#include <cstdio>
#include <functional>
#include <ostream>
#include <string>
#include "ncl/nxsdefs.h"





// Define HAVE_NCL_NXSSTRING_ENDL if your code needs it
#if ! defined (HAVE_NCL_NXSSTRING_ENDL)
#   define HIDE_NCL_NXSSTRING_ENDL
#else
#   warning "use of endl with NxsString instances has been deprecated"
#endif
class IndexSet;

/*!
	A string class for use with the Nexus Class Library. NxsString inherits most of its functionality from the standard
	template library class string, adding certain abilities needed for use in NCL, such as the ability to discern
	whether a short string represents an abbreviation for the string currently stored. Another important addition is
	the member function PrintF, which accepts a format string and an arbitrary number of arguments, allowing a string
	to be built in a manner similar to the standard C function printf. Many operators are also provided for appending
	numbers to the ends of strings, an ability which is very useful for producing default labels (e.g. taxon1, taxon2,
	etc.).
*/
class NxsString
  : public std::string
	{
	public:
		enum NxsQuotingRequirements
			{
			kNoQuotesNeededForNexus = 0, /// this enum value is kept equivalent to false
			kSingleQuotesNeededForNexus, /// punctuation or non-space whitespace characters present
			kUnderscoresSufficeForNexus  /// No nexus token-breakers
			};

		class NxsX_NotANumber {};	/* exception thrown if attempt to convert string to a number fails */
		static bool case_insensitive_equals(const char *o, const char * t);
		static bool to_long(const char *c, long *n);
		static bool to_double(const char *c, double *n);
		static std::string strip_leading_whitespace(const std::string & s);
		static std::string strip_trailing_whitespace(const std::string & s);
		static std::string strip_surrounding_whitespace(const std::string & s);
		static std::string strip_whitespace(const std::string & s);
		static void split(const std::string &s, std::list<std::string> *);
		static std::string get_upper(const std::string &s)
			{
			std::string t(s);
			to_upper(t);
			return t;
			}
		static std::string & to_lower(std::string & s);
		static std::string & to_upper(std::string & s);
		static NxsQuotingRequirements determine_quoting_requirements(const std::string &);
		static void blanks_to_underscores(std::string &s);
		static void add_nxs_quotes(std::string &s);
		static int index_in_vector(const std::string &t, const std::vector<std::string> &v);
		static int index_in_array(const std::string &t, const char * * v, const unsigned n);

		enum CmpEnum				/* enum that is used to specify string comparison modes */
			{
			respect_case,
			no_respect_case,
			abbrev
			};

							NxsString();
							NxsString(const char *s);
							NxsString(const NxsString &s);

		static std::string	GetEscaped(const std::string &s);
		static std::string	GetEscapedInt(const int &v);
		static std::string	GetEscapedDouble(const double &v);
		//	Accessors
		//
		bool				Abbreviates(const NxsString &s, NxsString::CmpEnum mode = NxsString::no_respect_case) const;
		unsigned			ConvertToUnsigned() const;
		int					ConvertToInt() const;
		long				ConvertToLong() const;
		double				ConvertToDouble() const;
		bool				Equals(const NxsString &s, NxsString::CmpEnum mode = respect_case) const;
		bool				EqualsCaseInsensitive(const NxsString &s) const;
		NxsString			GetQuoted() const;
		bool				IsADouble() const;
		bool				IsALong() const;
		bool				IsCapAbbreviation(const NxsString &s) const;
		bool				IsInVector(const NxsStringVector &s, NxsString::CmpEnum mode = respect_case) const;
		bool				IsStdAbbreviation(const NxsString &s, bool respectCase) const;
		static bool			IsNexusPunctuation(const char c);
		static bool         IsNewickPunctuation(const char c);
		bool				QuotesNeeded() const;
		NxsString 			UpperCasePrefix() const;
		friend std::ostream &operator<<(std::ostream &out, const NxsString &s);

		//	Modifiers
		//
		//NxsString		   &operator=(const NxsString &s);
		NxsString			&operator=(char);
		NxsString			&operator=(const char *s);
		NxsString			&operator+=(const char *s);
		NxsString			&operator+=(const std::string &s);
		NxsString			&operator+=(const char c);
		NxsString			&operator+=(const int i);
		NxsString			&operator+=(unsigned i);
		NxsString			&operator+=(unsigned long i);
		NxsString			&operator+=(const long l);
		NxsString			&operator+=(const double d);
		NxsString			&operator+=(const IndexSet &d);
		NxsString			&operator<<(int i);
		NxsString			&operator<<(unsigned i);
		NxsString			&operator<<(long l);
		NxsString			&operator<<(unsigned long l);
		NxsString			&operator<<(double d);
		NxsString			&operator<<(const char *c);
		NxsString			&operator<<(char c);
		NxsString			&operator<<(const std::string &s);
		NxsString			&operator<<(const IndexSet &s);
#       if ! defined(HIDE_NCL_NXSSTRING_ENDL)
            NxsString			&operator<<(NxsString &(*funcPtr)(NxsString	&));
#       endif

		// Functions that should be in base class string but aren't
		void				clear();

		int					PrintF(const char *formatStr, ...);

		unsigned char		*p_str(unsigned char *) const;

		NxsString			&AddQuotes();
		NxsString 			&AddTail(char c, unsigned n);
		NxsString			&NumberThenWord(unsigned i, NxsString s);
		NxsString 			&ShortenTo(unsigned n);
		NxsString			&AppendDouble(unsigned minFieldFormat, unsigned precFormat, double x);
		NxsString 			&Capitalize();

		NxsString 			&RightJustifyString(const NxsString &s, unsigned w, bool clear_first = false);
		NxsString 			&RightJustifyLong(long x, unsigned w, bool clear_first = false);
		NxsString 			&RightJustifyDbl(double x, unsigned w, unsigned p, bool clear_first = false);

		NxsString 			&ToLower()
			{
			to_lower(*this);
			return *this;
			}
		NxsString 			&ToUpper()
			{
			to_upper(*this);
			return *this;
			}


		NxsString 			&BlanksToUnderscores();
		NxsString 			&UnderscoresToBlanks();

		//	Debugging
		//
		static NxsString 	ToHex(long p, unsigned nFours);
	};

#if defined (NXS_SUPPORT_OLD_NAMES)
	typedef NxsString nxsstring;
#endif


/*!
	Replaces the stored string with a copy of itself surrounded by single quotes (single quotes inside the string are
	converted to the '' pair of characters that signify a single quote). Returns a reference to itself.
*/
inline NxsString &NxsString::AddQuotes()
	{
	add_nxs_quotes(*this);
	return *this;
	}

inline std::string	NxsString::GetEscapedDouble(const double &v)
	{
	NxsString s;
	s << v;
	return NxsString::GetEscaped(s);
	}

inline std::string	NxsString::GetEscapedInt(const int &v)
	{
	NxsString s;
	s << v;
	return NxsString::GetEscaped(s);
	}

inline std::string	NxsString::GetEscaped(const std::string &s)
	{
	NxsQuotingRequirements r = determine_quoting_requirements(s);
	if (r == kNoQuotesNeededForNexus)
		return s;
	std::string x(s.c_str());
	if (r == kUnderscoresSufficeForNexus)
		blanks_to_underscores(x);
	else
		add_nxs_quotes(x);
	return x;
	}


/*!
	Function object (Unary Predicate functor) that stores one string. The ()(const NxsString &) operator then returns the
	result of a case-insensitive compare. Useful for STL find algorithms. Could be made faster than sequential case
	insenstive comparisons, because the string stored in the object is just capitalized once.
*/
class NStrCaseInsensitiveEquals
	{
	public :

					NStrCaseInsensitiveEquals(const NxsString &s);
		bool		operator()(const NxsString &s);

	protected :

		NxsString	compStr;
	};

/*!
	Function object (Unary Predicate functor) that stores one string. The ()(const NxsString &) operator then returns the
	result of a case-sensitive compare. Useful for STL find algorithms.
*/
class NStrCaseSensitiveEquals
	{
	public :

					NStrCaseSensitiveEquals(const NxsString &s);
		bool		operator()(const NxsString &s) const;

	protected :

		NxsString	compStr;
	};

/*!
	Binary function class that performs case-Insensitive string compares.
*/
struct NxsStringEqual
  : public std::binary_function<NxsString, NxsString, bool>
	{
	bool operator()(const NxsString &x, const NxsString &y) const;
	};

// ############################# start NStrCaseInsensitiveEquals functions ##########################

/*!
	Creates a function object for case-insensitive comparisons of `s' to a container of strings.
*/
inline NStrCaseInsensitiveEquals::NStrCaseInsensitiveEquals(
  const NxsString &s)	/* the string to be compared */
	{
	compStr = s;
	compStr.Capitalize();
	}

/*!
	Returns the result of a case-sensitive compare of `s' and the string stored when the NStrCaseInsensitiveEquals object
	was created. Could be made more efficient (currently capitalizes the entire argument even though the first character may
	be wrong).
*/
inline bool NStrCaseInsensitiveEquals::operator()(
  const NxsString &s)	/* the string to be compared */
	{
	if (s.length() == compStr.length())
		{
		NxsString capS(s);
		capS.Capitalize();
		return capS == compStr;
		}
	return false;
	}

// ############################# start NStrCaseSensitiveEquals functions ##########################

/*!
	Creates a function object for case-sensitive comparisons of `s' to a container of strings.
*/
inline NStrCaseSensitiveEquals::NStrCaseSensitiveEquals(
  const NxsString &s)	/* the string that all other strings will be compared to when the (const NxsString &) operator is called */
	{
	compStr = s;
	}

/*!
	Returns the result of a case-sensitive compare of `s' and the string stored when the NStrCaseSensitiveEquals was
	created.
*/
inline bool NStrCaseSensitiveEquals::operator()(
  const NxsString &s)	/* the string to be compared */
  const
	{
	return (compStr == s);
	}

// ############################# start NxsStringEqual functions ##########################

/*!
	Returns true if the strings `x' and `y' are identical (NOT case sensitive)
*/
inline bool NxsStringEqual::operator()(
  const NxsString &x,	/* first string */
  const NxsString &y)	/* second string to be compared with `x' */
  const
	{
	return x.EqualsCaseInsensitive(y);
	}

// ############################# start NxsString functions ##########################

/*!
	The default constructor.
*/
inline NxsString::NxsString()
	{
	}

/*!
	Returns a single-quoted version of the NxsString. The calling object is not altered. Written for ease of use. Simply
	copies the stored string, then returns the copy after calling its AddQuotes function.
*/
inline NxsString NxsString::GetQuoted()
  const
	{
	NxsString s(*this);
	s.AddQuotes();
	return s;
	}

/*!
	Most containers in the standard template library can be completely erased using the clear function, but none is
	provided for the class string and hence is provided here.
*/
inline void NxsString::clear()
	{
	erase();
	}

/*!
	Returns true if the Equals comparison function is true for this or any element in the vector `s'.
*/
inline bool NxsString::IsInVector(
  const NxsStringVector &s, /* the vector of NxsString objects to be searched */
  NxsString::CmpEnum mode)	/* the argument passed to the Equals function, which is called for every element in the vector `s' */
  const
	{
	for (NxsStringVector::const_iterator sIt = s.begin(); sIt != s.end(); sIt++)
		{
		if (Equals(*sIt, mode))
			return true;
		}
	return false;
	}

/*!
	A copy constructor taking a C-string argument.
*/
inline NxsString::NxsString(
  const char *s)	/* the C-string that forms the basis for the new NxsString object */
	{
	assign(s);
	}

/*!
	A copy constructor taking a NxsString reference argument.
*/
inline NxsString::NxsString(
  const NxsString &s)	/* reference to a NxsString to be used to create this copy */
  :std::string()
	{
	assign(s);
	}

/*!
	Sets the stored string equal to the supplied C-string `s'.
*/
inline NxsString &NxsString::operator=(
  const char *s)	/* the string for comparison */
	{
	assign(s);
	return *this;
	}

//inline NxsString& NxsString::operator=(
//  const NxsString &s)
//	{
//	assign(s);
//	return *this;
//	}

/*!
	Appends the supplied C-string `s' to the stored string.
*/
inline NxsString &NxsString::operator+=(
  const char *s)	/* the C-string to be appended */
	{
	append(std::string(s));
	return *this;
	}

/*!
	Appends the characters in the supplied NxsString reference `s' to the stored string.
*/
inline NxsString &NxsString::operator+=(
  const std::string &s)	/* the string to append */
	{
	append(s);
	return *this;
	}

/*!
	Appends the character `c' to the stored string.
*/
inline NxsString &NxsString::operator+=(
  const char c)	/* the character to append */
	{
	char s[2];
	s[0] = c;
	s[1] = '\0';
	append(std::string(s));
	return *this;
	}

/*!
	Sets the stored std::string to the supplied character 'c'.
*/
inline NxsString &NxsString::operator=(
  char c)	/* the character to which the stored std::string should be set */
	{
	clear();
	return (*this += c);
	}

/*!
	Uses the standard C sprintf function to append the character representation of the supplied integer i' to the stored
	string (format code %d). For example, if the stored string is "taxon" and `i' is 9, the result is "taxon9".
*/
inline NxsString &NxsString::operator+=(
  const int i)	/* the int to append */
	{
	char tmp[81];
	std::sprintf(tmp, "%d", i);
	append(tmp);
	return *this;
	}

/*!
	Capitalizes all lower case letters in the stored string by calling ToUpper.
*/
inline NxsString &NxsString::Capitalize()
	{
	ToUpper();
	return *this;
	}

/*!
	Returns true if the stored string is an abbreviation (or complete copy) of the supplied string `s'.
*/
inline bool NxsString::Abbreviates(
  const NxsString	&s,		/* the full comparison string */
  NxsString::CmpEnum	mode)	/* if equal to abbrev, a non-case-sensitive comparison will be made, otherwise comparison will respect case */
  const
	{
	if (mode == NxsString::abbrev)
		return IsCapAbbreviation(s);
	else
		return IsStdAbbreviation(s, mode == respect_case);
	}

/*!
	Uses standard C function std::sprintf to append the unsigned integer `i' to the stored string (format code %u).
*/
inline NxsString& NxsString::operator+=(
  unsigned i)	/* the integer to be appended */
	{
	char tmp[81];
	std::sprintf(tmp, "%u", i);
	append(tmp);
	return *this;
	}

/*!
	Uses standard C function std::sprintf to append the long integer `l' to the stored string (format code %ld).
*/
inline NxsString& NxsString::operator+=(
  const long l)	/* the long integer to be appended */
	{
	char tmp[81];
	std::sprintf(tmp, "%ld", l);
	append(tmp);
	return *this;
	}

/*!
	Uses standard C function std::sprintf to append the unsigned long integer `l' to the stored string (format code %lu).
*/
inline NxsString& NxsString::operator+=(
  const unsigned long l)	/* the unsigned long integer to be appended */
	{
	char tmp[81];
	std::sprintf(tmp, "%lu", l);
	append(tmp);
	return *this;
	}

/*!
	Uses the mode argument to call (and return the result of) the correct string comparison function.
*/
inline bool NxsString::Equals(
  const NxsString &s,		/* the string to which *this is compared */
  NxsString::CmpEnum mode)	/* should be one of these three: respect_case, no_respect_case or abbrev */
  const
	{
	switch (mode) {
		case NxsString::respect_case :
			return (strcmp(this->c_str(), s.c_str()) == 0);
		case NxsString::no_respect_case :
			return this->EqualsCaseInsensitive(s);
		case NxsString::abbrev :
			return this->IsCapAbbreviation(s);
		default :
			NCL_ASSERT(0);// incorrect setting for mode
		}
	return false;
	}

# if ! defined(HIDE_NCL_NXSSTRING_ENDL)

	Allows functions that take and return references to NxsString strings to be placed in a series of << operators.
	See the NxsString endl function.
inline NxsString &NxsString::operator<<(
  NxsString &(*funcPtr)(NxsString &))	/* pointer to a function returning a reference to a NxsString */
	{
	return funcPtr(*this);
	}
#endif

/*!
	Returns true if `c' is any Nexus punctuation character:
>
	()[]{}/\,;:=*'"`-+<>
>
*/
inline bool NxsString::IsNexusPunctuation(
  const char c)	/* the character in question */
	{
	return (strchr("()[]{}/\\,;:=*\'\"`-+<>", c) != NULL);
	}


/*! Returns true if `c' is any Newick punctuation character:
>
	()[]':;,
>
List of punctuation taken from the "unquoted labels may not contain" section 
of http://evolution.genetics.washington.edu/phylip/newick_doc.html

Thanks to Andrew Lenards for pointing out the need for this when dealing with
non-NEXUS trees.
 parentheses, square brackets,
        single_quotes, colons, semicolons, or commas
*/
inline bool NxsString::IsNewickPunctuation(
  const char c)	/* the character in question */
	{
	const bool v = (strchr("()[]':;,", c) != NULL);
	return v;
	}


/*!
	Creates a new string (and returns a reference to the new string) composed of the integer `i' followed by a space and
	then the string `s'. If `i' is not 1, then an 's' character is appended to make `s' plural. For example, if `i' were 0,
	1, or 2, and `s' is "character", then the returned string would be "0 characters", "1 character" or "2 characters",
	respectively. Obviously this only works if adding an 's' to the supplied string makes it plural.
*/
inline NxsString &NxsString::NumberThenWord(
  unsigned i,			/* the number */
  const NxsString s)	/* the string needing to be pluralized */
  	{
	(*this).erase();
  	*this << i << ' ' << s;
  	if (i != 1)
  		*this << 's';
  	return *this;
  	}

/*!
	Another way to call the += operator (written to make it possible to use a NxsString like an std::ostream)
*/
inline NxsString &NxsString::operator<<(
  int i)	/* the integer to append */
  	{
  	return (*this += i);
  	}

/*!
	Another way to call the += operator (written to make it possible to use a NxsString like an std::ostream)
*/
inline NxsString &NxsString::operator<<(
  unsigned i)	/* the unsigned integer to append */
	{
	return (*this += (int) i);
	}

/*!
	Another way to call the += operator (written to make it possible to use a NxsString like an std::ostream)
*/
inline NxsString &NxsString::operator<<(
  long l)	/* the long integer to append */
	{
	return (*this += l);
	}

/*!
	Another way to call the += operator (written to make it possible to use a NxsString like an std::ostream)
*/
inline NxsString &NxsString::operator<<(
  unsigned long l)	/* the unsigned long integer to append */
	{
	return (*this += l);
	}

/*!
	Another way to call the += operator (written to make it possible to use a NxsString like an std::ostream)
*/
inline NxsString &NxsString::operator<<(
  double d)	/* the double floating point value to append */
	{
	return (*this += d);
	}

/*!
	Another way to call the += operator (written to make it possible to use a NxsString like an std::ostream)
*/
inline NxsString &NxsString::operator<<(
  const char *c)	/* the C-string to append */
	{
	return (*this += c);
	}

/*!
	Another way to call the += operator (written to make it possible to use a NxsString like an std::ostream)
*/
inline NxsString &NxsString::operator<<(
  char c)	/* the char to append */
	{
	return (*this += c);
	}

/*!
	Another way to call the += operator (written to make it possible to use a NxsString like an std::ostream)
*/
inline NxsString &NxsString::operator<<(
  const std::string &s)	/* the NxsString to append */
	{
	return (*this += s);
	}



/*!
	Returns string as a Pascal string (array of unsigned characters with the length in the first byte).
*/
inline unsigned char *NxsString::p_str(
  unsigned char *buffer)	/* buffer to receive current string in Pascal form (i.e. length in first byte) */
  const
	{
	memmove(buffer + 1, c_str(), length());
	buffer[0] = (unsigned char)length();
	return buffer;
	}



// ############################# start of standalone functions ##########################

# if ! defined(HIDE_NCL_NXSSTRING_ENDL)
/* "Define HIDE_NCL_NXSSTRING_ENDL to hide the dangerous definition of endl from NCLs nxsstring.h" */
/*!
	Appends a newline character to the string `s' and the returns a reference to `s'. Used with << operator to allow
	strings to be written to like std::ostreams.
*/
inline NxsString &endl(
  NxsString &s)	/* the string to which the newline character is to be appended */
	{
	return (s += '\n');
	}
#endif

/*!
	Appends a newline character to the string `s' and the returns a reference to `s'. Used with << operator to allow
	strings to be written to like std::ostreams.
*/
inline NxsString &nxsendl(
  NxsString &s)	/* the string to which the newline character is to be appended */
	{
	return (s += '\n');
	}

/*!
	Writes the string `s' to the std::ostream `out'.
*/
inline std::ostream &operator<<(
  std::ostream &out,			/* the stream to which the string `s' is to be written */
  const NxsString &s)	/* the string to write */
	{
	out << s.c_str();
	return out;
	}

NxsStringVector 	BreakPipeSeparatedList(const NxsString &strList);
NxsStringVector 	GetVecOfPossibleAbbrevMatches(const NxsString &testStr,const NxsStringVector &possMatches);
bool 				SetToShortestAbbreviation(NxsStringVector &strVec, bool allowTooShort = false);

#endif
