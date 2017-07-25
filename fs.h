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
	\brief File system declarations.

	SOSSE implements a very simple file system. Each file starts with
	following header:

	- 2 Byte: Content size
	- 2 Byte: ID
	- 1 Byte: Type
	- 1 Byte: Access conditions

	Directly after that the content area is located. If the file is a
	DF, this content area is directly used to add header/content
	sequences.

	After the content area of a file, a content size of 0 must be
	writen as an end of data marker. This means the data capacity
	of a MF or DF is the total content size of it minus 2.

	Currently the file system support only one level of DFs, i.e.
	the maximum depth is MF:DF:EF.

	There are probably a lot optimization possibilities in this
	file system, but I think this is not bad for my first designed and
	implemented file system.

	\todo
	Here are certainly some optimizations possible to reduce code size.

	$Id: fs.h,v 1.13 2002/12/22 15:42:55 m Exp $
*/

#ifndef SOSSE_FS_H
#define SOSSE_FS_H

#include "..\export_new\types.h"
#include "fstream.h"

/*! \brief File path specifier.

	\todo
	There is probably a potential for simplification, when the file
	type (DF/EF) is not handled so rigid.
*/
typedef struct s_fpath {
	iu16 df;	//!< ID of one DF level. Set to FFFF if unused.
	iu16 ef;	//!< IF of the EF. Set to FFFF if unused.
} S_FPATH;

/*! \brief File info struct. This is directly written as a file header in the
	file system.
*/
typedef struct s_finfo {
	iu16 size;	//!< Size of the file content.
	iu16 fid;		//!< ID of the file.
	iu8 type;		//!< Type of the file. (FS_TYPE_*)
	iu8 ac;	//!< Access conditions of the file. (See CREATE FILE dox.)
	iu16 DF;
	iu8 Ftype; //0 Binary,1 record,2 circle record
	iu8 circleNo;
	iu16 rfu16;
} S_FINFO;

//! File type DF
#define FS_TYPE_DF	0x38
//! File type EF
#define FS_TYPE_EF	0x00

//! Size of the file header, which is the same as S_FINFO.
#define FS_HEADER_SIZE	sizeof(S_FINFO)

/*! \brief Path specified of the currently selected file.

	\todo
	Saving here a S_FINFO and/or S_FSTREAM of the selected file might
	reduce the code size.
*/
extern S_FPATH selected;

/*! \brief Initialize file system variables.

	This looks e.g. for the start of the file system and it's size.
	It must be called after each reset.

    \retval TRUE on success.
    \retval FALSE on failure. Error code given in sw.
*/
bool fs_init( void );
/*! \brief Look for a file in a stream.

    \param fid File ID to look for.
    \param fs Pointer to S_FSTREAM, where the FID is searched for. It returns
		on success a stream on the file found.
	\param fi Pointer to S_FINFO, which will be filled with data on success.
	\param type Type of file wanted.

    \retval TRUE on success.
    \retval FALSE on failure. Error code given in sw.

	\todo
	There is probably a potential for simplification, when the file
	type is not handled so rigid.
*/
bool fs_seek( iu16 fid, S_FSTREAM *fs, S_FINFO *fi, iu8 type );		 
/*! \brief Look for the end of files in the stream.

    \param fs Pointer to S_FSTREAM, which supplies a directory stream and
		which will be used to return the end position on success.

    \retval TRUE on success.
    \retval FALSE on failure. Error code given in sw.
*/


#endif /* SOSSE_FS_H */

