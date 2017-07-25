#include "types.h"
//#include "fstream.h"
#include "..\..\Target_Driver\export_new\simcommand.h"
//#include "apdu.h"

#ifndef _FILEAPI_H_
#define _FILEAPI_H_

#define uint8 unsigned char                     /* defined for unsigned 8-bits integer variable 	无符号8位整型变量  */
#define uint16 unsigned short                   /* defined for unsigned 16-bits integer variable 	无符号16位整型变量 */
#define uint32 unsigned long                   /* defined for unsigned 16-bits integer variable 	无符号16位整型变量 */
#define HANDLE unsigned char 
//extern xdata uint8 g_pbyProMenuBuf[277];	//  menu Buffer
//#define FILE_BUFFER_LEN 277

//uint16 FILE_SW1SW2;
extern uint16 FILE_SW1SW2;

HANDLE OSFileOpen(uint16 FileID, uint8 byMode);
uint8 OSFileClose(HANDLE FileHandle);
bool OSFileSeek(HANDLE FileHandle,uint16 uioffset,uint8 byMode);
bool OSFileInit(void);
bool OSFileRead(uint8 *pbyDest,uint16 uiLen,HANDLE FileHandle);
bool OSFileWrite(uint8 *pbyDest,uint16 uiLen,HANDLE FileHandle,uint8 byMode);
bool OSFileCreate(uint16 fileSize,uint16 fileID,uint8 type,uint8 ac);//!< Access conditions of the file. (See CREATE FILE dox.)
bool OSFileCopy(HANDLE SrcFileHandle,HANDLE DestFileHandle,uint16 unLen);
bool PIN_PUK_Verify(uint8 *pbySrc,uint8);
bool PIN_Change(uint8 *pbySrc,uint8 PIN[10],uint8 PUKFlag);
bool PIN_Unblock(uint8 *pbySrc,uint8 PIN[10]);
bool FileAttribute(uint16 fileID,uint8 attribute[3]);
bool OSFileRecWrite(uint8 *pbyDest,HANDLE FileHandle,uint8 recNo,uint8 offset,uint8 len);
bool OSFileRecRead(uint8 *pbyDest,HANDLE FileHandle,uint8 recNo,uint8 offset,uint8 readLen);
bool SelectFile_CUP(uint8 *LV,uint8 *outBuff);
bool Get_Challenge(uint8 *LV,uint8 *Random);
bool Updata_Binary(uint8 *LV,uint8 *outBuff,uint16 FILE_ID);
bool Write_Key(uint8* LV,uint8 *Random,uint8 *outBuff);
bool Read_Binary(uint8 *LV,uint8 *outBuff,uint16 FILE_ID);
bool  Updata_Record(uint8 *LV,uint8 recNo,uint16 FILEID,uint8 offset);


#endif
