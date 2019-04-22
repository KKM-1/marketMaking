#pragma once
#include <iostream>
#include <map>
#include "baseClass/orderBase.h"
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
typedef boost::shared_ptr<CThostFtdcInputOrderField> CThostFtdcInputOrderFieldPtr;

class  tradeAdapterCTP : public traderAdapterBase, public CThostFtdcTraderSpi
{

private:
	int m_requestId = 0;
	//int m_frontId;    //ǰ�ñ��
	//int m_sessionId;    //�Ự���



	map<int, CThostFtdcInputOrderFieldPtr> m_ref2sentOrder;
	boost::detail::spinlock     m_ref2sentOrder_lock;
	map<int, CThostFtdcOrderFieldPtr> m_ref2order;
	boost::mutex                      m_ref2order_lock;

	CThostFtdcTraderApi*        m_pUserApi;
	CThostFtdcReqUserLoginField m_loginField;
	bool                           m_needAuthenticate;
	CThostFtdcReqAuthenticateField m_authenticateField;

private:

	char             m_orderRef[13];
	boost::mutex     m_orderRefLock;
	int updateOrderRef(){
		int nextOrderRef = atoi(m_orderRef)+1;
		sprintf(m_orderRef, "%012d", nextOrderRef);
		return nextOrderRef;
	};

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
	int confirmSettlementInfo();//ȷ�Ͻ�����

	//�µ�
	virtual int OrderInsert(string instrument, string exchange, char priceType, char dir,
		char ComOffsetFlag, char ComHedgeFlag, double price,
		int volume, char tmCondition, char volCondition, int minVol, char contiCondition,
		double stopPrz, char forceCloseReason);
	//����
	virtual int cancelOrder(int orderRef);

private:
	bool m_qryingOrder; //������ѯ������ʶ
	boost::mutex     m_qryOrderLock;
	athena_lag_timer    m_qryOrder_Timer;
	bool m_cancelQryTimer;
	void openOrderQrySwitch(){ if (m_cancelQryTimer) m_cancelQryTimer = false; 
		                       else m_qryingOrder = false;};
	void closeOrderQrySwitch(){ m_qryingOrder = true; };
public:
	void queryOrder();

private:
	athenathreadpoolPtr m_threadpool;
	athena_lag_timer    m_lag_Timer;
public:
	boost::function<void(string adapterID)> m_OnUserLogin;
	boost::function<void(string adapterID)> m_OnUserLogout;
	boost::function<void(string adapterID, string adapterType)> m_OnFrontDisconnected;
	boost::function<void(string, CThostFtdcOrderField*)> m_OnOrderRtn;
	boost::function<void(string, CThostFtdcTradeField*)> m_OnTradeRtn;
	boost::function<void(string, CThostFtdcInstrumentField*)> m_OnInstrumentsRtn;
	boost::function<void(CThostFtdcInvestorPositionField*)> m_OnInvestorPositionRtn;
	boost::function < void(string adapterID, CThostFtdcInputOrderActionField *pInputOrderAction,
		CThostFtdcRspInfoField *pRspInfo) > m_onRspCancel;

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
	///Ͷ���߽�����ȷ����Ӧ
	virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///����¼��������Ӧ
	virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	///����¼�����ر�
	virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo);

	///�����ѯ������Ӧ
	virtual void OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///����֪ͨ
	virtual void OnRtnOrder(CThostFtdcOrderField *pOrder);

	///�ɽ�֪ͨ
	virtual void OnRtnTrade(CThostFtdcTradeField *pTrade);

	///��������������Ӧ
	virtual void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	///������������ر�
	virtual void OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo);

};
