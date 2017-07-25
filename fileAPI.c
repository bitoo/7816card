#include "..\export_new\fileAPI.h"
//#include "fstream.h"
#include "fs.h"
#include "filePubFunc.h"

//#include <absacc.h>

#define MAXOPENFILE	8

//for test beging
//extern iu32 addr;
//extern iu32 addr1;
//for test end

typedef struct {
	HANDLE handle;//���ڴ�Ŵ��ļ��ľ������1��ʼ��5�����Ϊ00����ff��Ϊ��Ч
	uint16 fileID;//��ž����Ӧ���ļ�ID
	uint8 mode;		//���ļ���ģʽ������ģʽ��1��ֻ����2�����Ը�д
	iu32 SeekAddr;
	iu32 addr;
	iu16 SeekSize;
	S_FSTREAM Seekfs;
	S_FINFO fi;
}SFileHandle;

bit PIN_VER_FLAG;

extern uint16 CUP_SW1SW2;

//typedef unsigned char HANDLE;
static SFileHandle OpenedFile[MAXOPENFILE];
//! Start address of the MF file header.
extern iu32 fsstart;


bool Is_Key_File(HANDLE FileHandle);
uint8 Read_Access(HANDLE FileHandle);
bool Write_Access(HANDLE FileHandle);

extern void CUP_APDU_INIT();
/*
void AnswerSW(uint16 bylen)
{
	Sim_SendSCommChar0((uint8)(bylen>>8),ME);
	Sim_SendSCommChar0((uint8)bylen,ME);
}
*/

bool OSFileInit(void)
{
xdata uint8 g_file_Pub_Buffer[0x101];
	uint32 addr;
	uint16 i,j;

	//g_file_Pub_Buffer = g_pbyProMenuBuf;

//�ϵ籣��b
	for(i=0;i<8;i++)
	{
		g_file_Pub_Buffer[i] = Sim_ReadCode(POWERADDR+i);
	}
	if(1==g_file_Pub_Buffer[7])
	{
		addr = 0;
		for(i=0;i<4;i++)
		{
			addr |= g_file_Pub_Buffer[i];
			addr<<=8;
		}
		
		for(j=1;j<g_file_Pub_Buffer[i]+1;j++)
		 {
			for(i=0;i<BLOCKLEN;i++)
			{
				g_file_Pub_Buffer[i] = Sim_ReadCode(addr+i+j*BLOCKLEN);
			}
			Sim_EraseFlash(addr+j*BLOCKLEN,0x00);
			for(i=0;i<BLOCKLEN;i++)
			{
				Sim_WriteCode((addr+j*BLOCKLEN+i),*(g_file_Pub_Buffer+i));
			} 
		}	
	}
	
	g_file_Pub_Buffer[7] =  2;
	Sim_EraseFlash(POWERADDR,0x00);
	for(i=0;i<8;i++)
	{
		Sim_WriteCode((POWERADDR+i),*(g_file_Pub_Buffer+i));
	} 	

//�ϵ籣��e	
	
	for(i = 0;i<MAXOPENFILE;i++)
	{
		OpenedFile[i].handle = 0xff;
	}
//	g_SeekHandle = 0;
	PIN_VER_FLAG = 0;
	CUP_APDU_INIT();
	return fs_init();	
}

HANDLE OSFileOpen(uint16 FileID, uint8 byMode)
{
	uint8 i;
	
	S_FSTREAM fs;
	S_FINFO fi;
	//�鿴�ļ��Ƿ���ʵ����
	fs.start=fsstart+sizeof(S_FINFO);
	//fs.size=CBYTE[fs.start]*16+CBYTE[fs.start+1];
	fs.size=(uint16)(Sim_ReadCode(fs.start)*0x100+Sim_ReadCode(fs.start+1));//CWORD[fs.start];
	fs.pos=0;

	//AnswerSW(fs.start);//for temp
	if(!fs_seek(FileID, (S_FSTREAM*)(&fs), (S_FINFO*)(&fi), FS_TYPE_EF)) return NOTHISFILE;//0xfe;
	
	//AnswerSW(fs.start);//for temp
	//�����ļ��Ƿ��Ѿ��򿪣�����Ѿ���ֻ���޸�ģʽ
	for(i=0;i<MAXOPENFILE;i++)
	{
		if(OpenedFile[i].fileID == FileID) break;
	}
	if((i<MAXOPENFILE)&&(OpenedFile[i].handle<=MAXOPENFILE)) 
	{
		OpenedFile[i].mode = byMode;
		return OpenedFile[i].handle;
	}

	//���ļ��Ƿ��Ѿ��򿪵������Ŀ
	for(i=0;i<MAXOPENFILE;i++)
	{
		if((OpenedFile[i].handle == 0xff)||(OpenedFile[i].handle == 0)||(OpenedFile[i].handle > MAXOPENFILE)) break;
	}
	if(i>=MAXOPENFILE)	return OPENEDFILEFASLE;		

	OpenedFile[i].handle = i+1;
	OpenedFile[i].fileID = FileID;
	OpenedFile[i].mode = byMode;
	OpenedFile[i].SeekAddr = fs.start;
	OpenedFile[i].addr = fs.start;
	OpenedFile[i].SeekSize = fi.size;
	OpenedFile[i].Seekfs = fs;
	OpenedFile[i].fi = fi;

	return OpenedFile[i].handle;
}

uint8 OSFileClose(HANDLE FileHandle)
{
	if((FileHandle==0) || (FileHandle>MAXOPENFILE)) return false;
	OpenedFile[FileHandle-1].handle = 0xff;
	OpenedFile[FileHandle-1].fileID = 0xffff;
//	g_SeekHandle = 0;
	return true;
}

bool OSFileSeek(HANDLE FileHandle,uint16 uioffset,uint8 byMode)
{
//	S_FSTREAM fs;
//	S_FINFO fi;
	//���ļ�û�д�
	if(OpenedFile[FileHandle-1].handle != FileHandle)
	{
		CUP_SW1SW2 = SW_FILE_NOT_FOUND;
		return false;
	}
//	g_SeekHandle = OpenedFile[FileHandle-1].handle;

	
	//���ģʽΪ1���Ǵӵ�ǰ��ַ��ѯ
	if (byMode == 1)
	{
		//�����ȡ�ĳ��ȳ����ļ������ȣ����ش���
		if(OpenedFile[FileHandle-1].SeekSize+OpenedFile[FileHandle-1].addr<OpenedFile[FileHandle-1].SeekAddr+uioffset) 
		{
			CUP_SW1SW2 = SW_WRONG_LENGTH;
			return false; 
		}

		OpenedFile[FileHandle-1].SeekAddr += uioffset;
		
		CUP_SW1SW2 = SW_SUCCESS;
		return true;
	}
	else
	{
		//�����ȡ�ĳ��ȳ����ļ������ȣ����ش���
		if(OpenedFile[FileHandle-1].SeekSize<=uioffset) 
		{
			CUP_SW1SW2 = SW_OFFSET_OVER;
			return false; 
		} 

	   OpenedFile[FileHandle-1].SeekAddr = OpenedFile[FileHandle-1].addr+uioffset;
	}
	//���Ҷ�Ӧ���ļ���ַ
	/*fs.start=fsstart+sizeof(S_FINFO);
	//fs.size=CBYTE[fs.start]*16+CBYTE[fs.start+1];
	fs.size=(uint16)(Sim_ReadCode(fs.start)*0x100+Sim_ReadCode(fs.start+1));

	fs.pos=0;
	if(!fs_seek( OpenedFile[FileHandle-1].fileID, &fs, &fi, FS_TYPE_EF)) return false;
	//AnswerSW(fs.start);//for temp
	OpenedFile[FileHandle-1].SeekAddr = fs.start+uioffset;//+sizeof(S_FINFO);
	OpenedFile[FileHandle-1].SeekSize = fi.size;
	OpenedFile[FileHandle-1].Seekfs = fs; */
	CUP_SW1SW2 = SW_SUCCESS;
	return true;
}	
/*
bool APDU_OSFileRead(uint8 *pbyDest,uint16 uiLen,HANDLE FileHandle)
{
	uint16 i;
	if(pbyDest == NULL || 0==uiLen)return false;

	//�鿴�ļ��Ƿ��
	if(OpenedFile[FileHandle-1].handle != FileHandle)return false;

	//�����ȡ�ĳ��ȳ����ļ������ȣ����ش���
	if(OpenedFile[FileHandle-1].SeekSize+OpenedFile[FileHandle-1].addr<OpenedFile[FileHandle-1].SeekAddr+uiLen) return false; 

	//��Կ�ļ����ɶ�
	if(Is_Key_File(FileHandle)) 
	{
		CUP_SW1SW2 = 0X6982;
		return false;
	}

	//PINȨ���ж� ����ֵ��1�ɶ���2PINУ��ɶ���3���ɶ�
	i = Read_Access(FileHandle);
	if(3 == i)
	{
	 	CUP_SW1SW2 = 0X6982;
		return false;		
	}
	else if(2 == i)
	{
		if(0==PIN_VER_FLAG)
		{
	 		CUP_SW1SW2 = 0X6982;
			return false;
		}
	}

	
		//OSFileSeek(FileHandle,0,1);
		//MemoryCopy((iu8*)g_SeekAddr,pbyDest,uiLen);
	for(i=0;i<uiLen;i++)
	{
		pbyDest[i] = Sim_ReadCode(OpenedFile[FileHandle-1].SeekAddr+i);	
	}

	OpenedFile[FileHandle-1].SeekAddr += uiLen;
//		g_SeekHandle =0;
//	}
	CUP_SW1SW2 = 0X9000;
	return true;
} */

bool OSFileRead(uint8 *pbyDest,uint16 uiLen,HANDLE FileHandle)
{
	uint16 i;
	if(pbyDest == NULL || 0==uiLen)
	{
		CUP_SW1SW2 = SW_USE_CONDITION_NOT;
		return false;
	}

	//�鿴�ļ��Ƿ��
	if(OpenedFile[FileHandle-1].handle != FileHandle)
	{
		CUP_SW1SW2 = SW_USE_CONDITION_NOT;
		return false;
	}

	//�����ȡ�ĳ��ȳ����ļ������ȣ����ش���
	if(OpenedFile[FileHandle-1].SeekSize+OpenedFile[FileHandle-1].addr<OpenedFile[FileHandle-1].SeekAddr+uiLen) 
	{
		CUP_SW1SW2 = SW_WRONG_LENGTH;
		return false; 
	}

	for(i=0;i<uiLen;i++)
	{
		pbyDest[i] = Sim_ReadCode(OpenedFile[FileHandle-1].SeekAddr+i);	
	}

	OpenedFile[FileHandle-1].SeekAddr += uiLen;

	CUP_SW1SW2 = SW_SUCCESS;
	return true;
}

/*bool APDU_OSFileWrite(uint8 *pbyDest,uint16 uiLen,HANDLE FileHandle,uint8 byMode)
{	
	uint8 result;
	if(pbyDest == NULL || 0==uiLen)
	{
		CUP_SW1SW2 = 0X6581;	
		return false;
	}

	if(OpenedFile[FileHandle-1].handle != FileHandle)
	{
		CUP_SW1SW2 = 0X6581;	
		return false;
	}

	byMode = byMode;

	//��Կ�ļ����ɶ�
	if(Is_Key_File(FileHandle)) 
	{
		CUP_SW1SW2 = 0X6982;
		return false;
	}

	//PINȨ���ж� //����ֵ��1��д��2PINУ���д��3����д
	result = Write_Access(FileHandle);
	if(3 == result)
	{
	 //	CUP_SW1SW2 = 0X6982;
		return false;		
	}
	else if(2 == result)
	{
		if(0==PIN_VER_FLAG)
		{
	 	CUP_SW1SW2 = 0X6982;
			return false;
		}
	}
	//AnswerSW( 0x9010);
	
		//if (0==byMode)
		//{
		//	OSFileSeek(FileHandle,0,0);	
		//}
		//AnswerSW(g_SeekAddr);//for temp
		//�ж��Ƿ񳬳���Χ//
		if((OpenedFile[FileHandle-1].addr+OpenedFile[FileHandle-1].Seekfs.size)<(OpenedFile[FileHandle-1].SeekAddr+uiLen))
		{
			return false;	
		}
//		g_SeekHandle =0;
		//return writeFlash((iu8*)g_SeekAddr,pbyDest, uiLen);		
//	}

	//AnswerSW(g_SeekAddr);
	result = writeFlash(OpenedFile[FileHandle-1].SeekAddr,pbyDest, uiLen);
	
	if (false == result)
	{
		return false;
	}
	OpenedFile[FileHandle-1].SeekAddr += uiLen;
	CUP_SW1SW2 = 0X9000;
	return true;
}*/

bool OSFileWrite(uint8 *pbyDest,uint16 uiLen,HANDLE FileHandle,uint8 byMode)
{	
	uint8 result;
	if(pbyDest == NULL || 0==uiLen)
	{
		CUP_SW1SW2 = 0X6581;	
		return false;
	}

	if(OpenedFile[FileHandle-1].handle != FileHandle)
	{
		CUP_SW1SW2 = 0X6581;	
		return false;
	}

	byMode = byMode;

	//�ж��Ƿ񳬳���Χ//
	if((OpenedFile[FileHandle-1].addr+OpenedFile[FileHandle-1].Seekfs.size)<(OpenedFile[FileHandle-1].SeekAddr+uiLen))
	{
		CUP_SW1SW2 = SW_WRONG_LENGTH;
		return false;	
	}

	result = writeFlash(OpenedFile[FileHandle-1].SeekAddr,pbyDest, uiLen);
	
	if (false == result)
	{
		CUP_SW1SW2 = 0X6581;
		return false;
	}
	OpenedFile[FileHandle-1].SeekAddr += uiLen;

	CUP_SW1SW2 = SW_SUCCESS;
	return true;
}

bool OSFileCreate(uint16 fileSize,uint16 fileID,uint8 type,uint8 ac)//!< Access conditions of the file. (See CREATE FILE dox.)
{
	HANDLE FileHandle;
	uint8 src[6];
	
	//���ȿ����ļ��Ƿ��Ѿ�����
	FileHandle = OSFileOpen(fileID,0);
	if((HANDLE)NOTHISFILE!=FileHandle)
	{
		return FileHandle;	
	}
	
	//�����һ���ļ�
	FileHandle=OSFileOpen(0xfefe , 0);	 
	if (OPENEDFILEFASLE == FileHandle || NOTHISFILE == FileHandle)
	{
		return OSFileClose(FileHandle);
	}
	if(false ==OSFileSeek(FileHandle, 0, 0)) return false;	

	src[0] = OSFileClose(FileHandle); 

	OpenedFile[FileHandle-1].SeekAddr -= sizeof(S_FINFO); 

	src[0] = (fileSize>>8)&0xff;
	src[1] = (fileSize)&0xff;
	src[2] = (fileID>>8)&0xff;
	src[3] = (fileID)&0xff;
	src[4] = type;
	src[5] = ac;

	//addr = 	OpenedFile[FileHandle-1].SeekAddr;

	src[0] = writeFlash(OpenedFile[FileHandle-1].SeekAddr,src, 6); 	

	OpenedFile[FileHandle-1].SeekAddr += sizeof(S_FINFO); 	
	OpenedFile[FileHandle-1].SeekAddr += fileSize;
	
	//д�����ļ�
	src[0] = 0;
	src[1] = 6;
	src[2] = 0xfe;
	src[3] = 0xfe;
	src[4] = 0;
	src[5] = 0x2f;

	src[0] = writeFlash(OpenedFile[FileHandle-1].SeekAddr,src, 6);	
	FileHandle = OSFileOpen(fileID,0);
	CUP_SW1SW2 = 0X9000;
	return FileHandle; 
}

// �������� �ļ����� �ɲ���1ָ�����ļ����� д���ļ�2���� ������3ΪҪд��ĳ��� 
//���� �棺��ʾ���Ƴɹ� ��������ʧ�� 
static bool OSFileCopy_src(HANDLE SrcFileHandle,HANDLE DestFileHandle,uint16 srcOffset,uint16 destOffset, uint16 unLen) //
{
#define FILE_BUFFER_LEN 0x101

	uint8 i,No;
	uint16 j;
	uint8 g_tmp_Pub_Buffer[FILE_BUFFER_LEN];
	
	//�ж������ļ��Ƿ��Ѿ���
	if((OpenedFile[SrcFileHandle-1].handle != SrcFileHandle)
	||(OpenedFile[DestFileHandle-1].handle != DestFileHandle))
		return false;

	//�������С��Ҫcopy�ĳ��ȣ����ش���
	if(((OpenedFile[SrcFileHandle-1].Seekfs.size-srcOffset) < unLen)
	||((OpenedFile[DestFileHandle-1].Seekfs.size-destOffset) < unLen))
		return false; 
	 
	No = unLen/FILE_BUFFER_LEN;


	if(No>0)
	{
		for(i=0;i<No;i++)
		{
			//OSFileRead(g_file_Pub_Buffer,FILE_BUFFER_LEN,SrcFileHandle);
			//OSFileWrite(g_file_Pub_Buffer,FILE_BUFFER_LEN, DestFileHandle, 0);
			for(j=0;j<FILE_BUFFER_LEN;j++)
			{
				g_tmp_Pub_Buffer[j] = Sim_ReadCode(OpenedFile[SrcFileHandle-1].addr+srcOffset+i*FILE_BUFFER_LEN+j);		
			}
			j = writeFlash(OpenedFile[DestFileHandle-1].addr+destOffset+i*FILE_BUFFER_LEN,g_tmp_Pub_Buffer, FILE_BUFFER_LEN);
		}
	}

	for(j=0;j<unLen-No*FILE_BUFFER_LEN;j++)
	{
		g_tmp_Pub_Buffer[j] = Sim_ReadCode(OpenedFile[SrcFileHandle-1].addr+srcOffset+No*FILE_BUFFER_LEN+j);		
	}
	return writeFlash(OpenedFile[DestFileHandle-1].addr+destOffset+No*FILE_BUFFER_LEN,g_tmp_Pub_Buffer, unLen-No*FILE_BUFFER_LEN);

}/**/

bool OSFileCopy(HANDLE SrcFileHandle,HANDLE DestFileHandle,uint16 unLen) //
{
   return OSFileCopy_src(SrcFileHandle,DestFileHandle,0,0, unLen);
}

/*���ܣ������ڼ�¼�ļ��в����¼
 	������pbyDest �����ݴ�ţ�FileHandle �������ļ����,recNo��ȡ�ļ�¼�ţ���1��ʼ
 	���أ�0������1���ɹ�*/
bool OSFileRecRead(uint8 *pbyDest,HANDLE FileHandle,uint8 recNo,uint8 offset,uint8 readLen)
{
	//�ж��Ƿ��Ǽ�¼�ļ�
	if(false == OpenedFile[FileHandle-1].fi.type ) 
	{
		CUP_SW1SW2 = SW_COMMAND_NOT_FILE;
		return false;
	}

   if(0==readLen)
   {
   		readLen =  OpenedFile[FileHandle-1].fi.type;
   }

   	if(OpenedFile[FileHandle-1].fi.type<(readLen+offset)) 
   	{
		//CUP_SW1SW2 = SW_WRONG_LENGTH;
		CUP_SW1SW2 = SW_RECORD_NOT_FOUND;
   		return false;
	}

	//�����ڵļ�¼����SEEK������ͬʱҲ�ж����ļ��Ƿ��Ѿ���
	if(OpenedFile[FileHandle-1].fi.Ftype==3) //ѭ���ļ�
	{
		if(false == OSFileSeek(FileHandle,recNo-1,0)) 
		{
			return false;
		}

		if(false == OSFileRead(pbyDest,1,FileHandle))
		{
			return false;
		}

		recNo = pbyDest[0];
					
		if(false == OSFileSeek(FileHandle,OpenedFile[FileHandle-1].fi.type*(recNo-1)+offset+OpenedFile[FileHandle-1].fi.circleNo,0)) return false;
	}
	else
	{	   
   		if(false == OSFileSeek(FileHandle,OpenedFile[FileHandle-1].fi.type*(recNo-1)+offset,0)) return false;
	}
   //��ȡ��¼
   if(false == OSFileRead(pbyDest,readLen,FileHandle))
   {
		return false;
   }

   CUP_SW1SW2 = SW_SUCCESS;
   return true;
}

#define CircleWriterecNo 25 //ѭ���ļ���¼����
#define CircleFlag 0x03		//ѭ���ļ���ʶ
bool OSFileCircleRead (uint8 *pbyDest,HANDLE FileHandle,uint8 recNo,uint8 offset,uint8 len)
{
	uint8 No;
	
	No = OpenedFile[FileHandle-1].fi.circleNo;	

	//�ж��Ƿ���ѭ���ļ�
	if(CircleFlag != OpenedFile[FileHandle-1].fi.Ftype ) 	
	{	
		CUP_SW1SW2 = SW_COMMAND_NOT_FILE;
		return false;
	}

	if(0 == len)
	{
	 	len = OpenedFile[FileHandle-1].fi.type;	
	}

	//��ȡ������Χ
	if(OpenedFile[FileHandle-1].fi.type<(len+offset)) 
   	{
		//CUP_SW1SW2 = SW_WRONG_LENGTH;
		CUP_SW1SW2 = SW_RECORD_NOT_FOUND;
   		return false;
	}

	if(false == OSFileSeek(FileHandle,recNo-1,0)) return false;

	if(false == OSFileRead(pbyDest,1,FileHandle))
	{
		CUP_SW1SW2 = SW_FILE_NOT_FOUND;
		return false;
	}

	if((*pbyDest-1)>No)
	{
		CUP_SW1SW2 = SW_RECORD_NOT_FOUND;
	 	return false;	
	}

	//�����ڵļ�¼����SEEK������ͬʱҲ�ж����ļ��Ƿ��Ѿ���
	if(false == OSFileSeek(FileHandle,No+OpenedFile[FileHandle-1].fi.type*(*pbyDest-1)+offset,0)) 
	{
		CUP_SW1SW2 = SW_FILE_NOT_FOUND;
		return false;
	}

	//��ȡ��¼
	if(false == OSFileRead(pbyDest,len,FileHandle))
	{
		CUP_SW1SW2 = SW_FILE_NOT_FOUND;
		return false;
	}

	CUP_SW1SW2 = SW_SUCCESS;
	return true;
}


bool OSFileCircleWrite(uint8 *pbyDest,HANDLE FileHandle,uint8 offset,uint8 len)
{
	uint8 buf[CircleWriterecNo],i,No,j;

	No = OpenedFile[FileHandle-1].fi.circleNo;
	if(CircleWriterecNo<No) 
	{
		CUP_SW1SW2 = SW_RECORD_NOT_FOUND;
		return false;
	}

	//�жϳ����Ƿ񳬹�
	if((offset+len)>OpenedFile[FileHandle-1].fi.type)
	{
		CUP_SW1SW2 = SW_WRONG_LENGTH;
		return false;
	}

	//�ж��Ƿ���ѭ���ļ�
	if(CircleFlag != OpenedFile[FileHandle-1].fi.Ftype ) 
	{
		CUP_SW1SW2 = SW_COMMAND_NOT_FILE;
   		return false;
	}

	if(false == OSFileSeek(FileHandle,0,0)) 
	{		
		return false;
	}


	if(false == OSFileRead(buf,No,FileHandle))
	{
		return false;
	} 	

	//����������Ųһλ
	for(j=No-1;j>0;j--)
	{
		buf[j] = buf[j-1] ;	
	}

	//�ҵ������϶��ռ�¼,��buf���ӵڶ�����buf[1]���� buf[No-1]
	for(i=1;i<=No;i++)
	{
		for(j=1;j<No;j++)
		{
			if(buf[j] == i)
			break;	 	
		}

		if(j == No)
		break;
	}

	buf[0] = i;

	//2д��
	//�����ڵļ�¼����SEEK������ͬʱҲ�ж����ļ��Ƿ��Ѿ���
   if(false == OSFileSeek(FileHandle,0,0)) return false;
   

	//дѭ��˵��
	if(false == OSFileWrite(buf,No,FileHandle,0))
	{
		return false;
	}

	//д��¼
	if(false == OSFileSeek(FileHandle,No+(buf[0]-1)*OpenedFile[FileHandle-1].fi.type+offset,0)) return false;


	if(0 == len)
	{
	 	len = OpenedFile[FileHandle-1].fi.type;	
	}

	if(false == OSFileWrite(pbyDest,len,FileHandle,0))
	{
		return false;
	} 

	CUP_SW1SW2 = SW_SUCCESS;
	return true;
}

bool OSFileCircleDelete(HANDLE FileHandle,uint8 recNo)
{
	uint8 buf[CircleWriterecNo],i,No;

	No = OpenedFile[FileHandle-1].fi.circleNo;
	if(CircleWriterecNo<No) return false;

  	//�ж��Ƿ���ѭ���ļ�
	if(CircleFlag != OpenedFile[FileHandle-1].fi.Ftype ) return false;


	if(false == OSFileSeek(FileHandle,0,0)) return false;

	if(false == OSFileRead(buf,No,FileHandle))
	{
		return false;
	}

	for(i=recNo-1;i<No-1;i++)
	{
		buf[i] = buf[i+1];
	}

	buf[No-1] = 0xff;

	if(false == OSFileSeek(FileHandle,0,0)) return false; 
	if(false == OSFileWrite(buf,No,FileHandle,0))  	return false; 

	return true;
} 

/*
bool APDU_OSFileRecRead(uint8 *pbyDest,HANDLE FileHandle,uint8 recNo)
{
   uint8 result;
   //�ж��Ƿ��Ǽ�¼�ļ�
   if(false == OpenedFile[FileHandle-1].fi.type ) return false;

   //��Կ�ļ����ɶ�
	if(Is_Key_File(FileHandle)) 
	{
		CUP_SW1SW2 = 0X6982;
		return false;
	}

	//PINȨ���ж� //����ֵ��1��д��2PINУ���д��3����д
	result = Write_Access(FileHandle);
	if(3 == result)
	{
	 	CUP_SW1SW2 = 0X6982;
		return false;		
	}
	else if(2 == result)
	{
		if(0==PIN_VER_FLAG)
		{
	 		CUP_SW1SW2 = 0X6982;
			return false;
		}
	}
	
   //�����ڵļ�¼����SEEK������ͬʱҲ�ж����ļ��Ƿ��Ѿ���
   if(false == OSFileSeek(FileHandle,OpenedFile[FileHandle-1].fi.type*(recNo-1),0)) return false;

   //��ȡ��¼
   if(false == OSFileRead(pbyDest,OpenedFile[FileHandle-1].fi.type,FileHandle))
   {
		return false;
   }
   CUP_SW1SW2 = 0X9000;
   return true;
}
  */
/*���ܣ������ڼ�¼�ļ��в����¼	  
 	������pbyDest �����ݴ�ţ�FileHandle �������ļ����,recNoд��ļ�¼�ţ���1��ʼ
	offset, дƫ��,len Ϊ��Ҫд��ĳ���(lenΪ0ʱд��������¼)
 	���أ�0������1���ɹ�*/
/**/

bool OSFileRecWrite(uint8 *pbyDest,HANDLE FileHandle,uint8 recNo,uint8 offset,uint8 len)
{
	//�ж��Ƿ��Ǽ�¼�ļ�
	if(0 == OpenedFile[FileHandle-1].fi.type ) 
	{
		CUP_SW1SW2 = 0x6981;
		return false;
	}

	if(0 == len)
	{
	 	len = OpenedFile[FileHandle-1].fi.type;	
	}

	if((offset+len)>OpenedFile[FileHandle-1].fi.type) 
	{
		CUP_SW1SW2 = 0x6700;
		return false;
	} 	

	//�����ڵļ�¼����SEEK������ͬʱҲ�ж����ļ��Ƿ��Ѿ���
	if(OpenedFile[FileHandle-1].fi.Ftype==3) //ѭ���ļ�
	{	
		if(false == OSFileSeek(FileHandle,recNo-1,0)) return false;

		if(false == OSFileRead(pbyDest,1,FileHandle))
		{
			return false;
		}

		recNo = pbyDest[0];		
		if(false == OSFileSeek(FileHandle,OpenedFile[FileHandle-1].fi.type*(recNo-1)+offset+OpenedFile[FileHandle-1].fi.circleNo,0)) return false;
	}
	else
	{	   
   		if(false == OSFileSeek(FileHandle,OpenedFile[FileHandle-1].fi.type*(recNo-1)+offset,0)) return false;
	}

	//д��¼
	//
	if(false == OSFileWrite(pbyDest,len,FileHandle,0))
	{
		return false;
	}
   return true;
} 

/*
bool APDU_OSFileRecWrite(uint8 *pbyDest,HANDLE FileHandle,uint8 recNo)
{
	uint8 result;
	//�ж��Ƿ��Ǽ�¼�ļ�
	if(0 == OpenedFile[FileHandle-1].fi.type ) return false;

	//��Կ�ļ����ɶ�
	if(Is_Key_File(FileHandle)) 
	{
		CUP_SW1SW2 = 0X6982;
		return false;
	}

	//PINȨ���ж� //����ֵ��1��д��2PINУ���д��3����д
	result = Write_Access(FileHandle);
	if(3 == result)
	{
	 	CUP_SW1SW2 = 0X6982;
		return false;		
	}
	else if(2 == result)
	{
		if(0==PIN_VER_FLAG)
		{
	 		CUP_SW1SW2 = 0X6982;
			return false;
		}
	}

	//�����ڵļ�¼����SEEK������ͬʱҲ�ж����ļ��Ƿ��Ѿ���
   if(false == OSFileSeek(FileHandle,OpenedFile[FileHandle-1].fi.type*(recNo-1),0)) return false;

	//д��¼
	if(false == OSFileWrite(pbyDest,OpenedFile[FileHandle-1].fi.type,FileHandle,0))
	{
		return false;
	}
	CUP_SW1SW2 = 0X9000;
   return true;
}*/ 	

/*���ܣ������ڼ�¼�ļ��в����¼�������Ӽ�¼�ļ��ļ�¼��������ɾ�����һ����¼
 	������pbyDest ���������ݴ�ţ�FileHandle �������ļ����,recNo����ļ�¼�ţ���1��ʼ
 	���أ�0������1���ɹ� 
bool  OSFileRecInsert(uint8 *pbyDest,HANDLE FileHandle,uint8 recNo)//FileHandle 
{

	return true;
} */	

 /*	���ܣ�ɾ����¼�ļ���¼	
 	������FileHandle �������ļ����,recNo����ļ�¼�ţ���1��ʼ��wordɾ���������
 	���أ�0������1���ɹ�	  
bool  OSFileRecDelete(HANDLE FileHandle,uint8 recNo,uint8 word)
{

	return true;
} */

//�Ƿ�Ϊ��Կ�ļ��������ļ��Ǵ��ڵ�
//���أ�falseΪ����Կ�ļ���trueΪ��Կ�ļ�
/*bool Is_Key_File(HANDLE FileHandle)
{
	if(((OpenedFile[FileHandle-1].fi.ac)&0x01)!=0x01) 
	{
		return false;
	}
	return true;
} 
  */
//�Ƿ�ɶ�
//����ֵ��2�ɶ���1PINУ��ɶ���0���ɶ�
uint8 Read_Access(HANDLE FileHandle)
{
	if(((OpenedFile[FileHandle-1].fi.ac)&0x18)==0x00) 
	{
		return 0;
	}
	else if(((OpenedFile[FileHandle-1].fi.ac)&0x18)==0x08) 
	{
		return 1;
	}
	else if(((OpenedFile[FileHandle-1].fi.ac)&0x18)==0x10) 
	{
		return 2;
	}
	else return 0xff;
}
/**/
//���Ƿ���Ҫ����	  
/*
����ֵ
0���Ķ�
1����MAC��
2���Ķ�
3����MAC�� 
4����д
5����MACд
6����д
7����MACд
*//*
bool Read_Write_Enc_Access(HANDLE FileHandle)
{
	return (OpenedFile[FileHandle-1].fi.ac)>>5;
} 
*/
//�Ƿ��д
//����ֵ��2��д��1PINУ���д��0����д
bool Write_Access(HANDLE FileHandle)
{
	if(((OpenedFile[FileHandle-1].fi.ac)&0x06)==0x00) return 0;
	else if(((OpenedFile[FileHandle-1].fi.ac)&0x06)==0x02) return 1;
	else if(((OpenedFile[FileHandle-1].fi.ac)&0x06)==0x04) return 2;
	else return 0xff;
} 

//PINУ��
/*���룺pbySrcΪLV��
���أ�TRUE �ɹ���false У��ʧ��
flag:1,PINУ�飬2��PUK��У��*/
bool PIN_PUK_Verify(uint8 *pbySrc,uint8 flag)
{
	uint8 PIN[10],i;
	HANDLE FileHandle;


	if(1==flag)
	{
		FileHandle = OSFileOpen(0x0001, 0);
	}
	else if(2==flag)
	{
	 	FileHandle = OSFileOpen(0x0002, 0);	
	}
	else
	{
		CUP_SW1SW2 = SW_BAD_P1_P2;
		OSFileClose(FileHandle);
		return false;	
	}

	if(0xff == FileHandle) 
	{
		PIN_VER_FLAG = 0;
		OSFileClose(FileHandle);
		CUP_SW1SW2 = SW_BAD_P1_P2;
		return false;
	}

	if(false == OSFileSeek(FileHandle,0, 0)) 
	{
		PIN_VER_FLAG = 0;
		OSFileClose(FileHandle);
		CUP_SW1SW2 = SW_BAD_P1_P2;
		return false;
	}

	if(false ==  OSFileRead(PIN,10,FileHandle)) 
	{
		PIN_VER_FLAG = 0;
		OSFileClose(FileHandle);
		CUP_SW1SW2 = SW_BAD_P1_P2;
		return false;
	}

	//���Ƿ񳬹�������
	if(0 == PIN[9])
	{
	 	CUP_SW1SW2 = SW_PIN_LOCK;
		OSFileClose(FileHandle);
		return false;
	}

	for(i=0;i<9;i++)
	{
	 	if((*(pbySrc+i))==0xFF)
		{
		 	break;
		}	
	}

	//if((*pbySrc)<PIN[0])
	//{
	  	*pbySrc = i-1;
	//}
	

	for(i=0;i<PIN[0]+1;i++)
	{
	 	if(PIN[i]!=*(pbySrc+i)) break;
	}

	if(i<PIN[0]+1)
	{
		PIN_VER_FLAG = 0;
		PIN[9] = PIN[9]-1;
		if(false == OSFileSeek(FileHandle,0, 0))
		{
			CUP_SW1SW2 = 0X6B00;
			OSFileClose(FileHandle);
		 	return false;
		}
		if(false == OSFileWrite(PIN,10,FileHandle,0))
		{
		 	CUP_SW1SW2 = 0X6B00;
			OSFileClose(FileHandle);
		 	return false;	
		} 
		/*if(PIN[9]==0)
		{
			CUP_SW1SW2 = SW_PIN_LOCK;
		}
		else*/
		{
			CUP_SW1SW2 = 0X63c0+PIN[9];
		}
		OSFileClose(FileHandle);
		//CUP_SW1SW2 = 0X6B00	;
		return false;
	}
	else
	{
		if(1==flag)
	 	{
			PIN[9] = 0x03;
		}
		else
		{
		 	PIN[9] = 0x03;
		}
		if(false == OSFileSeek(FileHandle,0, 0))
		{
			CUP_SW1SW2 = 0X6B00;
			OSFileClose(FileHandle);
		 	return false;
		}
		if(false == OSFileWrite(PIN,10,FileHandle,0))
		{
		 	CUP_SW1SW2 = 0X6B00;
			OSFileClose(FileHandle);
		 	return false;	
		} 	
	}
	
	//
	PIN_VER_FLAG = 1;
	CUP_SW1SW2 = 0X9000;
	OSFileClose(FileHandle);
	return true;	
}/**/

//PIN�޸�
/* ���룺pbySrcΪLV������LΪ���ȣ�VΪPIN+FF+��PIN
���أ�TRUE �ɹ���false У��ʧ��
PUKFlag:2��У��puk��1У��PIN*/
bool PIN_Change(uint8 *pbySrc,uint8 PUKFlag)
{
	uint8 PIN[10];//,buf[8];
	uint8 i,j;
	HANDLE FileHandle;

	for(i=0;i<*pbySrc;i++)
	{
		PIN[i] = pbySrc[i];
	 	if(0xff == pbySrc[i]) break;
	}
	j=i+1;
	for(;j<*pbySrc;j++)
	{
		if(((pbySrc[j]&0xF0)>0x90)||((pbySrc[j]&0x0F)>0x09))
		{
			CUP_SW1SW2 = 0x6A80;
			return false;
		}
	}

	PIN[0] = i-1;

	if((*pbySrc-PIN[0]-1)>8)
	{
		CUP_SW1SW2 = SW_WRONG_LENGTH;
		return false;
	}



	if(1==PUKFlag)
	{
		if(false == PIN_PUK_Verify(PIN,1)) 	
		{
			//CUP_SW1SW2 = 0X6982;
			return false;
		}
		FileHandle = OSFileOpen(0x0001, 0);
	}
	else if(2==PUKFlag)
	{
	 	if(false == PIN_PUK_Verify(PIN,2)) 	
		{
			//CUP_SW1SW2 = 0X6982;
			return false;
		}
		FileHandle = OSFileOpen(0x0002, 0);	
	}
	else
	{
		CUP_SW1SW2 = 0X6A86;
	 	return false;
	}

	
	if(0xff == FileHandle) 
	{
		PIN_VER_FLAG = 0;
	   OSFileClose(FileHandle);
	   CUP_SW1SW2 = 0X6581;
		return false;
	}

	if(false == OSFileSeek(FileHandle,0, 0)) 
	{
		PIN_VER_FLAG = 0;
		OSFileClose(FileHandle);
		CUP_SW1SW2 = 0X6581;
		return false;
	}

	if((*pbySrc-i)>8)
	{
	 	CUP_SW1SW2 = SW_WRONG_LENGTH;
		return false;
	}

	*(pbySrc+i) =  *pbySrc-i;

	/*for(j=0;j<*pbySrc-i;j++)
	{
	 	if(((*(pbySrc+i+j+1))>0x39)|((*(pbySrc+i+j+1))<0x30))
		{
		 	CUP_SW1SW2 = 0X6984;
			return false;		
		}	
	}*/

	if(false ==  OSFileWrite(pbySrc+i,*pbySrc-i+1,FileHandle,0)) 
	{
		PIN_VER_FLAG = 0;
		OSFileClose(FileHandle);
		CUP_SW1SW2 = 0X6581;
		return false;
	}

	PIN_VER_FLAG = 0;
	OSFileClose(FileHandle);
	CUP_SW1SW2 = 0X9000;
	return true;
} 
 /**/
//PIN����
/* ���룺pbySrcΪLV������LΪ���ȣ�VΪPUK+FF+��PIN
���أ�TRUE �ɹ���false У��ʧ��*/
bool PIN_Unblock(uint8 *pbySrc)
{
	//У��PUK
	uint8 i;
	HANDLE FileHandle;
	uint8 PIN[10];
	uint8 buf[2];

	
	/*FileHandle = OSFileOpen(0x0002, 0);
	if(0xff == FileHandle) 
	{
		PIN_VER_FLAG = 0;
		return false;
	}

	if(false == OSFileSeek(FileHandle,0, 0)) 
	{
		PIN_VER_FLAG = 0;
		return false;
	}

	if(false ==  OSFileRead(PIN,10,FileHandle)) 
	{
		PIN_VER_FLAG = 0;
		return false;
	}

	//���Ƿ񳬹�������
	if(0 == PIN[9])
	{
	 	CUP_SW1SW2 = 0X63c0;
		OSFileClose(FileHandle);
		return false;
	}

	for(i=0;i<PIN[0];i++)
	{
	 	if(PIN[i+1]!=*(pbySrc+i+1)) break;
	}

	if((i<PIN[0])|(*(pbySrc+i+1)!=0xff))
	{
		PIN[9] = PIN[9]-1;
		if(false == OSFileSeek(FileHandle,0, 0))
		{
		 	CUP_SW1SW2 = 0X6B00;
		 	return false;	
		}
		if(false == OSFileWrite(PIN,10,FileHandle,0))
		{
		 	CUP_SW1SW2 = 0X6B00;
		 	return false;	
		} 
		CUP_SW1SW2 = 0X63c0+PIN[9];
		OSFileClose(FileHandle);
		return false;
		PIN_VER_FLAG = 0;
		CUP_SW1SW2 = 0X6B00	;
		return false;
	}
	else
	{
	 	PIN[9] = 0x0a;
		OSFileSeek(FileHandle,0, 0);
		OSFileWrite(PIN,10,FileHandle,0);
	}*/
	for(i=1;i<9;i++)
	{
		PIN[i] = pbySrc[i];
	 	//if(0xff == pbySrc[i]) break;
	}

	PIN[0] = 8;

	if(false == PIN_PUK_Verify(PIN,2)) //PUK vertified	
	{
		//CUP_SW1SW2 = 0X6982;
		return false;
	}

	//ȷ��PIN�Ƿ����������������6985
	FileHandle = OSFileOpen(0x0001, 0);
	if(0xff == FileHandle) 
	{
		PIN_VER_FLAG = 0;
		CUP_SW1SW2 = SW_FILE_NOT_FOUND;
		OSFileClose(FileHandle);
		return false;
	}

	if(false == OSFileSeek(FileHandle,0x09, 0)) 
	{
		PIN_VER_FLAG = 0;
		CUP_SW1SW2 = SW_FILE_NOT_FOUND;
		OSFileClose(FileHandle);
		return false;
	}

	if(false ==  OSFileRead(buf,1,FileHandle)) 
	{
		PIN_VER_FLAG = 0;
		CUP_SW1SW2 = SW_FILE_NOT_FOUND;
		OSFileClose(FileHandle);
		return false;
	}

	if(0!=buf[0])
	{
	 	PIN_VER_FLAG = 0;
		CUP_SW1SW2 = 0x6985;
		OSFileClose(FileHandle);
		return false;	
	}


	//PIN[0] =  *pbySrc - i-1;
	for(i=0;i<8;i++)
	{
	 	PIN[i+1] = *(pbySrc+(*pbySrc)-8+i+1);
		if(0xff == PIN[i+1]) break;
	}
	PIN[0] = i;
	//�޸�PIN
	FileHandle = OSFileOpen(0x0001, 0);
	if(0xff == FileHandle) 
	{
		PIN_VER_FLAG = 0;
	   OSFileClose(FileHandle);
	   CUP_SW1SW2 = 0X6581;
		return false;
	}

	if(false == OSFileSeek(FileHandle,0, 0)) 
	{
		PIN_VER_FLAG = 0;
		OSFileClose(FileHandle);
		CUP_SW1SW2 = 0X6581;
		return false;
	}

	PIN[9] = 3;
	if(false ==  OSFileWrite(PIN,10,FileHandle,0)) 
	{
		PIN_VER_FLAG = 0;
		OSFileClose(FileHandle);
		CUP_SW1SW2 = 0X6581;
		return false;
	}		

	PIN_VER_FLAG = 0;
	CUP_SW1SW2 = 0X9000;
	OSFileClose(FileHandle);
	return true;
} /**/

/* ���룺fileID ���ļ�ID
�����attribute[3]��
��һ���ֽ�Ϊ�ļ�����
1:�������ļ�
2����¼�ļ�
3��ѭ���ļ�

���Ϊ�������ļ����ڶ��������ֽ�Ϊ�ļ����ȣ�
���Ϊ��¼�ļ����ڶ����ֽ�Ϊ��¼�����������ֽ�Ϊ��¼����
���Ϊѭ���ļ����ڶ����ֽ�Ϊ��¼�����������ֽ�Ϊ��¼����

*/
/**/
bool FileAttribute(uint16 fileID,uint8 attribute[3])
{
	HANDLE FileHandle=0xFF;
	FileHandle = OSFileOpen(fileID, 0);	
	if(FileHandle == 0xff)
	{	
		CUP_SW1SW2 = 0X6A82;	
	 	return false;	
	}
	if(0==OpenedFile[FileHandle-1].fi.type)
	{
		attribute[0] = 	1; 	//͸���ļ�
		attribute[1] = (uint8)((OpenedFile[FileHandle-1].fi.size)>>8);
		attribute[2] = (uint8)(OpenedFile[FileHandle-1].fi.size);	
	}
	else
	{
		if(OpenedFile[FileHandle-1].fi.Ftype==3) //ѭ���ļ�
		{	 		
			attribute[2] =  OpenedFile[FileHandle-1].fi.type;
			//attribute[1] =  (OpenedFile[FileHandle-1].fi.circleNo);
			for(attribute[1]=0;attribute[1]<OpenedFile[FileHandle-1].fi.circleNo;attribute[1]++)
			{ 
				if(false ==  OSFileRead(attribute,1,FileHandle)) 
				{
					PIN_VER_FLAG = 0;
					//CUP_SW1SW2 = 0X6A82;
					OSFileClose(FileHandle);
					return false;
				}
				if(attribute[0]==0xFF) break;
			}

			attribute[1] = 	OpenedFile[FileHandle-1].fi.circleNo;
			attribute[0] = 	3;		 //ѭ����¼�ļ�
		}
		else
		{
		 	attribute[0] = 	2;		//��¼�ļ�
			attribute[1] =  (OpenedFile[FileHandle-1].fi.size)/(OpenedFile[FileHandle-1].fi.type); 
			attribute[2] =  OpenedFile[FileHandle-1].fi.type;	
		}

	}

	CUP_SW1SW2 = 0X9000;
	OSFileClose(FileHandle);
	return true; 
}

//�޸Ķ�дȨ��
bool AC_Change(uint16 fileID,uint8 ac)
{
 	HANDLE FileHandle;
	FileHandle = OSFileOpen(fileID, 0);	
	if(FileHandle == 0xff)
	{	
		CUP_SW1SW2 = 0X6A82;	
	 	return false;	
	}

	Sim_WriteCode(OpenedFile[FileHandle-1].addr-7,  ac);
	
	OSFileClose(FileHandle);	
}
/*
void SetPIN_ON()
{
 	PIN_VER_FLAG = 1;
	return;	
}

void SetPIN_CLOSE()
{
 	PIN_VER_FLAG = 0;
	return;	
}*/



