#include "strategy/cmMM01.h"
#include "glog\logging.h"

void cmMM01::daemonEngine(){
	m_tradeGrpBuffer.clear();

	for each(auto item in m_aliveTrdGrp)
		m_tradeGrpBuffer.push_back(item);
	m_aliveTrdGrp.clear();
	{
		boost::mutex::scoped_lock lock(m_cycle2tradeGrpLock);
		for each(auto item in m_cycle2tradeGrp)
			m_tradeGrpBuffer.push_back(item.second);
		m_cycle2tradeGrp.clear();
	}

	for each(auto item in m_tradeGrpBuffer)
	{
		map<int, bool> isOrderComplete;
		for each(auto orderRef in item->m_orderIdList)
		{
			map < int, orderRtnPtr>::iterator iter01;
			{
				read_lock lock2(m_orderRtnBuffLock);
				iter01 = m_orderRef2orderRtn.find(orderRef); 
				if (iter01 == m_orderRef2orderRtn.end() || 
					iter01->second->m_orderStatus == ORDER_STATUS_Unknown)
				{
					m_infra->queryOrder(m_tradeAdapterID); //��������ر�δ���ػ�״̬δ֪�����𱨵���ѯ
					continue;
				}
			}
			switch (iter01->second->m_orderStatus)
			{
				//�������漸��״̬������
				case ORDER_STATUS_PartTradedQueueing:///���ֳɽ����ڶ�����,
				case ORDER_STATUS_PartTradedNotQueueing:///���ֳɽ����ڶ�����,
				case ORDER_STATUS_NoTradeQueueing:///δ�ɽ����ڶ�����,
				case ORDER_STATUS_NoTradeNotQueueing:///δ�ɽ����ڶ�����,
				case ORDER_STATUS_NotTouched:///��δ����,
				case ORDER_STATUS_Touched:///�Ѵ���,
				{
					m_infra->cancelOrder(m_tradeAdapterID, orderRef,bind(&cmMM01::onRspCancel, this, _1));
					break;
				}
				//�������漸��״̬������Գ���
				case ORDER_STATUS_Canceled: ///����,
				case ORDER_STATUS_AllTraded:///ȫ���ɽ�,
				{
					break;
				}
			}
		}
	}

	m_daemonTimer.expires_from_now(boost::posix_time::millisec(1000*60)); //ÿ��������һ��
	m_daemonTimer.async_wait(boost::bind(&cmMM01::daemonEngine, this));
};

bool cmMM01::pauseMM(boost::function<void()> pauseHandler)
{
	write_lock lock(m_pauseReqLock);
	if (m_pauseReq)
		return false;
	else
	{
		m_pauseReq = true;
		m_oneTimeMMPausedHandler = pauseHandler;
		return true;
	}
};

void cmMM01::callPauseHandler()
{
	if (m_oneTimeMMPausedHandler)
	{
		m_oneTimeMMPausedHandler();
		m_oneTimeMMPausedHandler = NULL;
	}
};

void cmMM01::resumeMM()
{
	{
		boost::recursive_mutex::scoped_lock lock(m_strategyStatusLock); 
		write_lock lock1(m_pauseReqLock);
		m_strategyStatus = STRATEGY_STATUS_READY;
		m_pauseReq = false; 
		cout << m_strategyId << " resumed." << endl;
	}
};