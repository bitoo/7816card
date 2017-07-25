/*
	Simple Operating System for Smartcard Education
	Copyright (C) 2002  Matthias Bruestle <m@mbsks.franken.de>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*! @file
	\brief File stream functions.

	$Id: fstream.c,v 1.11 2002/12/22 15:42:55 m Exp $
*/

//#include "sw.h"
#include "fstream.h"
//#include <hal.h>
#include "..\export_new\types.h"
#include "filePubFunc.h"

bool fstream_read( S_FSTREAM *fs, iu8 *dst, iu8 len )
{
	if( !( fstream_test( fs, len ) &&
		hal_eeprom_read( dst, fs->start+fs->pos, len ) ) ) return FALSE;/**/ //delete for temp
	fs->pos += len;  
	return TRUE;
}


bool fstream_test( const S_FSTREAM *fs, iu16 len )
{
	if( fs->pos+len > fs->size ) 
	{
//		sw_set( SW_FILE_TO_SHORT );
//		return FALSE;
	}
	return TRUE;
}

//#endif
