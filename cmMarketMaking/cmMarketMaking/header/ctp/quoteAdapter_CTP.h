#pragma once
#include <vector>
#include "ctp/ThostFtdcMdApi.h"
#include "baseClass/adapterBase.h"
#include <boost\thread\mutex.hpp>
#include <boost\bind.hpp>
#include <boost\function.hpp>

using namespace std;

class quoteAdapter_CTP : public quoteAdapterBase, public CThostFtdcMdSpi
{
public:
	quoteAdapter_CTP(string adapterID, char * mdFront, char * broker, char * user, char * pwd);
	virtual void destroyAdapter();
	int init();
	int login();

private:
	int SubscribeMarketData(char * pInstrumentList);
	void UnSubscribeMarketData(char * pInstrumentList);

public:
	virtual void Subscribe(string instIdList, string exchange)
	{
		SubscribeMarketData((char*)instIdList.c_str());
	};
	virtual void UnSubscribe(string instIdList, string exchange)
	{
		UnSubscribeMarketData((char*)instIdList.c_str());
	};

public:

	///���ͻ����뽻�׺�̨������ͨ������ʱ����δ��¼ǰ�����÷��������á�
	virtual void OnFrontConnected();

	///���ͻ����뽻�׺�̨ͨ�����ӶϿ�ʱ���÷��������á���������������API���Զ��������ӣ��ͻ��˿ɲ�������
	///@param nReason ����ԭ��
	///        0x1001 �����ʧ��
	///        0x1002 ����дʧ��
	///        0x2001 ����������ʱ
	///        0x2002 ��������ʧ��
	///        0x2003 �յ�������
	virtual void OnFrontDisconnected(int nReason);

	///������ʱ���档����ʱ��δ�յ�����ʱ���÷��������á�
	///@param nTimeLapse �����ϴν��ձ��ĵ�ʱ��
	virtual void OnHeartBeatWarning(int nTimeLapse);

	///��¼������Ӧ
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///�ǳ�������Ӧ
	virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///����Ӧ��
	virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///��������Ӧ��
	virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///ȡ����������Ӧ��
	virtual void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

	///����ѯ��Ӧ��
	virtual void OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

	///ȡ������ѯ��Ӧ��
	virtual void OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

	///�������֪ͨ
	virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData);

	///ѯ��֪ͨ
	virtual void OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp) {};

public:

	//boost::function<void()> m_onLogin;
	boost::function<void(string, CThostFtdcDepthMarketDataField*)> m_onRtnMarketData;
	boost::function<void(string adapterID)> m_OnUserLogin;
	boost::function<void(string adapterID)> m_OnUserLogout;
	boost::function<void(string adapterID, string adapterType)> m_OnFrontDisconnected;

private:
	int m_requestId = 0;
	CThostFtdcMdApi* m_pUserApi;
	CThostFtdcReqUserLoginField m_loginField;

	vector<string> m_instrumentList;
	//boost::detail::spinlock m_instrumentList_lock;
	boost::mutex m_instrumentList_lock;

private:
	bool isErrorRespInfo(CThostFtdcRspInfoField *pRspInfo);
};
