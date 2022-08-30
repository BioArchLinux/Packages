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

#ifndef NCL_NXSEXCEPTION_H
#define NCL_NXSEXCEPTION_H

#include "ncl/nxsstring.h"

class NxsToken;
class ProcessedNxsToken;
class NxsTokenPosInfo;
/*!
	Exception class that conveys a message specific to the problem encountered.
*/
class NxsException: public std::exception
	{
	public:
		mutable NxsString	msg;	/* NxsString to hold message */
		file_pos	pos;	/* current file position */
		long		line;	/* current line in file */
		long		col;	/* column of current line */
		virtual ~NxsException() throw()
			{
			}

		NxsException(const std::string & s, file_pos fp = 0, long fl = 0L, long fc = 0L);
		NxsException(const std::string &s, const NxsToken &t);
		NxsException(const std::string &s, const ProcessedNxsToken &t);
		NxsException(const std::string &s, const NxsTokenPosInfo &t);
		const char * what () const throw ()
			{
			return msg.empty() ? "Unknown Nexus Exception" : msg.c_str();
			}
		const char * nxs_what () const;
		void addPositionInfo(const NxsToken & t);
		void addPositionInfo(const ProcessedNxsToken & t);
		void addPositionInfo(const NxsTokenPosInfo & t);
		void addPositionInfo(file_pos fp, long fl, long fc);
	};

typedef NxsException XNexus;

/*!
	Thrown when a programming error (a violation of one of the APIs used in NCL) is revealed.
*/
class NxsNCLAPIException: public NxsException
	{
	public:
		NxsNCLAPIException(NxsString s) :NxsException(s, 0, -1L,-1L){}
		NxsNCLAPIException(NxsString s, NxsToken &t) :NxsException(s, t){}
	};

/*!
	Thrown when an unimplemented method is called.
*/
class NxsUnimplementedException: public NxsNCLAPIException
	{
	public:
		NxsUnimplementedException(NxsString s):NxsNCLAPIException(s){}
		NxsUnimplementedException(NxsString s, NxsToken &t):NxsNCLAPIException(s,t){}
	};


class DuplicatedLabelNxsException: public NxsException
	{
	public:
		DuplicatedLabelNxsException(const std::string & s):NxsException(s){}
	};


/*------------------------------------------------------------------------------
 This exception will be thrown if NCL signal handling is activated (static
	methods in NxsReader control this) and a SIGINT is detected during a
	parse.
*/
class NxsSignalCanceledParseException: public NxsException
	{
	public:
		NxsSignalCanceledParseException(const std::string & s);
	};
#endif
