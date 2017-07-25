//CUP_APDU_ANALYSY.c
#include "..\export_new\fileAPI.h"
#include "..\..\cxcommon\Target_Driver\export_new\simcommand.h"
//#include "..\..\des.h"




code uint8 AID_DF[]={0XA0,0X00,0X00,0X03,0X33,0X43,0X55,0X50,0X2D,0X4D,0X4F,0X42,0X49,0X4C,0X45};
code uint8 AIDResponse[]={0x70,0x06,0x51,0x01,0xFF,0x52,0x01,0x0A};//DF 是移动支付应用(AID 为A0 00 00 03 33 CUP-MOBILE)，响应数据   
code uint8 EFResponse[]= {0x6F,0x07,0x82,0x01,0xFF,0x80,0x02,0xFF,0xFF};//EF文件响应数据

uint16 CUP_SW1SW2;
uint8 CUP_RSP_FLAG;
idata uint16 SELECT_FILEID;
idata uint8 CUP_FLAG;
idata uint8 g_NEED_ACCESS_RIGHT;

void CUP_APDU_INIT()
{
	CUP_SW1SW2 = 0x9000;
	CUP_RSP_FLAG = 0;
	g_NEED_ACCESS_RIGHT=0;	
}
extern uint8 CompareString(uint8 *pbySrc1, uint8 *pbySrc2, uint16 unLen);
extern void MemoryCopy(const uint8 * pbySrc, const uint8 * pbyDest, uint16 unLength);
extern uint8 Read_Access(HANDLE FileHandle); 
extern bool Write_Access(HANDLE FileHandle);
extern bit PIN_VER_FLAG;
//extern idata uint8  head[5];//用来接收函数5个字节的命令头
//extern xdata uint8 g_pbyProCmdBuf[255];	//  the Proactive Buffer

#define CMP_RESULT_EQUAL			 0x01
/*********************************************
//SELECT(选择)
//输入：LV为输入的apdu结果，包括长度，apdu头以及apdu体
**********************************************/
bool SelectFile_CUP(uint8 *dataBuffer,uint8 *head)
{
	uint8 result;
	HANDLE hdl = Not_Open_FILE;
	uint16 bylen;
	uint8 buf[5];
	//uint8 *DataBuffer=NULL;
	

	if((0x00==head[3])&&((0x00==head[2])||(0x04==head[2])))//选择AID目录
	{
		if(0x04==head[2])
		{ 
			

			if((head[4]>=0x05)&&head[4]<=0x10)
			{

				if(0x00 != head[0])
				{
				 	CUP_SW1SW2 = SW_BAD_CLA;
					return false;
				}
			
				if(((0x04 != head[2])&(head[2]!=0x00))||(head[3]!=0x00))
				{
				 	CUP_SW1SW2 = SW_BAD_P1_P2;
					return false;
				}

				result=CompareString(dataBuffer,AID_DF,head[4]);
				if(CMP_RESULT_EQUAL==result)
				{  
					*dataBuffer=sizeof(AIDResponse);
					MemoryCopy(AIDResponse,dataBuffer+1,sizeof(AIDResponse));
					///////////////////////////////////////////
					//获取应用版本号
					hdl= OSFileOpen(0xA002,0);
					if(hdl==Not_Open_FILE) //打开文件失败
					{
						CUP_SW1SW2 = SW_FILE_NOT_FOUND;
						OSFileClose(hdl);
						return false;
					}
					if(false == OSFileSeek(hdl,10, 0))
					{
					   	CUP_SW1SW2 = SW_COMMAND_NOT_FILE;
						OSFileClose(hdl);
						return false;
					}

					if(false ==OSFileRead(dataBuffer+5,1,hdl))
					{	
						//CUP_SW1SW2 = SW_COMMAND_NOT_FILE;
						OSFileClose(hdl);
						return false;
					}
					///////////////////////////////////////////
					//EF04记录数为0A
					//获取EF04有效记录数
					hdl= OSFileOpen(0xEF04,0);
					if(hdl==Not_Open_FILE) //打开文件失败
					{
						CUP_SW1SW2 = SW_FILE_NOT_FOUND;
						OSFileClose(hdl);
						return false;
					}

					result = 0;
					for(bylen=0;bylen<0x0A;bylen++)
					{
						if(false ==OSFileRecRead(dataBuffer+8,hdl,bylen+1,20,1))
						{	
							CUP_SW1SW2 = SW_FILE_NOT_FOUND;
							OSFileClose(hdl);
							return false;
						}

						if((((*(dataBuffer+8))&0x0F)>9)||((((*(dataBuffer+8))&0xF0)>>4)>9)||((((*(dataBuffer+8))&0xF0)>>4)==0))
						{
						 	continue;	
						}

					 	result++;	
					}

					*(dataBuffer+8) = result;
					///////////////////////////////////////////


					bylen = sizeof(AIDResponse);

					//CUP_SW1SW2 = 0x6100+bylen;
					CUP_RSP_FLAG = 1;
					CUP_FLAG = 1;
					PIN_VER_FLAG = 0;
					buf[0] = 0x80;
					buf[1] = 0xf6;
					buf[2] = 0x00;
					buf[3] = 0x01;
					buf[4] = 0x00;
					AccessSwitch(buf);
					
					CUP_SW1SW2 = 0x6100+bylen;	
					return true;
				}
				else
				{
				 	CUP_SW1SW2 = 0x6A82;
					return false;
				}
	
			}
			else
			{
			 	CUP_SW1SW2 = SW_WRONG_LENGTH;
				return false;	
			}
		}
		else//选择EF文件
		{
			if(0x02==head[4])
			{
				SELECT_FILEID =  dataBuffer[0]*0x100+dataBuffer[1];
				if((0x3F00==SELECT_FILEID)||(0x2F00==SELECT_FILEID))
				{
				 	CUP_SW1SW2 = 0xEEEE;
					return false;	
				}

				if(0x00 != head[0])
				{
				 	CUP_SW1SW2 = SW_BAD_CLA;
					return false;
				}
			
				if(((0x04 != head[2])&(head[2]!=0x00))|(head[3]!=0x00))
				{
				 	CUP_SW1SW2 = SW_BAD_P1_P2;
					return false;
				}

				if(false == FileAttribute(SELECT_FILEID,dataBuffer+20))
				{
				 	return false;
				}

				MemoryCopy(EFResponse,dataBuffer+1,sizeof(EFResponse));
			 	dataBuffer[5] = *(dataBuffer+20);
				dataBuffer[8] = *(dataBuffer+21);	   //
				dataBuffer[9] = *(dataBuffer+22);

				*dataBuffer =  sizeof(EFResponse);
				bylen = sizeof(EFResponse);
				CUP_SW1SW2 = 0x6100+bylen;
				return true;
			}
			else
			{
				//CUP_SW1SW2 = SW_WRONG_LENGTH;
				CUP_SW1SW2 = SW_BAD_P1_P2;
				return false;
			}
		}	
	}


	CUP_SW1SW2 = SW_BAD_P1_P2;
	return false;

	
}

/*********************************************
//GET CHALLENGE(取随机数)
//输入：LV，整个APDU，包括APDU头
输出：随机数
**********************************************/
extern unsigned char vRNG();
bool Get_Challenge(uint8 *LV,uint8 *Random) 
{
	uint8 i;	
	uint8 *head=NULL;

	//apdu头
	head = LV+1;

	
	
			
	
	Random[0]=head[4]+2;
	for(i=0;i<head[4];i++)
	{
		//Random[i+1]= 0x31+i;//vRNG();//
		Random[i+1]= vRNG();
	}
	Random[9] = 0x90;
	Random[10] = 0x00;
	CUP_SW1SW2 = 0x9000;
	return true;   
}

bool Updata_Binary(uint8 *databuffer,uint8 *head)
{
	HANDLE hdl = Not_Open_FILE;
	uint8 DataBuffer2[3];
	uint16 offset; 
	uint8 result;
	
	//CLA判断
	if(0x00!=head[0])
	{
	 	CUP_SW1SW2 = SW_BAD_CLA;
		return false;	
	}	

	if(0x80!=(head[2]&0x80))//通过select的文件ID读文件
	{
		//offset  = (uint16)((((uint16)(head[2]&0X7f))<<8) + (uint16)(head[3]));
		CUP_SW1SW2 = SW_BAD_P1_P2;
		return false;	
	}
	else //if(0x80==(head[2]&0x80))//通过短文件标示读文件
	{
		if(0x00!=(head[2]&0x60))
		{
			CUP_SW1SW2 = SW_BAD_P1_P2;
			return false;		 	
		}

		SELECT_FILEID=0xEF00+(uint16)(head[2]&0x1F);			
		offset =  (uint16)(head[3]);
	}
	
	if(false == FileAttribute(SELECT_FILEID,DataBuffer2))
	{
		//CUP_SW1SW2 = SW_FILE_NOT_FOUND;
	 	return false;
	}

	if(1!=DataBuffer2[0])
	{
	 	CUP_SW1SW2 = SW_COMMAND_NOT_FILE;
		return false;	
	}	

	hdl= OSFileOpen(SELECT_FILEID,0);
	if(hdl==Not_Open_FILE) //打开文件失败
	{
		CUP_SW1SW2 = SW_FILE_NOT_FOUND;
		OSFileClose(hdl);
		return false;
	}

	if(0==g_NEED_ACCESS_RIGHT)   //看是否关闭权限验证
	{
		//返回值：2可写，1PIN校验可写，0不可写
		result = Write_Access(hdl);
		if((0==result)|							//不可读
			((1==result)&&(0 == PIN_VER_FLAG)))	//需要PIN交易，但是未校验PIN
		{
			CUP_SW1SW2 = SW_VERIFY_CONDITION_NOT;
			OSFileClose(hdl);
			return false;
		}

	}

	if(false == OSFileSeek(hdl,offset, 0))
	{
	   	//CUP_SW1SW2 = SW_COMMAND_NOT_FILE;
		OSFileClose(hdl);
		return false;
	}
	if(false == OSFileWrite(databuffer,head[4],hdl,0))
	{	
		//CUP_SW1SW2 = SW_COMMAND_NOT_FILE;
		OSFileClose(hdl);
		return false;
	}

	OSFileClose(hdl);
	CUP_SW1SW2 = SW_SUCCESS;

	return true;  
}

//
bool Read_Binary(uint8 *databuf,uint8 *head)
{
	HANDLE hdl = Not_Open_FILE;
	uint8 result;
	uint16 offset;

	//CLA判断
	if(0x00!=head[0])
	{
	 	CUP_SW1SW2 = SW_BAD_CLA;
		return false;	
	}

	if(0x80!=(head[2]&0x80))//通过select的文件ID读文件
	{
		if(SELECT_FILEID==0x0000)
		{
		 	CUP_SW1SW2 = SW_BAD_P1_P2;
			return false;	
		}
		offset  = (uint16)((((uint16)(head[2]&0X7f))<<8) + (uint16)(head[3]));	 		
	}
	else// if(0x80==(head[2]&0x80))//通过短文件标示读文件
	{
		if(0x00 != (head[2]&0x60))
		{
		 	CUP_SW1SW2 = SW_BAD_P1_P2;
			return false;	
		}
		SELECT_FILEID=0xEF00+(uint16)(head[2]&0x1F);			
		offset =  (uint16)(head[3]);
	}

	if(false == FileAttribute(SELECT_FILEID,databuf))
	{
		CUP_SW1SW2 = SW_BAD_P1_P2;
	 	return false;
	}

	if(1!=databuf[0])
	{
	 	CUP_SW1SW2 = SW_COMMAND_NOT_FILE;
		return false;	
	}
	
	hdl= OSFileOpen(SELECT_FILEID,0);
	if(hdl==Not_Open_FILE) //打开文件失败
	{
		CUP_SW1SW2 = SW_FILE_NOT_FOUND;
		OSFileClose(hdl);
		return false;
	}	

	//返回值：0可读，1PIN校验可读，2不可读
	result = Read_Access(hdl);
	if((0==result)||							//不可读
		((1==result)&&(0 == PIN_VER_FLAG)))	//需要PIN交易，但是未校验PIN
	{
		CUP_SW1SW2 = SW_VERIFY_CONDITION_NOT;
		return false;
	} 
	
	if(false == OSFileSeek(hdl,offset, 0))
	{
	   	CUP_SW1SW2 = SW_OFFSET_OVER;
		OSFileClose(hdl);
		return false;
	}
	//if(false == OSFileWrite(apdu_data,head[4],hdl,0))
	//bool OSFileRead(uint8 *pbyDest,uint16 uiLen,HANDLE FileHandle)

	/*if(head[4]==0)
	{
	 	offset = databuf[1]*0x100+databuf[2]-offset;
	}
	else
	{
	 	offset = head[4];
	}*/

	if(false ==OSFileRead(databuf,head[4],hdl))
	{	
		CUP_SW1SW2 = SW_WRONG_LENGTH;
		OSFileClose(hdl);
		return false;
	}


	OSFileClose(hdl);
	CUP_SW1SW2 = SW_SUCCESS;

//	*outBuff = 0x02;
//	*(outBuff+1) = 0x90;
//	*(outBuff+2) = 0x00;

	return true;  
}

//读记录文件
bool Read_Record(uint8 *databuf,uint8 *head)
{
  	HANDLE hdl = Not_Open_FILE;
	uint8 result;

	//CLA判断
	if(0x00!=head[0])
	{
	 	CUP_SW1SW2 = SW_BAD_CLA;
		return false;	
	}

	//P2判断
	if(0x04!=(head[3]&0x04))
	{
	  	CUP_SW1SW2 = SW_BAD_P1_P2;
		return false;
	}

	if(0x00!=(head[3]&0xF8))//如果前5bit为0则不是SFI方式
	{
		SELECT_FILEID  = 0xEF00 + (uint16)(((uint16)(head[3]))>>3) ;	 		
	}
	else
	{
	  	CUP_SW1SW2 = SW_BAD_P1_P2;
		return false;
	}
	

	
	if(false == FileAttribute(SELECT_FILEID,databuf))
	{
	 	return false;
	}

	if(1==databuf[0]) //record 文件和循环文件
	{
	 	CUP_SW1SW2 = SW_COMMAND_NOT_FILE;
		return false;	
	}

	/*if(((0x00 == head[2])&(3!=databuf[0]))|((0x00 != head[2])&(3==databuf[0])))//p1==0表示为循环文件，如果不是循环文件返回错误
	{
	 	CUP_SW1SW2 = SW_BAD_P1_P2;
		return false;
	}

	if(((0x04!=(0x07&head[3]))&(2==databuf[0]))| //判断是否以记录号的方式读取（记录文件04，循环文件04，03，02）
	 ((0x04!=(0x07&head[3]))&(3==databuf[0]))|
	 ((0x04!=(0x07&head[3]))&(3==databuf[0]))|
	 ((0x04!=(0x07&head[3]))&(3==databuf[0]))
	)
	{
		CUP_SW1SW2 = SW_BAD_P1_P2;
		return false;		
	}*/

	if((head[2]==0)||(databuf[1]<head[2]))
	{
		CUP_SW1SW2 = SW_RECORD_NOT_FOUND;
		return false;	
	}

	if(head[4]>databuf[2])
	{
		CUP_SW1SW2 = SW_WRONG_LENGTH;
		return false;	
	}
	

	/*if((head[2]!=0)&&(databuf[1]<head[2]))
	{
	 	CUP_SW1SW2 = SW_BAD_P1_P2;
		return false;	
	}*/

/*	//返回值：0可读，1PIN校验可读，2不可读
	result = Read_Access(hdl);
	if((0==result)|							//不可读
		((1==result)&&(0 == PIN_VER_FLAG)))	//需要PIN交易，但是未校验PIN
	{
		CUP_SW1SW2 = SW_VERIFY_CONDITION_NOT;
		return false;
	}*/
	
	hdl= OSFileOpen(SELECT_FILEID,0);
	if(hdl==Not_Open_FILE) //打开文件失败
	{
		CUP_SW1SW2 = SW_FILE_NOT_FOUND;
		OSFileClose(hdl);
		return false;
	}	

	//返回值：0可读，1PIN校验可读，2不可读
	result = Read_Access(hdl);
	if((0==result)|							//不可读
		((1==result)&&(0 == PIN_VER_FLAG)))	//需要PIN交易，但是未校验PIN
	{
		CUP_SW1SW2 = SW_VERIFY_CONDITION_NOT;
		OSFileClose(hdl);
		return false;
	} 
	
	if((3==databuf[0])&(0==head[2]))
	{
		if((0x04!=(0x07&head[3]))&(0x03!=(0x07&head[3]))&(0x02!=(0x07&head[3]))) //判断是否以记录号的方式读取（记录文件04，循环文件04，03，02）
		{
			CUP_SW1SW2 = SW_BAD_P1_P2;
			return false;		
		}



		if(false ==OSFileCircleRead(databuf,hdl,1,0,head[4]))
		{	
			//CUP_SW1SW2 = SW_FILE_NOT_FOUND;
			OSFileClose(hdl);
			return false;
		}	
	}
	else
	{
		if(0x04!=(0x07&head[3])) //判断是否以记录号的方式读取（记录文件04，循环文件04，03，02）
		{
			CUP_SW1SW2 = SW_BAD_P1_P2;
			return false;		
		}

		if(false ==OSFileRecRead(databuf,hdl,head[2],0,head[4]))
		{	
			//CUP_SW1SW2 = SW_FILE_NOT_FOUND;
			OSFileClose(hdl);
			return false;
		}
	}


	OSFileClose(hdl);
	CUP_SW1SW2 = SW_SUCCESS;

//	*outBuff = 0x02;
//	*(outBuff+1) = 0x90;
//	*(outBuff+2) = 0x00;

	return true; 
}

/*********************************************
//UPDATE RECORD(更新记录文件)
//输入：LV为写入的内容，LV格式，recNo为记录号，
**********************************************/
bool  Updata_Record(uint8 *databuf,uint8 *head)//UPDATE RECORD(更新记录文件)
{
	HANDLE hdl = Not_Open_FILE;
	uint8 DataBuffer[3],i;

	//CLA判断
	if(0x00!=head[0])
	{
	 	CUP_SW1SW2 = SW_BAD_CLA;
		return false;	
	}

	if(0x00!=(head[3]&0xF8))//如果前5bit为0则不是SFI方式
	{
		SELECT_FILEID  = 0xEF00 + (uint16)(((uint16)(head[3]))>>3);	 		
	}

	if(false == FileAttribute(SELECT_FILEID,DataBuffer))
	{
	 	return false;
	}

	if(1==DataBuffer[0])
	{
	 	CUP_SW1SW2 = SW_COMMAND_NOT_FILE;
		return false;	
	}

	if(((0x00 != head[2])&(3==DataBuffer[0])))
	{
	 	CUP_SW1SW2 = 0x6B00;
		return false;
	}

	if(((0x00 == head[2])&(3!=DataBuffer[0]))|((0x00 != head[2])&(3==DataBuffer[0])))//p1==0表示为循环文件，如果不是循环文件返回错误
	{
	 	CUP_SW1SW2 = SW_BAD_P1_P2;
		return false;
	}

	if((head[2]!=0)&(DataBuffer[1]<head[2]))
	{
	 	CUP_SW1SW2 = 0x6a83;
		return false;	
	}

	if(0x04!=(0x07&head[3]))//判断是否以记录号的方式读取
	{
		CUP_SW1SW2 = SW_BAD_P1_P2;
		return false;		
	}

	hdl = OSFileOpen(SELECT_FILEID, 0x11);
	if(hdl==Not_Open_FILE)                       
	{                                        
		CUP_SW1SW2 = SW_FILE_NOT_FOUND;
		OSFileClose(hdl);
		return false;
	}

	//返回值：2可写，1PIN校验可写，0不可写
	i = Write_Access(hdl);
	if((0==i)|							//不可读
		((1==i)&&(0 == PIN_VER_FLAG)))	//需要PIN交易，但是未校验PIN
	{
		CUP_SW1SW2 = SW_VERIFY_CONDITION_NOT;
		OSFileClose(hdl);
		return false;
	}



	if(0 == head[2])
	{
		if(false == OSFileCircleWrite(databuf,hdl,0,head[4]))
		{
		 	//CUP_SW1SW2 = SW_BAD_P1_P2;
			OSFileClose(hdl);
			return false;		
		}
	}
	else
	{
		if(false == OSFileRecWrite(databuf,hdl,head[2],0,head[4]))
		{
		 	//CUP_SW1SW2 = SW_BAD_P1_P2;
			OSFileClose(hdl);
			return false;		
		}

		//EF04需要特殊处理，需要修改A002
		if((0xEF04 == SELECT_FILEID)&(0x6C==head[4]) )
		{
			hdl = OSFileOpen(0xA002, 0);
			if(hdl==Not_Open_FILE)                       
			{                                        
				CUP_SW1SW2 = 0X6981;
				OSFileClose(hdl);
				return false;
			}
			if(false == OSFileSeek(hdl,1, 0))
			{
			   	CUP_SW1SW2 = 0X6981;
				OSFileClose(hdl);
				return false;
			}
		
			i = 0x01;	
			if(false == OSFileWrite(&i,1,hdl,0))
			{	
				CUP_SW1SW2 = 0X6981;
				OSFileClose(hdl);
				return false;
			}
			OSFileClose(hdl);
		}
		///////////////////////////////////////////////////////	
	}
	OSFileClose(hdl);

	CUP_SW1SW2 = 0x9000;
	return true;

}

/*********************************************
//WRITE KEY(写密钥)
//输入：LV，整个APDU，包括APDU头
vDes flag =  0: single DES for encryption 
1: single DES for decryption 
2: triple DES for encryption 
3: triple DES for decryption
**********************************************/

bool Write_Key(uint8* LV,uint8 *Random,uint8 *outBuff)
{
	idata HANDLE hdl_Ctr = 0xFF;
	idata HANDLE hdl = 0xFF;
	idata HANDLE hdl_key = 0xFF;
	idata uint8 result,i,KID;  
	idata uint8 *head=NULL;	
	idata uint8 *apdu=NULL;
	idata uint8 *tmp=NULL;
	

	extern idata uint8 disp_test_1[11];
	extern void vDES(unsigned char *pOut,unsigned char *pIn,unsigned char *pKey, unsigned char flag);
	extern uint8 MAC_MUL_Algo_Final(uint8*src, uint16 mac_length,uint8 *keyarray,uint8 des_sel,uint8 *result,uint8 *init_mul_array);	
	//apdu头
	head = LV+1;
	apdu = LV + 6;	
	
	if(0x84 != head[0])
	{
	 	CUP_SW1SW2 = SW_BAD_CLA;
		return false;
	}  
	

	if(0x20==head[3])//传输控制密钥}
	{
		hdl=OSFileOpen(0x0004, 0);
	}
	else
	{
	 	hdl=OSFileOpen(0x0005, 0);
		//Random[1] += 1;
		//Random[2] += 1;
		if(0xff==Random[2])
		{
		 	Random[2] = 0;
			Random[1] += 1;
		}
		else
		{
			Random[2] += 1;	
		}
	}
	
	//计算mac
	if(hdl==Not_Open_FILE)                       
	{                                        
		CUP_SW1SW2 = SW_BAD_P1_P2;
		OSFileClose(hdl);
		return false;
	}	
										
	if(false == OSFileSeek(hdl,0, 0))
	{
	   	CUP_SW1SW2 = SW_COMMAND_NOT_FILE;
		OSFileClose(hdl);
		return false;
	}
	
	if(false == OSFileRead(outBuff,0x11,hdl))
	{	
		CUP_SW1SW2 = SW_COMMAND_NOT_FILE;
		OSFileClose(hdl_Ctr);
		return false;
	}
	outBuff += 1;
	 
	//随机数不是8字节时补00凑成8字节
	if(Random[0]<8)
	{
		for(i=8;i>Random[0];i--)
		{
			Random[i]=0;
		}
	}
		
	//算MAC
//	MAC_Algo(apdu, head[4]-4,outBuff,0,outBuff+9,Random); 
	
/*	disp_test_1[3] = outBuff[9];//retbuf[7];
	disp_test_1[4] = outBuff[10];
	disp_test_1[5] = outBuff[11];
	disp_test_1[6] = outBuff[12];
	disp_test_1[7] = outBuff[1];
	disp_test_1[8] = outBuff[2];
	disp_test_1[9] = outBuff[3];
	disp_test_1[10] = outBuff[4];
	disp_test_1[11] = outBuff[5];
*/	
	outBuff[0x12] =  head[4]-4+5;
	MemoryCopy(LV+1,outBuff+0x13,outBuff[0x12]);
	outBuff[0x13 + outBuff[0x12]] = 0x80;
	outBuff[0x12] ++;
	if(0!=(outBuff[0x12]%8))
	{
		result =  8-outBuff[0x12]%8;
	 	for(i=0;i<result;i++)
		{
		 	outBuff[0x12 + outBuff[0x12]+1] = 0x00;
			outBuff[0x12] ++;
		}
	}
		
	tmp = outBuff+outBuff[0x12]+0x13;
	MAC_MUL_Algo_Final(outBuff+0x13,outBuff[0x12],outBuff,0,tmp,Random+1);

	result=CompareString(tmp,apdu+head[4]-4,4);

/**///if(0)//CMP_RESULT_EQUAL!=result)
	if(CMP_RESULT_EQUAL!=result)
	{
		OSFileClose(hdl);
		CUP_SW1SW2 = SW_MAC_ERROR;
		return false;
	}
	
	//3DES-ECB加，解密
	result = (head[4]-4)%8;
	if(0==result)
	{
		result = (head[4]-4)/8;	
	}
	else
	{
		result = (head[4]-4)/8+1;
	}
	
	for(i=0;i<result;i++)
	{
		vDES(apdu,apdu,outBuff, 1);
		vDES(apdu,apdu,outBuff+8, 0);
		vDES(apdu,apdu,outBuff, 1);
		apdu += 8;
	}
	
	apdu = LV + 6;	
	if(0x20 == head[3])
	{
		KID = (*apdu);
	}
	else
	{
	 	KID = (*(apdu+1));	
	}
	//安装新密钥
	if(1==head[2])
	{
		apdu = apdu+3;
		*apdu = KID;
	}
	else//更新新密钥
	{
		//apdu = apdu+2;
		*apdu = head[3];
	}


	if(0x20==head[3])
	{
		hdl_key = OSFileOpen(0x0005, 0);
		if(hdl_key==Not_Open_FILE)                       
		{                                        
			CUP_SW1SW2 = SW_BAD_P1_P2;
			OSFileClose(hdl_key);
			return false;
		}
		if(false == OSFileSeek(hdl_key,0, 0))
		{
		   	CUP_SW1SW2 = SW_COMMAND_NOT_FILE;
			OSFileClose(hdl_key);
			return false;
		}
		
		if(false == OSFileWrite(apdu,17,hdl_key,0))
		{	
			CUP_SW1SW2 = SW_COMMAND_NOT_FILE;
			OSFileClose(hdl_key);
			return false;
		}
	}
	else
	{
		hdl_key = OSFileOpen(0x0003, 0x11);
		if(hdl_key==Not_Open_FILE)                       
		{                                        
			CUP_SW1SW2 = SW_BAD_P1_P2;
			OSFileClose(hdl_key);
			return false;
		}
	
		//KID = KID&0x0f;
		if(false == OSFileRecWrite(apdu,hdl_key,((KID&0x0f)-2),0,0))
		{
		 	CUP_SW1SW2 = SW_BAD_P1_P2;
			OSFileClose(hdl_key);
			return false;		
		}
	} 
	
	OSFileClose(hdl);
	CUP_SW1SW2 = 0x9000;
	return true;
}
/*分散密钥计算*/
uint8 Init_For_Descrypt(uint8 *databuf, uint8 *head)
{
	uint8 buf[32];
	uint8 key[16];
	uint8 i,hdl_key,j;
	uint8 en_key[16];

	if(0x80 != head[0])
	{
	 	CUP_SW1SW2 = SW_BAD_CLA;
		return false;
	}

	if(0!= (head[4]%8))
	{
	 	CUP_SW1SW2 = SW_WRONG_LENGTH;
		return false;	
	}

	if(0x01 != head[3])
	{
	 	CUP_SW1SW2 = 0x6A86;
		return false;
	}

	if(0x20 == (head[2]&0x20))//传输密钥
	{
		if(0x00 != (head[2]&0x0f))
		{
		 	CUP_SW1SW2 = 0x6A86;
			return false;
		}
		
		if(((head[2]&0x0F)>7)||((head[2]&0x0F)<3))
		{
		  	CUP_SW1SW2 = 0x6A86;
			return false;	
		}

	 	hdl_key = OSFileOpen(0x0005, 0);
		if(hdl_key==Not_Open_FILE)                       
		{                                        
			CUP_SW1SW2 = 0x6985;
			OSFileClose(hdl_key);
			return false;
		}
		if(false == OSFileSeek(hdl_key,1, 0))
		{
		   	CUP_SW1SW2 = 0x6985;
			OSFileClose(hdl_key);
			return false;
		}
		
		if(false == OSFileRead(key,16,hdl_key))
		{	
			CUP_SW1SW2 = 0x6985;
			OSFileClose(hdl_key);
			return false;
		}	
	}
	else //应用密钥
	{
		
		if(0x10 == (head[2]&0x10))
		{
		  	if(((head[2]&0x0F)>7)||((head[2]&0x0F)<3))
			{
			  	CUP_SW1SW2 = 0x6A86;
			  	OSFileClose(hdl_key);
				return false;	
			}	
		}
		else
		{
		  	CUP_SW1SW2 = 0x6A86;
		  	OSFileClose(hdl_key);
			return false;	
		}

		hdl_key = OSFileOpen(0x0003, 0);
		if(hdl_key==Not_Open_FILE)                       
		{                                        
			CUP_SW1SW2 = 0x6985;
			OSFileClose(hdl_key);
			return false;
		}

		i = 0x0f&head[2];
		if((i>7)||(i<3))
		{
		 	CUP_SW1SW2 = 0x6A86;
			OSFileClose(hdl_key);
			return false;
		}

		if(false == OSFileRecRead(key,hdl_key,i-2,1,16))
		{	
			CUP_SW1SW2 = 0x6985;
			OSFileClose(hdl_key);
			return false;
		}
	} 
	

	if(0x00!=(head[2]>>6))//判断是否分散
	{
		if(((head[2]>>6)*8)>=head[4])
		{
		 	CUP_SW1SW2 = 0x6A86;
			OSFileClose(hdl_key);
			return false;	
		}
		//进行密钥分散,左半部分
	 	for(i=0;i<(head[2]>>6);i++)
		{
			/*if(0!=i)
			{
				for(j=0;j<8;j++)
				{
				 	buf[j] ^= (*(databuf+i*8+j));
				}	
		 	}*/
			vDES(buf,databuf+i*8,key, 2);//3DES加密
		//}
			MemoryCopy(buf, en_key, 8);
		
		//y右半部分密钥
		//for(i=0;i<(head[2]>>6);i++)
		//{
			for(j=0;j<8;j++)
			{
			 	*(databuf+i*8+j) = ~*(databuf+i*8+j);	
			}

			/*if(0!=i)
			{
				for(j=0;j<8;j++)
				{
				 	buf[j] ^= (*(databuf+i*8+j));
				}	
		 	}*/
			vDES(buf,databuf+i*8,key, 2);//3DES加密		
		
			MemoryCopy(buf, en_key+8, 8);
			MemoryCopy(en_key,key,16);

		}
		buf[0] = 0x01;//置为有效
		MemoryCopy(en_key, buf+1, 16);

		hdl_key = OSFileOpen(0x0007, 0);
		if(hdl_key==Not_Open_FILE)                       
		{                                        
			CUP_SW1SW2 = 0x6985;
			OSFileClose(hdl_key);
			return false;
		}
		if(false == OSFileSeek(hdl_key,0, 0))
		{
		   	CUP_SW1SW2 = 0x6985;
			OSFileClose(hdl_key);
			return false;
		}
		
		if(false == OSFileWrite(buf,17,hdl_key,0))
		{	
			CUP_SW1SW2 = 0x6985;
			OSFileClose(hdl_key);
			return false;
		}
		
		MemoryCopy(en_key,key,16);	
	}
	else
	{
	 	if(head[4] != 8)
		{
			CUP_SW1SW2 = 0x6700;
		 	return false;	
		}
	}

	//计算过程密钥
	if(0x00==(head[2]&0x10))
	{
	 	CUP_SW1SW2 = SW_SUCCESS;	
		return true;
	}
	vDES(en_key,databuf+(head[2]>>6)*8,key, 2);//3DES加密
	buf[0] = 0x01;//置为有效
	MemoryCopy(en_key, buf+1, 8);

	hdl_key = OSFileOpen(0x0006, 0);
	if(hdl_key==Not_Open_FILE)                       
	{                                        
		CUP_SW1SW2 = 0x6985;
		OSFileClose(hdl_key);
		return false;
	}
	if(false == OSFileSeek(hdl_key,0, 0))
	{
	   	CUP_SW1SW2 = 0x6985;
		OSFileClose(hdl_key);
		return false;
	}
	
	if(false == OSFileWrite(buf,9,hdl_key,0))
	{	
		CUP_SW1SW2 = 0x6985;
		OSFileClose(hdl_key);
		return false;
	}

	CUP_SW1SW2 = SW_SUCCESS;	
	return true;	 	
}

uint8 DEScrypt(uint8 *databuf,uint8 *head)
{
	uint8 buf[32];
	uint8 key[16];
	uint8 hdl_key,i,len;
	uint8 *pTmp;
	uint8 init_array[8]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};	

   	if(0!= (head[4]%8))
	{
	 	CUP_SW1SW2 = SW_WRONG_LENGTH;
		return false;	
	}

	//读取密钥，判断密钥的有效性
	hdl_key = OSFileOpen(0x0006, 0);
	if(hdl_key==Not_Open_FILE)                       
	{                                        
		CUP_SW1SW2 = 0x6985;
		OSFileClose(hdl_key);
		return false;
	}

	if(false == OSFileSeek(hdl_key,0, 0))
	{
	   	CUP_SW1SW2 = 0x6985;
		OSFileClose(hdl_key);
		return false;
	}
	
	if(false == OSFileRead(buf,9,hdl_key))
	{	
		CUP_SW1SW2 = 0x6985;
		OSFileClose(hdl_key);
		return false;
	}

	if(0 == buf[0])
	{
	 	CUP_SW1SW2 = 0x6985;
		OSFileClose(hdl_key);
		return false;
	}

	MemoryCopy(buf+1,key,8);
	
	if((head[2]>7))
	{
		CUP_SW1SW2 = 0x6985;
		OSFileClose(hdl_key);
		return false;
	}

	if(0==(head[2]&0x01))//加密
	{
		for(i=0;i<head[4]/8;i++)
		{
			vDES(databuf+i*8+1,databuf+i*8+1,key, 0);//DESjia密	
		}

		if(0x00 == (head[2]&0x20))
		{
			hdl_key = OSFileOpen(0x0006, 0);
			if(hdl_key==Not_Open_FILE)                       
			{                                        
				CUP_SW1SW2 = 0x6985;
				OSFileClose(hdl_key);
				return false;
			}

			if(false == OSFileSeek(hdl_key,0, 0))
			{
			   	CUP_SW1SW2 = 0x6985;
				OSFileClose(hdl_key);
				return false;
			}
			
			buf[0] = 1;
			if(false == OSFileWrite(buf,1,hdl_key,0))
			{	
				CUP_SW1SW2 = 0x6985;
				OSFileClose(hdl_key);
				return false;
			}			 				
		}

		OSFileClose(hdl_key);
		databuf[0] = head[4];
		CUP_SW1SW2 = 0x6100+head[4]; 	
		return true; 		
					
	}
	else//MAC
	{
				
		//读取初始是否有，判断密钥的有效性
		hdl_key = OSFileOpen(0x0008, 0);
		if(hdl_key==Not_Open_FILE)                       
		{                                        
			CUP_SW1SW2 = 0x6985;
			OSFileClose(hdl_key);
			return false;
		}
	
		if(false == OSFileSeek(hdl_key,0, 0))
		{
		   	CUP_SW1SW2 = 0x6985;
			OSFileClose(hdl_key);
			return false;
		}
		
		

		if(0x00 == (head[2]&0x04)) //无初始值
		{
			if(false == OSFileRead(buf,9,hdl_key))
			{	
				CUP_SW1SW2 = 0x6985;
				OSFileClose(hdl_key);
				return false;
			}

			if(0x01 == buf[0])
			{
				MemoryCopy(buf+1,init_array,8);
			}
			pTmp = 	databuf+1;
			len =  head[4];
			//MAC_MUL_Algo_Final(g_pbyProCmdBuf+1, head[4],key, 0,buf,init_array);						 		
		}
		else   //有初始值
		{
			pTmp = databuf+8+1;
			MemoryCopy(databuf+1,init_array,8);
			len =  head[4]-8;
			//MAC_MUL_Algo_Final(g_pbyProCmdBuf+8+1, head[4],key, 0,buf,g_pbyProCmdBuf+1); 			
		}

		*(pTmp+len) = 0x80;//首先强补80
		len++;

		if(0!=len%8)
		{
		 	for(i=0;i<(8-len%8);i++)
			{
			 	*(pTmp+len+i) = 0x00; 				
			}
			len += i;
		}

		MAC_MUL_Algo_Final(pTmp, len,key, 0,buf,init_array);

		//判断有无后续数据
		if(0x02 == (head[2]&0x02)) //有后续数据
		{
		   	i = 1;
		}
		else
		{
			i = 0;
		}

		if(false == OSFileSeek(hdl_key,0, 0))
		{
		   	CUP_SW1SW2 = 0x6985;
			OSFileClose(hdl_key);
			return false;
		}

		if(false == OSFileWrite(&i,1,hdl_key,0))
		{	
			CUP_SW1SW2 = 0x6985;
			OSFileClose(hdl_key);
			return false;
		}

	   	//去掉过程密钥有效性
		hdl_key = OSFileOpen(0x0006, 0);
		if(hdl_key==Not_Open_FILE)                       
		{                                        
			CUP_SW1SW2 = 0x6985;
			OSFileClose(hdl_key);
			return false;
		}

		if(false == OSFileSeek(hdl_key,0, 0))
		{
		   	CUP_SW1SW2 = 0x6985;
			OSFileClose(hdl_key);
			return false;
		}

		if(false == OSFileWrite(&i,1,hdl_key,0))
		{	
			CUP_SW1SW2 = 0x6985;
			OSFileClose(hdl_key);
			return false;
		}



		if(0x02 == (head[2]&0x02)) //有后续数据
		{
			CUP_SW1SW2 = SW_SUCCESS; 
		}
		else
		{
			MemoryCopy(buf,databuf+1,4);
			*databuf  = 4;
			CUP_SW1SW2 = 0x6104; 
		}
	}

	OSFileClose(hdl_key);
	//CUP_SW1SW2 = SW_SUCCESS; 	
	return true;		
}/**/

uint8 GetBankCardFileEntry(uint8 *databuf,uint8 *head)
{
	HANDLE hdl;	
	uint8 DataBuffer[3],buf[10];
	uint8 i,j;
	uint16 	ret;

	if(false == FileAttribute(0xEF04,DataBuffer))
	{
	 	return false;
	}

//	if(2!=DataBuffer[0])
//	{
//	 	CUP_SW1SW2 = 0x6981;
//		return false;	
//	}

	hdl = OSFileOpen(0xEF04, 0);
	if(hdl==Not_Open_FILE)                       
	{                                        
		CUP_SW1SW2 = SW_BAD_P1_P2;
		OSFileClose(hdl);
		return false;
	}
 	
 	if(2==head[3])
	{
		for(i=0;i<DataBuffer[1];i++)
		{
			if(false ==OSFileRecRead(buf,hdl,i+1,20,10))
			{	
				//CUP_SW1SW2 = SW_COMMAND_NOT_FILE;
				OSFileClose(hdl);
				return false;
			}
			
			for(j=0;j<10;j++)
			{
				if(databuf[j]!=buf[j])
					break; 	
			}

			if(j==10)
			 	break;	   				
		}
		
		if(i != DataBuffer[1])
		{
			ret = 0x8000>>i;
				 	
		}
		else
		{
			 ret = 0x0000;
		}
		CUP_SW1SW2 = 0x6100+0x02;
		databuf[0] = 0x02;
		databuf[1] = (uint8)(ret>>8);
		databuf[2] = (uint8)ret;
		OSFileClose(hdl);
		return true;		
	}
	else if(1==head[3])
	{
		ret = 0x0000;
	 	for(i=0;i<DataBuffer[1];i++)
		{
			if(false ==OSFileRecRead(buf,hdl,i+1,20,10))
			{	
				//CUP_SW1SW2 = SW_COMMAND_NOT_FILE;
				OSFileClose(hdl);
				return false;
			}

			if((buf[0]!=0xff) & (buf[0]!=0x00))
			{
			 	ret = ret | 0x01; 					
			}

			ret = ret<<1;		
				   				
		}

		ret = ret<<(16-DataBuffer[1]-1);
		
		ret = ~ret;
		ret = ret&0xffc0;		

		databuf[0] = 0x02;
		databuf[1] = (uint8)(ret>>8);
		databuf[2] = (uint8)ret;
		CUP_SW1SW2 = 0x6C02;
		OSFileClose(hdl);
		return true; 
	}
	
	CUP_SW1SW2 = 0x9000;
	OSFileClose(hdl);
	return true;	
}


typedef struct s_bankInfoFile{
 	uint8 name[20];
	uint8 ID[10];
	uint8 CD2[20];
	uint8 CD3[54];
	uint8 eff[2];
	uint8 rfu[2];
}S_BankInfoFile;

extern uint8 vRNG();
extern idata uint8 Random[11];
uint8 GetBankCardInfo(uint8 *databuf,uint8 *head,uint8 F8Count,uint8 *random)
{
	uint8 key[16];
	uint8 hdl_key,FileHandle;
	uint8 buf[32];
	uint8 recNo,i;
	S_BankInfoFile sBankInfoFile;

	for(i=0;i<8;i++)
	{
		//random[i+1]= 0x31+i;//vRNG();//
		Random[i+1]= vRNG();
	}

	random[0] = 8;

	

	if(0x80 != head[0])
	{
	 	CUP_SW1SW2 = SW_BAD_CLA;
		return false;
	}

	if((8!= (head[4]))||(head[4]==0))
	{
	 	CUP_SW1SW2 = SW_WRONG_LENGTH;
		return false;	
	}

	if((0x02 != head[2]))
	{
	 	CUP_SW1SW2 = 0x6a86;
		return false;
	}
	if((head[3]>10))
	{
	 	CUP_SW1SW2 = 0x6a83;
		return false;
	}

	//判断PIN是否被锁定、、、、、、、、、、、、
	FileHandle = OSFileOpen(0x0001, 0);
	
	if(0xff == FileHandle) 
	{
		CUP_SW1SW2 = 0x6a83;
		OSFileClose(FileHandle);
		return false;
	}

	if(false == OSFileSeek(FileHandle,9, 0)) 
	{
		CUP_SW1SW2 = 0x6a83;
		OSFileClose(FileHandle);
		return false;
	}

	if(false ==  OSFileRead(buf,1,FileHandle)) 
	{
		CUP_SW1SW2 = 0x6a83;
		OSFileClose(FileHandle);
		return false;
	}

	if(0==buf[0])
	{
		CUP_SW1SW2 = 0x6983;
		OSFileClose(FileHandle);
		return false;
	}

	if(15==F8Count)
	{
	 	//判断PIN是否被锁定,连续读15次

		if(false == OSFileSeek(FileHandle,9, 0)) 
		{
			CUP_SW1SW2 = 0x6a83;
			OSFileClose(FileHandle);
			return false;
		}

		buf[0] = 0;
		
		if(false == OSFileWrite(buf,1,FileHandle,0))
		{	
			CUP_SW1SW2 = 0x6a83;
			OSFileClose(FileHandle);
			return false;
		}
		
		OSFileClose(FileHandle);
		CUP_SW1SW2 = 0x6983;
		return false;			
	}

	

	/////////////////////////////////////////////

	//用16密钥对获取的时间进行加密生成过程密钥
	//读取密钥
   	hdl_key = OSFileOpen(0x0003, 0);
	if(hdl_key==Not_Open_FILE)                       
	{                                        
		CUP_SW1SW2 = SW_NOT_FIND_KEY;
		OSFileClose(hdl_key);
		return false;
	}

	if(false == OSFileRecRead(key,hdl_key,4,1,16))
	{	
		CUP_SW1SW2 = SW_NOT_FIND_KEY;
		OSFileClose(hdl_key);
		return false;
	}

	OSFileClose(hdl_key);

	vDES(key,databuf,key, 2);//3//DES加密

	vDES(key,random+1,key,0);	 //对随机数加密

	if(0 == head[3])//读取默认银行卡号
	{
		hdl_key = OSFileOpen(0xEF03, 0);
		if(hdl_key==Not_Open_FILE)                       
		{                                        
			CUP_SW1SW2 = SW_NOT_FIND_KEY;
			OSFileClose(hdl_key);
			return false;
		}

		if(false == OSFileSeek(hdl_key,0, 0))
		{
		   	CUP_SW1SW2 = SW_NOT_FIND_KEY;
			OSFileClose(hdl_key);
			return false;
		}
		
		if(false == OSFileRead(buf,1,hdl_key))
		{	
			CUP_SW1SW2 = SW_NOT_FIND_KEY;
			OSFileClose(hdl_key);
			return false;
		}

		OSFileClose(hdl_key);

		recNo = buf[0];			
									 	
	}
	else
	{
	 	recNo = head[3];
	}

	//读取银行卡记录
	hdl_key = OSFileOpen(0xEF04, 0);
	if(hdl_key==Not_Open_FILE)                       
	{                                        
		CUP_SW1SW2 = SW_RECORD_NOT_FOUND;
		OSFileClose(hdl_key);
		return false;
	} 

	if(false == OSFileRecRead((uint8*)(&sBankInfoFile),hdl_key,recNo,0,0))
	{	
		CUP_SW1SW2 = SW_RECORD_NOT_FOUND;
		OSFileClose(hdl_key);
		return false;
	}
	OSFileClose(hdl_key);

	//记录为空报错
	if((0xff==(sBankInfoFile.ID[0]))||(0x00==(sBankInfoFile.ID[0])))
	{
		CUP_SW1SW2 = SW_RECORD_NOT_INITALIZE;
		return false;	
	}
	
	//对磁道2信息加密
	recNo = (((sBankInfoFile.CD2[0])>>4)*10)+((sBankInfoFile.CD2[0])&0x0F);//转换为十六进制
	//if(1==(recNo%2))
	//{
	/// 	recNo = (recNo+1)/2;
	//}
	//else
	//{
		recNo = recNo/2;
	//}

	for(i=0;i<recNo/8;i++)
	{
		vDES(sBankInfoFile.CD2+1+8*i,sBankInfoFile.CD2+1+8*i,key,0);//DES加密		
	}

	//对磁道3信息加密
	for(i=0;i<4;i++)
	{
		vDES(sBankInfoFile.CD3+2+8*i,sBankInfoFile.CD3+2+8*i,key,0);//DES加密		
	}
	/**/
	databuf[0] = 108+8;
	MemoryCopy((uint8*)(&sBankInfoFile),databuf+1,108);
	//MemoryCopy(key,databuf+1+108,8);
	MemoryCopy(random+1,databuf+1+108,8);

   	CUP_SW1SW2 = 0x616C+8;
	return true;
}

uint8 AccessSwitch(uint8 *head)
{
   uint8 hdl_key;

   if(0x80 != head[0])
	{
	 	CUP_SW1SW2 = SW_BAD_CLA;
		return false;
	}

	if((0x00 != head[2])|(head[3]>2)|(head[3]==0))
	{
	 	CUP_SW1SW2 = SW_BAD_P1_P2;
		return false;
	}

	if(0x00 != head[4])
	{
	 	CUP_SW1SW2 = SW_WRONG_LENGTH;
		return false;
	}

 	hdl_key = OSFileOpen(0x0009, 0);

	if(hdl_key==Not_Open_FILE)                       
	{                                        
		CUP_SW1SW2 = SW_NOT_FIND_KEY;
		OSFileClose(hdl_key);
		return false;
	}

	if(false == OSFileSeek(hdl_key,0, 0))
	{
	   	CUP_SW1SW2 = SW_NOT_FIND_KEY;
		OSFileClose(hdl_key);
		return false;
	}
	
	if(false == OSFileWrite(head+3,1,hdl_key,0))
	{	
		CUP_SW1SW2 = SW_NOT_FIND_KEY;
		OSFileClose(hdl_key);
		return false;
	}

	OSFileClose(hdl_key);
	CUP_SW1SW2 = SW_SUCCESS;
	return true;  
		
}

uint8 GetAccessSwitch()
{
	uint8 buf[1];
	uint8 hdl_key;

	hdl_key = OSFileOpen(0x0009, 0);
	if(hdl_key==Not_Open_FILE)                       
	{                                        
		CUP_SW1SW2 = SW_NOT_FIND_KEY;
		return false;
	}

	if(false == OSFileSeek(hdl_key,0, 0))
	{
	   	CUP_SW1SW2 = SW_NOT_FIND_KEY;
		return false;
	}
	
	if(false == OSFileRead(buf,1,hdl_key))
	{	
		OSFileClose(hdl_key);
		return false;
	}

	OSFileClose(hdl_key);

	return buf[0];
}

uint8 UpdateCardName(uint8 *databuf,uint8 *head)
{
	uint8 hdl_key;	


  	if(0x80 != head[0])
	{
	 	CUP_SW1SW2 = SW_BAD_CLA;
		return false;
	}

	if((0x24 != head[3]))
	{
	 	CUP_SW1SW2 = SW_BAD_P1_P2;
		return false;
	}

	if((head[2]>10)|(head[2]==0))
	{
		CUP_SW1SW2 = 0x6a83;
		return false;
	}

	if(0x14 != head[4])
	{
	 	CUP_SW1SW2 = SW_WRONG_LENGTH;
		return false;
	}
	if(0 == PIN_VER_FLAG)
	{
	  	CUP_SW1SW2 = 0x6982;
		return false;
	}

	//读取银行卡记录
	hdl_key = OSFileOpen(0xEF04, 0);
	if(hdl_key==Not_Open_FILE)                       
	{                                        
		CUP_SW1SW2 = 0x6a83;
		OSFileClose(hdl_key);
		return false;
	} 

	if(false == OSFileRecWrite(databuf+1,hdl_key,head[2],0,head[4]))
	{	
		CUP_SW1SW2 = 0x6a83;
		OSFileClose(hdl_key);
		return false;
	}
 	
}

uint8 LockCard(uint8 *head)
{
	uint8 i,hdl;

	if(0x80 != head[0])
	{
	 	CUP_SW1SW2 = SW_BAD_CLA;
		return false;
	}

	if((0x00 != head[3])|(head[2]!=0x00))
	{
	 	CUP_SW1SW2 = SW_BAD_P1_P2;
		return false;
	}

	if(0x00 != head[4])
	{
	 	CUP_SW1SW2 = SW_WRONG_LENGTH;
		return false;
	}

 	i = 0x01;
	hdl = OSFileOpen(0xA002, 0);
	if(hdl==Not_Open_FILE)                       
	{                                        
		CUP_SW1SW2 = 0X6981;
		OSFileClose(hdl);
		return false;
	}
	if(false == OSFileSeek(hdl,0, 0))
	{
	   	CUP_SW1SW2 = 0X6981;
		OSFileClose(hdl);
		return false;
	}
	
	if(false == OSFileWrite(&i,1,hdl,0))
	{	
		CUP_SW1SW2 = 0X6981;
		OSFileClose(hdl);
		return false;
	}

	AC_Change(0xEF01,0x10);
	AC_Change(0xEF02,0x10);
	AC_Change(0xEF03,0x12);

	Set_ACCESS_RIGHT();

	CUP_SW1SW2 = SW_SUCCESS;
	return true; 
}

uint8 StartPersonal(uint8 *head)
{
	uint8 i,hdl;
	hdl = OSFileOpen(0xA002, 0);

	if(0x80 != head[0])
	{
	 	CUP_SW1SW2 = SW_BAD_CLA;
		return false;
	}

	if((0x00 != head[3])|(head[2]!=0x00))
	{
	 	CUP_SW1SW2 = SW_BAD_P1_P2;
		return false;
	}

	if(0x00 != head[4])
	{
	 	CUP_SW1SW2 = SW_WRONG_LENGTH;
		return false;
	}

	if(hdl==Not_Open_FILE)                       
	{                                        
		CUP_SW1SW2 = 0X6981;
		OSFileClose(hdl);
		return false;
	}
	if(false == OSFileSeek(hdl,0, 0))
	{
	   	CUP_SW1SW2 = 0X6981;
		OSFileClose(hdl);
		return false;
	}
	
	if(false == OSFileRead(&i,1,hdl))
	{	
		CUP_SW1SW2 = 0X6981;
		OSFileClose(hdl);
		return false;
	}

	if((0x01==i)|(0x02==i))
	{
	 	CUP_SW1SW2 = 0x6D00;
		return false;	
	}
	else
	{
		Close_ACCESS_RIGHT();
	 	CUP_SW1SW2 = 0x9000;
	}
	
	return true;  	
}

void Close_ACCESS_RIGHT()
{
	g_NEED_ACCESS_RIGHT=1;
}

void Set_ACCESS_RIGHT()
{
	g_NEED_ACCESS_RIGHT=0;
}

bool SavePersonalData(uint8 *databuff,uint8 *head)
{
	uint8 hdl,i,j;
	uint8 pbyBuff[60];

	//计算mac
	hdl = OSFileOpen(0x00E0, 0x11);
	if(hdl==Not_Open_FILE)                       
	{                                        
		OSFileClose(hdl);
		return false;
	}	
										
	if(false == OSFileSeek(hdl,0,0))
	{
		OSFileClose(hdl);
		return false;
	}
	
	if(false == OSFileRead(pbyBuff,1,hdl))
	{	
		OSFileClose(hdl);
		return false;
	}
	
	if((0xff==pbyBuff[0])|(pbyBuff[0]>0x30)|(0==pbyBuff[0]))
	{
		pbyBuff[0]=0;

	}
	
	pbyBuff[0]++;

	if(false == OSFileSeek(hdl,0, 0))
	{
		OSFileClose(hdl);
		return false;
	}

	if(false == OSFileWrite(pbyBuff,1,hdl,0))
	{	
		OSFileClose(hdl);
		return false;
	}

	pbyBuff[0]--;
	j = pbyBuff[0];
	
	for(i=0;i<j;i++)
	{
		if(false == OSFileRead(pbyBuff,1,hdl))
		{	
			OSFileClose(hdl);
			return false;
		}

	 	if(false == OSFileSeek(hdl,pbyBuff[0], 1))
		{
			OSFileClose(hdl);
			return false;
		} 					
	}

	pbyBuff[0] = 5+head[4];
	if(false == OSFileWrite(pbyBuff,1,hdl,0))
	{	
		OSFileClose(hdl);
		return false;
	}
	
	if(false == OSFileWrite(head,5,hdl,0))
	{	
		OSFileClose(hdl);
		return false;
	}
	
	if(false == OSFileWrite(databuff,head[4],hdl,0))
	{	
		OSFileClose(hdl);
		return false;
	}				
}

uint8 PersonalDataCmdHandle(uint8 *pbyCmd,uint8 *Random)
{
	uint8 byCmdType=*(pbyCmd+2);

	uint8 *pbyRet=pbyCmd+50;

	switch (byCmdType)
	{
		case 0xA4:
		{
			SelectFile_CUP(pbyCmd+6,pbyCmd+1);	
			break;
		}
		case 0xD6:
		{
			//Updata_Binary_New(pbyCmd,pbyRet);
			Close_ACCESS_RIGHT();
			Updata_Binary(pbyCmd+6,pbyCmd+1);
			Set_ACCESS_RIGHT();	

			break;
		}
		case 0xF4:
		{
			//Updata_Binary_New(pbyCmd,pbyRet); 
			Write_Key(pbyCmd,Random,pbyRet);			 
			break;
		}
		case 0x84:
		{
			//Updata_Binary_New(pbyCmd,pbyRet);
			Get_Challenge(pbyCmd,Random);
			break;
		}
		case 0x26:
		{
			//Updata_Binary_New(pbyCmd,pbyRet); 
			LockCard(pbyCmd+1);
			break;
		}
		case 0xE0:
		{
			//Updata_Binary_New(pbyCmd,pbyRet); 
			StartPersonal(pbyCmd+1); 
			break;
		}
		default:
		{
			break;
		}

	}
	return true;

}

bool HandlePersonalData(uint8 *Random)
{
	uint8 hdl,i,j;
	uint8 pbyBuff[255];
	uint16 SW;

	//计算mac
	hdl = OSFileOpen(0x00E0, 0x11);
	if(hdl==Not_Open_FILE)                       
	{                                        
		OSFileClose(hdl);
		return false;
	}	
										
	if(false == OSFileSeek(hdl,0,0))
	{
		OSFileClose(hdl);
		return false;
	}
	
	if(false == OSFileRead(pbyBuff,1,hdl))
	{	
		OSFileClose(hdl);
		return false;
	}
	
	if((0xff==pbyBuff[0])|(pbyBuff[0]>0x30)|(0==pbyBuff[0]))
	{
		return false;
	}

	j=pbyBuff[0];
	
	/*for test 
		Random[0] = 0x08; 
		Random[1] = 0x31; 
		Random[2] = 0x32; 
		Random[3] = 0x33; 
		Random[4] = 0x34; 
		Random[5] = 0x35; 
		Random[6] = 0x36; 
		Random[7] = 0x37; 
		Random[8] = 0x38; 
		//////////////////////////for test*/

	for(i=0;i<j;i++)
	{
		if(false == OSFileRead(pbyBuff,1,hdl))
		{	
			OSFileClose(hdl);
			return false;
		}

		if(false == OSFileRead(pbyBuff+1,pbyBuff[0],hdl))
		{
			OSFileClose(hdl);
			return false;
		}
		
		

		PersonalDataCmdHandle(pbyBuff,Random); 

		SW = CUP_SW1SW2;
		
		if(SW != 0x9000)
		{
			break;
		}
		
			 						
	}
	
	if(false == OSFileSeek(hdl,0,0))
	{
		OSFileClose(hdl);
		return false;
	}

	pbyBuff[0] = 0;
	if(false == OSFileWrite(pbyBuff,1,hdl,0))
	{	
		OSFileClose(hdl);
		return false;
	}

	CUP_SW1SW2= SW;
		
	OSFileClose(hdl); 
	return true;			
}
