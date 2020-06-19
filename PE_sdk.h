#ifndef PE_SDK_H_
#define PE_SDK_H_


#include "communal.h"
//===================中国银联前置==============

typedef struct
{
	char  pID[24];// 标签
	char  pSid[24];// 签名的标签 签名标签要改写为小写的
	char  pData[64]; // 数据
}pItemData;

typedef struct
{
	u16			index;
	pItemData Item[32]; // 总共可32个标签
}pDataTable;

extern int PE_JsonDataParse(char *pIndata,int inLen);
extern void PE_JsonFree(void);
extern char* PE_GetRecvIdPar(char* pPostID);
extern int PE_CheckRecvIdPar(char* pPostID,char *pCheck);
extern	int PE_ReadRecvIdPar(char* pPostID,char *pOutData);

extern int PE_CheckRecvIdPar(char* pPostID,char *pCheck);

extern int PE_CheckRecvData(char* pInRecv,int Len);

extern int InputAuthcodeByCamScan(void);
extern int InputTotalFee(char *pTitle);

extern int APP_ScanInput(char* pTitle,char* pFrontTextBuf,char* pAfterTextBuf,char* pInputDef,int InputMinLen,int InputMaxlen);
extern	int PE_CamScan(char flagd,char* pHintOrTMoney,char *pOutCode,int MinLen,int MaxLen,int timeOutMs);
extern int PE_ShowEnquiriesMsg(char* status,char *pTmoney,char* pDateTime);
extern int PE_ShowMsg(char *pTitle,char* pTopMsg,char *pMsg,char *prompt, int timeOutMs);

extern int PE_CheckRecvData(char* pInRecv,int Len);



#endif 











