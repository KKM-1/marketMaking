#pragma once
#include <list>
#include <iostream>
#include "infrastructure.h"
#include "threadpool\threadpool.h"
#include "baseClass/strategyBase.h"
#include "baseClass/orderBase.h"
#include "baseClass/Utils.h"
#include <boost\circular_buffer.hpp>

using namespace std;

#define apcosm_Buff_size 100

enum enum_cmSepc01_strategy_status
{//����״̬�������Ƿ����µ�ί��
	STRATEGY_STATUS_INIT,
	STRATEGY_STATUS_START,
	STRATEGY_STATUS_STOP,
};

typedef boost::circular_buffer<double> cirBuff;

class cmSepc01 : public strategyBase
{
private:
	string m_strategyId;
	string m_strategyTyp;
	string m_productId;
	string m_exchange;
	string m_quoteAdapterID;
	string m_tradeAdapterID;
	double m_tickSize;
	double m_orderQty;
	int    m_volumeMultiple;

	infrastructure* m_infra;
	Json::Value m_strategyConfig;
	athenathreadpoolPtr m_quoteTP;
	athenathreadpoolPtr m_tradeTP;

	list< pair <int, int> > m_openTimeList;
	bool isInOpenTime();

private:
	enum_cmSepc01_strategy_status m_strategyStatus;

public:
	cmSepc01(string strategyId, string strategyTyp, string productId, string exchange,
		string quoteAdapterID, string tradeAdapterID, double tickSize, 
		double orderQty, int volMulti,
		athenathreadpoolPtr quoteTP, athenathreadpoolPtr tradeTP, infrastructure* infra,
		Json::Value config);
	~cmSepc01();
	virtual void startStrategy();
	virtual void stopStrategy(){};


public: //���ⲿ���õ���Ӧ���� | �ڲ����̳߳��е�����Ӧ�Ĵ�����
	void onRtnMD(futuresMDPtr);
	void onOrderRtn(orderRtnPtr pOrder){ m_tradeTP->getDispatcher().post(bind(&cmSepc01::processOrder, this, pOrder)); };
	void onTradeRtn(tradeRtnPtr ptrade){ m_tradeTP->getDispatcher().post(bind(&cmSepc01::processTrade, this, ptrade)); };
	void onRspCancel(cancelRtnPtr pCancel){ m_tradeTP->getDispatcher().post(bind(&cmSepc01::processCancelRes, this, pCancel)); };

private:

	boost::mutex m_lastQuoteLock;
	futuresMDPtr m_lastQuotePtr;
	double m_upline;
	double m_downline;
	double m_lastprice;
	double m_lastprice_1;
	cirBuff m_yieldBuff_short{ 10 };
	cirBuff m_yieldBuff_long{ 10 * 20 };
	cirBuff m_avg_true_range{ 10 * 20 };
	cirBuff m_Apcosm_Buff{ apcosm_Buff_size };
	void quoteEngine();

private:
	int m_bidOrderRef;
	int m_askOrderRef;
	void startCycle();
	void refreshCycle();
	void orderPrice(double* bidprice, double* askprice); //����ҵ��۸�
	void processOrder(orderRtnPtr);
	void processTrade(tradeRtnPtr);
	void daemonEngine(); //�ػ��߳�����

private:
	int m_cancelBidOrderRC;
	int m_cancelAskOrderRC;
	void processCancelRes(cancelRtnPtr);

private: // for clear cycle
	tradeGroupBufferPtr     m_ptradeGrp; //���ڼ�¼�������ױջ������б�����
	map < int, int >        m_orderRef2cycle;     //orderRef -> cycle Id
	boost::shared_mutex     m_orderRef2cycleRWlock; //����m_orderRef2cycle�Ķ�д��

	list<tradeGroupBufferPtr>      m_tradeGrpBuffer;//����ÿ�����ױջ������ֳ�
	list<tradeGroupBufferPtr>      m_aliveTrdGrp;//����ÿ�����ױջ������ֳ�
	map <int, tradeGroupBufferPtr> m_cycle2tradeGrp;//cycle Id -> trade group pointer
	boost::mutex            m_cycle2tradeGrpLock;   //���ڻ������m_cycle2tradeGrp
	void registerTrdGrpMap(int cycleId, tradeGroupBufferPtr pGrp){ //��m_cycle2tradeGrp�в����¼
		boost::mutex::scoped_lock lock(m_cycle2tradeGrpLock);
		m_cycle2tradeGrp[cycleId] = pGrp;
	};

	int  m_cycleHedgeVol;
	bool isOrderComplete(int orderRef, int& tradedVol);
	void sendCycleNetHedgeOrder();
	void sendCycleNetHedgeOrder(int);
	void processCycleNetHedgeOrderRtn(orderRtnPtr);
	void processCycleNetHedgeTradeRtn(tradeRtnPtr);
	athena_lag_timer m_daemonTimer;
	double        m_cycleNetHedgeVol;
	boost::mutex  m_cycleNetHedgeVolLock;
};