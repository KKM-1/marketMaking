#include "strategy/cmMM01.h"
#include "glog\logging.h"

bool cmMM01::isInOpenTime()
{
	time_t t;
	tm* local;
	t = time(NULL);
	local = localtime(&t);
	int timeSec = (local->tm_hour * 100 + local->tm_min) * 100 + local->tm_sec;
	for (auto item : m_openTimeList)
		if (item.first <= timeSec && timeSec <= item.second)
			return true;
	return false;
};

bool cmMM01::isOrderComplete(int orderRef, int& tradedVol)
{
	bool isTrdComplete = true;
	int orderTradedVol = 0;
	auto iter01 = m_orderRef2orderRtn.find(orderRef);
	if (iter01 == m_orderRef2orderRtn.end() ||
		iter01->second->m_orderStatus == ORDER_STATUS_Unknown)
	{
		m_tradeTP->getDispatcher().post(boost::bind(&infrastructure::queryOrder,
			m_infra, m_tradeAdapterID)); //��������ر�δ���ػ�״̬δ֪�����𱨵���ѯ
		return false;
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
		//m_tradeTP->getDispatcher().post(boost::bind(&infrastructure::cancelOrder, m_infra,
		//	m_tradeAdapterID, orderRef, bind(&cmMM01::onRspCancel, this, _1)));
		m_infra->cancelOrder(m_tradeAdapterID, orderRef, bind(&cmMM01::onRspCancel, this, _1));
		isTrdComplete = false;
		break;
	}
	//�������漸��״̬������Գ���
	case ORDER_STATUS_Canceled: ///����,
	case ORDER_STATUS_AllTraded:///ȫ���ɽ�,
	{
		orderTradedVol += iter01->second->m_volumeTraded;
		break;
	}
	}//end:����ÿ��״̬

	int tradeRtnedVol = 0; //ͳ�Ʊ�order���صĳɽ���
	{
		auto iter02 = m_orderRef2tradeRtn.find(orderRef);
		if (iter02 != m_orderRef2tradeRtn.end())
		{
			for each(auto item2 in iter02->second)
				tradeRtnedVol += item2.second->m_volume;
		}
	}
	//����гɽ���������orderRtn�еĳɽ�����tradeRtn�е�����һ�£�
	// ��ʾ����tradeRtnû�յ�����Ϊ����δ���
	if (orderTradedVol != 0 && tradeRtnedVol != orderTradedVol)
		isTrdComplete = false;
	tradedVol += orderTradedVol;
	return isTrdComplete;
};

void cmMM01::daemonEngine(){

	if (!isInOpenTime())
	{
		if (STRATEGY_STATUS_INIT != m_strategyStatus)
		{
			write_lock lock(m_breakReqLock);
			m_breakReq = true;
		}
		m_daemonTimer.expires_from_now(boost::posix_time::millisec(1000 * 60));
		m_daemonTimer.async_wait(boost::bind(&cmMM01::daemonEngine, this));
		return;
	}
	{
		boost::recursive_mutex::scoped_lock lock(m_strategyStatusLock);
		switch (m_strategyStatus)
		{
		case STRATEGY_STATUS_INIT:
		case STRATEGY_STATUS_BREAK:
		{
			write_lock lock(m_breakReqLock);
			m_breakReq = false;
			startStrategy();
			break;
		}
		}
	}

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

	read_lock lock2(m_orderRtnBuffLock);
	read_lock lock3(m_tradeRtnBuffLock);
	int totalTradedVol = 0; //����ɵıջ���ȫ���ɽ���
	for each(auto item in m_tradeGrpBuffer)
	{
		bool isTrdGrpComplete = true;
		int  absTradedVol = 0;
		for each(auto orderRef in item->m_orderIdList)
		{
			if (!isOrderComplete(orderRef, absTradedVol))
				isTrdGrpComplete = false;
		}//end: ѭ������ÿһ��order

		if (!isTrdGrpComplete)//�ջ�δ���
			m_aliveTrdGrp.push_back(item);
		else if (absTradedVol != 0)//�ջ����, ���н��׷���
		{
			LOG(INFO) << m_strategyId<< ": -----------CycleStatisticsStart-----------" << endl;
			int  cycleTradedVol = 0;
			double cycleProfit = 0.0;
			for each(auto orderRef in item->m_orderIdList)
			{
				int orderTradedVol = 0;
				orderRtnPtr pOrder = m_orderRef2orderRtn[orderRef];
				cycleTradedVol += pOrder->m_direction == ORDER_DIR_BUY ?
					pOrder->m_volumeTraded : (pOrder->m_volumeTraded * -1);
				auto iter03 = m_orderRef2tradeRtn.find(orderRef);
				if (iter03 != m_orderRef2tradeRtn.end())
				{
					for each(auto item2 in iter03->second)
					{
						double orderProfit = pOrder->m_direction == ORDER_DIR_SELL ?
							(item2.second->m_price * item2.second->m_volume * m_volumeMultiple) : 
							(item2.second->m_price * item2.second->m_volume * m_volumeMultiple*-1);
						LOG(INFO) << m_strategyId << ",tradeInfo," << item2.second->m_orderRef
							<< "," << item2.second->m_tradeId << "," << item2.second->m_instId
							<< "," << item2.second->m_orderDir << "," << item2.second->m_price
							<< "," << item2.second->m_volume << "," << orderProfit << endl;
						cycleProfit += orderProfit;
					}
				}

			}//end: ѭ������ÿһ��order
			totalTradedVol += cycleTradedVol;
			LOG(INFO) << m_strategyId << ",cycleProfit," << item->m_Id
				<< "," << cycleProfit << endl;
			LOG(INFO) << m_strategyId << "-----------CycleStatisticsEnd-----------" << endl;
		}

	} //end: ѭ������ÿһ���ջ�

	if (totalTradedVol != 0)
	{
		m_cycleHedgeVol = totalTradedVol;
		interruptMM(boost::bind(&cmMM01::sendCycleNetHedgeOrder, this));
	}

	m_daemonTimer.expires_from_now(boost::posix_time::millisec(1000*60)); //ÿ��������һ��
	m_daemonTimer.async_wait(boost::bind(&cmMM01::daemonEngine, this));
};

void cmMM01::sendCycleNetHedgeOrder()
{
	int hedgeVol = m_cycleHedgeVol * -1;
	sendCycleNetHedgeOrder(hedgeVol);
};

void cmMM01::sendCycleNetHedgeOrder(int hedgeVol)
{
	enum_order_dir_type dir = hedgeVol > 0.0 ? ORDER_DIR_BUY : ORDER_DIR_SELL;
	double price = (dir == ORDER_DIR_BUY) ?
		m_lastQuotePtr->UpperLimitPrice : m_lastQuotePtr->LowerLimitPrice;
	int cycleNetHedgeOrderRef = m_infra->insertOrder(m_tradeAdapterID, m_productId, m_exchange,
		ORDER_TYPE_LIMIT, dir, POSITION_EFFECT_OPEN, FLAG_SPECULATION, price, abs(hedgeVol),
		bind(&cmMM01::onCycleNetHedgeOrderRtn, this, _1),
		bind(&cmMM01::onCycleNetHedgeTradeRtn, this, _1));
	if (cycleNetHedgeOrderRef > 0)
	{
		write_lock lock0(m_orderRef2cycleRWlock);
		m_orderRef2cycle[cycleNetHedgeOrderRef] = m_cycleId;
		LOG(INFO) << m_strategyId << ": send cycle net hedge order succ. code: " << m_productId
			<< ", dir: " << ((dir == ORDER_DIR_BUY) ? "buy, vol: " : "sell, vol: ")
			<< abs(hedgeVol) << endl;
		{
			boost::mutex::scoped_lock lock(m_cycleNetHedgeVolLock);
			m_cycleNetHedgeVol = hedgeVol;
		}
	}
	else
		LOG(INFO) << m_strategyId << " | ERROR: send cycle net hedge order failed, rc = " << cycleNetHedgeOrderRef << endl;
};
void cmMM01::processCycleNetHedgeOrderRtn(orderRtnPtr pOrder){
	write_lock lock(m_orderRtnBuffLock);
	m_orderRef2orderRtn[pOrder->m_orderRef] = pOrder;
};

void cmMM01::processCycleNetHedgeTradeRtn(tradeRtnPtr ptrade)
{
	double orderProfit = ptrade->m_orderDir == ORDER_DIR_SELL ?
		(ptrade->m_price * ptrade->m_volume * m_volumeMultiple) :
		(ptrade->m_price * ptrade->m_volume * m_volumeMultiple*-1);
	LOG(INFO) << m_strategyId << ",cycleNetHedgeTrade," << ptrade->m_orderRef
		<< "," << ptrade->m_tradeId << "," << ptrade->m_instId
		<< "," << ptrade->m_orderDir << "," << ptrade->m_price
		<< "," << ptrade->m_volume << "," << orderProfit << endl;
	{
		boost::mutex::scoped_lock lock(m_cycleNetHedgeVolLock);
		m_cycleNetHedgeVol -= ptrade->m_orderDir == ORDER_DIR_BUY ? ptrade->m_volume :
			(ptrade->m_volume * -1);
		if (m_cycleNetHedgeVol == 0)
			resumeMM();
	}
};

void cmMM01::interruptMM(boost::function<void()> pauseHandler)
{
	if (!pauseMM(pauseHandler) && m_strategyStatus != STRATEGY_STATUS_BREAK)
	{
		m_pauseLagTimer.expires_from_now(boost::posix_time::millisec(1000));
		m_pauseLagTimer.async_wait(boost::bind(&cmMM01::interruptMM, this, pauseHandler));
	}
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