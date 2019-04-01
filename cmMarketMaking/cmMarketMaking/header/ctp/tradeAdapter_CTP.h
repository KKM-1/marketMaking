#pragma once
#include <iostream>
#include <map>
#include "ctp/ThostFtdcTraderApi.h"
#include "threadpool/threadpool.h"
#include <boost/shared_ptr.hpp>
#include <boost/smart_ptr/detail/spinlock.hpp>
#include <boost\function.hpp>
#include <boost\asio.hpp>
#include <boost\bind.hpp>
#include "baseClass/adapterBase.h"

using namespace std;

typedef boost::shared_ptr<CThostFtdcOrderField> CThostFtdcOrderFieldPtr;

class  tradeAdapterCTP : public traderAdapterBase, public CThostFtdcTraderSpi
{

private:
	int m_requestId = 0;
	int m_frontId;    //ǰ�ñ��
	int m_sessionId;    //�Ự���
	char m_orderRef[13];
	map<int, CThostFtdcOrderFieldPtr> m_ref2order;
	boost::detail::spinlock m_ref2order_lock;
	CThostFtdcTraderApi* m_pUserApi;
	CThostFtdcReqUserLoginField m_loginField;
	bool m_needAuthenticate;
	CThostFtdcReqAuthenticateField m_authenticateField;

public:
	tradeAdapterCTP(string adapterID, char* tradeFront, char* broker, char* user, char* pwd, 
		athenathreadpoolPtr tp);
	tradeAdapterCTP(string adapterID, char* tradeFront, char* broker, char* user, char* pwd,
		char * userproductID, char * authenticateCode, athenathreadpoolPtr tp);

	virtual void destroyAdapter();

	int init();
	int login();

	int queryTradingAccount();//��ѯ�ʽ�
	int queryInvestorPosition();//��ѯ�ֲ�
	int queryAllInstrument();//��ѯȫ����Լ

	//�µ�
	virtual int OrderInsert(string instrument, char priceType, char dir,
		char ComOffsetFlag, char ComHedgeFlag, double price,
		int volume, char tmCondition, char volCondition, int minVol, char contiCondition,
		double stopPrz, char forceCloseReason);
	//����
	virtual void cancelOrder(int orderRef);

private:
	athenathreadpoolPtr m_threadpool;
	boost::asio::deadline_timer m_lag_Timer;

public:
	boost::function<void(string adapterID)> m_OnUserLogin;
	boost::function<void(string adapterID)> m_OnUserLogout;
	boost::function<void(string adapterID, string adapterType)> m_OnFrontDisconnected;
	boost::function<void(string, CThostFtdcOrderField*)> m_OnOrderRtn;
	boost::function<void(string, CThostFtdcTradeField*)> m_OnTradeRtn;
	boost::function<void(string, CThostFtdcInstrumentField*)> m_OnInstrumentsRtn;
	boost::function<void(CThostFtdcInvestorPositionField*)> m_OnInvestorPositionRtn;

private:
	bool isErrorRespInfo(CThostFtdcRspInfoField *pRspInfo);

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

	///�ͻ�����֤��Ӧ
	virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///��¼������Ӧ
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///�ǳ�������Ӧ
	virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///�����ѯ�ʽ��˻���Ӧ
	virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	///�����ѯͶ���ֲ߳���Ӧ
	virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	///�����ѯ��Լ��Ӧ
	virtual void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///����¼��������Ӧ
	virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	///����¼�����ر�
	virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo);

	///����֪ͨ
	virtual void OnRtnOrder(CThostFtdcOrderField *pOrder);

	///�ɽ�֪ͨ
	virtual void OnRtnTrade(CThostFtdcTradeField *pTrade);

	///��������������Ӧ
	virtual void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	///������������ر�
	virtual void OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo);

};
