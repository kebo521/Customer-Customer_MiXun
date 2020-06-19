#ifndef _CUSTOMER_
#define _CUSTOMER_



#define 	CONNECT_MYSELF							//直接连交易平台
#define  	HTTP_TRADE_ADDERR   			"https://pos.weixincore.com"	//https://pos.weixincore.com/merchant/User/login
#define 	HTTP_TRADE_PORT					443	

#define		DEVELOPER_ID					"100001"		//developerId
#define		TM_SIGNKEY						"ac6d97e67b444b7a43edfc9182634786"		//signKey

#define	TermModel			"KNS260"			//终端型号
#define	Version				"V2.01.004"			//软件版本 
//============中国银联
#define	CustomerVersion		"太米-WIFI"			//客户版本

#define	VersionDate			"2020-06-08"		//软件版本日期


typedef struct
{
	/* 网络参数 */
	char	CustVer[32+1];//
	char    ServerIp[64];
	char    ServerPort[16];
	//==============
	char	developerId[16+1];	//系统分配给开发者的唯一id
	char	terminalType[1+1];	//终端类型 1：门店，此时shopId必传 ,2：终端，此时sn号必传
	char	merchantId[12+1];			//品牌id 
	char	merchantSecretKey[32+1];	//品牌密钥 
	char	sn[32];				//终端SN号
	char	shopId[12+1];		//门店Id
	u8		userNum,userIndex;	//员工数量,当前员工
	char	userCode[8][12+1];	//门店员工账号Id  
}TERM_PAR;
extern TERM_PAR    Term_Par;

//=================交易记录定义  Termfile文件中需要客户定义==========================
typedef struct
{
	char		 money[9+1];
	char         date[10+1];
	char         time[8+1];
	char         type[20+1];// 交易类型,不能超过一行
	char         order[32+1];// 订单号32字节
}TradeRecordItem; //整个结构总字节数为4的整数倍
#define TRADE_RECORD_MAX   	8		// TradeRecordItem 总交易记录数


extern void TermParSetDefault(void);
extern void MachDatainit(void);
extern void customer_main(unsigned long argc,void* lpThreadParameter);
extern int WifiInfoMenu(char* title);
extern int customer_MainMenu(char* title);
extern int APP_TradeMainMenu(char* title);


#include "PE_SweepPay.h"
#include "PE_sdk.h"



#endif


