#ifndef _FILEPUBFUNC_H_
#define _FILEPUBFUNC_H_

#include "..\export_new\fileapi.h"
#include "fs.h"

#define BLOCKLEN 0x100
//断电保护b
#define POWERADDR 0xfc00	
//断电保护e

bool hal_eeprom_read( iu8 *dst, iu32 src, iu8 len );
bool writeFlash(iu32 des,iu8 *src,iu16 len);
#endif
