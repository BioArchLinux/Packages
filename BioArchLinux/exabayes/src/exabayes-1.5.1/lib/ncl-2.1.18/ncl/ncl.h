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

#ifndef NCL_NCL_H
#define NCL_NCL_H

#if !defined(__DECCXX)
#	include <cctype>
#	include <cmath>
#	include <cstdarg>
#	include <cstdio>
#	include <cstdarg>
#	include <cstdlib>
#	include <ctime>
#	include <climits>
#	include <cfloat>
#else
#	include <ctype.h>
#	include <stdarg.h>
#	include <math.h>
#	include <stdarg.h>
#	include <stdio.h>
#	include <stdlib.h>
#	include <time.h>
#	include <float.h>
#endif

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#if defined(__GNUC__)
#	if __GNUC__ < 3
#		include <strstream>
#	else
#		include <sstream>
#	endif
#endif
#include <vector>

# if ! defined (NCL_AVOID_USING_STD)
	using namespace std;
#endif

#if defined( __BORLANDC__ )
#	include <dos.h>
#endif

#if defined(__MWERKS__)
#	define HAVE_PRAGMA_UNUSED
		// mwerks (and may be other compilers) want return values even if the function throws an exception
		//
#	define DEMANDS_UNREACHABLE_RETURN

#endif

#include "ncl/nxsdefs.h"
#include "ncl/nxsstring.h"
#include "ncl/nxsexception.h"
#include "ncl/nxstoken.h"
#include "ncl/nxsblock.h"
#include "ncl/nxsreader.h"
#include "ncl/nxssetreader.h"
#include "ncl/nxstaxablock.h"
#include "ncl/nxstreesblock.h"
#include "ncl/nxsdistancedatum.h"
#include "ncl/nxsdistancesblock.h"
#include "ncl/nxsdiscretedatum.h"
#include "ncl/nxscharactersblock.h"
#include "ncl/nxsassumptionsblock.h"
#include "ncl/nxsdatablock.h"
#include "ncl/nxsunalignedblock.h"
#include "ncl/nxspublicblocks.h"
#include "ncl/nxsmultiformat.h"

#endif
