#include "communal.h"

//======================Json================================
static sIdDataTable* pJsonTable=NULL;

int PE_JsonDataParse(char *pIndata,int inLen)
{
	sIdDataTable* pJson =Conv_SingleJSON_GetMsg(pIndata,pIndata+inLen);
	TRACE("PE_JsonDataParse[%x]\r\n",pJson);
	if(pJson == NULL)
		return -1;
	pJson->pPrevious=pJsonTable;
	pJsonTable=pJson;
	return 0;	
}

void PE_JsonFree(void)
{
	if(pJsonTable)
	{
		sIdDataTable* pPrevious;
		pPrevious=pJsonTable->pPrevious;
		free(pJsonTable);
		pJsonTable=pPrevious;
	}
}

char* PE_GetRecvIdPar(char* pPostID)
{
	if(pJsonTable==NULL)
		return NULL;
	return Conv_GetParFindID(pJsonTable,pPostID);
}

//=========读取ID数据=================================
int PE_ReadRecvIdPar(char* pPostID,char *pOutData)
{
	char *pData;
	if(pJsonTable==NULL)
		return 1;
	pData=Conv_GetParFindID(pJsonTable,pPostID);
	if(pData)
	{
		API_strcpy(pOutData,pData);
		return OPER_OK;
	}
	return 1;
}

//=========比对ID对应的参数================================
int PE_CheckRecvIdPar(char* pPostID,char *pCheck)
{
	char *pData;
	if(pJsonTable==NULL)
		return OPER_ERR;
	pData=Conv_GetParFindID(pJsonTable,pPostID);
	if(pData)
	{
		if(0 == API_strcmp(pCheck,pData))
			return OPER_OK;
		TRACE("Id[%s]CheckData target[%s],Read[%s]\r\n",pPostID,pCheck,pData);
	}
	return OPER_ERR;
}

int PE_Sign_Check(const char* KeyName,char* KeyData)
{
	return Conv_Sign_Check(pJsonTable,KeyName,KeyData);
}
//=======================================================================================

int PE_CheckRecvData(char* pInRecv,int Len)
{
	// 校验数据
	pInRecv[Len]=0;
	TRACE("Network_Recv[%s]OK\r\n",pInRecv);

	if(!PE_JsonDataParse(pInRecv,Len))
	{
		if (OPER_CRCERR == PE_Sign_Check("key",g_ColData.signkey)){
			APP_ShowTradeMsg("验证签名错", 5000);
			PE_JsonFree();
			return OPER_CRCERR;
		} 
		return 0;
	}
	TRACE("SingleJSON Data[%s]\r\n",pInRecv);
	APP_ShowTradeMsg("检查数据错", 5000);
	return OPER_ERR;
 }

int InputTotalFee(char *pTitle)
{
	CLEAR(g_ColData.payAmount);
	API_strcpy(g_ColData.payAmount,"0");
	if(0>APP_EditSum(pTitle,'S',g_ColData.payAmount,60*1000))
	{
		return -1;
	}
	
    TRACE("g_ColData.amt:%s\r\n",g_ColData.payAmount);
	return 0;
}

int InputAuthcodeByCamScan(void)
{
	CLEAR(g_ColData.authCode);
	if(0 > APP_CamScan('S',g_ColData.payAmount,g_ColData.authCode,10,64,20*1000)) 
	{
		return -1;
	}
	return 0;
}
int APP_ScanInput(char* pTitle,char* pFrontTextBuf,char* pAfterTextBuf,char* pInputDef,int InputMinLen,int InputMaxlen)	
{
       int 	ret;
	EDIT_DATA	EditData;	
	CLEAR(EditData);
	EditData.pTitle	      =pTitle;
	EditData.pFrontText	=pFrontTextBuf;
	EditData.pAfterText      =pAfterTextBuf;
	EditData.Min		=InputMinLen;
	EditData.Max		=InputMaxlen;
	EditData.Way		=IME_NUM;
	EditData.Limit		=IME_NUM|IME_ABC|IME_abc;
	EditData.timeOutMs=30*1000;

	ret =APP_Edit(&EditData,pInputDef);
	if(ret>=0)
		return 0;
	else
		return 1;	
}
int PE_CamScan(char flagd,char* pHintOrTMoney,char *pOutCode,int MinLen,int MaxLen,int timeOutMs)
{
	u16 	flagKey=0x00;
	char 	sShowStr[12];
	RECTL	Rect;
	Rect.left	= UI_MAP_TITLE_X;
	Rect.top	= UI_MAP_TITLE_Y;
	Rect.width	= UI_MAP_TITLE_W;
	Rect.height = UI_MAP_TITLE_H;
	if(flagd=='D')
		UI_ShowPictureFile(&Rect,"InSumTd.clz");
	else if(flagd=='R')
		UI_ShowPictureFile(&Rect,"InSumTr.clz");
	else if(flagd=='S')
		UI_ShowPictureFile(&Rect,"InSumTs.clz");
	else if(flagd=='Q')
		UI_ShowPictureFile(&Rect,"InSumTq.clz");
	
	Rect.top	= UI_MAP_CONT_Y;
	Rect.height = UI_MAP_CONT_H;
	if(flagd=='R' || flagd=='Q')
	{
		UI_ShowPictureFile(&Rect,"ScanIpt.clz");
		Rect.top	= UI_SCAN_TEXT_Y;
		if(flagd=='R')	// "订单退款"
			flagKey	= 0x02;
		else			// "交易查询"
			flagKey	= 0x02;
	}
	else
	{
		UI_ShowPictureFile(&Rect,"ScanCod.clz");
		Rect.top	= UI_SCAN_MONEY_Y;
	}
	if(pHintOrTMoney)
	{
		Rect.left	= UI_EDIT_sMONEY_X;
		Rect.width	= UI_EDIT_sMONEY_W;
		Rect.height = UI_EDIT_sMONEY_H;
		if(pHintOrTMoney[0]>='0' && pHintOrTMoney[0]<='9')
		{
			Conv_TmoneyToDmoney(sShowStr,pHintOrTMoney);
			UI_ShowSmallNum(&Rect,1,sShowStr);
		}
		else
		{
			POINT rclTrg;
			u16		Width;
			Width = API_strlen(pHintOrTMoney)*FONT_SIZE/2;
			if(Width < Rect.width)
				rclTrg.left=Rect.left+(Rect.width-Width)/2;
			else rclTrg.left=Rect.left;
			rclTrg.top =Rect.top;
			UI_SetColorRGB565(RGB565_WITHE,RGB565_PARENT);
			UI_DrawString(&rclTrg,pHintOrTMoney);
		}
	}
	API_GUI_Show();	
	return APP_OnlyCamScan(flagKey,MinLen,MaxLen,pOutCode,timeOutMs);
}
int PE_ShowMsg(char *pTitle,char* pTopMsg,char *pMsg,char *prompt, int timeOutMs)
{
	API_GUI_CreateWindow(pTitle,TOK,TCANCEL,0);
	if(pTopMsg)
		API_GUI_Info(NULL,TEXT_ALIGN_CENTER|TEXT_VALIGN_TOP,pTopMsg);// 顶部，
	if(pMsg)
		API_GUI_Info(NULL,TEXT_ALIGN_CENTER|TEXT_VALIGN_CENTER,pMsg);
	if(prompt)
	API_GUI_Info(NULL,TEXT_ALIGN_RIGHT|TEXT_VALIGN_BOTTOM|TEXT_EXSTYLE_OVERLINE,prompt);
	API_GUI_Show();
	return APP_WaitUiEvent(timeOutMs);
}

int PE_ShowEnquiriesMsg(char* status,char *pTmoney,char* pDateTime)
{
	RECTL	Rect;
	char 	sBuff[22]={0};
	u16		sLen;
	POINT 	rclTrg;
	Rect.left	= SCREEN_APP_X;
	Rect.top	= SCREEN_APP_Y;
	Rect.width	= SCREEN_APP_W;
	Rect.height = SCREEN_APP_H;
	UI_ShowPictureFile(&Rect,"Empty.clz");
	//-------显示金额----------------
	Rect.left	= UI_ENQU_MONEY_X;
	Rect.top	= UI_ENQU_MONEY_Y+UI_ENQU_SPACING_H;
	Rect.width	= UI_ENQU_MONEY_W;
	Rect.height = UI_ENQU_MONEY_H;
	//-----------
	
	sLen=API_strlen(status);
	rclTrg.left=Rect.left+(Rect.width - sLen*FONT_SIZE/2)/2;
	rclTrg.top=Rect.top;
	UI_SetColorRGB565(RGB565_WITHE,RGB565_PARENT);
	UI_DrawString(&rclTrg,status);//

	Rect.top += FONT_SIZE+UI_ENQU_SPACING_H;
	sLen=API_strlen("交易金额:");
	rclTrg.left=Rect.left+(Rect.width - sLen*FONT_SIZE/2)/2;
	rclTrg.top =Rect.top;
	UI_SetColorRGB565(RGB565_WITHE,RGB565_PARENT);
	UI_DrawString(&rclTrg,"交易金额:");// 
	Rect.top  +=FONT_SIZE;
	CLEAR(sBuff);
	Conv_TmoneyToDmoney(sBuff,pTmoney);
	UI_ShowSmallNum(&Rect,1,sBuff);
	//-------显示交易日期时间---居中显示-------------
	Rect.top += FONT_SIZE+UI_ENQU_SPACING_H+UI_ENQU_SPACING_H;
	API_strcpy(sBuff,"交易日期时间");
	sLen=API_strlen(sBuff);
	rclTrg.left=Rect.left+(Rect.width - sLen*FONT_SIZE/2)/2;
	rclTrg.top=Rect.top;
	UI_SetColorRGB565(RGB565_WITHE,RGB565_PARENT);
	UI_DrawString(&rclTrg,sBuff);// 交易时间
	Rect.top += FONT_SIZE;
	sLen=API_strlen(pDateTime);
	rclTrg.left=Rect.left+(Rect.width - sLen*FONT_SIZE/2)/2;
	rclTrg.top=Rect.top;
	UI_SetColorRGB565(RGB565_WITHE,RGB565_PARENT);
	UI_DrawString(&rclTrg,pDateTime);
    //-----------------商户单号-------------
    Rect.top += FONT_SIZE+UI_ENQU_SPACING_H+UI_ENQU_SPACING_H;
	API_strcpy(sBuff,"商户单号");
	sLen=API_strlen(sBuff);
	rclTrg.left=Rect.left+(Rect.width - sLen*FONT_SIZE/2)/2;
	rclTrg.top=Rect.top;
	UI_SetColorRGB565(RGB565_WITHE,RGB565_PARENT);
	UI_DrawString(&rclTrg,sBuff);//
    Rect.top += FONT_SIZE;
	CLEAR(sBuff);
	API_memcpy(sBuff,g_ColData.authCode,10);
	sLen=API_strlen(sBuff);
	rclTrg.left=Rect.left+(Rect.width - sLen*FONT_SIZE/2)/2;
	rclTrg.top=Rect.top;
	UI_SetColorRGB565(RGB565_WITHE,RGB565_PARENT);
	UI_DrawString(&rclTrg,sBuff);// 太长可以自动换行显示
	Rect.top += FONT_SIZE;
	sLen=API_strlen((g_ColData.authCode+10));
	rclTrg.left=Rect.left+(Rect.width - sLen*FONT_SIZE/2)/2;
	rclTrg.top=Rect.top;
	UI_SetColorRGB565(RGB565_WITHE,RGB565_PARENT);
	UI_DrawString(&rclTrg,g_ColData.authCode+10);// 太长可以自动换行显示
	API_GUI_Show();
	//--------------------------------------------------------
	 return APP_WaitUiEvent(10*1000);
}






