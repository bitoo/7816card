#ifndef _FILEPUBFUNC_H_
#define _FILEPUBFUNC_H_

#include "..\export_new\fileapi.h"
#include "fs.h"

#define BLOCKLEN 0x100
//�ϵ籣��b
#define POWERADDR 0xfc00	
//�ϵ籣��e

bool hal_eeprom_read( iu8 *dst, iu32 src, iu8 len );
bool writeFlash(iu32 des,iu8 *src,iu16 len);
#endif
