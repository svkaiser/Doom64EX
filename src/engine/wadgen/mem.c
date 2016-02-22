// Emacs style mode select       -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: Mem.c 744 2010-08-01 05:13:52Z svkaiser $
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// $Author: svkaiser $
// $Revision: 744 $
// $Date: 2010-07-31 22:13:52 -0700 (Sat, 31 Jul 2010) $
//
// DESCRIPTION: Memory allocation stuff
//
//-----------------------------------------------------------------------------
#ifdef RCSID
static const char rcsid[] = "$Id: Mem.c 744 2010-08-01 05:13:52Z svkaiser $";
#endif

#include "wadgen.h"
#include <stdlib.h>

//**************************************************************
//**************************************************************
//      Mem_Alloc
//**************************************************************
//**************************************************************

void *Mem_Alloc(int size)
{
	void *ret = calloc(1, size);
	if (!ret)
		WGen_Complain("Mem_Alloc: Out of memory");

	return ret;
}

//**************************************************************
//**************************************************************
//      Mem_Free
//**************************************************************
//**************************************************************

void Mem_Free(void **ptr)
{
	if (!*ptr)
		WGen_Complain("Mem_Free: Tried to free NULL");

	free(*ptr);
	*ptr = NULL;
}
