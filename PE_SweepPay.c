#include "communal.h"


DfFlow_Link  pFlow={0};
QR_COL_Data  g_ColData;

//static pDataTable  pItemdata;

int CheckMchidIsNull()
{
	{	
	    return 0;
	}
	//APP_ShowMsg("提示","请先导入商户信息",3000); 
	//return 1;
}
void DataInit()
{
	if(PE_SendBuf==NULL)
	{
		PE_SendBuf=malloc(BufSize);
	}
	PE_pSend = PE_SendBuf;	
	if(pFlow.pMd5Start==NULL)
	{
		pFlow.pMd5Start=malloc(BufSize);
	}
	PE_pMD5 = pFlow.pMd5Start;
}
void DataFree(void)
{
	PE_JsonFree();
}

int Full_CheckRecv(char *pRecv,int ReadLen)
{
	if(ReadLen > 0)
	{
		int		cycle;
		char	*pStrStart,*pEnd;
		pStrStart = pRecv;
		pEnd = pStrStart+ReadLen;
		//------------找到起点-----------------------
		while(pStrStart < pEnd)
		{
			if(*pStrStart++ == '{') break;
		}
		if(pStrStart >= pEnd) return 0;
		//------------找到终点-----------------------
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
	char sPayType[12],*payType;
	APP_ShowTradeOK(g_ColData.payAmount); 
	payType=PE_GetRecvIdPar("payType");
	/*
	wx_pay：微信支付；
	ali_pay：支付宝；
	union_offline：银行卡；
	union_qrcode：银联扫码；
	union_online：银联钱包；
	member_wallet：会员钱包；
	cash：现金
	*/
	if(API_strcmp(payType,"wx_pay")==0)
	{
		API_strcpy(sPayType,"微信收款");
	}
	else if(API_strcmp(payType,"ali_pay")==0)
	{
		API_strcpy(sPayType,"支付宝收款");
	}
	else //union_offline：银行卡；union_qrcode：银联扫码；union_online：银联钱包
	{
		API_strcpy(sPayType,"收款成功");
	}	
	Conv_TmoneyToDmoney(DisplayMoney,g_ColData.payAmount);
    APP_TTS_PlayText("%s%s元",sPayType,DisplayMoney);
	//-------更新本地时间----------
	SetSysDateTime(PE_GetRecvIdPar("payTime"));
	return APP_WaitUiEvent(10*1000);
}

void Send_add_item(char* pID,char* pData)
{
	if(API_strlen(pData))
	{
		PE_pSend=Conv_SetPackageJson(PE_pSend,pID,pData);
		*PE_pSend++= ',';	//连接下一条
		PE_pMD5=Conv_SetPackageHttpGET(PE_pMD5,pID,pData);
		*PE_pMD5++= '&';	//连接下一条
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



// 第1条的附属信息  也是跟前置约定的类型，类型不通链接不同
void TradeDataPacked_start(char *pTradeAddress)
{  
	DataInit();
	PE_sLenStart=Conv_Post_SetHead(pTradeAddress,"application/json",PE_pSend);
	//API_sprintf(pSendLenAddr[7], "% 3d\r\n\r\n",API_strlen(param)) 预留7个字节空间
	PE_pSend=PE_sLenStart+7;
	*PE_pSend++ ='{';
}

void TradeDataPacked_end(void)
{	
	char sendLen[8];
	*PE_pSend++ = '}';
	//----------------把3个字节固定长度写入buff区---------------------
	API_sprintf(sendLen,"%3d\r\n\r\n",PE_pSend-PE_sLenStart-7); //预留7个字节空间
	API_memcpy(PE_sLenStart,sendLen,7);
	*PE_pSend++ = '0';
	*PE_pSend = '\0';
	pFlow.pMsgLen=PE_pSend-PE_SendBuf;
	TRACE("->PE_SendBuf:\r\n%s\r\n", PE_SendBuf);
}

void UpData_timestampAndNoncestr(int GenId)
{
	u32 timeStamp;
	ST_TIME tTime;
	OsGetTime(&tTime);
	Conv_DateToTimestamp(&tTime,8,&timeStamp);
	sprintf(g_ColData.timestamp,"%d",timeStamp);
	memcpy(g_ColData.nonceStr,g_ColData.sn+2,8);
	sprintf(g_ColData.nonceStr+8,"%s",g_ColData.timestamp);
	if(GenId)
		sprintf(g_ColData.outTradeId,"%s%s",g_ColData.shopId,g_ColData.timestamp);
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
	SEND_ADD_ITEM(shopId);
	SEND_ADD_ITEM(sn);
	SEND_ADD_ITEM(terminalType);
	SEND_ADD_ITEM(timestamp);
	Send_add_end_Sign();
}

/*
//-------------三先一----------------
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

	if(g_ColData.terminalType[0] == '2')
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
	if(g_ColData.terminalType[0] == '2')
	{
		SEND_ADD_ITEM(sn);
		SEND_ADD_ITEM(terminalType);
		SEND_ADD_ITEM(timestamp);
	}
	else if(g_ColData.terminalType[0] == '1')
	{
		SEND_ADD_ITEM(shopId);
		SEND_ADD_ITEM(terminalType);
		SEND_ADD_ITEM(timestamp);
		SEND_ADD_ITEM(userCode);
	}
	Send_add_end_Sign();
}

void PackData_shiftRecordV2(void)
{
	UpData_timestampAndNoncestr(0);
	SEND_ADD_ITEM(count);
	SEND_ADD_ITEM(developerId);
	SEND_ADD_ITEM(nonceStr);
	SEND_ADD_ITEM(nowPage);
	if(g_ColData.terminalType[0] == '2')
	{
		SEND_ADD_ITEM(sn);
		SEND_ADD_ITEM(terminalType);
		SEND_ADD_ITEM(timestamp);
	}
	else if(g_ColData.terminalType[0] == '1')
	{
		SEND_ADD_ITEM(shopId);
		SEND_ADD_ITEM(terminalType);
		SEND_ADD_ITEM(timestamp);
		SEND_ADD_ITEM(userCode);
	}
	Send_add_end_Sign();
}

void PackData_shiftConfirm(void)
{
	UpData_timestampAndNoncestr(0);
	SEND_ADD_ITEM(developerId);
	SEND_ADD_ITEM(nonceStr);
	SEND_ADD_ITEM(recordId);
	if(g_ColData.terminalType[0] == '2')
	{
		SEND_ADD_ITEM(sn);
		SEND_ADD_ITEM(terminalType);
		SEND_ADD_ITEM(timestamp);
	}
	else if(g_ColData.terminalType[0] == '1')
	{
		SEND_ADD_ITEM(shopId);
		SEND_ADD_ITEM(terminalType);
		SEND_ADD_ITEM(timestamp);
		SEND_ADD_ITEM(userCode);
	}
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
		API_strcpy(sShowStr,"金额:￥");
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
	char GbkMsg[128]={0};
	API_sprintf(codeInfo,"应答码[%s]",pCode);
	API_Utf8ToGbk(GbkMsg,sizeof(GbkMsg),PE_GetRecvIdPar("errMsg"));
	PE_ShowMsg("提示",codeInfo,GbkMsg,NULL,10*1000);
 }


int inside_OrderQuery(BOOL notDisplay)
{
	int ret;
	TCP_SetInterNotDisplay(notDisplay);
	// 组建数据包,发送请求
	ret = Tcp_Link("连接中心..");
	if(ret) return ret;
	// 组建数据包,发送请求
	TradeDataPacked_start(HTTP_TRADE_ADDERR"/open/Pay/orderQuery");
	PackData_OrderQuery();
	TradeDataPacked_end();
	// 发送接收数据
	ret = Tcp_SocketData("数据交互",Full_CheckRecv);
	Tcp_Close(NULL);
	return ret;
}


int MicroPay(char* title)
{    
	char *pCode;
	do{
		if(CheckMchidIsNull())return -1;
		if(0 != InputTotalFee(title)){
			return -1;
		}
		APP_TTS_PlayText("请示二维码");
		if(0 != InputAuthcodeByCamScan())
		{
			return -1;
		}
		TCP_SetInterNotDisplay(FALSE);
		if(Tcp_Link("连接中心..")) break;
		// 组建数据包,发送请求
		TradeDataPacked_start(HTTP_TRADE_ADDERR"/open/Pay/microPayV2");
		SweepPackData_V2();
		TradeDataPacked_end();
		// 发送接收数据
		if(Tcp_SocketData("数据交互",Full_CheckRecv)) break;
	RE_QUERY_ADDER:
		pCode=PE_GetRecvIdPar("errCode");
		TRACE("reCode:%s",pCode); 
		if(pCode[0] == '0') //返回成功，可以解析下一层 数据。
		{
			char *tradeInfo;
			tradeInfo = PE_GetRecvIdPar("tradeInfo");
			if(tradeInfo)
			{
				if(!PE_JsonDataParse(tradeInfo,API_strlen(tradeInfo)))
				{
					char *payStatus;
					int ret;
					payStatus = PE_GetRecvIdPar("payStatus");
					if(API_strcmp(payStatus,"SUCCESS")==0)
					{
						PE_ReadRecvIdPar("incomeAmount",g_ColData.payAmount);
						SuccessTradeProcess();	
					}
					else if(API_strcmp(payStatus,"CLOSE")==0)
					{
						APP_ShowTradeMsg("订单已关闭",10000);
					}
					else if(API_strcmp(payStatus,"PAYERROR")==0)
					{
						APP_ShowTradeFA("支付失败",10000);
					}
					else if(API_strcmp(payStatus,"NOTPAY")==0)
					{
						APP_ShowTradeMsg("未支付",10000);
					}
					else if(API_strcmp(payStatus,"REVOKED")==0)
					{
						APP_ShowTradeMsg("订单已撤销",10000);
					}
					else if(API_strcmp(payStatus,"USERPAYING")==0)
					{
						Tcp_Close(NULL);
						PE_ReadRecvIdPar("tradeId",g_ColData.tradeId);
						PE_ReadRecvIdPar("outTradeId",g_ColData.outTradeId);
						PE_JsonFree();
						if(EVENT_CANCEL == API_WaitEvent(2*1000,EVENT_UI,EVENT_NONE))
						{
							break;
						}
						APP_SetKeyAccept(0x01);
						ret = inside_OrderQuery(TRUE);
						if(ret == OPER_RET)
						{
							break;
						}
						else if(ret)
						{
							APP_ShowTradeMsg("订单支付中,稍候查询确认结果",10000);
							break;
						}
						goto RE_QUERY_ADDER; 
						
					}
					PE_JsonFree();
				}
			}
			//pSdkFun->net->KeyAccept(0x00);
			break;
		}
		else //if(API_strcmp(code,"500"))
		{
			PE_ShowfailMsg(pCode);
		 }
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
		if(CheckMchidIsNull())return -1;
		g_ColData.tradeId[0]='\0';
		g_ColData.outTradeId[0]='\0';	
		g_ColData.transactionId[0]='\0';
		ret=APP_CamScan('Q',NULL,g_ColData.transactionId,8,32,30*1000);
		if(ret<0) return -1;
		if(ret==OPER_HARD)
		{
		    if(APP_ScanInput(pTitle,"请手输入订单号",NULL,g_ColData.transactionId,10,32)) 
				return -1;
		}
		g_ColData.tradeId[0]=0;
		g_ColData.outTradeId[0]=0;
		if(inside_OrderQuery(FALSE))
			break;
		pCode=PE_GetRecvIdPar("errCode");
	    TRACE("reCode:%s",pCode); 
	    if(pCode[0] == '0') 
	    {
			char *tradeInfo;
			tradeInfo = PE_GetRecvIdPar("tradeInfo");
			if(tradeInfo)
			{
				if(!PE_JsonDataParse(tradeInfo,API_strlen(tradeInfo)))
				{
					char *pPar,*pShow;
					char showBuff[256];
					pShow=showBuff;
					pPar=PE_GetRecvIdPar("id");
					if(pPar)
					{
						pShow += sprintf(pShow,"太米流水:%s\n",pPar);
					}
					pPar=PE_GetRecvIdPar("outTradeId");
					if(pPar)
					{
						pShow += sprintf(pShow,"第三流水:%s\n",pPar);
					}
					
					pPar=PE_GetRecvIdPar("transactionId");
					if(pPar)
					{
						pShow += sprintf(pShow,"交易流水:%s\n",pPar);
					}
					pPar=PE_GetRecvIdPar("orderNum");
					if(pPar)
					{
						pShow += sprintf(pShow,"商户订单号:%s\n",pPar);
					}
					pPar=PE_GetRecvIdPar("module");
					if(pPar)
					{
						pShow += sprintf(pShow,"module:%s\n",pPar);
					}
					pPar=PE_GetRecvIdPar("shopId");
					if(pPar)
					{
						pShow += sprintf(pShow,"门店号:%s  ",pPar);
					}
					pPar=PE_GetRecvIdPar("merchantUserId");
					if(pPar)
					{
						pShow += sprintf(pShow,"店员号:%s\n",pPar);
					}



					pPar=PE_GetRecvIdPar("orderAmount");
					if(pPar)
					{
						char DisplayMoney[16];
						Conv_TmoneyToDmoney(DisplayMoney,pPar);
						pShow += sprintf(pShow,"订单总额:%s\n",DisplayMoney);
					}
					pPar=PE_GetRecvIdPar("incomeAmount");
					if(pPar)
					{
						char DisplayMoney[16];
						Conv_TmoneyToDmoney(DisplayMoney,pPar);
						pShow += sprintf(pShow,"实收金额:%s\n",DisplayMoney);
					}
					pPar=PE_GetRecvIdPar("payTime");
					if(pPar)
					{
						pShow += sprintf(pShow,"支付时间:\n%s\n",pPar);
					}
					APP_ShowInfo(pTitle,showBuff,30*1000);
					PE_JsonFree();
				}
			}
			//pSdkFun->net->KeyAccept(0x00);
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
int RefundFlow(char* pTitle)
{
	int ret=1;
	char* pCode;
	do{
		if(CheckMchidIsNull())return -1;
		//if(APP_InputPin(title,"请输入管理员密码:","请按数字键输入", "888888")) return -1;
		g_ColData.tradeId[0]='\0';
		g_ColData.outTradeId[0]='\0';	
		g_ColData.transactionId[0]='\0';
		ret=APP_CamScan('R',NULL,g_ColData.transactionId,10,32,20*1000);
		if(ret<0) return -1;
		if(ret == OPER_HARD)
		{
		    if(APP_ScanInput(pTitle,"请手输入订单号",NULL,g_ColData.transactionId,10,32)) 
				return -1;
		}
		if(inside_OrderQuery(FALSE))
			break;
		
		pCode=PE_GetRecvIdPar("errCode");
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
					APP_ShowTradeMsg("无可退金额",10*1000);
                       break;
				}
				*/
				sprintf(g_ColData.refundFee,"%d",income);
				PE_JsonFree();
			}
			else
			{
				APP_ShowTradeMsg("查无此订单信息",10*1000);
                break;
			}
		}
		else 
		{
			PE_ShowfailMsg(pCode);
			break;
		} 
		PE_JsonFree();
		
		if(0>APP_EditSum(pTitle,'R',g_ColData.refundFee,60*1000))
		{
			return -1;
		}
		if(Tcp_Link("连接中心..")) break;
		// 组建数据包,发送请求
		TradeDataPacked_start(HTTP_TRADE_ADDERR"/open/Pay/refund");
		PackData_refund();
		TradeDataPacked_end();
		// 发送接收数据
		if(Tcp_SocketData("数据交互",Full_CheckRecv)) break;
		
		pCode=PE_GetRecvIdPar("errCode");
		if(pCode[0] == '0') 
		{
			APP_ShowRefundsOK(g_ColData.refundFee);
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
		if(CheckMchidIsNull())return -1;
		APP_ShowSta(pTitle,"请扫优惠券核销码");
		ret=APP_OnlyCamScan(0x03,8,sizeof(g_ColData.code)-1,g_ColData.code,30*1000);
		if(ret<0) return -1;
		if(ret==OPER_HARD)
		{
			if(APP_ScanInput(pTitle,"请手输入订单号",NULL,g_ColData.code,10,32)) 
				return -1;
		}
		TCP_SetInterNotDisplay(FALSE);
		// 组建数据包,发送请求
		if(Tcp_Link("连接中心..")) break;
		// 组建数据包,发送请求
		TradeDataPacked_start(HTTP_TRADE_ADDERR"/open/Card/consumeCard");
		PackData_ConsumeCard();
		TradeDataPacked_end();
		// 发送接收数据
		if(Tcp_SocketData("数据交互",Full_CheckRecv)) break;
		pCode=PE_GetRecvIdPar("errCode");
		TRACE("reCode:%s",pCode); 
		if(pCode[0] == '0') 
		{
			APP_ShowTradeOK(NULL);
			APP_WaitUiEvent(10*1000);
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
	char* pCode;
	do{
		if(CheckMchidIsNull())return -1;
		// 组建数据包,发送请求
		TCP_SetInterNotDisplay(FALSE);
		if(Tcp_Link("连接中心..")) break;
		// 组建数据包,发送请求
		TradeDataPacked_start(HTTP_TRADE_ADDERR"/open/Staff/shiftStatV2");
		PackData_shiftStatV2();
		TradeDataPacked_end();
		// 发送接收数据
		if(Tcp_SocketData("数据交互",Full_CheckRecv)) break;
		pCode=PE_GetRecvIdPar("errCode");
		TRACE("reCode:%s",pCode); 
		if(pCode[0] == '0') 
		{
			char *stat=PE_GetRecvIdPar("stat");
			PE_ReadRecvIdPar("recordId",g_ColData.recordId); 
			if((stat!= NULL) && (API_strlen(stat)>8))
			{	
				char showbuff[1024],*pshow;
				char IndexBuff[64],*pPar;
				int i,total;
				dfJsonTable *pJsonData,*pStartJson = Conv_JSON_GetMsg(stat,stat+API_strlen(stat));
				pJsonData = pStartJson;
				showbuff[0] = '\0';
				pshow = showbuff;
				while(pJsonData)
				{
					pPar=Conv_GetJsonValue(pJsonData,"title",NULL);
					API_Utf8ToGbk(IndexBuff,sizeof(IndexBuff),pPar);
					pshow=eStrcpy(pshow,IndexBuff);
					*pshow++ =':';
					*pshow++ ='\n';
					pPar=Conv_GetJsonValue(pJsonData,"totalAmount",NULL);
					sprintf(IndexBuff,"总金额:%s",pPar);
					pshow=eStrcpy(pshow,IndexBuff);
					pshow=eStrcpy(pshow,"元");
					*pshow++ =' ';
					*pshow++ =' ';
					pPar=Conv_GetJsonValue(pJsonData,"totalCount",NULL);
					total=API_atoi(pPar);
					sprintf(IndexBuff,"项数:%s",pPar);
					pshow=eStrcpy(pshow,IndexBuff);
					*pshow++ ='\n';
					for(i=0;i<total;i++)
					{
						sprintf(IndexBuff,"module[%d]/payType",i);
						pPar=Conv_GetJsonValue(pJsonData,IndexBuff,NULL);
						if(pPar == NULL) break;
						API_Utf8ToGbk(IndexBuff,sizeof(IndexBuff),pPar);
						pshow=eStrcpy(pshow,IndexBuff);
						*pshow++ =':';
						*pshow++ =' ';
						sprintf(IndexBuff,"module[%d]/count",i);
						pPar=Conv_GetJsonValue(pJsonData,IndexBuff,NULL);
						sprintf(IndexBuff,"%s次",pPar);
						pshow=eStrcpy(pshow,IndexBuff);
						*pshow++ = ',';
						sprintf(IndexBuff,"module[%d]/amount",i);
						pPar=Conv_GetJsonValue(pJsonData,IndexBuff,NULL);
						sprintf(IndexBuff,"%s元",pPar);
						pshow=eStrcpy(pshow,IndexBuff);
						*pshow++ = '\n';
					}
					pJsonData = pJsonData->pNext;
					*pshow++ = '\n';
				}
				*pshow='\0';
				APP_ShowInfo(pTitle,showbuff,10*1000);
				Conv_JSON_free(pStartJson);
			}
			else
			{
				APP_ShowMsg(pTitle,"无交易记录",5000);
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


int shiftRecordV2(char* pTitle)
{
	char* pCode;
	int recordMax;
	u16 count,nowPage;
	if(CheckMchidIsNull())return -1;
	// 组建数据包,发送请求
	TCP_SetInterNotDisplay(FALSE);
	nowPage=1;
	count = 4;
	do{
	RETURN_DISPLAY_PAGE:
		if(Tcp_Link("连接中心..")) break;
		// 组建数据包,发送请求
		TradeDataPacked_start(HTTP_TRADE_ADDERR"/open/Staff/shiftRecordV2");
		sprintf(g_ColData.nowPage,"%d",nowPage);
		sprintf(g_ColData.count,"%d",count);
		PackData_shiftRecordV2();
		TradeDataPacked_end();
		// 发送接收数据
		if(Tcp_SocketData("数据交互",Full_CheckRecv)) break;
		Tcp_Close(NULL);
		pCode=PE_GetRecvIdPar("errCode");
		TRACE("reCode:%s",pCode); 
		if(pCode[0] == '0') 
		{
			char *recordCount=PE_GetRecvIdPar("recordCount");
			if(recordCount!=NULL && (recordMax=API_atoi(recordCount))>0)
			{
				char *recordList=PE_GetRecvIdPar("recordList");
				if((recordList!= NULL) && (API_strlen(recordList)>8))
				{	
					u8 type;
					char showbuff[1023];
					char IndexBuff[64];
					char *pshow,*pPar;
                                      //int i,total;
					dfJsonTable *pStat,*pModule;
					dfJsonTable *pJsonData,*pStartJson = Conv_JSON_GetMsg(recordList,recordList+API_strlen(recordList));
					pJsonData = pStartJson;
					showbuff[0] = '\0';
					pshow = showbuff;
					while(pJsonData)
					{
						pPar=Conv_GetJsonValue(pJsonData,"recordId",NULL);
						pshow += sprintf(pshow,"交班记录id:%s\n",pPar);
						pPar=Conv_GetJsonValue(pJsonData,"startTime",NULL);
						pshow += sprintf(pshow,"上班时间:\n%s\n",pPar);
						pPar=Conv_GetJsonValue(pJsonData,"endTime",NULL);
						pshow += sprintf(pshow,"下班时间:\n%s\n",pPar);
						pPar=Conv_GetJsonValue(pJsonData,"staff",NULL);
						if(g_ColData.terminalType[0] == '2')
							pshow += sprintf(pshow,"终端:%s\n",pPar);
						else if(g_ColData.terminalType[0] == '1')
							pshow += sprintf(pshow,"员工:%s\n",pPar);
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
									pshow=eStrcpy(pshow,IndexBuff);
									*pshow++ =':';
									*pshow++ ='\n';
								}
								
								pPar=Conv_GetJsonValue(pStat,"totalAmount",NULL);
								if(pPar)
								{
									sprintf(IndexBuff,"总金额:%s",pPar);
									pshow=eStrcpy(pshow,IndexBuff);
									pshow=eStrcpy(pshow,"元");
									*pshow++ =' ';
									*pshow++ =' ';
								}
								pPar=Conv_GetJsonValue(pStat,"totalCount",NULL);
								if(pPar!=NULL && API_strlen(pPar)>0)
								{
									//total=API_atoi(pPar);
									sprintf(IndexBuff,"项数:%s",pPar);
									pshow=eStrcpy(pshow,IndexBuff);
								}
								*pshow++ ='\n';
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
										if(pPar)
										{
											sprintf(IndexBuff,"%s次",pPar);
											pshow=eStrcpy(pshow,IndexBuff);
											*pshow++ = ',';
										}
										pPar=Conv_GetJsonValue(pModule,"amount",NULL);
										if(pPar)
										{
											sprintf(IndexBuff,"%s元",pPar);
											pshow=eStrcpy(pshow,IndexBuff);
										}
										*pshow++ = '\n';
										pModule = pModule->pNext;
									}
								}
								/*
								for(i=0;i<total;i++)
								{
									sprintf(IndexBuff,"module[%d]/payType",i);
									pPar=Conv_GetJsonValue(pStat,IndexBuff,NULL);
									if(pPar == NULL) break;
									API_Utf8ToGbk(IndexBuff,sizeof(IndexBuff),pPar);
									pshow=eStrcpy(pshow,IndexBuff);
									*pshow++ =':';
									*pshow++ =' ';
									sprintf(IndexBuff,"module[%d]/count",i);
									pPar=Conv_GetJsonValue(pStat,IndexBuff,NULL);
									sprintf(IndexBuff,"%s次",pPar);
									pshow=eStrcpy(pshow,IndexBuff);
									*pshow++ = ',';
									sprintf(IndexBuff,"module[%d]/amount",i);
									pPar=Conv_GetJsonValue(pStat,IndexBuff,NULL);
									sprintf(IndexBuff,"%s元",pPar);
									pshow=eStrcpy(pshow,IndexBuff);
									*pshow++ = '\n';
								}
								*/
								pStat = pStat->pNext;
								*pshow++ = '\n';
							}
						}
						*pshow++ = '\n';
						pJsonData = pJsonData->pNext;
					}
					*(--pshow)='\0';	//最后一行不用换行
					
					{
						int evet;
						if((nowPage*count) < recordMax)
							sprintf(IndexBuff,"当前%d总%d,确认向下",nowPage,(recordMax+count-1)/count);
						else
							sprintf(IndexBuff,"当前%d总%d,确认结束",nowPage,(recordMax+count-1)/count);
						evet=APP_ShowInfoMsg(pTitle,showbuff,IndexBuff,30*1000);
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
					Conv_JSON_free(pStartJson);
				}
			}
			else
			{
				APP_ShowTradeMsg("无交易记录",5000);
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


int shiftConfirm(char* pTitle)
{
	char* pCode;
	if(CheckMchidIsNull())return -1;
	if(API_strlen(g_ColData.recordId) == 0)
	{
		APP_ShowTradeFA("请先做交班统计",3000);
		return 0;
	}
	// 组建数据包,发送请求
	TCP_SetInterNotDisplay(FALSE);
	do{
		if(Tcp_Link("连接中心..")) break;
		// 组建数据包,发送请求
		TradeDataPacked_start(HTTP_TRADE_ADDERR"/open/Staff/shift");
		PackData_shiftConfirm();
		TradeDataPacked_end();
		// 发送接收数据
		if(Tcp_SocketData("数据交互",Full_CheckRecv)) break;
		pCode=PE_GetRecvIdPar("errCode");
		TRACE("reCode:%s",pCode); 
		if(pCode[0] == '0') 
		{
			APP_ShowTradeOK("交班成功");
			g_ColData.recordId[0]='\0';
			APP_WaitUiEvent(5*1000);
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


int ShiftMenu(char* pTitle)
{
	CMenuItemStru MenuStruPar[]=
	{
		"交班统计",		shiftStatV2,
		"确认交班",		shiftConfirm,
		"查看交班记录",	shiftRecordV2,
	};
	APP_CreateNewMenuByStruct(pTitle,sizeof(MenuStruPar)/sizeof(CMenuItemStru),MenuStruPar,30*1000);
	APP_AddCurrentMenuOtherFun(MENU_BACK_MAP,NULL,"shift.clz");
	return 0;
}


