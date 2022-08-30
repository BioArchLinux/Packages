//	Copyright (C) 1999-2003 Paul O. Lewis
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

#ifndef NCL_NXSTOKEN_H
#define NCL_NXSTOKEN_H

#include "ncl/nxsexception.h"
class NxsToken;

class NxsX_UnexpectedEOF: public NxsException
	{
	public:
		NxsX_UnexpectedEOF(NxsToken &);
	};


/*!
     General notes on NexusTokenizing


  File position information (pos, line and column) refer to the end of the token.

  Note 1:  the GetEmbeddedComments methods of ProcessedNxsToken and NxsToken can be tricky to use if detailed
    position location of the comment is required.  A vector of "embedded comments" in the NCL context is a collection of
    all comments that were encountered during a GetNextToken operation.  The behavior depends on whether the tokenizer
    can tell if a section of text is has the potential to span comment. Potentially comment-spanning tokens have to be
    read until a token-breaker is found.  Thus they include trailing comments.  Thus it is not always easy (or possible)
    for client code to determine  whether a specifie comment belongs "with" a particular NEXUS token rather than the
    previous or next token.
    For example:
  Text                            Result as (token, {"embedded comment"}) pairs    Explanation
  ============================    =============================================   =====================================
  ;a[1]b;                         (;, {}), (ab, {1}), (;, {})                     ab is a comment-spanning token
  ;[1]a[2]b;                      (;, {}), (ab, {1, 2}), (;, {})                  tokenizer realizes that ; is always a single token
                                                                                    so [1] is not encountered until the second GetNextToken() call.
  a[1];[2]b;                      (a, {1}), (;, {}) (b, {2}), (;, {})             First GetNextToken() call reads "a" token until ; (thus reading "[1]")
                                                                                    ; is a single character token, so no comments are read, thus making
                                                                                    [2] part of the third GetNextToken call().

    In some cases the comment position information and token position information may reveal the exact location of the
    comments.  Fortunately the relative order of comments is retained and the exact position is rarely needed.\

  Note 2: Using the NxsToken class with the saveCommandComments LabileFlag causes [&comment text here] comments to be
    returned as tokens ONLY if they are not preceded by potentially comment-spanning tokens. This "feature" is new to
	 NCL v2.1 and is the result of a bug-fix (previous versions of NCL incorrectly broke tokens at the start of any comment).

  Text                      Result as in saveCommandComments mode                Explanation
  ========================= =============================================      =====================================
  =[&R](1,                  ("=",{}) ("&R", {}), ("(",{}), ("1",{}), (",",{})  [&R] is not in the middle of potentially-comment-spanning token.
  a[&R]b,                   ("ab",{"&R"}), (",",{})                            [&R] is in the middle of comment-spanning token "ab"
  a[&R],                    ("a",{"&R"}), (",",{})                             [&R] is in on the trailing end of a potentially-comment-spanning token "a"
                                                                                the tokenizer
    This wart makes it more tedious to deal with command comments. However it is tolerable becuase the only supported use of command
    comments in NCL is after single-character tokens (for example after = in a tree descpription).

    The NHX command comments are not processed by NCL, but they occur in contexts in which it will be possible to determine
    the correct location of the comment  (though it is necessary to check the embedded comments when processing NHX trees):

 Text                      Result as NOT IN saveCommandComments mode                        Explanation
 ========================= ===================================================      =====================================
 ):3.5[&&NHXtext],         (")",{}) (":", {}), ("3.5",{"&&NHXtext"}), (",",{})      "3.5" is potentially-comment-spanning, but the comment still
                                                                                      is stored with other metadata for the same edge.
 )[&&NHXtext],             (")",{}) (",", {"&&NHXtext"})                            NHX comment is parsed with the second token, but
                                                                                      because , is NOT potentially-comment-spanning
                                                                                      know that [&&NHXtext] must have preceded the comma (the
                                                                                      token and comment column numbers would also make this clear.
*/

/*!
 New in 2.1
	 - See Note 2 above (bug-fix, but wart introduced).  This could lead to loss of backward compatibility if client
		code relies of saveCommandComments in contexts in which command comments occur within potentially comment-spanning
       tokens.
	 - NxsComment class.
    - Comments are stored in tokenization (the GetNextToken() call will trash the previous comments, so client code
		must store comments if they are needed permanently).
	 - NxsToken::SetEOFAllowed method added and SetEOFAllowed(false) is called when entering a block.  This means that
		when parsing block contents the NxsToken tokenizer will raise an NxsException if it runs out of file (thus
		Block reader code no longer needs to check for atEOF() constantly to issue an appropriate error).
	 - ProcessedNxsToken class, NxsToken:ProcessAsSimpleKeyValuePairs, and NxsToken:ProcessAsCommand methods.  This makes
		it easier to parse commands (by allowing random access to the tokens in a command). These methods are not appropriate
		for very long commands (such as MATRIX) or commands that require fiddling with the tokenizing rules (such as disabling
		the hyphen as a token breaker)
	 - lazy NxsToken::GetFilePosition() and low level io operations dramatically speed up tokenization (~10-20 times faster).
	 - some other utility functions were added, and some refactoring (delegation to NxsString) was done to clean up
*/




/*!
   Storage for a comment text and (end of the comment) file position information
*/
class NxsComment
	{
	public:
		NxsComment(const std::string & s, long lineNumber, long colNumber)
			:body(s),
			line(lineNumber),
			col(colNumber)
			{}
		long		GetLineNumber() const
			{
			return line;
			}
		long 		GetColumnNumber() const
			{
			return col;
			}
		const std::string & GetText() const
			{
			return body;
			}
		void WriteAsNexus(std::ostream &out) const
			{
			out << '[' << body << ']';
			}
	private:
		std::string body;
		long line;
		long col;
	};


/*!
   Storage for a file position, line number and column number.
*/
class NxsTokenPosInfo
	{
	public:
		NxsTokenPosInfo()
			:pos(0),
			line(-1),
			col(-1)
			{}
		NxsTokenPosInfo(file_pos position, long lineno, long columnno)
			:pos(position),
			line(lineno),
			col(columnno)
			{}
		NxsTokenPosInfo(const NxsToken &);

		file_pos 	GetFilePosition() const
			{
			return pos;
			}

		long		GetLineNumber() const
			{
			return line;
			}

		long 		GetColumnNumber() const
			{
			return col;
			}


		file_pos	pos;	/* current file position */
		long		line;	/* current line in file */
		long		col;	/* column of current line */
	};


/*!
	A structure for storing the name of a command and to maps of option names
 		to value strings.
	Produced by ProcessedNxsToken::ParseSimpleCmd (see that commands comments for rules on how it parses a NEXUS
	command into a NxsSimpleCommandStrings struct).
*/
class NxsSimpleCommandStrings
	{
	public:
		typedef std::vector<std::string> VecString;
		typedef std::list<VecString> MatString;
		typedef std::pair<NxsTokenPosInfo, std::string> SingleValFromFile;
		typedef std::pair<NxsTokenPosInfo, VecString > MultiValFromFile;
		typedef std::pair<NxsTokenPosInfo, MatString > MatFromFile;
		typedef std::map<std::string, SingleValFromFile> StringToValFromFile;
		typedef std::map<std::string,  MultiValFromFile> StringToMultiValFromFile;
		typedef std::map<std::string,  MatFromFile> StringToMatFromFile;

		// Looks for k in opts and multiOpts. Returns all of the values
		// 	for the command option (will be an empty vector of strings if the option was not found).
		// Case-sensitive!
		// If an option is in multiOpts and opts, then only the value from opts will be returned!
		MultiValFromFile GetOptValue(const std::string &k) const
			{
			MultiValFromFile mvff;
			StringToValFromFile::const_iterator s = this->opts.find(k);
			if (s != this->opts.end())
				{
				const SingleValFromFile & v(s->second);
				mvff.first = v.first;
				mvff.second.push_back(v.second);
				}
			else
				{
				StringToMultiValFromFile::const_iterator m = this->multiOpts.find(k);
				if (m != this->multiOpts.end())
					{
					const MultiValFromFile & mv(m->second);
					mvff.first = mv.first;
					mvff.second  = mv.second;
					}
				}
			return mvff;
			}

		MatFromFile GetMatOptValue(const std::string & k) const
			{
			StringToMatFromFile::const_iterator mIt = this->matOpts.find(k);
			if (mIt ==  this->matOpts.end())
				return MatFromFile();
			return mIt->second;
			}

		bool HasKey(const std::string k) const
			{
			if (this->opts.find(k) !=  this->opts.end())
				return true;
			return ((this->multiOpts.find(k) !=  this->multiOpts.end()) || (this->matOpts.find(k) !=  this->matOpts.end()));
			}

	  	std::string cmdName;
	  	NxsTokenPosInfo cmdPos;
		StringToValFromFile opts;
		StringToMultiValFromFile multiOpts;
		StringToMatFromFile matOpts;
	};

/*!
   Storage for a single NEXUS token, and embedded comments, along with end-of-the-token file position information.
*/
class ProcessedNxsToken
	{
	public:
		static void IncrementNotLast(std::vector<ProcessedNxsToken>::const_iterator & it,
									 const std::vector<ProcessedNxsToken>::const_iterator &endIt,
									 const char * context);
		static NxsSimpleCommandStrings ParseSimpleCmd(const std::vector<ProcessedNxsToken> &, bool convertToLower);


		ProcessedNxsToken(const NxsToken &t);

		ProcessedNxsToken(std::string &s)
			:token(s)
			{}

		ProcessedNxsToken(std::string &s, file_pos position,long lineno, long columnno)
			:token(s),
			posInfo(position, lineno, columnno)
			{}

		std::string GetToken() const
			{
			return token;
			}

		const std::vector<NxsComment> & GetEmbeddedComments() const
			{
			return embeddedComments;
			}

		NxsTokenPosInfo 	GetFilePosInfo() const
			{
			return posInfo;
			}
		const NxsTokenPosInfo & GetFilePosInfoConstRef() const
			{
			return posInfo;
			}

		file_pos 	GetFilePosition() const
			{
			return posInfo.GetFilePosition();
			}

		long		GetLineNumber() const
			{
			return posInfo.GetLineNumber();
			}

		long 		GetColumnNumber() const
			{
			return posInfo.GetColumnNumber();
			}

		bool		Equals(const char *c) const
			{
			return NxsString::case_insensitive_equals(token.c_str(), c);
			}
		bool		EqualsCaseSensitive(const char *c) const
			{
			return (strcmp(token.c_str(), c) == 0);
			}

		void 		SetEmbeddedComments(const std::vector<NxsComment> & c)
			{
			embeddedComments = c;
			}

		void WriteAsNexus(std::ostream &out) const
			{
			for(std::vector<NxsComment>::const_iterator cIt = embeddedComments.begin(); cIt != embeddedComments.end(); ++cIt)
				cIt->WriteAsNexus(out);
			out << NxsString::GetEscaped(token);
			}
	private:
		std::string token;
		NxsTokenPosInfo posInfo;
		std::vector<NxsComment> embeddedComments; /* comments that were processed in the same GetToken operation that created this token. */
	};

/*!
  ProcessedNxsCommand is merely of a collection of ProcessedNxsToken objects. The NxsToken object can use a ; as a
	separator to parse of its input stream until the next ";" and return a ProcessedNxsCommand.

	See NxsToken::ProcessAsCommand method.
*/
typedef std::vector<ProcessedNxsToken> ProcessedNxsCommand;
bool WriteCommandAsNexus(std::ostream &, const ProcessedNxsCommand &);


/**---------------------------------------------------------------------------------------------------------------------
	NxsToken objects are used by NxsReader to extract words (tokens) from a NEXUS data file. NxsToken objects know to
	correctly skip NEXUS comments and understand NEXUS punctuation, making reading a NEXUS file as simple as repeatedly
	calling the GetNextToken() function and then interpreting the token returned. If the token object is not attached
	to an input stream, calls to GetNextToken() will have no effect. If the token object is not attached to an output
	stream, output comments will be discarded (i.e., not output anywhere) and calls to Write or Writeln will be
	ineffective. If input and output streams have been attached to the token object, however, tokens are read one at a
	time from the input stream, and comments are correctly read and either written to the output stream (if an output
	comment) or ignored (if not an output comment). Sequences of characters surrounded by single quotes are read in as
	single tokens. A pair of adjacent single quotes are stored as a single quote, and underscore characters are stored
	as blanks.
*/
class NxsToken
	{
	public:
		static std::string	EscapeString(const std::string &);
		static bool 		NeedsQuotes(const std::string &);
		static std::string	GetQuoted(const std::string &);
		static void 		DemandEndSemicolon(NxsToken &token, NxsString & errormsg, const char *contextString);
		static unsigned 	DemandPositiveInt(NxsToken &token, NxsString & errormsg, const char *contextString);
		static std::map<std::string, std::string> ParseAsSimpleKeyValuePairs(const ProcessedNxsCommand & tv, const char *cmdName);

		static std::vector<ProcessedNxsToken> Tokenize(const std::string & );

		enum NxsTokenFlags	/* For use with the variable labileFlags */
			{
			saveCommandComments		= 0x0001,	/* if set, command comments of the form [&X] are not ignored but are instead saved as regular tokens (without the square brackets, however) */
			parentheticalToken		= 0x0002,	/* if set, and if next character encountered is a left parenthesis, token will include everything up to the matching right parenthesis */
			curlyBracketedToken		= 0x0004,	/* if set, and if next character encountered is a left curly bracket, token will include everything up to the matching right curly bracket */
			doubleQuotedToken		= 0x0008,	/* if set, grabs entire phrase surrounded by double quotes */
			singleCharacterToken	= 0x0010,	/* if set, next non-whitespace character returned as token */
			newlineIsToken			= 0x0020,	/* if set, newline character treated as a token and atEOL set if newline encountered */
			tildeIsPunctuation		= 0x0040,	/* if set, tilde character treated as punctuation and returned as a separate token */
			useSpecialPunctuation	= 0x0080,	/* if set, character specified by the data member special is treated as punctuation and returned as a separate token */
			hyphenNotPunctuation	= 0x0100,	/* if set, the hyphen character is not treated as punctutation (it is normally returned as a separate token) */
			preserveUnderscores		= 0x0200,	/* if set, underscore characters inside tokens are not converted to blank spaces (normally, all underscores are automatically converted to blanks) */
			ignorePunctuation		= 0x0400	/* if set, the normal punctuation symbols are treated the same as any other darkspace characters */
			};

		NxsString		errormsg;

						NxsToken(std::istream &i);
		virtual			~NxsToken();

		bool			AtEOF();
		bool			AtEOL();
		bool			Abbreviation(NxsString s);
		bool			Begins(NxsString s, bool respect_case = false);
		void			BlanksToUnderscores();
		bool			Equals(NxsString s, bool respect_case = false) const;
		bool		EqualsCaseSensitive(const char *c) const
			{
			return (strcmp(token.c_str(), c) == 0);
			}

		long			GetFileColumn() const;
		file_pos		GetFilePosition() const;
		long			GetFileLine() const;
		void			GetNextToken();
		NxsString		GetToken(bool respect_case = true);
		const char		*GetTokenAsCStr(bool respect_case = true);
		const NxsString	&GetTokenReference() const;
		int				GetTokenLength() const;
		bool			IsPlusMinusToken();
		bool			IsPunctuationToken();
		bool			IsWhitespaceToken();
		bool			IsPlusMinusToken(const std::string & t);
		bool			IsPunctuationToken(const std::string & t);
		bool			IsWhitespaceToken(const std::string & t);
		std::map<std::string, std::string> ProcessAsSimpleKeyValuePairs(const char *cmdName);
		void 			ProcessAsCommand(ProcessedNxsCommand *tokenVec);
		void			ReplaceToken(const NxsString s);
		void			ResetToken();
		void			SetSpecialPunctuationCharacter(char c);
		void			SetLabileFlagBit(int bit);
		bool			StoppedOn(char ch);
		void			StripWhitespace();
		void			ToUpper();
		void			Write(std::ostream &out);
		void			Writeln(std::ostream &out);

		virtual void	OutputComment(const NxsString &msg);

		void			SetEOFAllowed(bool e)
			{
			eofAllowed = e;
			}
		bool			GetEOFAllowed() const
			{
			return eofAllowed;
			}
		void			SetBlockName(const char *);
		std::string 	GetBlockName();
		const std::vector<NxsComment> & GetEmbeddedComments() const
			{
			return embeddedComments;
			}
		char			PeekAtNextChar() const;
		
		/// Calling with `true` will force the NxsToken to only consider newick's
		//		punctuation characters to be punctuation (newick's punctuation
		//		chars are ()[]':;, this is a subset of NEXUS punctuation.
		//	Calling with `false` will restore NEXUS punctuation rules.
		void UseNewickTokenization(bool v);

	protected:

		void			AppendToComment(char ch);
		void			AppendToToken(char ch);
		bool			GetComment();
		void			GetCurlyBracketedToken();
		void			GetDoubleQuotedToken();
		void			GetQuoted();
		void			GetParentheticalToken();
		bool			IsPunctuation(char ch);
		bool			IsWhitespace(char ch);

	private:
		void AdvanceToNextCharInStream();
		char			GetNextChar();
		//char ReadNextChar();

		std::istream	&inputStream;		/* reference to input stream from which tokens will be read */
		signed char		nextCharInStream;
		file_pos		posOffBy;			/* offset of the file pos (according to the stream) and the tokenizer (which is usually a character or two behind, due to saved chars */
		file_pos		usualPosOffBy;		/* default of posOffBy.  Usually this is -1, but it can be positive if a tokenizer is created from a substring of the file */
		long			fileLine;			/* current file line */
		long			fileColumn;			/* current column in current line (refers to column immediately following token just read) */
		NxsString		token;				/* the character buffer used to store the current token */
		NxsString		comment;			/* temporary buffer used to store output comments while they are being built */
		bool			eofAllowed;
		signed char		saved;				/* either '\0' or is last character read from input stream */
		bool			atEOF;				/* true if end of file has been encountered */
		bool			atEOL;				/* true if newline encountered while newlineIsToken labile flag set */
		char			special;			/* ad hoc punctuation character; default value is '\0' */
		int				labileFlags;		/* storage for flags in the NxsTokenFlags enum */
		char			whitespace[4];		/* stores the 3 whitespace characters: blank space, tab and newline */
		std::string 	currBlock;
		std::vector<NxsComment>		embeddedComments;
		typedef bool (* CharPredFunc)(const char);
		CharPredFunc    isPunctuationFn;
	};

typedef NxsToken NexusToken;


inline ProcessedNxsToken::ProcessedNxsToken(const NxsToken &t)
	:token(t.GetTokenReference()),
	posInfo(t)
	{}

inline NxsTokenPosInfo::NxsTokenPosInfo(const NxsToken &t)
	:pos(t.GetFilePosition()),
	line(t.GetFileLine()),
	col(t.GetFileColumn())
	{}

/*!
	Stores the current block name (for better error reporting only).  Use NULL to clear the currBlock name.
*/
inline void NxsToken::SetBlockName(const char *c)
	{
	if (c == 0L)
		currBlock.clear();
	else
		currBlock.assign(c);
	}

/*!
	Returns the token's block name (for better error reporting)
*/
inline std::string NxsToken::GetBlockName()
	{
	return currBlock;
	}

/*!
	Returns copy of s but with quoting according to the NEXUS Standard iff s needs to be quoted.
*/
inline std::string NxsToken::EscapeString(const std::string &s)
	{
	return NxsString::GetEscaped(s);
	}

/*!
	Returns the token for functions that only need read only access - faster than GetToken.
*/
inline const NxsString &NxsToken::GetTokenReference() const
	{
	return token;
	}

/**
  This function is called whenever an output comment (i.e., a comment beginning with an exclamation point) is found
	in the data file.

  This base-class version of OutputComment suppresses these messages. You can override this virtual function to display
    the output comment in the most appropriate way for application platform you are supporting.
*/
inline void NxsToken::OutputComment(
  const NxsString &)	/* the contents of the printable comment discovered in the NEXUS data file */
	{
	}

/*!
	Adds `ch' to end of comment NxsString.
*/
inline void NxsToken::AppendToComment(
  char ch)	/* character to be appended to comment */
	{
	comment += ch;
	}

/*!
	Adds `ch' to end of current token.
*/
inline void NxsToken::AppendToToken(
  char ch)	/* character to be appended to token */
	{
	token.push_back(ch);
	}


/*!
	Returns true if character supplied is considered a whitespace character. Note: treats '\n' as darkspace if labile
	flag newlineIsToken is in effect.
*/
inline bool NxsToken::IsWhitespace(
  char ch)	/* the character in question */
	{
	bool ws = false;

	// If ch is found in the whitespace array, it's whitespace
	//
	if (strchr(whitespace, ch) != NULL)
		ws = true;

	// Unless of course ch is the newline character and we're currently
	// treating newlines as darkspace!
	//
	if (labileFlags & newlineIsToken && ch == '\n')
		ws = false;

	return ws;
	}

/*!
	Returns true if and only if last call to GetNextToken encountered the end-of-file character (or for some reason the
	input stream is now out of commission).
*/
inline bool NxsToken::AtEOF()
	{
	return atEOF;
	}

/*!
	Returns true if and only if last call to GetNextToken encountered the newline character while the newlineIsToken
	labile flag was in effect.
*/
inline bool NxsToken::AtEOL()
	{
	return atEOL;
	}

/*!
	Converts all blanks in token to underscore characters. Normally, underscores found in the tokens read from a NEXUS
	file are converted to blanks automatically as they are read; this function reverts the blanks back to underscores.
*/
inline void NxsToken::BlanksToUnderscores()
	{
	token.BlanksToUnderscores();
	}

/*!
	Returns value stored in `filecol', which keeps track of the current column in the data file (i.e., number of
	characters since the last new line was encountered).
*/
inline long  NxsToken::GetFileColumn() const
	{
	return fileColumn;
	}

/*!
	Returns value stored in filepos, which keeps track of the current position in the data file (i.e., number of
	characters since the beginning of the file).  Note: for Metrowerks compiler, you must use the offset() method of
	the streampos class to use the value returned.
*/
inline file_pos  NxsToken::GetFilePosition() const
	{
	return inputStream.rdbuf()->pubseekoff(0,std::ios::cur, std::ios::in) + posOffBy;
	}

/*!
	Returns value stored in `fileline', which keeps track of the current line in the data file (i.e., number of new
	lines encountered thus far).
*/
inline long  NxsToken::GetFileLine() const
	{
	return fileLine;
	}

/*!
	Returns the data member `token'. Specifying false for`respect_case' parameter causes all characters in `token'
	to be converted to upper case before `token' is returned. Specifying true results in GetToken returning exactly
	what it read from the file.
*/
inline NxsString NxsToken::GetToken(
  bool respect_case)	/* determines whether token is converted to upper case before being returned */
	{
	if (!respect_case)
		ToUpper();

	return token;
	}

/*!
	Returns the data member `token' as a C-style string. Specifying false for`respect_case' parameter causes all
	characters in `token' to be converted to upper case before the `token' C-string is returned. Specifying true
	results in GetTokenAsCStr returning exactly what it read from the file.
*/
inline const char *NxsToken::GetTokenAsCStr(
  bool respect_case)	/* determines whether token is converted to upper case before being returned */
	{
	if (!respect_case)
		ToUpper();

	return token.c_str();
	}

/*!
	Returns token.size().
*/
inline int NxsToken::GetTokenLength() const
	{
	return (int)token.size();
	}

/*!
	Returns true if current token is a single character and this character is either '+' or '-'.
*/
inline bool NxsToken::IsPlusMinusToken()
	{
	return IsPlusMinusToken(token);
	}

/*!
	Returns true if t is a single character and this character is either '+' or '-'.
*/
inline bool NxsToken::IsPlusMinusToken(const std::string &t)
	{
	return (t.size() == 1 && ( t[0] == '+' || t[0] == '-') );
	}


/*!
	Returns true if character supplied is considered a punctuation character. The following twenty characters are
	considered punctuation characters:
>
	()[]{}/\,;:=*'"`+-<>
>
	Exceptions:
~
	o The tilde character ('~') is also considered punctuation if the tildeIsPunctuation labile flag is set
	o The special punctuation character (specified using the SetSpecialPunctuationCharacter) is also considered
	  punctuation if the useSpecialPunctuation labile flag is set
	o The hyphen (i.e., minus sign) character ('-') is not considered punctuation if the hyphenNotPunctuation
	  labile flag is set
~
	Use the SetLabileFlagBit method to set one or more NxsLabileFlags flags in `labileFlags'
*/
inline bool NxsToken::IsPunctuation(
  char ch)	/* the character in question */
	{

	// PAUP 4.0b10
	//  o allows ]`<> inside taxon names
	//  o allows `<> inside taxset names
	//
	if (isPunctuationFn(ch))
		{
		if  (labileFlags & hyphenNotPunctuation)
#			if defined(NCL_VERSION_2_STYLE_HYPHEN) && NCL_VERSION_2_STYLE_HYPHEN
				return (ch != '-');
#			else
				return (ch != '-'  && ch != '+');
#			endif
		return true;
		}
	if (labileFlags & tildeIsPunctuation  && ch == '~')
		return true;
	return (labileFlags & useSpecialPunctuation  && ch == special);
	}


/*!
	Returns true if current token is a single character and this character is a punctuation character (as defined in
	IsPunctuation function).
*/
inline bool NxsToken::IsPunctuationToken()
	{
	return IsPunctuationToken(token);
	}

/*!
	Returns true if t is a single character and this character is a punctuation character (as defined in
	IsPunctuation function).
*/
inline bool NxsToken::IsPunctuationToken(const std::string &t)
	{
	return (t.size() == 1 && IsPunctuation(t[0]));
	}


/*!
	Returns true if current token is a single character and this character is a whitespace character (as defined in
	IsWhitespace function).
*/
inline bool NxsToken::IsWhitespaceToken()
	{
	return IsWhitespaceToken(token);
	}

/*!
	Returns true if t is a single character and this character is a whitespace character (as defined in IsWhitespace function).
*/
inline bool NxsToken::IsWhitespaceToken(const std::string &t)
	{
	return (t.size() == 1 && IsWhitespace( t[0]));
	}

/*!
	Replaces current token NxsString with s.
*/
inline void NxsToken::ReplaceToken(
  const NxsString s)	/* NxsString to replace current token NxsString */
	{
	token = s;
	}

/*!
	Sets token to the empty NxsString ("").
*/
inline void NxsToken::ResetToken()
	{
	token.clear();
	embeddedComments.clear();
	}

/*!
	Sets the special punctuation character to `c'. If the labile bit useSpecialPunctuation is set, this character will
	be added to the standard list of punctuation symbols, and will be returned as a separate token like the other
	punctuation characters.
*/
inline void NxsToken::SetSpecialPunctuationCharacter(
  char c)	/* the character to which `special' is set */
	{
	special = c;
	}

/*!
	Sets the bit specified in the variable `labileFlags'. The available bits are specified in the NxsTokenFlags enum.
	All bits in `labileFlags' are cleared after each token is read.
*/
inline void NxsToken::SetLabileFlagBit(
  int bit)	/* the bit (see NxsTokenFlags enum) to set in `labileFlags' */
	{
	labileFlags |= bit;
	}

/*!
	Checks character stored in the variable saved to see if it matches supplied character `ch'. Good for checking such
	things as whether token stopped reading characters because it encountered a newline (and labileFlags bit
	newlineIsToken was set):
>
	StoppedOn('\n');
>
	or whether token stopped reading characters because of a punctuation character such as a comma:
>
	StoppedOn(',');
>
*/
inline bool NxsToken::StoppedOn(
  char ch)	/* the character to compare with saved character */
	{
	if (saved == ch)
		return true;
	else
		return false;
	}
inline char NxsToken::PeekAtNextChar() const
	{
	return nextCharInStream;
	}
/*!
	Simply outputs the current NxsString stored in `token' to the output stream `out'. Does not send a newline to the
	output stream afterwards.
*/
inline void NxsToken::Write(
  std::ostream &out)	/* the output stream to which to write token NxsString */
	{
	out << token;
	}

/*!
	Simply outputs the current NxsString stored in `token' to the output stream `out'. Sends a newline to the output
	stream afterwards.
*/
inline void NxsToken::Writeln(
  std::ostream &out)	/* the output stream to which to write `token' */
	{
	out << token << std::endl;
	}

inline std::map<std::string, std::string> NxsToken::ProcessAsSimpleKeyValuePairs(const char *cmdName)
	{
	ProcessedNxsCommand tokenVec;
	ProcessAsCommand(&tokenVec);
	return ParseAsSimpleKeyValuePairs(tokenVec, cmdName);
	}

/*!
	Returns true if token NxsString exactly equals `s'. If abbreviations are to be allowed, either Begins or
	Abbreviation should be used instead of Equals.
*/
inline bool NxsToken::Equals(
  NxsString s, /* the string for comparison to the string currently stored in this token */
  bool respect_case) const	/* if true, comparison will be case-sensitive */
	{
	if (respect_case)
		return (strcmp(token.c_str(), s.c_str()) == 0);
	return NxsString::case_insensitive_equals(token.c_str(), s.c_str());
	}

#endif
