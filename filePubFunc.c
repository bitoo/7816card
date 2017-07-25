#include "filePubFunc.h"
#include "..\export_new\types.h"
#include "..\..\Target_Driver\export_new\simcommand.h"
#include "..\export_new\fileapi.h"



bool hal_eeprom_read( iu8 *dst, iu32 src, iu8 len )
{
	int i;
	if((dst==NULL)||
		(src==NULL)||
		(0==len)) 
		return false;
	//MemoryCopy((iu8*)src,dst,len);
	for(i=0;i<len;i++)
	{
		//dst[i] = CBYTE[src+i];	
		dst[i] = Sim_ReadCode(src+i);
	}
	return true;
}  

bool writeFlash(iu32 des,iu8 *src,iu16 len)
{
	xdata uint8 g_file_Pub_Buffer[0x101];
	uint16 i,No,j;
	//uint8 j;
	uint8 *pT=NULL;
	iu32 tp=des;

	tp = tp/BLOCKLEN;
	tp *=BLOCKLEN;	
		

//断电保护b
	 //写相关参数，0,block个数1byte，1.长度2byte，2.地址tp，4个字节,3.掉电保护区状态，一个自己，0为开始写保护区，1为保护区完成，2为保护区失效
	 
	 g_file_Pub_Buffer[7] =  0;	 
	 Sim_EraseFlash(POWERADDR,0x00);

	 for(i=0;i<8;i++)
	 {
	 	Sim_WriteCode((POWERADDR+i),*(g_file_Pub_Buffer+i));
	 }

	 No = (des+len-tp)/BLOCKLEN+1; 
	 //写一页
	 if(No<4)
	 {
		for(j=1;j<No+1;j++)
		{
			for(i=0;i<BLOCKLEN;i++)
			{
				g_file_Pub_Buffer[i] = Sim_ReadCode(tp+i+j*BLOCKLEN);
			}
			Sim_EraseFlash((POWERADDR+(j*BLOCKLEN)),0x00);
			for(i=0;i<BLOCKLEN;i++)
			{
				Sim_WriteCode((POWERADDR+(j*BLOCKLEN)+i),*(g_file_Pub_Buffer+i));
			} 
		}
	 }

	//掉电保护完成
	 //No = (des+len-tp)/BLOCKLEN+1; 
	 g_file_Pub_Buffer[0] =  No;
	 g_file_Pub_Buffer[1] =  ((des+len-tp)>>8)&0xff;
	 g_file_Pub_Buffer[2] =  (des+len-tp)&0xff;
	 g_file_Pub_Buffer[3] =  (tp>>24)&0xff;
	 g_file_Pub_Buffer[4] =  (tp>>16)&0xff;
	 g_file_Pub_Buffer[5] =  (tp>>8)&0xff;
	 g_file_Pub_Buffer[6] =  (tp)&0xff;
	 g_file_Pub_Buffer[7] =  1;

	 for(i=0;i<8;i++)
	 {
	 	Sim_WriteCode((POWERADDR+i),*(g_file_Pub_Buffer+i));
	 }

//断电保护e	
	
	for(i=0;i<BLOCKLEN;i++)
	{
		g_file_Pub_Buffer[i] = Sim_ReadCode(tp+i);
	}
	
	Sim_EraseFlash(tp,0x00);

	//change the block
	if((tp+0x100)>des+len)
	{//不跨页，在一页以内
		//MCopy(src,g_file_Pub_Buffer+des-tp,len);
		for(i=0;i<len;i++)
		{
			g_file_Pub_Buffer[i+des-tp] = src[i];
		}

		pT = (uint8*)(src);
	}
	else
	{//跨页
		No = (des+len - (tp+0x100))%0x100;
		No = (len-No)%0x100;
		//MCopy(src,g_file_Pub_Buffer+des-tp,No);
		for(i=0;i<No;i++)
		{
			g_file_Pub_Buffer[i+des-tp] = src[i];
		}
		pT = (uint8*)(src + No);
	}

		
	//写第一页
	for(i=0;i<BLOCKLEN;i++)
	{
		Sim_WriteCode((tp+i),*(g_file_Pub_Buffer+i));
	} 
	

	if((tp+0x100)<=(des+len))
	{
		//先写第一页?
		//读取数据到g_pub_buffer
		tp += 0x100;
		//No = des-tp;
		No = (des+len-tp)/0x100; 

		if(No>=1)
		{
			//erase zhe block
			for(i=0;i<No;i++)
			{
				Sim_EraseFlash(tp,0x00);
				for(j=0;j<BLOCKLEN;j++)
				{
					Sim_WriteCode(tp+j,*pT);
					//Sim_WriteCode(tp+j,0xdd);
					pT++;
				}
				tp += 0x100;
			}
		}
		for(i=0;i<BLOCKLEN;i++)
		{
			g_file_Pub_Buffer[i] = Sim_ReadCode(tp+i);	
		}	
		
		

		//change the block
		No = (des+len - (tp))%0x100;
		if(No==0)return true;

		//erase zhe block
		Sim_EraseFlash(tp,0x00);

		//MCopy(pT,g_file_Pub_Buffer,No);
		for(i=0;i<No;i++)
		{
			g_file_Pub_Buffer[i] = pT[i];
		}
	
		for(i=0;i<BLOCKLEN;i++)
		{
			Sim_WriteCode(tp+i,*(g_file_Pub_Buffer+i));
			//Sim_WriteCode(tp+i,0xcc);
		}
		//写第二页
		
	}

//断电保护把b
	//写完成
	g_file_Pub_Buffer[7] =  2;	
	Sim_EraseFlash(POWERADDR,0x00);

	for(i=0;i<8;i++)
	{
		Sim_WriteCode((POWERADDR+i),*(g_file_Pub_Buffer+i));
	}

//断电保护e	

	////获得此页的首地址
	return true;		
}

