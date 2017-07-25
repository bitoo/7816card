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
	\brief File system.

	$Id: fs.c,v 1.23 2002/12/22 15:42:55 m Exp $
*/

#include "..\export_new\config.h"
//#include "sw.h"
#include "fs.h"
//#include <hal.h>
//#include <log.h>
//#include <string.h>
//#include "transaction.h"
#include "..\export_new\types.h"
#include "filePubFunc.h"
//#include <ABSACC.H>
#include "..\..\Target_Driver\export\simcommand.h"
//! Start address of the MF file header.
iu32 fsstart;
//! Size of the content area of the MF.
static iu16 fssize;

//for test beging
//extern iu32 addr;
//extern iu32 addr1;
//for test end

S_FPATH selected;
/*
void AnswerSW(uint16 bylen)
{
	Sim_SendSCommChar0((uint8)(bylen>>8),ME);
	Sim_SendSCommChar0((uint8)bylen,ME);
}
*/
bool fs_init( void )
{
	//Get file head start address
	if( !hal_eeprom_read( (iu8*)&fsstart, FS_START_PTR_ADDR,
		sizeof(fsstart) ) )
		return FALSE;
	/*fsstart =  FS_START_PTR_ADDR;*/
	//for test beging
//	addr = 	 fsstart;
	//for test end
	//AnswerSW(fsstart);

	if( !hal_eeprom_read( (iu8*)&fssize, fsstart, sizeof(fssize) ) )
		return FALSE;


	//Init selected file to be null
	//MemorySet( (iu8*)(&selected), 0xFF, sizeof(selected) );/**/  
	selected.df = 0xffff;
	selected.ef = 0xffff;

	return TRUE;
}

bool fs_seek( iu16 fid, S_FSTREAM *fs, S_FINFO *fi, iu8 type )
{
//	iu8 i=0;
	//extern iu8 tBuffer[100];
	/* We have nothing to do */
	if( fid==0xFFFF ) return TRUE;

	/* We want MF */
	if( fid==0x3F00 ) {
		fs->start=fsstart;
		fs->size=fssize;
		fs->pos=0;
		return TRUE;
	}

	type = type;

	

	while( fstream_read( fs, (iu8 *)fi, sizeof(iu16) ) ) {
		//fs->pos-=sizeof(iu16);
		if( !fi->size ) {
//			sw_set( SW_FILE_NOT_FOUND );
			return FALSE;	      
		}						 	
		if( !fstream_read( fs, (iu8 *)fi+sizeof(iu16),
			sizeof(S_FINFO)-sizeof(iu16) ) ) break;

	 	
//////////////////////////////////////
			//if(i)		AnswerSW(fs->pos+0x9100);   //for temp
			//i++;
////////////////////////////////////
		if( fid==fi->fid ) 
		{	    			
			/*if( fi->type!=type ) {	     
				return FALSE;
			}*///delete in 2010-8-18 chen
			fs->start+=fs->pos;
			fs->size=fi->size;
			fs->pos=0;  
			return TRUE;
		} 
		else 
		{
			fs->pos+=fi->size;
			//if(!(i--))		AnswerSW(fs->pos+0x9100);   //for temp
			//tBuffer[i] = fs->pos;
			//i++;
			if(fs->pos>fs->size)	//add by chenjingbo 用于如果当前的文件太长，指向下一个文件
			{
				fs->start += (fs->size+sizeof(S_FINFO));
				//fs->size=CBYTE[fs->start]*16+CBYTE[fs->start+1];
				fs->size=(uint16)(Sim_ReadCode(fs->start)*0x100+Sim_ReadCode(fs->start+1));
				fs->pos = 0; 
			}
		}
		//AnswerSW(fs->size);   //for temp
	}

//	sw_set( SW_FILE_NOT_FOUND );

	return FALSE;  

}
