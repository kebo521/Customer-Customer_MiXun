#ifndef _PE_UNIONPAY_H_
#define  _PE_UNIONPAY_H_

              
typedef struct	
{
	char* 			pSendCurr;
	char*			pSendLen;	//pSendLen7Addr
	char*           pMd5Start;
	char*           pMd5SCurr;
	char* 			pMsgData;
	u16				pMsgLen;			//DataLen[2];	有效数据长度
	u16				SendLen;
}DfFlow_Link;

extern   DfFlow_Link  pFlow;

#define    PE_SendBuf			pFlow.pMsgData
#define    PE_pSend 			pFlow.pSendCurr
#define    PE_pMD5 				pFlow.pMd5SCurr
#define    PE_sLenStart 		pFlow.pSendLen
#define    BufSize				2048


typedef struct
{
	char    developerId[16+1];// 开发者id  
	char	merchantId[12+1];			//品牌id 
	char	merchantSecretKey[32+1];	//品牌密钥 
	char	terminalType[1+1];	// 终端类型，1：门店，此时shopId必传 2：终端，此时sn号必传	
	char	sn[32];			// 终端sn号  
	char	shopId[12+1];		//门店Id
	char	userCode[12+1];		// 门店员工账号Id  
	char	payAmount[12+1];	//支付金额，单位分 
	char	refundFee[12+1];	//退款金额，单位分，不传默认为全额退款 
	char	payType[12+1];		//支付方式，如果不传，系统会通过authCode获取对应的支付方式 wx_pay：微信支付；ali_pay：支付宝；union_online：银联钱包；member_wallet：会员钱包；
	char    tradeId[32+1];		//太米系统流水Id，tradeId、transactionId和outTradeId至少传入一个，优先级tradeId > outTradeId > transactionId  
	char	outTradeId[31+1];		// 第三方内部流水号，tradeId、transactionId和outTradeId至少传入一个，优先级tradeId > outTradeId > transactionId  
	char	transactionId[32+1];	//微信或支付宝或银联的订单号，或太米商户订单号，tradeId、transactionId和outTradeId至少传入一个，优先级tradeId > outTradeId > transactionId。由于个别渠道不会返回该参数，因此有可能无效	
	char	returnContent[1+1]; //  返回数据内容 1：返回详细信息 2：仅返回退款结果 默认1 
	char	authCode[256+1];		//支付授权码或会员卡动态码，如果是会员卡动态码则会调用会员余额支付 
	char	code[128+1]; 		//优惠券核销码
	char	body[32+1]; 		//商品描述 
	char    timestamp[12];	// 时间戳	
	char    nonceStr[32];	//  随机字符串  
	char	nowPage[3+1]; 	//交班记录页码，从1开始
	char	count[2+1]; 	//每页的记录数量，最大10
	char	recordId[12+1]; //交班记录Id 
	char	sign[32+1]; 	//签名
	char	signkey[32+1];	//签名密钥
}QR_COL_Data;


extern QR_COL_Data  g_ColData;

extern void Send_add_item(char* pID,char* pData);
#define SEND_ADD_ITEM(s) 	Send_add_item(#s,g_ColData.##s) 

extern int MicroPay(char* pTitle);
extern  int OrderQuery(char* pTitle);
extern	int RefundFlow(char* pTitle);
extern	int ConsumeCard(char* pTitle);
extern	int ShiftMenu(char* pTitle);


#endif 
