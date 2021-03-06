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
	char *pPayType,*payType;
	int ret;
	Conv_TmoneyToDmoney(DisplayMoney,g_ColData.payAmount);
	//APP_ShowTradeOK(g_ColData.payAmount); 
	APP_ShowTrade("TradeOK.clz",DisplayMoney,"交易成功");
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
		pPayType="微信收款";
	}
	else if(API_strcmp(payType,"ali_pay")==0)
	{
		pPayType="支付宝收款";
	}
	else //union_offline：银行卡；union_qrcode：银联扫码；union_online：银联钱包
	{
		pPayType="收款成功";
	}	
    APP_TTS_PlayText("%s%s元",pPayType,DisplayMoney);
	ret=APP_WaitUiEvent(6*1000);
	//-------更新本地时间----------
	SetSysDateTime(PE_GetRecvIdPar("payTime"));
	return ret;
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
	char GbkMsg[256]={0};
	API_sprintf(codeInfo,"应答码[%s]",pCode);
	API_Utf8ToGbk(GbkMsg,sizeof(GbkMsg),PE_GetRecvIdPar("errMsg"));
	PE_ShowMsg("提示",codeInfo,GbkMsg,NULL,10*1000);
 }


int inside_OrderQuery(int PeekLink)
{
	int ret;
	// 组建数据包,发送请求
	if(PeekLink == 0)
	{
		ret = Tcp_Link("连接中心..");
		if(ret) return ret;
	}
	// 组建数据包,发送请求
	TradeDataPacked_start(HTTP_TRADE_ADDERR"/open/Pay/orderQuery");
	PackData_OrderQuery();
	TradeDataPacked_end();
	// 发送接收数据
	ret = Tcp_SocketData("数据交互",Full_CheckRecv);
	Tcp_Close(NULL);
	return ret;
}

int inside_MicroPay(int Errtimes)
{
	int ret;
	char *pCode;
	// 组建数据包,发送请求
	TradeDataPacked_start(HTTP_TRADE_ADDERR"/open/Pay/microPayV2");
	SweepPackData_V2();
	TradeDataPacked_end();
	// 发送接收数据
	ret = Tcp_SocketData("数据交互",Full_CheckRecv);
	Tcp_Close(NULL);
	if(ret) 
	{
		if(ret == OPER_RECV_ERR)
		{
			APP_ShowTradeMsg("提醒!由于网络问题\n未收到交易结果\n请从手机或终端查询功能\n确定结果!",5000);
		}
		return ret;
	}
RE_QUERY_ADDER:
	pCode=PE_GetRecvIdPar("errCode");
	if(pCode==NULL)
	{
		APP_ShowTradeFA("接收数据异常",10000);
		return OPER_ERR;
	}
	TRACE("reCode:%s",pCode); 
	if(pCode[0] == '0') //返回成功，可以解析下一层 数据。
	{
		char *tradeInfo;
		tradeInfo = PE_GetRecvIdPar("tradeInfo");
		if(tradeInfo!= NULL && 0==PE_JsonDataParse(tradeInfo,API_strlen(tradeInfo)))
		{
			char *payStatus;
			payStatus = PE_GetRecvIdPar("payStatus");
			if(payStatus == NULL)
			{
				APP_ShowTradeMsg("无状态信息",10000);
				PE_JsonFree(1);
				return OPER_ERR;
			}
			if(API_strcmp(payStatus,"SUCCESS")==0)
			{
				PE_ReadRecvIdPar("incomeAmount",g_ColData.payAmount);
				SuccessTradeProcess();	
				PE_JsonFree(1);
				return OPER_OK;
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
				PE_ReadRecvIdPar("tradeId",g_ColData.tradeId);
				PE_ReadRecvIdPar("outTradeId",g_ColData.outTradeId);
				PE_JsonFree(1);
				if(EVENT_CANCEL == API_WaitEvent(2*1000,EVENT_UI,EVENT_NONE))
				{
					return OPER_RET;
				}
				APP_SetKeyAccept(0x01);
				PE_JsonFree(1);
				ret = inside_OrderQuery(0);
				if(ret == OPER_RET)
				{
					return OPER_RET;
				}
				else if(ret)
				{
					APP_ShowTradeMsg("订单支付中,稍候查询确认结果",10000);
					return OPER_NEW;
				}
				if(Errtimes-- <= 0)
				{
					APP_ShowTradeMsg("订单支付中,\n查询结果超时\n稍候手动查询确认结果",10000);
					return OPER_NEW;
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
	return OPER_NEW;
}

int MicroPay(char* title)
{    
	TCP_SetInterNotDisplay(FALSE);
	while(1)
	{
		if(Tcp_PeekLink()) //Tcp_Link(NULL);
			return -2;
		while(1)
		{
			if(0 != InputTotalFee(title)){
				Tcp_Close(NULL);
				return -1;
			}
			APP_TTS_PlayText("请示二维码");
			if(0 != InputAuthcodeByCamScan())
			{
				continue;
			}
			break;
		}
		APP_ShowWaitFor(NULL);//STR_NET_LINK_WLAN
		if(0==inside_MicroPay(10))
		{
			PE_JsonFree(0);
			continue;
		}
		break;
	}
    DataFree();
	Tcp_Close(NULL);
	return 0;	
}


int FixedMicroPay(char* title)
{    
	int ret=0;
	TCP_SetInterNotDisplay(FALSE);
	while(1)
	{
		if(Tcp_PeekLink()) //Tcp_Link(NULL);
			return -2;
		if(0>APP_EditSum(title,'S',g_ColData.FixedAmount,60*1000))
		{
			Tcp_Close(NULL);
			return -1;
		}
		API_strcpy(g_ColData.payAmount,g_ColData.FixedAmount);
		APP_TTS_PlayText("请示二维码");
		CLEAR(g_ColData.authCode);
		ret = APP_CamScan('S',g_ColData.payAmount,g_ColData.authCode,10,sizeof(g_ColData.authCode)-1,20*1000);
		if(ret < 0) 
		{
			Tcp_Close(NULL);
			return ret;
		}
		APP_ShowWaitFor(NULL);//STR_NET_LINK_WLAN
		if(0 == inside_MicroPay(10))
		{
			PE_JsonFree(0);
			continue;
		}
		break;
	}
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
		    if(APP_ScanInput(pTitle,"请手动输入商户单号",NULL,g_ColData.transactionId,8,32)) 
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
			APP_ShowTradeMsg("接收数据异常",10000);
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
					pShow += sprintf(pShow,"门店:%s  ",pPar);
				}
				pPar=PE_GetRecvIdPar("merchantUserId");
				if(pPar)
				{
					pShow += sprintf(pShow,"收银员:%s\n",pPar);
				}
				*/
				pPar=PE_GetRecvIdPar("payStatus");
				if(pPar)
				{//0表示未退款，1表示转入退款
					pShow = eStrcpy(pShow,"订单状态:");
					if(strstr(pPar,"SUCCESS"))
					{//SUCCESS：
						pShow = eStrcpy(pShow,"支付成功");
					}
					else if(strstr(pPar,"NOTPAY"))
					{//NOTPAY：未支付�
						pShow = eStrcpy(pShow,"未支付");
					}
					else if(strstr(pPar,"CLOSE"))
					{//CLOSE：已关闭
						pShow = eStrcpy(pShow,"已关闭");
					}
					else if(strstr(pPar,"REVOKED"))
					{//REVOKED：已撤销
						pShow = eStrcpy(pShow,"已撤销");
					}
					else if(strstr(pPar,"USERPAYING"))
					{//USERPAYING：用户支付中
						pShow = eStrcpy(pShow,"用户支付中");
					}
					else if(strstr(pPar,"PAYERROR"))
					{//PAYERROR：支付失败
						pShow = eStrcpy(pShow,"支付失败");
					}
					else if(strstr(pPar,"REFUND"))
					{//REFUND：
						pShow = eStrcpy(pShow,"转入退款");
					}
					else if(strstr(pPar,"PERATE_FAIL"))
					{//PERATE_FAIL：预授权请求操作失败；
						pShow = eStrcpy(pShow,"预授权请求操作失败");
					}
					else if(strstr(pPar,"OPERATE_SETTLING"))
					{//OPERATE_SETTLING：押金消费已受理；
						pShow = eStrcpy(pShow,"押金消费已受理");
					}
					else if(strstr(pPar,"REVOKED_SUCCESS"))
					{//REVOKED_SUCCESS：预授权请求撤销成功
						pShow = eStrcpy(pShow,"预授权请求撤销成功");
					}
					*pShow++ = '\n';
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
				pPar=PE_GetRecvIdPar("refundAmount");
				if(pPar)
				{
					char DisplayMoney[16];
					Conv_TmoneyToDmoney(DisplayMoney,pPar);
					pShow += sprintf(pShow,"已退金额:%s\n",DisplayMoney);
				}
				pPar=PE_GetRecvIdPar("payTime");
				if(pPar)
				{
					pShow += sprintf(pShow,"支付时间:\n%s\n",pPar);
				}
				
				pPar=PE_GetRecvIdPar("id");
				if(pPar)
				{
					pShow += sprintf(pShow,"太米流水:%s\n",pPar);
				}
				pPar=PE_GetRecvIdPar("outTradeId");
				if(pPar)
				{
					pShow += sprintf(pShow,"外部交易号:%s\n",pPar);
				}
				
				pPar=PE_GetRecvIdPar("transactionId");
				if(pPar)
				{
					pShow += sprintf(pShow,"交易号:%s\n",pPar);
				}
				pPar=PE_GetRecvIdPar("orderNum");
				if(pPar)
				{
					pShow += sprintf(pShow,"内部交易号:%s\n",pPar);
				}
				pPar=PE_GetRecvIdPar("module");
				if(pPar)
				{
					pShow = eStrcpy(pShow,"模块:");
					if(strstr(pPar,"pay"))
					{//pay：收银；
						pShow = eStrcpy(pShow,"收银");
					}
					else if(strstr(pPar,"mall"))
					{//mall：优选卡券货架；
						pShow = eStrcpy(pShow,"优选卡券货架");
					}
					else if(strstr(pPar,"recharge"))
					{//recharge：充值；
						pShow = eStrcpy(pShow,"充值");
					}
					else if(strstr(pPar,"become_member"))
					{//become_member：会员购买；
						pShow = eStrcpy(pShow,"会员购买");
					}
					else if(strstr(pPar,"eatIn"))
					{//eatIn：店内下单；
						pShow = eStrcpy(pShow,"店内下单");
					}
					else if(strstr(pPar,"takeOut"))
					{//takeOut：外送订单；
						pShow = eStrcpy(pShow,"外送订单");
					}
					else if(strstr(pPar,"selfTake"))
					{//selfTake：预约自取；
						pShow = eStrcpy(pShow,"预约自取");
					}
					else if(strstr(pPar,"payment_card"))
					{//payment_card：付费卡券；
						pShow = eStrcpy(pShow,"付费卡券");
					}
					else if(strstr(pPar,"times_card"))
					{//times_card：次/月卡
						pShow = eStrcpy(pShow,"次/月卡");
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
					pShow = eStrcpy(pShow,"订单来源:");
					if(strstr(pPar,"wx"))
					{//wx：微信收款码；
						pShow = eStrcpy(pShow,"支付成功");
					}
					else if(strstr(pPar,"alipay"))
					{//alipay：支付宝收款码
						pShow = eStrcpy(pShow,"未支付");
					}
					else if(strstr(pPar,"web"))
					{//web：Web页面
						pShow = eStrcpy(pShow,"未支付");
					}
					else if(strstr(pPar,"alipay"))
					{//mini：小程序；
						pShow = eStrcpy(pShow,"未支付");
					}
				}
				*/
				if(pShow > showBuff)
				{
					*(--pShow)='\0';	//最后一行不用换行
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
		//if(APP_InputPin(title,"请输入管理员密码:","请按数字键输入", "888888")) return -1;
		g_ColData.tradeId[0]='\0';
		g_ColData.outTradeId[0]='\0';	
		g_ColData.transactionId[0]='\0';
		ret=APP_CamScan('R',NULL,g_ColData.transactionId,8,32,20*1000);
		if(ret<0) break;
		if(ret == OPER_HARD)
		{
		    if(APP_ScanInput(pTitle,"请手动输入商户单号",NULL,g_ColData.transactionId,8,32)) 
				break;
		}
		APP_ShowWaitFor(NULL);//STR_NET_LINK_WLAN
		if(inside_OrderQuery(1))
			break;
		pCode=PE_GetRecvIdPar("errCode");
		if(pCode==NULL)
		{
			APP_ShowTradeMsg("接收数据异常",10000);
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
					APP_ShowTradeMsg("无可退金额",10*1000);
                       break;
				}
				*/
				sprintf(g_ColData.refundFee,"%d",income);
				PE_JsonFree(1);
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
		PE_JsonFree(1);
		
		if(0>APP_EditSum(pTitle,'R',g_ColData.refundFee,60*1000))
		{
			break;
		}
		if(Tcp_Link("连接中心..")) break;
		// 组建数据包,发送请求
		TradeDataPacked_start(HTTP_TRADE_ADDERR"/open/Pay/refund");
		PackData_refund();
		TradeDataPacked_end();
		// 发送接收数据
		ret = Tcp_SocketData("数据交互",Full_CheckRecv);
		Tcp_Close(NULL);
		if(ret)
		{
			if(ret == OPER_RECV_ERR)
			{
				APP_ShowTradeMsg("提醒!由于网络问题\n未收到退款结果\n请从手机或本机查询功能\n确定退款结果!",5000);
			}
			break;
		}
		pCode=PE_GetRecvIdPar("errCode");
		if(pCode==NULL)
		{
			APP_ShowTradeMsg("接收数据异常",10000);
			break;
		}
		if(pCode[0] == '0') 
		{
			APP_ShowRefundsOK(g_ColData.refundFee);
			APP_TTS_PlayText("退款成功");
			APP_WaitUiEvent(6*1000);
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
		APP_ShowSta(pTitle,"请扫优惠券核销码");
		g_ColData.code[0]='\0';
		ret=APP_OnlyCamScan(0x02,4,sizeof(g_ColData.code)-1,g_ColData.code,30*1000);
		if(ret<0) break;
		if(ret==OPER_HARD)
		{
			if(APP_ScanInput(pTitle,"请输入核销码",NULL,g_ColData.code,4,32)) 
				break;
		}
		APP_ShowWaitFor(NULL);//STR_NET_LINK_WLAN
		TCP_SetInterNotDisplay(FALSE);
		// 组建数据包,发送请求
		//if(Tcp_Link("连接中心..")) break;
		// 组建数据包,发送请求
		TradeDataPacked_start(HTTP_TRADE_ADDERR"/open/Card/consumeCard");
		PackData_ConsumeCard();
		TradeDataPacked_end();
		// 发送接收数据
		ret = Tcp_SocketData("数据交互",Full_CheckRecv);
		Tcp_Close(NULL);
		if(ret)
		{
			if(ret == OPER_RECV_ERR)
			{
				APP_ShowTradeMsg("提醒!由于网络问题\n未收到核销结果\n请从手机确定核销结果!",5000);
			}
			break;
		}
		pCode=PE_GetRecvIdPar("errCode");
		if(pCode==NULL)
		{
			APP_ShowTradeMsg("接收数据异常",10000);
			break;
		}
		TRACE("reCode:%s",pCode); 
		if(pCode[0] == '0') 
		{
			APP_ShowTrade("TradeOK.clz",NULL,"核销成功");
			//APP_ShowTradeMsg("\n   核销成功",0);
			APP_TTS_PlayText("核销成功");
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

int shiftConfirm(char* pTitle)
{
	int ret;
	char* pCode;
	if(Tcp_Link("连接中心..")) return -1;
	// 组建数据包,发送请求
	TradeDataPacked_start(HTTP_TRADE_ADDERR"/open/Staff/shift");
	PackData_shiftConfirm();
	TradeDataPacked_end();
	// 发送接收数据
	ret=Tcp_SocketData("数据交互",Full_CheckRecv);
	Tcp_Close(NULL);
	if(ret) return -2;
	pCode=PE_GetRecvIdPar("errCode");
	if(pCode==NULL)
	{
		APP_ShowTradeMsg("接收数据异常",10000);
	}
	else
	{
		TRACE("reCode:%s",pCode); 
		if(pCode[0] == '0') 
		{
			APP_ShowTrade("TradeOK.clz",NULL,"交班成功");
			g_ColData.recordId[0]='\0';
			APP_WaitUiEvent(5*1000);
		}
		else 
		{
			PE_ShowfailMsg(pCode);
		} 
	}
	PE_JsonFree(1);
	return 0;
}

int shiftStatV2(char* pTitle)
{
	int ret;
	char* pCode;
	do{
		// 组建数据包,发送请求
		TCP_SetInterNotDisplay(FALSE);
		if(Tcp_Link("连接中心..")) break;
		// 组建数据包,发送请求
		TradeDataPacked_start(HTTP_TRADE_ADDERR"/open/Staff/shiftStatV2");
		PackData_shiftStatV2();
		TradeDataPacked_end();
		// 发送接收数据
		ret = Tcp_SocketData("数据交互",Full_CheckRecv);
		Tcp_Close(NULL);
		if(ret) break;
		pCode=PE_GetRecvIdPar("errCode");
		if(pCode==NULL)
		{
			APP_ShowTradeMsg("接收数据异常",10000);
			break;
		}
		if(pCode[0] == '0') 
		{
			char showbuff[1024],*pshow;
			char IndexBuff[63],type;
			char *pPar,*stat;
			showbuff[0] = '\0';
			pshow = showbuff;
			pPar=PE_GetRecvIdPar("staff");
			if(pPar)
			{
				API_Utf8ToGbk(IndexBuff,sizeof(IndexBuff),pPar);
				pshow += sprintf(pshow,"收银员:%s\n",IndexBuff);
			}
			pPar=PE_GetRecvIdPar("startTime");
			if(pPar)
			{
				pshow += sprintf(pshow,"上班时间:\n%s\n",pPar);
			}
			pPar=PE_GetRecvIdPar("endTime");
			if(pPar)
			{
				pshow += sprintf(pshow,"下班时间:\n%s\n",pPar);
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
								pshow += sprintf(pshow,"%s笔 ",pPar);
							}
							pPar=Conv_GetJsonValue(pModule,"amount",NULL);
							if(pPar != NULL && API_strlen(pPar)>0)
							{
								pshow += sprintf(pshow,"%s",pPar);
							}
							*pshow++ = '\n';
							pModule = pModule->pNext;
						}
					}
					pPar=Conv_GetJsonValue(pJsonData,"totalCount",NULL);
					if(pPar != NULL && API_strlen(pPar)>0)
					{
						pshow += sprintf(pshow,"总计:%s笔  ",pPar);
					}
					pPar=Conv_GetJsonValue(pJsonData,"totalAmount",NULL);
					if(pPar != NULL && API_strlen(pPar)>0)
					{
						pshow += sprintf(pshow,"合计:%s元",pPar);
					}
					*pshow++ ='\n';
					pJsonData = pJsonData->pNext;
				}
				Conv_JSON_free(pStartJson);
			}
			if(pshow > showbuff)
			{
				*(--pshow)='\0';	//最后一行不用换行
				if(EVENT_OK == APP_ShowInfoMsg(pTitle,showbuff,"按[确认]键交班",30*1000))
				{
					PE_ReadRecvIdPar("recordId",g_ColData.recordId); 
					shiftConfirm("交班确认");
				}
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
	// 组建数据包,发送请求
	TCP_SetInterNotDisplay(FALSE);
	nowPage=1;
	count = 1;
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
		ret=Tcp_SocketData("数据交互",Full_CheckRecv);
		Tcp_Close(NULL);
		if(ret) break;
		pCode=PE_GetRecvIdPar("errCode");
		if(pCode==NULL)
		{
			APP_ShowTradeMsg("接收数据异常",10000);
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
						APP_ShowTradeMsg("申请内存失败",5000);
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
					//		pshow += sprintf(pshow,"终端:%s\n",pPar);
					//	else if(g_ColData.terminalType[0] == '1')
						if(pPar)
						{
							API_Utf8ToGbk(IndexBuff,sizeof(IndexBuff),pPar);
							pshow += sprintf(pshow,"收银员:%s\n",IndexBuff);
						}
					//	pPar=Conv_GetJsonValue(pJsonData,"recordId",NULL);
					//	pshow += sprintf(pshow,"交班记录id:%s\n",pPar);
						pPar=Conv_GetJsonValue(pJsonData,"startTime",NULL);
						pshow += sprintf(pshow,"上班时间:\n%s\n",pPar);
						pPar=Conv_GetJsonValue(pJsonData,"endTime",NULL);
						pshow += sprintf(pshow,"下班时间:\n%s\n",pPar);
						
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
											pshow += sprintf(pshow,"%s笔 ",pPar);
										}
										pPar=Conv_GetJsonValue(pModule,"amount",NULL);
										if(pPar!=NULL && API_strlen(pPar)>0)
										{
											pshow += sprintf(pshow,"%s",pPar);
										}
										*pshow++ = '\n';
										pModule = pModule->pNext;
									}
								}
								pPar=Conv_GetJsonValue(pStat,"totalCount",NULL);
								if(pPar!=NULL && API_strlen(pPar)>0)
								{
									pshow += sprintf(pshow,"总计:%s笔  ",pPar);
								}
								pPar=Conv_GetJsonValue(pStat,"totalAmount",NULL);
								if(pPar)
								{
									pshow += sprintf(pshow,"合计:%s元",pPar);
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
						*(--pshow)='\0';	//最后一行不用换行
						if((nowPage*count) < recordMax)
							sprintf(IndexBuff,"当前%d总%d,确认向下",nowPage,(recordMax+count-1)/count);
						else
							sprintf(IndexBuff,"当前%d总%d,确认结束",nowPage,(recordMax+count-1)/count);
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
				APP_ShowTradeMsg("无交班记录",5000);
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





int ShiftMenu(char* pTitle)
{
	CMenuItemStru MenuStruPar[]=
	{
		"交班统计",		shiftStatV2,
		//"确认交班",		shiftConfirm,
		"查看交班记录",	shiftRecordV2,
	};
	APP_CreateNewMenuByStruct(pTitle,sizeof(MenuStruPar)/sizeof(CMenuItemStru),MenuStruPar,30*1000);
//	APP_AddCurrentMenuOtherFun(MENU_BACK_MAP,NULL,"shift.clz");
	return 0;
}


