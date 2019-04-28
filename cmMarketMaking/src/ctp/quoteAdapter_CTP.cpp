#include <iostream>
#include "ctp/quoteAdapter_CTP.h"
#include <vector>
#include <boost\thread.hpp>
#include "glog\logging.h"

using namespace std;

quoteAdapter_CTP::quoteAdapter_CTP(string adapterID, char * mdFront, char * broker, char * user, char * pwd)
{
	m_adapterID = adapterID;

	m_pUserApi = CThostFtdcMdApi::CreateFtdcMdApi();
	m_pUserApi->RegisterSpi(this);
	m_pUserApi->RegisterFront(mdFront);

	memset(&m_loginField, 0, sizeof(m_loginField));
	strncpy(m_loginField.BrokerID, broker, sizeof(m_loginField.BrokerID));
	strncpy(m_loginField.UserID, user, sizeof(m_loginField.UserID));
	strncpy(m_loginField.Password, pwd, sizeof(m_loginField.Password));
}
void quoteAdapter_CTP::destroyAdapter()
{
	m_status = ADAPTER_STATUS_DISCONNECT;
	try{
		m_pUserApi->Release(); 
	}
	catch (exception& e)
	{
		LOG(INFO)  << e.what() << endl;
	}
};
int quoteAdapter_CTP::init()
{
	m_pUserApi->Init();
	m_status = ADAPTER_STATUS_CONNECTING;
	return 0;
}

int quoteAdapter_CTP::login()
{
	int ret = m_pUserApi->ReqUserLogin(&m_loginField, ++m_requestId);
#ifdef ADAPTER_LOGGING
	LOG(INFO)  << m_adapterID << ":  req | user login ... " << ((ret == 0) ? "succ" : "fail") << endl;
#endif
	return ret;
};

void quoteAdapter_CTP::OnFrontConnected()
{
	LOG(WARNING) << m_adapterID << ": ctp quote connected!" << endl;
	login();
};

void quoteAdapter_CTP::OnFrontDisconnected(int nReason)
{
	LOG(INFO)  << "quote adapterCTP disconnected!" << endl;
	if (m_status != ADAPTER_STATUS_DISCONNECT && m_OnFrontDisconnected)
	{
		m_OnFrontDisconnected(m_adapterID, "quote");
	}
	m_status = ADAPTER_STATUS_DISCONNECT;
};

void quoteAdapter_CTP::OnHeartBeatWarning(int nTimeLapse)
{
	LOG(INFO)  << "heartbeat warning: " << nTimeLapse << "s." << endl;
};

void quoteAdapter_CTP::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (isErrorRespInfo(pRspInfo))
#ifdef ADAPTER_LOGGING
		LOG(INFO)  << m_adapterID << ": quote login error | ErrorID: " << pRspInfo->ErrorID << ", ErrorMsg: " << pRspInfo->ErrorMsg << endl;
#else
		cout << m_adapterID << ": quote login error | ErrorID: " << pRspInfo->ErrorID << ", ErrorMsg: " << pRspInfo->ErrorMsg << endl;
#endif
	else
	{
#ifdef ADAPTER_LOGGING
		LOG(INFO)  << m_adapterID << ": quote login succ!" << endl;
#else
		cout << m_adapterID << ": quote login succ!" << endl;
#endif
		
		auto iter = m_instrumentList.begin();
		while (iter != m_instrumentList.end())
		{
			int len = m_instrumentList.size();
			char** pInstId = new char*[len];
			for (unsigned int i = 0; i < len; i++) pInstId[i] = (char*)m_instrumentList[i].c_str();
			int ret = m_pUserApi->SubscribeMarketData(pInstId, len);
			LOG(INFO)  << " req | send quote sub ... " << ((ret == 0) ? "succ!" : "fail!") << endl;
			iter++;
		}
		if (m_OnUserLogin != NULL)
		{
			m_OnUserLogin(m_adapterID);
		}
		m_status = ADAPTER_STATUS_LOGIN;
	}
};

void quoteAdapter_CTP::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	LOG(INFO)  << m_adapterID << ": quoteAdapter_CTP logout!" << endl;

	m_status = ADAPTER_STATUS_LOGOUT;
	if (m_OnUserLogout != NULL)
	{
		m_OnUserLogout(m_adapterID);
	}
}

int quoteAdapter_CTP::SubscribeMarketData(char * pInstrumentList)
{
	vector<char*> list;
	char *token = strtok(pInstrumentList, ",");
	while (token != NULL){
		list.push_back(token);
		token = strtok(NULL, ",");
	}
	unsigned int len = list.size();
	char** pInstId = new char*[len];
	for (unsigned int i = 0; i < len; i++)
	{
		boost::mutex::scoped_lock lock(m_instrumentList_lock);

		pInstId[i] = list[i];
		string instTemp = string(list[i]);
		auto iter = find(m_instrumentList.begin(), m_instrumentList.end(), instTemp);
		if (iter == m_instrumentList.end())
			m_instrumentList.push_back(instTemp);
	}
	int ret = m_pUserApi->SubscribeMarketData(pInstId, len);

#ifdef ADAPTER_LOGGING
	LOG(INFO)  << m_adapterID << ":  req | subscribe quote ... " << ((ret == 0) ? "succ" : "fail") << endl;
#else
	cout << m_adapterID << ":  req | subscribe quote ... " << ((ret == 0) ? "succ" : "fail") << endl;
#endif
	
	return ret;
};


void quoteAdapter_CTP::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo,
	int nRequestID, bool bIsLast)
{
	if (isErrorRespInfo(pRspInfo))
		LOG(INFO)  << m_adapterID << ":  resp | subscribe quote " << (isErrorRespInfo(pRspInfo) ? "fail:" : "succ: ") << pSpecificInstrument->InstrumentID << endl;
};


void quoteAdapter_CTP::UnSubscribeMarketData(char * pInstrumentList)
{
	vector<char*> list;
	char *token = strtok(pInstrumentList, ",");
	while (token != NULL){
		list.push_back(token);
		token = strtok(NULL, ",");
	}
	unsigned int len = list.size();
	char** pInstId = new char*[len];
	for (unsigned int i = 0; i < len; i++)
	{
		pInstId[i] = list[i];
		string instTemp = string(list[i]);
		auto iter = find(m_instrumentList.begin(), m_instrumentList.end(), instTemp);
		if (iter == m_instrumentList.end())
			m_instrumentList.push_back(instTemp);
	}
	int ret = m_pUserApi->UnSubscribeMarketData(pInstId, len);
	LOG(INFO)  << m_adapterID << ":  req | cancel quote subscription ... " << ((ret == 0) ? "succ" : "fail") << endl;
	return;
};

void quoteAdapter_CTP::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
	//CThostFtdcDepthMarketDataField *pMarketData = new CThostFtdcDepthMarketDataField(*pDepthMarketData);
	/*LOG(INFO) << " ���� | ��Լ:" << pDepthMarketData->InstrumentID
	<< ", ��ͣ�ۣ�"<<pDepthMarketData->UpperLimitPrice
	<< ", ��ͣ�ۣ�"<<pDepthMarketData->LowerLimitPrice<<endl;*/
	//	<< " �ּ�:" << pDepthMarketData->LastPrice
	//	<< " ��߼�:" << pDepthMarketData->HighestPrice
	//	<< " ��ͼ�:" << pDepthMarketData->LowestPrice
	//	<< " ��һ��:" << pDepthMarketData->AskPrice1
	//	<< " ��һ��:" << pDepthMarketData->AskVolume1
	//	<< " ��һ��:" << pDepthMarketData->BidPrice1
	//	<< " ��һ��:" << pDepthMarketData->BidVolume1
	//	<< " �ֲ���:" << pDepthMarketData->OpenInterest << endl;

#ifdef	ADAPTER_LOGGING
	LOG(INFO) << "," << m_adapterID << ",����,tradingDate:"<< pDepthMarketData ->TradingDay<<",instrument:" << pDepthMarketData->InstrumentID << ",lastPrice:" << pDepthMarketData->LastPrice << endl;
#else
	cout << m_adapterID << ",����,tradingDate:"<< pDepthMarketData ->TradingDay<<",instrument:" << pDepthMarketData->InstrumentID << ",lastPrice:" << pDepthMarketData->LastPrice << endl;
#endif

	if (m_onRtnMarketData != NULL)
		m_onRtnMarketData(m_adapterID, pDepthMarketData);
};

void quoteAdapter_CTP::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	LOG(INFO)  << " resp |  error response requestID: " << nRequestID
		<< ", ErrorID: " << pRspInfo->ErrorID << ", ErrorMsg: " << pRspInfo->ErrorMsg
		<< endl;
};

bool quoteAdapter_CTP::isErrorRespInfo(CThostFtdcRspInfoField *pRspInfo)
{
	if (pRspInfo == nullptr || pRspInfo->ErrorID != 0)
		return true;
	return false;
};