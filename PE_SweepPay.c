#include "communal.h"


DfFlow_Link  pFlow={0};
QR_COL_Data  g_ColData;

//static pDataTable  pItemdata;

void DataInit()
{
	if(PE_SendBuf==NULL)
	{
		PE_SendBuf=malloc(BufSize);
	}
	PE_pSend = PE_SendBuf;	
	if(pFlow.pMd5Start==NULL)
	{
		pFlow.pMd5Start=malloc(2048);
	}
	PE_pMD5 = pFlow.pMd5Start;
}
void DataFree(void)
{
	PE_JsonFree(0);
}

void DataEnd(void)
{
	if(PE_SendBuf)
	{
		free(PE_SendBuf); PE_SendBuf=NULL;
	}
	if(pFlow.pMd5Start)
	{
		free(pFlow.pMd5Start); pFlow.pMd5Start=NULL;
	}
}


int Full_CheckRecv(char *pRecv,int ReadLen)
{
	if(ReadLen > 0)
	{
		int		cycle;
		char	*pStrStart,*pEnd;
		pStrStart = pRecv;
		pEnd = pStrStart+ReadLen;
		//------------ÕÒµ½Æðµã-----------------------
		while(pStrStart < pEnd)
		{
			if(*pStrStart++ == '{') break;
		}
		if(pStrStart >= pEnd) return 0;
		//------------ÕÒµ½ÖÕµã-----------------------
		cycle = 1;
		while(pStrStart < pEnd)
		{
			if(*pStrStart == '{')
				cycle++;
			else if(*pStrStart == '}')
				cycle--;
			if(cycle == 0) break;
			pStrStart++;
		}
		if(cycle) return 0;
		pStrStart++;
		return (pStrStart-pRecv);
	}
	//TRACE("RecvErrData[%s]\r\n",pRecv);
	return 0;	
}



int SuccessTradeProcess(void)
{
	char DisplayMoney[16];
	char *pPayType,*payType;
	Conv_TmoneyToDmoney(DisplayMoney,g_ColData.payAmount);
	//APP_ShowTradeOK(g_ColData.payAmount); 
	APP_ShowTrade("TradeOK.clz",DisplayMoney,"½»Ò×³É¹¦");
	payType=PE_GetRecvIdPar("payType");
	/*
	wx_pay£ºÎ¢ÐÅÖ§¸¶£»
	ali_pay£ºÖ§¸¶±¦£»
	union_offline£ºÒøÐÐ¿¨£»
	union_qrcode£ºÒøÁªÉ¨Âë£»
	union_online£ºÒøÁªÇ®°ü£»
	member_wallet£º»áÔ±Ç®°ü£»
	cash£ºÏÖ½ð
	*/
	if(API_strcmp(payType,"wx_pay")==0)
	{
		pPayType="Î¢ÐÅÊÕ¿î";
	}
	else if(API_strcmp(payType,"ali_pay")==0)
	{
		pPayType="Ö§¸¶±¦ÊÕ¿î";
	}
	else //union_offline£ºÒøÐÐ¿¨£»union_qrcode£ºÒøÁªÉ¨Âë£»union_online£ºÒøÁªÇ®°ü
	{
		pPayType="ÊÕ¿î³É¹¦";
	}	
    APP_TTS_PlayText("%s%sÔª",pPayType,DisplayMoney);
	APP_WaitUiEvent(10*1000);
	//-------¸üÐÂ±¾µØÊ±¼ä----------
	SetSysDateTime(PE_GetRecvIdPar("payTime"));
	return 0;
}

void Send_add_item(char* pID,char* pData)
{
	if(API_strlen(pData))
	{
		PE_pSend=Conv_SetPackageJson(PE_pSend,pID,pData);
		*PE_pSend++= ',';	//Á¬½ÓÏÂÒ»Ìõ
		PE_pMD5=Conv_SetPackageHttpGET(PE_pMD5,pID,pData);
		*PE_pMD5++= '&';	//Á¬½ÓÏÂÒ»Ìõ
	}
}

void Send_add_end_Sign(void)
{
	u32 len;
	u8 uMd5Out[20]; 
	PE_pMD5=Conv_SetPackageHttpGET(PE_pMD5,"key",g_ColData.signkey);
	len = PE_pMD5-pFlow.pMd5Start;
	md5((u8*)pFlow.pMd5Start,len,uMd5Out);
	Conv_BcdToStr(uMd5Out,16,g_ColData.sign);
	PE_pSend=Conv_SetPackageJson(PE_pSend,"sign",g_ColData.sign);
}



// µÚ1ÌõµÄ¸½ÊôÐÅÏ¢  Ò²ÊÇ¸úÇ°ÖÃÔ¼¶¨µÄÀàÐÍ£¬ÀàÐÍ²»Í¨Á´½Ó²»Í¬
void TradeDataPacked_start(char *pTradeAddress)
{  
	DataInit();
	PE_sLenStart=Conv_Post_SetHead(pTradeAddress,"application/json",PE_pSend);
	//API_sprintf(pSendLenAddr[7], "% 3d\r\n\r\n",API_strlen(param)) Ô¤Áô7¸ö×Ö½Ú¿Õ¼ä
	PE_pSend=PE_sLenStart+7;
	*PE_pSend++ ='{';
}

void TradeDataPacked_end(void)
{	
	char sendLen[8];
	*PE_pSend++ = '}';
	//----------------°Ñ3¸ö×Ö½Ú¹Ì¶¨³¤¶ÈÐ´ÈëbuffÇø---------------------
	API_sprintf(sendLen,"%3d\r\n\r\n",PE_pSend-PE_sLenStart-7); //Ô¤Áô7¸ö×Ö½Ú¿Õ¼ä
	API_memcpy(PE_sLenStart,sendLen,7);
	*PE_pSend++ = '0';
	*PE_pSend = '\0';
	pFlow.pMsgLen=PE_pSend-PE_SendBuf;
	TRACE("->PE_SendBuf:\r\n%s\r\n", PE_SendBuf);
}

void UpData_timestampAndNoncestr(int GenId)
{
	u32 timeStamp;
	char timeMs[4];
	ST_TIME tTime;
	OsGetTime(&tTime);
	Conv_DateToTimestamp(&tTime,8,&timeStamp);
	sprintf(g_ColData.timestamp,"%d",timeStamp);
	
	timeStamp=API_TimeCurrMS();
	timeMs[3]= '\0';
	timeMs[2]='0'+ (timeStamp%10); 
	timeStamp /= 10;
	timeMs[1]='0'+ (timeStamp%10); 
	timeStamp /= 10;
	timeMs[0]='0'+ (timeStamp%10);
	
	memcpy(g_ColData.nonceStr,g_ColData.sn,8);
	sprintf(g_ColData.nonceStr+8,"%s%s",timeMs,g_ColData.timestamp);
	if(GenId)
		sprintf(g_ColData.outTradeId,"%s%s%s",g_ColData.sn+4,g_ColData.timestamp,timeMs);
}


void SweepPackData_V2(void)
{
	UpData_timestampAndNoncestr(1);
	SEND_ADD_ITEM(authCode);
	SEND_ADD_ITEM(developerId);
	SEND_ADD_ITEM(nonceStr);
	SEND_ADD_ITEM(outTradeId);
	SEND_ADD_ITEM(payAmount);
//	SEND_ADD_ITEM(returnContent);
//	SEND_ADD_ITEM(shopId);
	SEND_ADD_ITEM(sn);
	SEND_ADD_ITEM(terminalType);
	SEND_ADD_ITEM(timestamp);
	Send_add_end_Sign();
}

/*
//-------------ÈýÏÈÒ»----------------
SEND_ADD_ITEM(tradeId);
SEND_ADD_ITEM(outTradeId);
SEND_ADD_ITEM(transactionId);

*/
void PackData_OrderQuery(void)
{
	//g_ColData.payType; 
	UpData_timestampAndNoncestr(0);
	SEND_ADD_ITEM(developerId);
	SEND_ADD_ITEM(nonceStr);
	SEND_ADD_ITEM(outTradeId);
	SEND_ADD_ITEM(shopId);
	SEND_ADD_ITEM(sn);
	SEND_ADD_ITEM(terminalType);
	SEND_ADD_ITEM(timestamp);
	SEND_ADD_ITEM(tradeId);
	SEND_ADD_ITEM(transactionId);
	Send_add_end_Sign();
}

//   /open/Pay/refund
void PackData_refund(void)
{
	//g_ColData.payType; 
	UpData_timestampAndNoncestr(0);
	SEND_ADD_ITEM(developerId);
	SEND_ADD_ITEM(nonceStr);

//	if(g_ColData.terminalType[0] == '2')
	{
		//SEND_ADD_ITEM(outTradeId);
		SEND_ADD_ITEM(refundFee);
//		SEND_ADD_ITEM(returnContent);
		SEND_ADD_ITEM(sn);
		SEND_ADD_ITEM(terminalType);
		SEND_ADD_ITEM(timestamp);
		SEND_ADD_ITEM(tradeId);
		SEND_ADD_ITEM(transactionId);
	}
/*
	else if(g_ColData.terminalType[0] == '1')
	{
		//SEND_ADD_ITEM(outTradeId);
		SEND_ADD_ITEM(refundFee);
//		SEND_ADD_ITEM(returnContent);
		SEND_ADD_ITEM(shopId);		
		SEND_ADD_ITEM(terminalType);
		SEND_ADD_ITEM(timestamp);
		SEND_ADD_ITEM(tradeId);
		SEND_ADD_ITEM(transactionId);
		SEND_ADD_ITEM(userCode);
	}
	*/
	Send_add_end_Sign();
}


void PackData_ConsumeCard(void)
{
	//g_ColData.payType; 
	UpData_timestampAndNoncestr(0);
	SEND_ADD_ITEM(code);
	SEND_ADD_ITEM(developerId);
	SEND_ADD_ITEM(nonceStr);
	SEND_ADD_ITEM(shopId);
	SEND_ADD_ITEM(sn);
	SEND_ADD_ITEM(terminalType);
	SEND_ADD_ITEM(timestamp);
	Send_add_end_Sign();
}


void PackData_shiftStatV2(void)
{
	UpData_timestampAndNoncestr(0);
	SEND_ADD_ITEM(developerId);
	SEND_ADD_ITEM(nonceStr);
	//if(g_ColData.terminalType[0] == '2')
	{
		SEND_ADD_ITEM(sn);
		SEND_ADD_ITEM(terminalType);
		SEND_ADD_ITEM(timestamp);
	}
	/*
	else if(g_ColData.terminalType[0] == '1')
	{
		SEND_ADD_ITEM(shopId);
		SEND_ADD_ITEM(terminalType);
		SEND_ADD_ITEM(timestamp);
		SEND_ADD_ITEM(userCode);
	}
	*/
	Send_add_end_Sign();
}

void PackData_shiftRecordV2(void)
{
	UpData_timestampAndNoncestr(0);
	SEND_ADD_ITEM(count);
	SEND_ADD_ITEM(developerId);
	SEND_ADD_ITEM(nonceStr);
	SEND_ADD_ITEM(nowPage);
	//if(g_ColData.terminalType[0] == '2')
	{
		SEND_ADD_ITEM(sn);
		SEND_ADD_ITEM(terminalType);
		SEND_ADD_ITEM(timestamp);
	}
	/*
	else if(g_ColData.terminalType[0] == '1')
	{
		SEND_ADD_ITEM(shopId);
		SEND_ADD_ITEM(terminalType);
		SEND_ADD_ITEM(timestamp);
		SEND_ADD_ITEM(userCode);
	}
	*/
	Send_add_end_Sign();
}

void PackData_shiftConfirm(void)
{
	UpData_timestampAndNoncestr(0);
	SEND_ADD_ITEM(developerId);
	SEND_ADD_ITEM(nonceStr);
	SEND_ADD_ITEM(recordId);
	//if(g_ColData.terminalType[0] == '2')
	{
		SEND_ADD_ITEM(sn);
		SEND_ADD_ITEM(terminalType);
		SEND_ADD_ITEM(timestamp);
	}
	/*
	else if(g_ColData.terminalType[0] == '1')
	{
		SEND_ADD_ITEM(shopId);
		SEND_ADD_ITEM(terminalType);
		SEND_ADD_ITEM(timestamp);
		SEND_ADD_ITEM(userCode);
	}
	*/
	Send_add_end_Sign();
}


int PE_ShowQRcodeDis(char* pQRcode)
{
	RECTL	Rect;
	Rect.left	= SCREEN_APP_X;
	Rect.top	= SCREEN_APP_Y;
	Rect.width	= SCREEN_APP_W;
	Rect.height = SCREEN_APP_H;
	UI_ShowPictureFile(&Rect,"QRcode.clz");

	Rect.left	= UI_QR_CODE_X;
	Rect.top	= UI_QR_CODE_Y+5;
	Rect.width	= UI_QR_CODE_W;
	Rect.height = UI_QR_CODE_H;
	API_GUI_Draw565QRcode(&Rect,pQRcode,RGB565_BLACK);
	{
		POINT tFontXY;
		u16 width;
		char sShowStr[32];
		API_strcpy(sShowStr,"½ð¶î:£¤");
		Conv_TmoneyToDmoney(sShowStr+API_strlen(sShowStr),g_ColData.payAmount);
		width=API_strlen(sShowStr)*FONT_SIZE/2;
		tFontXY.left =UI_QR_MONEY_X+ (UI_QR_MONEY_W-width)/2;
		tFontXY.top =UI_QR_MONEY_Y;
		UI_SetColorRGB565(RGB565_BLACK,RGB565_PARENT);
		UI_DrawString(&tFontXY,sShowStr);
	}
	API_GUI_Show();
	return 0;
}


void PE_ShowfailMsg(char* pCode)
{
	char codeInfo[32]={0};
	char GbkMsg[256]={0};
	API_sprintf(codeInfo,"Ó¦´ðÂë[%s]",pCode);
	API_Utf8ToGbk(GbkMsg,sizeof(GbkMsg),PE_GetRecvIdPar("errMsg"));
	PE_ShowMsg("ÌáÊ¾",codeInfo,GbkMsg,NULL,10*1000);
 }


int inside_OrderQuery(int PeekLink)
{
	int ret;
	// ×é½¨Êý¾Ý°ü,·¢ËÍÇëÇó
	if(PeekLink == 0)
	{
		ret = Tcp_Link("Á¬½ÓÖÐÐÄ..");
		if(ret) return ret;
	}
	// ×é½¨Êý¾Ý°ü,·¢ËÍÇëÇó
	TradeDataPacked_start(HTTP_TRADE_ADDERR"/open/Pay/orderQuery");
	PackData_OrderQuery();
	TradeDataPacked_end();
	// ·¢ËÍ½ÓÊÕÊý¾Ý
	ret = Tcp_SocketData("Êý¾Ý½»»¥",Full_CheckRecv);
	Tcp_Close(NULL);
	return ret;
}

void inside_MicroPay(int Errtimes)
{
	int ret;
	char *pCode;
	// ×é½¨Êý¾Ý°ü,·¢ËÍÇëÇó
	TradeDataPacked_start(HTTP_TRADE_ADDERR"/open/Pay/microPayV2");
	SweepPackData_V2();
	TradeDataPacked_end();
	// ·¢ËÍ½ÓÊÕÊý¾Ý
	ret = Tcp_SocketData("Êý¾Ý½»»¥",Full_CheckRecv);
	Tcp_Close(NULL);
	if(ret) return;
RE_QUERY_ADDER:
	pCode=PE_GetRecvIdPar("errCode");
	if(pCode==NULL)
	{
		APP_ShowTradeFA("½ÓÊÕÊý¾ÝÒì³£",10000);
		return;
	}
	TRACE("reCode:%s",pCode); 
	if(pCode[0] == '0') //·µ»Ø³É¹¦£¬¿ÉÒÔ½âÎöÏÂÒ»²ã Êý¾Ý¡£
	{
		char *tradeInfo;
		tradeInfo = PE_GetRecvIdPar("tradeInfo");
		if(tradeInfo!= NULL && 0==PE_JsonDataParse(tradeInfo,API_strlen(tradeInfo)))
		{
			char *payStatus;
			payStatus = PE_GetRecvIdPar("payStatus");
			if(payStatus == NULL)
			{
				APP_ShowTradeMsg("ÎÞ×´Ì¬ÐÅÏ¢",10000);
				PE_JsonFree(1);
				return;
			}
			if(API_strcmp(payStatus,"SUCCESS")==0)
			{
				PE_ReadRecvIdPar("incomeAmount",g_ColData.payAmount);
				SuccessTradeProcess();	
			}
			else if(API_strcmp(payStatus,"CLOSE")==0)
			{
				APP_ShowTradeMsg("¶©µ¥ÒÑ¹Ø±Õ",10000);
			}
			else if(API_strcmp(payStatus,"PAYERROR")==0)
			{
				APP_ShowTradeFA("Ö§¸¶Ê§°Ü",10000);
			}
			else if(API_strcmp(payStatus,"NOTPAY")==0)
			{
				APP_ShowTradeMsg("Î´Ö§¸¶",10000);
			}
			else if(API_strcmp(payStatus,"REVOKED")==0)
			{
				APP_ShowTradeMsg("¶©µ¥ÒÑ³·Ïú",10000);
			}
			else if(API_strcmp(payStatus,"USERPAYING")==0)
			{
				PE_ReadRecvIdPar("tradeId",g_ColData.tradeId);
				PE_ReadRecvIdPar("outTradeId",g_ColData.outTradeId);
				PE_JsonFree(1);
				if(EVENT_CANCEL == API_WaitEvent(2*1000,EVENT_UI,EVENT_NONE))
				{
					return;
				}
				APP_SetKeyAccept(0x01);
				PE_JsonFree(1);
				ret = inside_OrderQuery(0);
				if(ret == OPER_RET)
				{
					return;
				}
				else if(ret)
				{
					APP_ShowTradeMsg("¶©µ¥Ö§¸¶ÖÐ,ÉÔºò²éÑ¯È·ÈÏ½á¹û",10000);
					return;
				}
				if(Errtimes-- <= 0)
				{
					APP_ShowTradeMsg("¶©µ¥Ö§¸¶ÖÐ,\n²éÑ¯½á¹û³¬Ê±\nÉÔºòÊÖ¶¯²éÑ¯È·ÈÏ½á¹û",10000);
					return;
				}
				goto RE_QUERY_ADDER; 
				
			}
			PE_JsonFree(1);
		}
		//pSdkFun->net->KeyAccept(0x00);
	}
	else //if(API_strcmp(code,"500"))
	{
		PE_ShowfailMsg(pCode);
	}
}

int MicroPay(char* title)
{    
	do{
		TCP_SetInterNotDisplay(FALSE);
		if(Tcp_PeekLink()) //Tcp_Link(NULL);
			return -2;
		while(1)
		{
			if(0 != InputTotalFee(title)){
				Tcp_Close(NULL);
				return -1;
			}
			APP_TTS_PlayText("ÇëÊ¾¶þÎ¬Âë");
			if(0 != InputAuthcodeByCamScan())
			{
				continue;
			}
			break;
		}
		APP_ShowWaitFor(NULL);//STR_NET_LINK_WLAN
		inside_MicroPay(10);
	}while(0);
    DataFree();
	Tcp_Close(NULL);
	return 0;	
}


int FixedMicroPay(char* title)
{    
	int ret=0;
	do{
		TCP_SetInterNotDisplay(FALSE);
		if(Tcp_PeekLink()) //Tcp_Link(NULL);
			return -2;
		if(0>APP_EditSum(title,'S',g_ColData.FixedAmount,60*1000))
		{
			Tcp_Close(NULL);
			return -1;
		}
		API_strcpy(g_ColData.payAmount,g_ColData.FixedAmount);
		APP_TTS_PlayText("ÇëÊ¾¶þÎ¬Âë");
		CLEAR(g_ColData.authCode);
		ret = APP_CamScan('S',g_ColData.payAmount,g_ColData.authCode,10,sizeof(g_ColData.authCode)-1,20*1000);
		if(ret < 0) 
		{
			Tcp_Close(NULL);
			return ret;
		}
		APP_ShowWaitFor(NULL);//STR_NET_LINK_WLAN
		inside_MicroPay(10);
	}while(0);
    DataFree();
	Tcp_Close(NULL);
	return 0;	
}


int OrderQuery(char* pTitle)
{
	char* pCode;
	int ret;
	do{
		TCP_SetInterNotDisplay(FALSE);
		if(Tcp_PeekLink()) //Tcp_Link(NULL);
			return -2;
		
		g_ColData.tradeId[0]='\0';
		g_ColData.outTradeId[0]='\0';	
		g_ColData.transactionId[0]='\0';
		ret=APP_CamScan('Q',NULL,g_ColData.transactionId,8,32,30*1000);
		if(ret<0) break;
		if(ret==OPER_HARD)
		{
		    if(APP_ScanInput(pTitle,"ÇëÊÖ¶¯ÊäÈëÉÌ»§µ¥ºÅ",NULL,g_ColData.transactionId,10,32)) 
				break;
		}
		APP_ShowWaitFor(NULL);//STR_NET_LINK_WLAN
		g_ColData.tradeId[0]=0;
		g_ColData.outTradeId[0]=0;
		if(inside_OrderQuery(1))
			break;
		pCode=PE_GetRecvIdPar("errCode");
		if(pCode==NULL)
		{
			APP_ShowTradeMsg("½ÓÊÕÊý¾ÝÒì³£",10000);
			break;
		}
	    TRACE("reCode:%s",pCode); 
	    if(pCode[0] == '0') 
	    {
			char *tradeInfo;
			tradeInfo = PE_GetRecvIdPar("tradeInfo");
			if(tradeInfo != NULL && 0==PE_JsonDataParse(tradeInfo,API_strlen(tradeInfo)))
			{
				char *pPar,*pShow;
				char showBuff[1024];
				pShow=showBuff;
				/*
				pPar=PE_GetRecvIdPar("shopId");
				if(pPar)
				{
					pShow += sprintf(pShow,"ÃÅµê:%s  ",pPar);
				}
				pPar=PE_GetRecvIdPar("merchantUserId");
				if(pPar)
				{
					pShow += sprintf(pShow,"ÊÕÒøÔ±:%s\n",pPar);
				}
				*/
				pPar=PE_GetRecvIdPar("payStatus");
				if(pPar)
				{//0±íÊ¾Î´ÍË¿î£¬1±íÊ¾×ªÈëÍË¿î
					pShow = eStrcpy(pShow,"¶©µ¥×´Ì¬:");
					if(strstr(pPar,"SUCCESS"))
					{//SUCCESS£º
						pShow = eStrcpy(pShow,"Ö§¸¶³É¹¦");
					}
					else if(strstr(pPar,"NOTPAY"))
					{//NOTPAY£ºÎ´Ö§¸¶£
						pShow = eStrcpy(pShow,"Î´Ö§¸¶");
					}
					else if(strstr(pPar,"CLOSE"))
					{//CLOSE£ºÒÑ¹Ø±Õ
						pShow = eStrcpy(pShow,"ÒÑ¹Ø±Õ");
					}
					else if(strstr(pPar,"REVOKED"))
					{//REVOKED£ºÒÑ³·Ïú
						pShow = eStrcpy(pShow,"ÒÑ³·Ïú");
					}
					else if(strstr(pPar,"USERPAYING"))
					{//USERPAYING£ºÓÃ»§Ö§¸¶ÖÐ
						pShow = eStrcpy(pShow,"ÓÃ»§Ö§¸¶ÖÐ");
					}
					else if(strstr(pPar,"PAYERROR"))
					{//PAYERROR£ºÖ§¸¶Ê§°Ü
						pShow = eStrcpy(pShow,"Ö§¸¶Ê§°Ü");
					}
					else if(strstr(pPar,"REFUND"))
					{//REFUND£º
						pShow = eStrcpy(pShow,"×ªÈëÍË¿î");
					}
					else if(strstr(pPar,"PERATE_FAIL"))
					{//PERATE_FAIL£ºÔ¤ÊÚÈ¨ÇëÇó²Ù×÷Ê§°Ü£»
						pShow = eStrcpy(pShow,"Ô¤ÊÚÈ¨ÇëÇó²Ù×÷Ê§°Ü");
					}
					else if(strstr(pPar,"OPERATE_SETTLING"))
					{//OPERATE_SETTLING£ºÑº½ðÏû·ÑÒÑÊÜÀí£»
						pShow = eStrcpy(pShow,"Ñº½ðÏû·ÑÒÑÊÜÀí");
					}
					else if(strstr(pPar,"REVOKED_SUCCESS"))
					{//REVOKED_SUCCESS£ºÔ¤ÊÚÈ¨ÇëÇó³·Ïú³É¹¦
						pShow = eStrcpy(pShow,"Ô¤ÊÚÈ¨ÇëÇó³·Ïú³É¹¦");
					}
					*pShow++ = '\n';
				}
				pPar=PE_GetRecvIdPar("orderAmount");
				if(pPar)
				{
					char DisplayMoney[16];
					Conv_TmoneyToDmoney(DisplayMoney,pPar);
					pShow += sprintf(pShow,"¶©µ¥×Ü¶î:%s\n",DisplayMoney);
				}
				pPar=PE_GetRecvIdPar("incomeAmount");
				if(pPar)
				{
					char DisplayMoney[16];
					Conv_TmoneyToDmoney(DisplayMoney,pPar);
					pShow += sprintf(pShow,"ÊµÊÕ½ð¶î:%s\n",DisplayMoney);
				}
				pPar=PE_GetRecvIdPar("refundAmount");
				if(pPar)
				{
					char DisplayMoney[16];
					Conv_TmoneyToDmoney(DisplayMoney,pPar);
					pShow += sprintf(pShow,"ÒÑÍË½ð¶î:%s\n",DisplayMoney);
				}
				pPar=PE_GetRecvIdPar("payTime");
				if(pPar)
				{
					pShow += sprintf(pShow,"Ö§¸¶Ê±¼ä:\n%s\n",pPar);
				}
				
				pPar=PE_GetRecvIdPar("id");
				if(pPar)
				{
					pShow += sprintf(pShow,"Ì«Ã×Á÷Ë®:%s\n",pPar);
				}
				pPar=PE_GetRecvIdPar("outTradeId");
				if(pPar)
				{
					pShow += sprintf(pShow,"Íâ²¿½»Ò×ºÅ:%s\n",pPar);
				}
				
				pPar=PE_GetRecvIdPar("transactionId");
				if(pPar)
				{
					pShow += sprintf(pShow,"½»Ò×ºÅ:%s\n",pPar);
				}
				pPar=PE_GetRecvIdPar("orderNum");
				if(pPar)
				{
					pShow += sprintf(pShow,"ÄÚ²¿½»Ò×ºÅ:%s\n",pPar);
				}
				pPar=PE_GetRecvIdPar("module");
				if(pPar)
				{
					pShow = eStrcpy(pShow,"Ä£¿é:");
					if(strstr(pPar,"pay"))
					{//pay£ºÊÕÒø£»
						pShow = eStrcpy(pShow,"ÊÕÒø");
					}
					else if(strstr(pPar,"mall"))
					{//mall£ºÓÅÑ¡¿¨È¯»õ¼Ü£»
						pShow = eStrcpy(pShow,"ÓÅÑ¡¿¨È¯»õ¼Ü");
					}
					else if(strstr(pPar,"recharge"))
					{//recharge£º³äÖµ£»
						pShow = eStrcpy(pShow,"³äÖµ");
					}
					else if(strstr(pPar,"become_member"))
					{//become_member£º»áÔ±¹ºÂò£»
						pShow = eStrcpy(pShow,"»áÔ±¹ºÂò");
					}
					else if(strstr(pPar,"eatIn"))
					{//eatIn£ºµêÄÚÏÂµ¥£»
						pShow = eStrcpy(pShow,"µêÄÚÏÂµ¥");
					}
					else if(strstr(pPar,"takeOut"))
					{//takeOut£ºÍâËÍ¶©µ¥£»
						pShow = eStrcpy(pShow,"ÍâËÍ¶©µ¥");
					}
					else if(strstr(pPar,"selfTake"))
					{//selfTake£ºÔ¤Ô¼×ÔÈ¡£»
						pShow = eStrcpy(pShow,"Ô¤Ô¼×ÔÈ¡");
					}
					else if(strstr(pPar,"payment_card"))
					{//payment_card£º¸¶·Ñ¿¨È¯£»
						pShow = eStrcpy(pShow,"¸¶·Ñ¿¨È¯");
					}
					else if(strstr(pPar,"times_card"))
					{//times_card£º´Î/ÔÂ¿¨
						pShow = eStrcpy(pShow,"´Î/ÔÂ¿¨");
					}
					else
					{//pShow += sprintf(pShow,"module:%s\n",pPar);
						pShow = eStrcpy(pShow,pPar);
					}
					*pShow++ = '\n';
				}
				/*
				pPar=PE_GetRecvIdPar("fromType");
				if(pPar)
				{
					pShow = eStrcpy(pShow,"¶©µ¥À´Ô´:");
					if(strstr(pPar,"wx"))
					{//wx£ºÎ¢ÐÅÊÕ¿îÂë£»
						pShow = eStrcpy(pShow,"Ö§¸¶³É¹¦");
					}
					else if(strstr(pPar,"alipay"))
					{//alipay£ºÖ§¸¶±¦ÊÕ¿îÂë
						pShow = eStrcpy(pShow,"Î´Ö§¸¶");
					}
					else if(strstr(pPar,"web"))
					{//web£ºWebÒ³Ãæ
						pShow = eStrcpy(pShow,"Î´Ö§¸¶");
					}
					else if(strstr(pPar,"alipay"))
					{//mini£ºÐ¡³ÌÐò£»
						pShow = eStrcpy(pShow,"Î´Ö§¸¶");
					}
				}
				*/
				if(pShow > showBuff)
				{
					*(--pShow)='\0';	//×îºóÒ»ÐÐ²»ÓÃ»»ÐÐ
					APP_ShowInfo(pTitle,showBuff,30*1000);
				}
				PE_JsonFree(1);
			}
		}
		else 
		{
			PE_ShowfailMsg(pCode);
	 	} 
	}while(0);
    DataFree();
	Tcp_Close(NULL);
	return 0;	
}
int RefundFlow(char* pTitle)
{
	int ret=1;
	char* pCode;
	do{
		TCP_SetInterNotDisplay(FALSE);
		if(Tcp_PeekLink()) //Tcp_Link(NULL);
			return -2;
		//if(APP_InputPin(title,"ÇëÊäÈë¹ÜÀíÔ±ÃÜÂë:","Çë°´Êý×Ö¼üÊäÈë", "888888")) return -1;
		g_ColData.tradeId[0]='\0';
		g_ColData.outTradeId[0]='\0';	
		g_ColData.transactionId[0]='\0';
		ret=APP_CamScan('R',NULL,g_ColData.transactionId,10,32,20*1000);
		if(ret<0) break;
		if(ret == OPER_HARD)
		{
		    if(APP_ScanInput(pTitle,"ÇëÊÖ¶¯ÊäÈëÉÌ»§µ¥ºÅ",NULL,g_ColData.transactionId,10,32)) 
				break;
		}
		APP_ShowWaitFor(NULL);//STR_NET_LINK_WLAN
		if(inside_OrderQuery(1))
			break;
		pCode=PE_GetRecvIdPar("errCode");
		if(pCode==NULL)
		{
			APP_ShowTradeMsg("½ÓÊÕÊý¾ÝÒì³£",10000);
			break;
		}
		if(pCode[0] == '0') 
		{
			char *tradeInfo = PE_GetRecvIdPar("tradeInfo");
			if(tradeInfo!=NULL && 0==PE_JsonDataParse(tradeInfo,API_strlen(tradeInfo)))
			{
				char *pPar;
				u32 income=0,refund;
				pPar=PE_GetRecvIdPar("id");
				if(pPar)
					API_strcpy(g_ColData.tradeId,pPar);
			
				pPar=PE_GetRecvIdPar("incomeAmount");
				if(pPar)
					income = API_atoi(pPar);
				pPar=PE_GetRecvIdPar("refundAmount");
				if(pPar)
				{
					refund=API_atoi(pPar);
					if(income < refund)
						income = 0;
					else income -= refund;
				}
				/*
				if(income == 0)
				{
					APP_ShowTradeMsg("ÎÞ¿ÉÍË½ð¶î",10*1000);
                       break;
				}
				*/
				sprintf(g_ColData.refundFee,"%d",income);
				PE_JsonFree(1);
			}
			else
			{
				APP_ShowTradeMsg("²éÎÞ´Ë¶©µ¥ÐÅÏ¢",10*1000);
                break;
			}
		}
		else 
		{
			PE_ShowfailMsg(pCode);
			break;
		} 
		PE_JsonFree(1);
		
		if(0>APP_EditSum(pTitle,'R',g_ColData.refundFee,60*1000))
		{
			break;
		}
		if(Tcp_Link("Á¬½ÓÖÐÐÄ..")) break;
		// ×é½¨Êý¾Ý°ü,·¢ËÍÇëÇó
		TradeDataPacked_start(HTTP_TRADE_ADDERR"/open/Pay/refund");
		PackData_refund();
		TradeDataPacked_end();
		// ·¢ËÍ½ÓÊÕÊý¾Ý
		ret = Tcp_SocketData("Êý¾Ý½»»¥",Full_CheckRecv);
		Tcp_Close(NULL);
		if(ret) break;
		
		pCode=PE_GetRecvIdPar("errCode");
		if(pCode==NULL)
		{
			APP_ShowTradeMsg("½ÓÊÕÊý¾ÝÒì³£",10000);
			break;
		}
		if(pCode[0] == '0') 
		{
			APP_ShowRefundsOK(g_ColData.refundFee);
			APP_TTS_PlayText("ÍË¿î³É¹¦");
			APP_WaitUiEvent(10*1000);
		}
		else //if(API_strcmp(code,"500"))
		{
			PE_ShowfailMsg(pCode);
		}
	}while(0);
	DataFree();
	APP_Network_Disconnect(50);
	return 0;		
}

int ConsumeCard(char* pTitle)
{
	char* pCode;
	int ret;
	do{
		TCP_SetInterNotDisplay(FALSE);
		if(Tcp_PeekLink()) //Tcp_Link(NULL);
			return -2;
		APP_ShowSta(pTitle,"ÇëÉ¨ÓÅ»ÝÈ¯ºËÏúÂë");
		g_ColData.code[0]='\0';
		ret=APP_OnlyCamScan(0x02,4,sizeof(g_ColData.code)-1,g_ColData.code,30*1000);
		if(ret<0) break;
		if(ret==OPER_HARD)
		{
			if(APP_ScanInput(pTitle,"ÇëÊäÈëºËÏúÂë",NULL,g_ColData.code,4,32)) 
				break;
		}
		APP_ShowWaitFor(NULL);//STR_NET_LINK_WLAN
		TCP_SetInterNotDisplay(FALSE);
		// ×é½¨Êý¾Ý°ü,·¢ËÍÇëÇó
		//if(Tcp_Link("Á¬½ÓÖÐÐÄ..")) break;
		// ×é½¨Êý¾Ý°ü,·¢ËÍÇëÇó
		TradeDataPacked_start(HTTP_TRADE_ADDERR"/open/Card/consumeCard");
		PackData_ConsumeCard();
		TradeDataPacked_end();
		// ·¢ËÍ½ÓÊÕÊý¾Ý
		ret = Tcp_SocketData("Êý¾Ý½»»¥",Full_CheckRecv);
		Tcp_Close(NULL);
		if(ret) break;
		pCode=PE_GetRecvIdPar("errCode");
		if(pCode==NULL)
		{
			APP_ShowTradeMsg("½ÓÊÕÊý¾ÝÒì³£",10000);
			break;
		}
		TRACE("reCode:%s",pCode); 
		if(pCode[0] == '0') 
		{
			APP_ShowTrade("TradeOK.clz",NULL,"ºËÏú³É¹¦");
			//APP_ShowTradeMsg("\n   ºËÏú³É¹¦",0);
			APP_TTS_PlayText("ºËÏú³É¹¦");
			APP_WaitUiEvent(10000);
			break;
		}
		else 
		{
			PE_ShowfailMsg(pCode);
		} 
	}while(0);
	DataFree();
	Tcp_Close(NULL);
	return 0;	
}

int shiftStatV2(char* pTitle)
{
	int ret;
	char* pCode;
	do{
		// ×é½¨Êý¾Ý°ü,·¢ËÍÇëÇó
		TCP_SetInterNotDisplay(FALSE);
		if(Tcp_Link("Á¬½ÓÖÐÐÄ..")) break;
		// ×é½¨Êý¾Ý°ü,·¢ËÍÇëÇó
		TradeDataPacked_start(HTTP_TRADE_ADDERR"/open/Staff/shiftStatV2");
		PackData_shiftStatV2();
		TradeDataPacked_end();
		// ·¢ËÍ½ÓÊÕÊý¾Ý
		ret = Tcp_SocketData("Êý¾Ý½»»¥",Full_CheckRecv);
		Tcp_Close(NULL);
		if(ret) break;
		pCode=PE_GetRecvIdPar("errCode");
		if(pCode==NULL)
		{
			APP_ShowTradeMsg("½ÓÊÕÊý¾ÝÒì³£",10000);
			break;
		}
		if(pCode[0] == '0') 
		{
			char showbuff[1024],*pshow;
			char IndexBuff[63],type;
			char *pPar,*stat;
			PE_ReadRecvIdPar("recordId",g_ColData.recordId); 
			showbuff[0] = '\0';
			pshow = showbuff;
			pPar=PE_GetRecvIdPar("staff");
			if(pPar)
			{
				API_Utf8ToGbk(IndexBuff,sizeof(IndexBuff),pPar);
				pshow += sprintf(pshow,"ÊÕÒøÔ±:%s\n",IndexBuff);
			}
			pPar=PE_GetRecvIdPar("startTime");
			if(pPar)
			{
				pshow += sprintf(pshow,"ÉÏ°àÊ±¼ä:\n%s\n",pPar);
			}
			pPar=PE_GetRecvIdPar("endTime");
			if(pPar)
			{
				pshow += sprintf(pshow,"ÏÂ°àÊ±¼ä:\n%s\n",pPar);
			}
			stat=PE_GetRecvIdPar("stat");
			if((stat!= NULL) && ((ret=API_strlen(stat))>8))
			{	
				dfJsonTable *pModule;
				dfJsonTable *pJsonData,*pStartJson = Conv_JSON_GetMsg(stat,stat+ret);
				pJsonData = pStartJson;
				while(pJsonData)
				{
					pPar=Conv_GetJsonValue(pJsonData,"title",NULL);
					if(pPar)
					{
						API_Utf8ToGbk(IndexBuff,sizeof(IndexBuff),pPar);
						pshow += sprintf(pshow,"=======%s=======\n",IndexBuff);
					}
					type = 0;
					pModule=(dfJsonTable *)Conv_GetJsonValue(pJsonData,"module",(u8*)&type);
					if(type >= ITEM_STRUCT)
					{
						while(pModule)
						{
							pPar=Conv_GetJsonValue(pModule,"payType",NULL);
							if(pPar)
							{
								API_Utf8ToGbk(IndexBuff,sizeof(IndexBuff),pPar);
								pshow=eStrcpy(pshow,IndexBuff);
								*pshow++ =':';
								*pshow++ =' ';
							}
							pPar=Conv_GetJsonValue(pModule,"count",NULL);
							if(pPar != NULL && API_strlen(pPar)>0)
							{
								pshow += sprintf(pshow,"%s±Ê ",pPar);
							}
							pPar=Conv_GetJsonValue(pModule,"amount",NULL);
							if(pPar)
							{
								pshow += sprintf(pshow,"%sÔª",pPar);
							}
							*pshow++ = '\n';
							pModule = pModule->pNext;
						}
					}
					pPar=Conv_GetJsonValue(pJsonData,"totalCount",NULL);
					if(pPar != NULL && API_strlen(pPar)>0)
					{
						pshow += sprintf(pshow,"×Ü¼Æ:%s±Ê  ",pPar);
					}
					pPar=Conv_GetJsonValue(pJsonData,"totalAmount",NULL);
					if(pPar)
					{
						pshow += sprintf(pshow,"ºÏ¼Æ:%sÔª",pPar);
					}
					*pshow++ ='\n';
					pJsonData = pJsonData->pNext;
				}
				Conv_JSON_free(pStartJson);
			}
			if(pshow > showbuff)
			{
				*(--pshow)='\0';	//×îºóÒ»ÐÐ²»ÓÃ»»ÐÐ
				APP_ShowInfo(pTitle,showbuff,30*1000);
			}
		}
		else 
		{
			PE_ShowfailMsg(pCode);
		} 
	}while(0);
	DataFree();
	return 0;	
}


int shiftRecordV2(char* pTitle)
{
	char* pCode;
	int ret;
	u16 recordMax;
	u16 count,nowPage;
	char *showbuff=NULL;
	// ×é½¨Êý¾Ý°ü,·¢ËÍÇëÇó
	TCP_SetInterNotDisplay(FALSE);
	nowPage=1;
	count = 1;
	do{
	RETURN_DISPLAY_PAGE:
		if(Tcp_Link("Á¬½ÓÖÐÐÄ..")) break;
		// ×é½¨Êý¾Ý°ü,·¢ËÍÇëÇó
		TradeDataPacked_start(HTTP_TRADE_ADDERR"/open/Staff/shiftRecordV2");
		sprintf(g_ColData.nowPage,"%d",nowPage);
		sprintf(g_ColData.count,"%d",count);
		PackData_shiftRecordV2();
		TradeDataPacked_end();
		// ·¢ËÍ½ÓÊÕÊý¾Ý
		ret=Tcp_SocketData("Êý¾Ý½»»¥",Full_CheckRecv);
		Tcp_Close(NULL);
		if(ret) break;
		pCode=PE_GetRecvIdPar("errCode");
		if(pCode==NULL)
		{
			APP_ShowTradeMsg("½ÓÊÕÊý¾ÝÒì³£",10000);
			break;
		}
		TRACE("reCode:%s",pCode); 
		if(pCode[0] == '0')
		{
			char *recordCount=PE_GetRecvIdPar("recordCount");
			if(recordCount!=NULL && (recordMax=API_atoi(recordCount))>0)
			{
				char *recordList=PE_GetRecvIdPar("recordList");
				if((recordList!= NULL) && ((ret=API_strlen(recordList))>8))
				{	
					u8 type;
					char IndexBuff[64];
					char *pshow,*pPar;
					dfJsonTable *pStat,*pModule;
					dfJsonTable *pJsonData,*pStartJson;
					if(showbuff) free(showbuff);
					showbuff=(char*)malloc(ret+512);
					if(showbuff == NULL)
					{
						APP_ShowTradeMsg("ÉêÇëÄÚ´æÊ§°Ü",5000);
						break;
					}
					pStartJson = Conv_JSON_GetMsg(recordList,recordList+ret);
					pJsonData = pStartJson;
					showbuff[0] = '\0';
					pshow = showbuff;
					while(pJsonData)
					{
						pPar=Conv_GetJsonValue(pJsonData,"staff",NULL);
					//	if(g_ColData.terminalType[0] == '2')
					//		pshow += sprintf(pshow,"ÖÕ¶Ë:%s\n",pPar);
					//	else if(g_ColData.terminalType[0] == '1')
						if(pPar)
						{
							API_Utf8ToGbk(IndexBuff,sizeof(IndexBuff),pPar);
							pshow += sprintf(pshow,"ÊÕÒøÔ±:%s\n",IndexBuff);
						}
					//	pPar=Conv_GetJsonValue(pJsonData,"recordId",NULL);
					//	pshow += sprintf(pshow,"½»°à¼ÇÂ¼id:%s\n",pPar);
						pPar=Conv_GetJsonValue(pJsonData,"startTime",NULL);
						pshow += sprintf(pshow,"ÉÏ°àÊ±¼ä:\n%s\n",pPar);
						pPar=Conv_GetJsonValue(pJsonData,"endTime",NULL);
						pshow += sprintf(pshow,"ÏÂ°àÊ±¼ä:\n%s\n",pPar);
						
						type = 0;
						pStat=(dfJsonTable *)Conv_GetJsonValue(pJsonData,"stat",&type);
						if(type >= ITEM_STRUCT)
						{
							while(pStat)
							{
								pPar=Conv_GetJsonValue(pStat,"title",NULL);
								if(pPar)
								{
									API_Utf8ToGbk(IndexBuff,sizeof(IndexBuff),pPar);
									pshow += sprintf(pshow,"=======%s=======\n",IndexBuff);
								}
								type = 0;
								pModule=(dfJsonTable *)Conv_GetJsonValue(pStat,"module",&type);
								if(type >= ITEM_STRUCT)
								{
									while(pModule)
									{
										pPar=Conv_GetJsonValue(pModule,"payType",NULL);
										if(pPar)
										{
											API_Utf8ToGbk(IndexBuff,sizeof(IndexBuff),pPar);
											pshow=eStrcpy(pshow,IndexBuff);
											*pshow++ =':';
											*pshow++ =' ';
										}
										pPar=Conv_GetJsonValue(pModule,"count",NULL);
										if(pPar!=NULL && API_strlen(pPar)>0)
										{
											pshow += sprintf(pshow,"%s±Ê ",pPar);
										}
										pPar=Conv_GetJsonValue(pModule,"amount",NULL);
										if(pPar)
										{
											pshow += sprintf(pshow,"%sÔª",pPar);
										}
										*pshow++ = '\n';
										pModule = pModule->pNext;
									}
								}
								pPar=Conv_GetJsonValue(pStat,"totalCount",NULL);
								if(pPar!=NULL && API_strlen(pPar)>0)
								{
									pshow += sprintf(pshow,"×Ü¼Æ:%s±Ê  ",pPar);
								}
								pPar=Conv_GetJsonValue(pStat,"totalAmount",NULL);
								if(pPar)
								{
									pshow += sprintf(pshow,"ºÏ¼Æ:%sÔª",pPar);
								}
								*pshow++ ='\n';
								pStat = pStat->pNext;
								//*pshow++ = '\n';
							}
						}
						//pshow=eStrcpy(pshow,"======================\n");
						pJsonData = pJsonData->pNext;
					}
					Conv_JSON_free(pStartJson);
					PE_JsonFree(1);
					if(pshow > showbuff)
					{
						int evet;
						*(--pshow)='\0';	//×îºóÒ»ÐÐ²»ÓÃ»»ÐÐ
						if((nowPage*count) < recordMax)
							sprintf(IndexBuff,"µ±Ç°%d×Ü%d,È·ÈÏÏòÏÂ",nowPage,(recordMax+count-1)/count);
						else
							sprintf(IndexBuff,"µ±Ç°%d×Ü%d,È·ÈÏ½áÊø",nowPage,(recordMax+count-1)/count);
						evet=APP_ShowInfoMsg(pTitle,showbuff,IndexBuff,30*1000);
						free(showbuff); showbuff=NULL;
						if(evet & EVENT_OK)
						{
							if((nowPage*count) < recordMax)
							{
								nowPage ++;
								goto RETURN_DISPLAY_PAGE;
							}
						}
						//if(evet & EVENT_CANCEL) break;
						//if(evet & EVENT_TIMEOUT) break;
					}
					else
					{
						free(showbuff); showbuff=NULL;
					}
				}
			}
			else
			{
				APP_ShowTradeMsg("ÎÞ½»Ò×¼ÇÂ¼",5000);
			}
		}
		else 
		{
			PE_ShowfailMsg(pCode);
		} 
	}while(0);
	DataFree();
	return 0;	
}


int shiftConfirm(char* pTitle)
{
	int ret;
	char* pCode;
	if(API_strlen(g_ColData.recordId) == 0)
	{
		APP_ShowTradeFA("ÇëÏÈ×ö½»°àÍ³¼Æ",3000);
		return 0;
	}
	// ×é½¨Êý¾Ý°ü,·¢ËÍÇëÇó
	TCP_SetInterNotDisplay(FALSE);
	do{
		if(Tcp_Link("Á¬½ÓÖÐÐÄ..")) break;
		// ×é½¨Êý¾Ý°ü,·¢ËÍÇëÇó
		TradeDataPacked_start(HTTP_TRADE_ADDERR"/open/Staff/shift");
		PackData_shiftConfirm();
		TradeDataPacked_end();
		// ·¢ËÍ½ÓÊÕÊý¾Ý
		ret=Tcp_SocketData("Êý¾Ý½»»¥",Full_CheckRecv);
		Tcp_Close(NULL);
		if(ret) break;
		pCode=PE_GetRecvIdPar("errCode");
		if(pCode==NULL)
		{
			APP_ShowTradeMsg("½ÓÊÕÊý¾ÝÒì³£",10000);
			break;
		}
		TRACE("reCode:%s",pCode); 
		if(pCode[0] == '0') 
		{
			//APP_ShowTradeOK("½»°à³É¹¦");
			APP_ShowTrade("TradeOK.clz",NULL,"½»°à³É¹¦");
			g_ColData.recordId[0]='\0';
			APP_WaitUiEvent(5*1000);
		}
		else 
		{
			PE_ShowfailMsg(pCode);
		} 
	}while(0);
	DataFree();
	return 0;	
}


int ShiftMenu(char* pTitle)
{
	CMenuItemStru MenuStruPar[]=
	{
		"½»°àÍ³¼Æ",		shiftStatV2,
		"È·ÈÏ½»°à",		shiftConfirm,
		"²é¿´½»°à¼ÇÂ¼",	shiftRecordV2,
	};
	APP_CreateNewMenuByStruct(pTitle,sizeof(MenuStruPar)/sizeof(CMenuItemStru),MenuStruPar,30*1000);
//	APP_AddCurrentMenuOtherFun(MENU_BACK_MAP,NULL,"shift.clz");
	return 0;
}


