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
#include <cstdlib>
#include <cassert>
#include <sstream>
#include "ncl/nxstoken.h"

using namespace std;

#define NEW_NXS_TOKEN_READ_CHAR




/*!
 * Parses a ProcessedNxsCommand assuming that it has the form:
 *		cmd_name opt_name = opt_val multi_opt_name = (opt_val_1 opt_val2) ;
 * Errors are produced if opt_names (or multi_opt_names) are repeated within a command.
 */
NxsSimpleCommandStrings ProcessedNxsToken::ParseSimpleCmd(
  const std::vector<ProcessedNxsToken> &pnc,
	bool convertToLower)
{
	NxsSimpleCommandStrings nscs;
	if (pnc.empty())
		return nscs;


	std::vector<ProcessedNxsToken>::const_iterator wordIt = pnc.begin();

	nscs.cmdName = wordIt->GetToken();
	if (convertToLower)
		NxsString::to_lower(nscs.cmdName);
	nscs.cmdPos = wordIt->GetFilePosInfoConstRef();
	++wordIt;

	std::string key;

	NxsString errorMsg;
	NxsTokenPosInfo keyPos = nscs.cmdPos;
	bool eqRead = false;
	for (; wordIt != pnc.end(); ++wordIt)
		{
		std::string w = wordIt->GetToken();
		if (convertToLower)
			NxsString::to_lower(w);
		if (key.empty())
			{
			key = w;
			if (nscs.HasKey(key))
				{
				errorMsg << "Command option (" << key << ") repeated in the " << nscs.cmdName << " command.";
				throw NxsException(errorMsg, wordIt->GetFilePosInfoConstRef());
				}
			keyPos = wordIt->GetFilePosInfoConstRef();
			}
		else if (!eqRead)
			{
			if (w != "=")
				{
				errorMsg << "Expecting an = after the  " << key << " command option of the  " << nscs.cmdName << " command.";
				throw NxsException(errorMsg, wordIt->GetFilePosInfoConstRef());
				}
			eqRead = true;
			}
		else {
			if (w == "(")
				{
				++wordIt;
				w = wordIt->GetToken();
				std::vector<std::string> vals;
				NxsSimpleCommandStrings::MatString mat;
				if (w == "(")
					{
					while (w != ")")
						{
						if (w != "(")
							{
							errorMsg << "Expecting a ( to begin another row of values in the " << key << " command option of the  " << nscs.cmdName << " command.";
							throw NxsException(errorMsg, keyPos);
							}

						++wordIt;
						w = wordIt->GetToken();
						while (wordIt != pnc.end())
							{
							w = wordIt->GetToken();
							if (convertToLower)
								NxsString::to_lower(w);
							if (w == ")")
								break;
							vals.push_back(w);
							++wordIt;
							}
						if (wordIt == pnc.end())
							{
							errorMsg << "Expecting a ) to end the list of values for the " << key << " command option of the  " << nscs.cmdName << " command.";
							throw NxsException(errorMsg, keyPos);
							}
						++wordIt;
						mat.push_back(vals);
						vals.clear();
						w = wordIt->GetToken();
						if (wordIt == pnc.end())
							{
							errorMsg << "Expecting a ) to end the list of values for the " << key << " command option of the  " << nscs.cmdName << " command.";
							throw NxsException(errorMsg, keyPos);
							}
						}
					nscs.matOpts[key] = NxsSimpleCommandStrings::MatFromFile(wordIt->GetFilePosInfoConstRef(), mat);
					}
				else
					{
					while (wordIt != pnc.end())
						{
						w = wordIt->GetToken();
						if (convertToLower)
							NxsString::to_lower(w);
						if (w == ")")
							break;
						vals.push_back(w);
						++wordIt;
						}
					if (wordIt == pnc.end())
						{
						errorMsg << "Expecting a ) to end the list of values for the " << key << " command option of the  " << nscs.cmdName << " command.";
						throw NxsException(errorMsg, keyPos);
						}
					nscs.multiOpts[key] = NxsSimpleCommandStrings::MultiValFromFile(wordIt->GetFilePosInfoConstRef(), vals);
					}
				}
			else
				{
				std::string val = w;
				nscs.opts[key] = NxsSimpleCommandStrings::SingleValFromFile( wordIt->GetFilePosInfoConstRef(), val);
				}
			eqRead = false;
			key.clear();
			}
		}
	if (eqRead)
		{
		errorMsg << "Expecting a value after the = sign in the  " << key << " command option of the  " << nscs.cmdName << " command.";
		throw NxsException(errorMsg, keyPos);
		}
	if (!key.empty())
		{
		errorMsg << "Expecting an = after the  " << key << " command option of the  " << nscs.cmdName << " command.";
		throw NxsException(errorMsg, keyPos);
		}
	return nscs;
}




NxsX_UnexpectedEOF::NxsX_UnexpectedEOF(NxsToken &token)
	:NxsException("Unexpected end-of-file", token)
	{
	std::string t = token.GetBlockName();
	NxsString::to_upper(t);
	if (!t.empty())
		msg << " while reading " << t << " block.";
	}

/*! Writes the command `c` (with all embedded comments) a terminating ; will be written if any tokens are written. */
bool WriteCommandAsNexus(std::ostream & out, const ProcessedNxsCommand &c)
	{
	if (c.empty())
		return false;
	out << "   "; /* command indentation  - 1 space*/
	for(ProcessedNxsCommand::const_iterator cIt = c.begin(); cIt != c.end(); ++cIt)
		{
		out << ' ';
		cIt->WriteAsNexus(out);
		}
	out << ";";
	return true;
	}



/*!
	Convenience function.
 	Raises an aprropriate NxsException (by appending  `contextString` to the phrase Unexpected ; "), if incrementing
		`tokIt` makes it equal to `endIt`
*/
void ProcessedNxsToken::IncrementNotLast(std::vector<ProcessedNxsToken>::const_iterator & tokIt, const std::vector<ProcessedNxsToken>::const_iterator &endIt, const char * contextString)
	{
	++tokIt;
	if (tokIt == endIt)
		{
		NxsString errormsg = "Unexpected ; ";
		if (contextString)
			errormsg.append(contextString);
		--tokIt;
		throw NxsException(errormsg, *tokIt);
		}
	}
/*!
 	Advance the stream and store it in nextCharInStream.  Deal with the 3 ways of specifying return charaters
		(nextCharInStream will be set to \n if any of the return styles are found)
*/
inline void NxsToken::AdvanceToNextCharInStream()
	{
	if (nextCharInStream == EOF)
		return;
	nextCharInStream  = (signed char) (inputStream.rdbuf())->sbumpc();
	posOffBy = -1;
	if (nextCharInStream == 13 || nextCharInStream == 10)
		{
		if(nextCharInStream == 13)
			{
			if ((inputStream.rdbuf())->sgetc() == 10)	//peeks at the next char
				{
				(inputStream.rdbuf())->sbumpc();
				posOffBy = -2;
				}
			}
		nextCharInStream = '\n';
		}
	}


#if defined(NEW_NXS_TOKEN_READ_CHAR)
/*!
	returns the character that had been stored in nextCharInStream, but also calls AdvanceToNextCharInStream() so
	nextCharInStream is advanced.
	Does all of the fileposition bookkeeping.
	Throws an NxsX_UnexpectedEOF exception if eof is found but eofAllowed is false.
*/
inline char NxsToken::GetNextChar()
	{
	//
	// 	Why this was changed:  calls to tellg seem slow and unnecessary - we're storing filepos in terms of the
	//	number of times we call sbumpc().
	//	if we go back to getting the filepos via in.tellg(), remember to call it
	//	twice after both sgetc() calls in the case of the \13\10 endline

	signed char ch = nextCharInStream;
	AdvanceToNextCharInStream();
	if(ch == EOF)
		{
		atEOF = true;
		if (eofAllowed)
			return '\0';
		throw NxsX_UnexpectedEOF(*this);
		}
	if(ch == '\n')
		{
		fileLine++;
		fileColumn = 1L;
		atEOL = true;
		return '\n';
		}
	if (ch == '\t')
		fileColumn += 4 - ((fileColumn - 1)%4);	//@assumes that tab will be 4 in the editor we use
	else
		fileColumn++;
	atEOL = false;
	return ch;
	}

#else //	if  !defined(NEW_NXS_TOKEN_READ_CHAR)

/*!
	Reads next character from in and does all of the following before returning it to the calling function:
~
	o if character read is either a carriage return or line feed, the variable line is incremented by one and the
	  variable col is reset to zero
	o if character read is a carriage return, and a peek at the next character to be read reveals that it is a line
	  feed, then the next (line feed) character is read
	o if either a carriage return or line feed is read, the character returned to the calling function is '\n' if
	  character read is neither a carriage return nor a line feed, col is incremented by one and the character is
	  returned as is to the calling function
	o in all cases, the variable filepos is updated using a call to the tellg function of istream.
~
*/
inline char NxsToken::GetNextChar()
	{
	int ch = inputStream.get();
	int failed = inputStream.bad();
	if (failed)
		{
		errormsg = "Unknown error reading data file (check to make sure file exists)";
		throw NxsException(errormsg);
		}

	if (ch == 13 || ch == 10)
		{
		fileLine++;
		fileColumn = 1L;

		if (ch == 13 && (int)inputStream.peek() == 10)
			ch = inputStream.get();

		atEOL = 1;
		}
	else if (ch == EOF)
		atEOF = 1;
	else
		{
		fileColumn++;
		atEOL = 0;
		}

#	if defined(__DECCXX)
		filepos = 0L;
#	else
		file_pos filepos = inputStream.tellg();
#	endif

	if (atEOF)
		return '\0';
	else if (atEOL)
		return '\n';
	else
		return (char)ch;
	}
#endif

std::map<std::string, std::string> NxsToken::ParseAsSimpleKeyValuePairs(const ProcessedNxsCommand & tv, const char *cmdName)
	{
	std::map<std::string, std::string> kv;
	std::string key;
	ProcessedNxsCommand::const_iterator tvIt = tv.begin();
	ProcessedNxsCommand::const_iterator prevIt;
	ProcessedNxsCommand::const_iterator endIt = tv.end();
	while (tvIt != endIt)
		{
		key = tvIt->GetToken().c_str();
		prevIt = tvIt++;
		if (tvIt == endIt || tvIt->GetToken() != "=")
			{
			NxsString m("Expecting = after ");
			m += key.c_str();
			m += " in ";
			m += cmdName;
			m += " command.";
			if (tvIt == endIt)
				throw NxsException(m, prevIt->GetFilePosition(), prevIt->GetLineNumber(), prevIt->GetColumnNumber());
			else
				throw NxsException(m, tvIt->GetFilePosition(), tvIt->GetLineNumber(), tvIt->GetColumnNumber());
			}
		prevIt = tvIt++;
		if (tvIt == endIt)
			{
			NxsString m("Expecting a value after = in the  ");
			m += key.c_str();
			m += " subcommand of the in ";
			m += cmdName;
			m += " command.";
			throw NxsException(m, prevIt->GetFilePosition(), prevIt->GetLineNumber(), prevIt->GetColumnNumber());
			}
		kv[key] = tvIt->GetToken();
		tvIt++;
		}
	return kv;
	}


std::vector<ProcessedNxsToken> NxsToken::Tokenize(const std::string & toTokenize)
    {
	std::string bogusStr = toTokenize;
	bogusStr.append(1, '\n');
	std::istringstream bogusStream(bogusStr);
	NxsToken bogusToken(bogusStream);
	bogusToken.GetNextToken();
	std::vector<ProcessedNxsToken>  tokenVec;
	while (!bogusToken.AtEOF())
		{
		tokenVec.push_back(ProcessedNxsToken(bogusToken));
		bogusToken.GetNextToken();
		}
    return tokenVec;
    }


/*!
	Reads until ";" and fills the vector of ProcessedNxsToken objects.
	Note the ";" is not included in the ProcessedNxsCommand, but it can be assumed that the semicolon followed.
	The NxsToken objects file position will reflect the location of the semicolon.
*/
void NxsToken::ProcessAsCommand(ProcessedNxsCommand *tokenVec)
	{
	;
	while (!this->Equals(";"))
		{
		if (tokenVec)
			tokenVec->push_back(ProcessedNxsToken(*this));
		this->GetNextToken();
		}
	}


/*!
	Returns copy of s but with quoting according to the NEXUS Standard (single quotes around the token and all internal
		single quotes replaced with a pair of single quotes.
*/
std::string NxsToken::GetQuoted(const std::string &s)
	{
	std::string withQuotes;
	withQuotes.reserve(s.length() + 4);
	withQuotes.push_back('\'');
	for (NxsString::const_iterator sIt = s.begin(); sIt != s.end(); sIt++)
		{
		withQuotes.push_back(*sIt);
		if (*sIt == '\'')
			withQuotes.push_back('\'');
		}
	withQuotes.push_back('\'');
	return withQuotes;
	}

/*!
	Advances the token, and returns the unsigned int that the token represents

 	Sets errormsg and raises a NxsException on failure.
	`contextString` is used in error messages:
		"${contextString} must be a number greater than 0"
*/
unsigned NxsToken::DemandPositiveInt(NxsToken &token, NxsString & errormsg, const char *contextString)
	{
	token.GetNextToken();
	int i = -1;
	try {
	    i = token.GetToken().ConvertToInt();
	    }
	catch (NxsString::NxsX_NotANumber &x)
	    {
	    }
//	int i = atoi(token.GetToken().c_str());
	if (i <= 0)
		{
		errormsg.assign(contextString);
		errormsg += " must be a number greater than 0. Found ";
		errormsg += token.GetToken();
		errormsg += " instead";
		throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
		}
	return (unsigned) i;
	}


/*!
	Advances the token, and returns the unsigned int that the token represents

 	Sets errormsg and raises a NxsException on failure.
	`contextString` is used in error messages:
		"Expecting ';' to terminate the ${contextString} command"
*/
void NxsToken::DemandEndSemicolon(NxsToken &token, NxsString & errormsg, const char *contextString)
	{
	token.GetNextToken();
	if (!token.Equals(";"))
		{
		errormsg = "Expecting ';' to terminate the ";
		errormsg += contextString;
		errormsg += " command, but found ";
		errormsg += token.GetToken();
		errormsg += " instead";
		throw NxsException(errormsg, token.GetFilePosition(), token.GetFileLine(), token.GetFileColumn());
		}
	}
/*!
	Returns copy of s but with quoting according to the NEXUS Standard (single quotes around the token and all internal
		single quotes replaced with a pair of single quotes.
*/
bool NxsToken::NeedsQuotes(const std::string &s)
	{
	for (std::string::const_iterator sIt = s.begin(); sIt != s.end(); sIt++)
		{
		const char &c = (*sIt);
		if (!isgraph(c))
			return true;
		else if (strchr("\'[(){}\"-]/\\,;:=*`+<>", c) != NULL)
			{
			// ' and [ always need quotes.  other punctuation needs quotes if it is in a word of length > 1
			if (c == '\'' || c == '[')
				return true;
			return (s.length() > 1);
			}
		}
	return false;
	}



/*!
	Sets atEOF and atEOL to false, comment and token to the empty string, fileColumn and fileLine to 1, filepos to 0,
	labileFlags to 0 and saved and special to the null character. Initializes the istream reference data
	member in to the supplied istream `i'.
*/
NxsToken::NxsToken(
  istream &i)	/* the istream object to which the token is to be associated */
  : inputStream(i),
	eofAllowed(true)
	{
	posOffBy = 0;
	atEOF		= false;
	atEOL		= false;
	comment.clear();
	fileColumn	= 1L;
	fileLine	= 1L;
	labileFlags	= 0;
	saved		= '\0';
	special		= '\0';

	whitespace[0]  = ' ';
	whitespace[1]  = '\t';
	whitespace[2]  = '\n';
	whitespace[3]  = '\0';
#	if defined(NEW_NXS_TOKEN_READ_CHAR)
		nextCharInStream = 'a';	//anything other than EOF will work
		AdvanceToNextCharInStream();
#	endif
    this->isPunctuationFn = &(NxsString::IsNexusPunctuation);
	}

/*!
	Nothing needs to be done; all objects take care of deleting themselves.
*/
NxsToken::~NxsToken()
	{
	}

/*!
	Reads rest of comment (starting '[' already input) and acts accordingly. If comment is an output comment, and if
	an output stream has been attached, writes the output comment to the output stream. Otherwise, output comments are
	simply ignored like regular comments.
	If the labileFlag bit saveCommandComments is in effect, and we are NOT in the middle of a token then the comment
		(without the [] braces) will be stored in token.
	All other comments are stored as embeddedComments.
	Returns true if a command comment was read and stored as the token
*/
bool NxsToken::GetComment()
	{
	// Set comment level to 1 initially.  Every ']' encountered reduces
	// level by one, so that we know we can stop when level becomes 0.
	//
	NxsString currentComment;
	bool command = false;

	bool formerEOFAllowed = eofAllowed;
	eofAllowed = false;
	try
		{
		char ch = GetNextChar();
		// See if first character is the output comment symbol ('!')
		// or command comment symbol (&)
		//
		int printing = 0;
		if (ch == '!')
			printing = 1;
		else if (ch == '&' && (labileFlags & saveCommandComments) && token.empty())
			command = true;
		currentComment.push_back(ch);
		if (ch != ']')
			{
			int level = 1;
			for(;;)
				{
				ch = GetNextChar();
				if (ch == ']')
					{
					level--;
					if (level == 0)
						break;
					}
				else if (ch == '[')
					level++;
				currentComment.push_back(ch);
				}

			if (printing)
				{
				// Allow output comment to be printed or displayed in most appropriate
				// manner for target operating system
				//
				NxsString foroutput(currentComment.c_str() + 1);
				comment = foroutput;
				OutputComment(foroutput);
				}
			if (command)
				token = currentComment;
			else
				embeddedComments.push_back(NxsComment(currentComment, GetFileLine(), GetFileColumn()));
			}
		}
	catch (NxsX_UnexpectedEOF & x)
		{
		x.msg << " (end-of-file inside comment)";
		eofAllowed = formerEOFAllowed;
		throw x;
		}
	eofAllowed = formerEOFAllowed ;
	return command;
	}

/*!
	Reads rest of a token surrounded with curly brackets (the starting '{' has already been input) up to and including
	the matching '}' character. All nested curly-bracketed phrases will be included.
*/
void NxsToken::GetCurlyBracketedToken()
	{
	bool formerEOFAllowed = eofAllowed;
	eofAllowed = false;
	try
		{
		int level = 1;
		while(level > 0)
			{
			char ch = GetNextChar();
			if (ch == '}')
				level--;
			else if (ch == '{')
				level++;
			AppendToToken(ch);
			}
		}
	catch (NxsX_UnexpectedEOF & x)
		{
		x.msg << " (end-of-file inside {} braced statement)";
		eofAllowed = formerEOFAllowed;
		throw x;
		}
	eofAllowed = formerEOFAllowed ;
	}

/*!
	Gets remainder of a double-quoted NEXUS word (the first double quote character was read in already by GetNextToken).
	This function reads characters until the next double quote is encountered. Tandem double quotes within a
	double-quoted NEXUS word are not allowed and will be treated as the end of the first word and the beginning of the
	next double-quoted NEXUS word. Tandem single quotes inside a double-quoted NEXUS word are saved as two separate
	single quote characters; to embed a single quote inside a double-quoted NEXUS word, simply use the single quote by
	itself (not paired with another tandem single quote).
*/
void NxsToken::GetDoubleQuotedToken()
	{
	bool formerEOFAllowed = eofAllowed;
	eofAllowed = false;
	try
		{
		for(;;)
			{
			char ch = GetNextChar();
			if (ch == '\"')
				break;
			else
				AppendToToken(ch);
			}
		}
	catch (NxsX_UnexpectedEOF & x)
		{
		x.msg << " (end-of-file inside \" quoted statement)";
		eofAllowed = formerEOFAllowed;
		throw x;
		}
	eofAllowed = formerEOFAllowed ;
	}

/*!
	Gets remainder of a quoted NEXUS word (the first single quote character was read in already by GetNextToken). This
	function reads characters until the next single quote is encountered. An exception occurs if two single quotes occur
	one after the other, in which case the function continues to gather characters until an isolated single quote is
	found. The tandem quotes are stored as a single quote character in the token NxsString.
*/
void NxsToken::GetQuoted()
	{
	bool formerEOFAllowed = eofAllowed;
	eofAllowed = false;
	long fl = fileLine;
	long fc = fileColumn;

	try
		{
		for(;;)
			{
			char ch = GetNextChar();
			if (ch == '\'')
				{
				ch = GetNextChar();
				if (ch == '\'')
					AppendToToken(ch);
				else
					{
					saved = ch;
					break;
					}
				}
			else
				AppendToToken(ch);
			}
		}
	catch (NxsX_UnexpectedEOF & x)
		{
		x.msg << " (end-of-file inside \' quoted token that started on line " << fl<< ", column " <<fc << ')';
		eofAllowed = formerEOFAllowed;
		throw x;
		}
	eofAllowed = formerEOFAllowed ;
	}

/*!
	Reads rest of parenthetical token (starting '(' already input) up to and including the matching ')' character.  All
	nested parenthetical phrases will be included.
*/
void NxsToken::GetParentheticalToken()
	{
	// Set level to 1 initially.  Every ')' encountered reduces
	// level by one, so that we know we can stop when level becomes 0.
	//
	int level = 1;
	std::vector<NxsComment> prevEmbedded = embeddedComments;
	embeddedComments.clear();
	char ch;
	ch = GetNextChar();
	for(;;)
		{
		if (atEOF)
			break;

		if (ch == '\'')
			{
			AppendToToken('\'');
			GetQuoted();
			AppendToToken('\'');
			ch = saved;
			saved = '\0';
			if (atEOF)
				{
				if (ch == ')' && level == 1)
					{
					AppendToToken(')');
					break;
					}
				else
					{
					NxsX_UnexpectedEOF x(*this);
					x.msg << "(end-of-file inside () statement)";
					}
				}
			continue;
			}
		if (ch == '[')
			{
			GetComment();
			assert(embeddedComments.size() == 1);
			AppendToToken('[');
			const std::string & body =  embeddedComments[0].GetText();
			token.append(body.begin(), body.end());
			AppendToToken(']');
			embeddedComments.clear();

			}
		else
			{
			if (ch == ')')
				level--;
			else if (ch == '(')
				level++;

			AppendToToken(ch);
			}

		if (level == 0)
			break;
		ch = GetNextChar();
		}
	embeddedComments = prevEmbedded;
	}

/*!
	Returns true if token begins with the capitalized portion of `s' and, if token is longer than `s', the remaining
	characters match those in the lower-case portion of `s'. The comparison is case insensitive. This function should be
	used instead of the Begins function if you wish to allow for abbreviations of commands and also want to ensure that
	user does not type in a word that does not correspond to any command.
*/
bool NxsToken::Abbreviation(
  NxsString s)	/* the comparison string */
	{
	int k;
	int slen = (int)s.size();
	int tlen = (int)token.size();
	char tokenChar, otherChar;

	// The variable mlen refers to the "mandatory" portion
	// that is the upper-case portion of s
	//
	int mlen;
	for (mlen = 0; mlen < slen; mlen++)
		{
		if (!isupper(s[mlen]))
			break;
		}

	// User must have typed at least mlen characters in
	// for there to even be a chance at a match
	//
	if (tlen < mlen)
		return false;

	// If user typed in more characters than are contained in s,
	// then there must be a mismatch
	//
	if (tlen > slen)
		return false;

	// Check the mandatory portion for mismatches
	//
	for (k = 0; k < mlen; k++)
		{
		tokenChar = (char)toupper( token[k]);
		otherChar = s[k];
		if (tokenChar != otherChar)
			return false;
		}

	// Check the auxiliary portion for mismatches (if necessary)
	//
	for (k = mlen; k < tlen; k++)
		{
		tokenChar = (char)toupper( token[k]);
		otherChar = (char)toupper( s[k]);
		if (tokenChar != otherChar)
			return false;
		}

	return true;
	}

/*!
	Returns true if token NxsString begins with the NxsString `s'. This function should be used instead of the Equals
	function if you wish to allow for abbreviations of commands.
*/
bool NxsToken::Begins(
  NxsString s,			/* the comparison string */
  bool respect_case)	/* determines whether comparison is case sensitive */
	{
	unsigned k;
	char tokenChar, otherChar;

	unsigned slen = (unsigned)s.size();
	if (slen > token.size())
		return false;

	for (k = 0; k < slen; k++)
		{
		if (respect_case)
			{
			tokenChar = token[k];
			otherChar = s[k];
			}
		else
			{
			tokenChar = (char)toupper( token[k]);
			otherChar = (char)toupper( s[k]);
			}

		if (tokenChar != otherChar)
			return false;
		}

	return true;
	}

/*!
	Reads characters from in until a complete token has been read and stored in token. GetNextToken performs a number
	of useful operations in the process of retrieving tokens:
~
	o any underscore characters encountered are stored as blank spaces (unless the labile flag bit preserveUnderscores
	  is set)
	o if the first character of the next token is an isolated single quote, then the entire quoted NxsString is saved
	  as the next token
	o paired single quotes are automatically converted to single quotes before being stored
	o comments are handled automatically (normal comments are treated as whitespace and output comments are passed to
	  the function OutputComment which does nothing in the NxsToken class but can be overridden in a derived class to
	  handle these in an appropriate fashion)
	o leading whitespace (including comments) is automatically skipped
	o if the end of the file is reached on reading this token, the atEOF flag is set and may be queried using the AtEOF
	  member function
	o punctuation characters are always returned as individual tokens (see the Maddison, Swofford, and Maddison paper
	  for the definition of punctuation characters) unless the flag ignorePunctuation is set in labileFlags,
	  in which case the normal punctuation symbols are treated just like any other darkspace character.
~
	The behavior of GetNextToken may be altered by using labile flags. For example, the labile flag saveCommandComments
	can be set using the member function SetLabileFlagBit. This will cause comments of the form [&X] to be saved as
	tokens (without the square brackets), but only for the aquisition of the next token. Labile flags are cleared after
	each application.
*/
void NxsToken::GetNextToken()
	{
	ResetToken();

	char ch = ' ';
	if (saved == '\0' || IsWhitespace(saved))
		{
		// Skip leading whitespace

		while( IsWhitespace(ch) && !atEOF)
			ch = GetNextChar();
		saved = ch;
		}

	for(;;)
		{
		// Break now if singleCharacterToken mode on and token length > 0.
		//
		if (labileFlags & singleCharacterToken && !token.empty())
			break;

		// Get next character either from saved or from input stream.
		//
		if (saved != '\0')
			{
			ch = saved;
			saved = '\0';
			}
		else
			ch = GetNextChar();
		// Break now if we've hit EOF.
		//
		if (atEOF)
			break;
		if (strchr("\n\r \t", ch) != NULL)//!isgraph(ch))
			{
			if (ch == '\n' && labileFlags & newlineIsToken)
				{
				if (token.empty())
					{
					atEOL = 1;
					AppendToToken(ch);
					}
				else
					{
					// Newline came after token, save newline until next time when it will be
					// reported as a separate token.
					//
					atEOL = 0;
					saved = ch;
					}
				break;
				}
			else
				{
				// Break only if we've begun adding to token (remember, if we hit a comment before a token,
				// there might be further white space between the comment and the next token).
				//
				if (!token.empty())
					break;
				}
			}
		else if (ch == '_')
			{
			// If underscores are discovered in unquoted tokens, they should be
			// automatically converted to spaces.
			//
			if (!(labileFlags & preserveUnderscores))
				ch = ' ';
			AppendToToken(ch);
			}

		else if (ch == '[')
			{
			// Get rest of comment and deal with it, but notice that we only break if the comment ends a token,
			// not if it starts one (comment counts as whitespace). In the case of command comments
			// (if saveCommandComment) GetComment will add to the token NxsString, causing us to break because
			// token.size() will be greater than 0.
			comment.clear();
			if (GetComment())
				break;
			}
		else if (IsPunctuation(ch))
			{
			if (ch == '(' && (labileFlags & parentheticalToken))
				{
				AppendToToken(ch);
				GetParentheticalToken();
				}
			else if (ch == '{' && (labileFlags & curlyBracketedToken))
				{
				AppendToToken(ch);
				GetCurlyBracketedToken();
				}
			else if (ch == '\"' && (labileFlags & doubleQuotedToken))
				GetDoubleQuotedToken();
			else if (ch == '\'' && token.empty())
				GetQuoted();
			else
				{
				//save if we have started a token, consider the punctuation to
				// be the full token.
				if (token.size() > 0)
					saved = ch;
				else
					AppendToToken(ch);
				}
			break;
			}
		else
			AppendToToken(ch);
		}

	labileFlags = 0;
	}

/*!
	Strips whitespace from currently-stored token. Removes leading, trailing, and embedded whitespace characters.
*/
void NxsToken::StripWhitespace()
	{
	NxsString s;
	for (unsigned j = 0; j < token.size(); j++)
		{
		if (IsWhitespace( token[j]))
			continue;
		s += token[j];
		}
	token = s;
	}

/*!
	Converts all alphabetical characters in token to upper case.
*/
void NxsToken::ToUpper()
	{
	for (unsigned i = 0; i < token.size(); i++)
		token[i] = (char)toupper(token[i]);
	}


void NxsToken::UseNewickTokenization(bool v)
    {
    if (v)
        {
        this->isPunctuationFn = &(NxsString::IsNewickPunctuation);
        }
    else
        {
        this->isPunctuationFn = &(NxsString::IsNexusPunctuation);
        }
    }    
