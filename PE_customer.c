#include "communal.h"


TERM_PAR    Term_Par;
//dfJsonTable *pTrandMsg=NULL;

void TermParSetDefault(void)
{
	CLEAR(Term_Par);

	API_strcpy(Term_Par.developerId,"100001");
	API_strcpy(Term_Par.terminalType,"2");
	API_strcpy(Term_Par.shopId,"488");
	//API_strcpy(Term_Par.ServerIp,"www.uutianfu.com");
	//API_strcpy(Term_Par.ServerPort,"443");
	API_strcpy(Term_Par.CustVer,CustomerVersion);
}

void MachDatainit(void)
{
	int ret;
	CLEAR(g_ColData);
	API_SetLanguage(_LANG_ID_);
	//pSdkFun->sdk->language(1);// 语言设置 0 英文，1 中文
	ret=APP_GetHardMsg(TYPE_TERM_HARD_SN,g_ColData.sn,sizeof(g_ColData.sn));
	if(ret<0)
	{
		APP_ShowMsg("警告","终端不可用，请做终端序列号申请",3000);
		return ;
	}
	API_strcpy(g_ColData.developerId,DEVELOPER_ID);//Term_Par.developerId);
	API_strcpy(g_ColData.signkey,TM_SIGNKEY);		//Term_Par.developerId);
	API_strcpy(g_ColData.merchantId,Term_Par.userCode[0]);
	API_strcpy(g_ColData.merchantSecretKey,Term_Par.merchantSecretKey);
	API_strcpy(g_ColData.shopId,Term_Par.shopId);
	//API_strcpy(g_ColData.userCode,Term_Par.userCode[0]);
	API_strcpy(g_ColData.terminalType,Term_Par.terminalType);
	API_strcpy(g_ColData.userCode,"K21234567890");
//	API_strcpy(g_ColData.returnContent,"1");

	//======导入交易地址=============
	Tcp_LoadMsg(HTTP_TRADE_ADDERR,HTTP_TRADE_PORT);
}


int ScanMerchantInfo(char *title)
{
    char OutCode[128]={0};
    char shopId[32];
    int  ret,IndexId;
	API_GUI_CreateWindow(title,NULL,NULL,0);
	API_GUI_Info(NULL,TEXT_ALIGN_LEFT|TEXT_VALIGN_TOP|TEXT_EXSTYLE_UNDERLINE,"请对准品牌信息码");
	API_GUI_Info(NULL,TEXT_ALIGN_RIGHT|TEXT_VALIGN_BOTTOM|TEXT_EXSTYLE_OVERLINE,"按[取消]键退出");
	API_GUI_Show();
    ret=APP_OnlyCamScan(0,20,50,OutCode,30*1000);
    if(ret<0) return 1;
    //------------取ID-----------------
    for(IndexId=0;IndexId<ret;IndexId++)
    {
        if(OutCode[IndexId] <= ' ') break;
    }
    if(IndexId<2 || IndexId>16)
    {
        APP_ShowMsg(title,"品牌id",5000);
        return -1;
    }
    API_memcpy(shopId,OutCode,IndexId);
    shopId[IndexId]='\0';

    while(OutCode[IndexId] <= ' ') IndexId++;   //过滤空格获'\'
    //------------取KEY-----------------
	while(OutCode[ret] <= ' ') ret--;	//过滤后面的
	ret -= IndexId;
	if(ret >= sizeof(Term_Par.merchantSecretKey)) 
		ret = sizeof(Term_Par.merchantSecretKey)-1;
	OutCode[IndexId+ret]='\0';
    if(ret < 8)
    {
        APP_ShowMsg(title,"密钥长度错",5000);
        return -2;
    }
    API_strcpy(Term_Par.merchantId, shopId);
    API_strcpy(Term_Par.merchantSecretKey, OutCode+IndexId);

    //Par_Set(Term_Par.merchantId, sizeof(Term_Par.merchantId));
    //Par_Set(Term_Par.signkey, sizeof(Term_Par.signkey));
	Par_SaveAll();
	API_strcpy(g_ColData.shopId,Term_Par.shopId);
	API_sprintf(OutCode,"导入成功\n\n品牌id:\n%s\n品牌密钥 :\n%s",Term_Par.merchantId,Term_Par.merchantSecretKey);
    return APP_ShowInfo("品牌信息",OutCode,10000);
}




int SoftInfo(char*  ptitle)
{
    char Msg[128]={0};
	API_strcat(Msg,CustomerVersion);
	API_strcat(Msg,"\n");
    API_strcat(Msg,VersionDate);
	API_strcat(Msg,"\n");
    API_strcat(Msg,TermModel);
	API_strcat(Msg,"\n");
    API_strcat(Msg,Version);
	API_strcat(Msg,"\nSN:");
	API_strcat(Msg,Term_Par.shopId);
	API_GUI_CreateWindow("软件版本",NULL,NULL,0);
	API_GUI_Info(NULL,TEXT_ALIGN_CENTER|TEXT_VALIGN_TOP,Msg);
	API_GUI_Info(NULL,TEXT_ALIGN_RIGHT|TEXT_VALIGN_BOTTOM,"按[确认]键检测更新");
	API_GUI_Show();
	if(EVENT_OK==APP_WaitUiEvent(30*1000))
	{
		// APP_ShowMsg("提示","正在检测版本...",1);
		//if(0==pSdkFun->sdk->tmsSyn("版本检测"))
		  //   APP_ShowMsg("提示","无版本更新",3000);
	}
	return 0;
}


int APP_WIFIFunction(char* title)
{
	CMenuItemStru MenuStruPar[]=
	{
		"WIFI开关",					APP_WIFI_EN,
		"设置WIFI密码",				APP_WIFI_SetKey,
	};
	return APP_CreateNewMenuByStruct(title,sizeof(MenuStruPar)/sizeof(CMenuItemStru),MenuStruPar,30*1000);
}


int SetUserID(char* title)
{
	if(APP_InputAbc(title,"设置操作员",  STR_PRESS_NUBER_KEYS_ENTER, Term_Par.userCode[0],0, sizeof(Term_Par.userCode[0])-1)==0)
	{
		API_strcpy(g_ColData.userCode, Term_Par.userCode[0]);
        Par_Set(Term_Par.userCode[0], sizeof(Term_Par.userCode[0]));
	}
	return 0;
}

int SetShopID(char* title)
{
	if(APP_InputAbc(title,"请输入门店编号",  STR_PRESS_NUBER_KEYS_ENTER, Term_Par.shopId,0, sizeof(Term_Par.shopId)-1)==0)
	{
		API_strcpy(g_ColData.shopId, Term_Par.shopId);
        Par_Set(Term_Par.shopId, sizeof(Term_Par.shopId));
	}
	return 0;
}


//终端类型，1：，此时shopId必传 2：，此时sn号必传
int APP_TermType(char* pTitle)
{
	const char* pTraceMenu[]={"门店","终端"};
	int ret;
	ret=APP_GUI_Menu(pTitle,NULL,sizeof(pTraceMenu)/sizeof(pTraceMenu[0]),(Term_Par.terminalType[0]&0x0f)-1,pTraceMenu,30*1000);
	if(ret <= (sizeof(pTraceMenu)/sizeof(pTraceMenu[0])))
	{
		if(ret >= 0)
		{
			Term_Par.terminalType[0]= ret+'1';
			Par_Set(Term_Par.terminalType, sizeof(Term_Par.terminalType));
		}
		else return EVENT_NONE;
	}
	return EVENT_NONE;
}


int APP_TermMenu(char* title)
{
	CMenuItemStru MenuStruPar[]=
	{
		//"扫码导入品牌信息",		ScanMerchantInfo,
		"设置终端类型",			APP_TermType,
		"设置门店编号",			SetShopID,
		"设置店员号",			SetUserID,
		"WIFI功能",				APP_WIFIFunction,
		"设备信息",				SoftInfo,
		"检查更新",				pSdkFun->app->NetInstallAPP,
	};
	return APP_CreateNewMenuByStruct(title,sizeof(MenuStruPar)/sizeof(CMenuItemStru),MenuStruPar,30*1000);
}

extern int TEST_JSON(char *pTitle);

int APP_TradeMainMenu(char* title)
{
	CMenuItemStru MenuStruPar[]=
	{
		"收款",			MicroPay,
		"查询",			OrderQuery,
		"退款",			RefundFlow,
		"核销",			ConsumeCard,
		"交班",			ShiftMenu,
	//	"JSON测试", 			TEST_JSON, 
	};
	APP_CreateNewMenuByStruct(title,sizeof(MenuStruPar)/sizeof(CMenuItemStru),MenuStruPar,-1);//30*1000
	APP_AddCurrentMenuOtherFun(MENU_BACK_MAP,NULL,"taimi.clz");
	APP_AddCurrentMenuOtherFun(MENU_KEY_FUN,(void*)APP_TermMenu,"终端管理");
	return 0;
}


int customer_MainMenu(char* title)
{
	// 关闭公共区自动提示
	TCP_SetInterNotDisplay(TRUE);	
	UI_SetMenuItem(5);
	//for(;;)
	{
		APP_TradeMainMenu(title);
		if(EVENT_QUIT&APP_ShowMenuProsse()) return EVENT_QUIT;
	}
	return 0;
}
