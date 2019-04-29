#include "strategy/cmMM01.h"
#include "glog\logging.h"

cmMM01::cmMM01(string strategyId, string strategyTyp, string productId, string exchange,
	string quoteAdapterID, string tradeAdapterID, double tickSize, double miniOrderSpread,
	double orderQty, int volMulti,
	athenathreadpoolPtr quoteTP, athenathreadpoolPtr tradeTP, infrastructure* infra,
	Json::Value config)
	:m_strategyId(strategyId), m_strategyTyp(strategyTyp), m_productId(productId), m_exchange(exchange),
	m_quoteAdapterID(quoteAdapterID), m_tradeAdapterID(tradeAdapterID), m_tickSize(tickSize),
	m_miniOrderSpread(miniOrderSpread), m_orderQty(orderQty), m_volumeMultiple(volMulti),
	m_quoteTP(quoteTP), m_tradeTP(tradeTP), m_infra(infra), m_cycleId(0), m_pauseReq(false),
	m_breakReq(false), m_strategyConfig(config),
	m_cancelConfirmTimer(tradeTP->getDispatcher()), m_cancelHedgeTimer(tradeTP->getDispatcher()),
	m_daemonTimer(tradeTP->getDispatcher()), m_pauseLagTimer(tradeTP->getDispatcher())
{
	int openNum = m_strategyConfig["openTime"].size();
	for (int i = 0; i < openNum; ++i)
	{
		Json::Value openInterval = m_strategyConfig["openTime"][i];
		int startTime = openInterval["start"].asInt() * 100 +1;
		int endTime = openInterval["end"].asInt() * 100 + 00;
		m_openTimeList.push_back(make_pair(startTime, endTime));
	}
	m_strategyStatus = STRATEGY_STATUS_INIT;
	daemonEngine();
};

void cmMM01::startStrategy(){
	cout << m_strategyId << " starting..." << endl;
	if (STRATEGY_STATUS_INIT == m_strategyStatus)
	{
		m_infra->subscribeFutures(m_quoteAdapterID, m_exchange, m_productId, bind(&cmMM01::onRtnMD, this, _1));
	}
	m_strategyStatus = STRATEGY_STATUS_READY;
};

void cmMM01::resetStrategyStatus(){ //�ȴ����鴥��cycle
	m_strategyStatus = STRATEGY_STATUS_READY;
};

//���鴦��
//    ������Դ��� READY      ״̬���µ�
//    ������Դ��� ORDER_SENT ״̬�������������µ�
void cmMM01::quoteEngine()
{
	boost::recursive_mutex::scoped_lock lock(m_strategyStatusLock);
	LOG(INFO) << m_strategyId << " | status: " << m_strategyStatus << endl;
	switch (m_strategyStatus)
	{
	case STRATEGY_STATUS_READY:
	{
		startCycle();
		break;
	}
	case STRATEGY_STATUS_ORDER_SENT:
	{
		m_strategyStatus = STRATEGY_STATUS_CLOSING_POSITION;
		m_tradeTP->getDispatcher().post(boost::bind(&cmMM01::refreshCycle, this)); //,boost::asio::placeholders::error));
		break;
	}
	case STRATEGY_STATUS_PAUSE:
	{
		cout << m_strategyId << ": market making paused!" << endl;
		break;
	}
	case STRATEGY_STATUS_BREAK:
	{
		cout << m_strategyId << " breaking ..." << endl;
		break;
	}
	}
};

void cmMM01::orderPrice(double* bidprice, double* askprice)
{
	futuresMDPtr plastQuote;
	{
		boost::mutex::scoped_lock lock(m_lastQuoteLock);
		plastQuote = m_lastQuotePtr;
	}
	int quoteSpread = round((plastQuote->askprice[0] - plastQuote->bidprice[0]) / m_tickSize);
	if (quoteSpread > m_miniOrderSpread) return;
	switch (quoteSpread)
	{
	case 1:
	case 2:
	{
		*bidprice = plastQuote->bidprice[0] - m_tickSize;
		break;
	}
	case 3:
	case 4:
	{
		*bidprice = plastQuote->bidprice[0];
		break;
	}
	}
	//*bidprice = plastQuote->bidprice[0] + int((quoteSpread - m_miniOrderSpread) / 2) * m_tickSize;
	//*bidprice = plastQuote->askprice[0]; //���Գɽ�
	*askprice = *bidprice + m_tickSize * m_miniOrderSpread;
};

void cmMM01::startCycle()
{
	{
		read_lock lock(m_breakReqLock);
		if (m_breakReq)
		{
			m_strategyStatus = STRATEGY_STATUS_BREAK; //interrupt�����������鴥���µĽ���
			return;
		}
	}
	{
		read_lock lock(m_pauseReqLock);
		if (m_pauseReq)
		{
			m_strategyStatus = STRATEGY_STATUS_PAUSE; //interrupt�����������鴥���µĽ���
			callPauseHandler();
			return;
		}
	}

	m_cycleId++;
	m_bidOrderRef = 0;
	m_askOrderRef = 0;
	m_cancelBidOrderRC = 0;
	m_cancelAskOrderRC = 0;
	m_cancelConfirmTimerCancelled = false;
	m_cancelHedgeTimerCancelled = false;
	m_ptradeGrp = tradeGroupBufferPtr(new tradeGroupBuffer());
	m_ptradeGrp->m_Id = m_cycleId;
	LOG(INFO) << m_strategyId << ": starting new cycle." << endl;
	double bidprice = 0.0, askprice = 0.0;
	orderPrice(&bidprice, &askprice);
	if (0.0 == bidprice || 0.0 == askprice)
	{
		boost::recursive_mutex::scoped_lock lock(m_strategyStatusLock);
		m_strategyStatus = STRATEGY_STATUS_READY;
		LOG(INFO) << m_strategyId << ": warning | spread is too wide, no order sent." << endl;
		return;
	}
	m_bidOrderRef = m_infra->insertOrder(m_tradeAdapterID, m_productId, m_exchange, ORDER_TYPE_LIMIT,
		ORDER_DIR_BUY, POSITION_EFFECT_OPEN, FLAG_SPECULATION, bidprice, m_orderQty,
		bind(&cmMM01::onOrderRtn, this, _1), bind(&cmMM01::onTradeRtn, this, _1));
	if (m_bidOrderRef > 0)
	{
		write_lock lock0(m_orderRef2cycleRWlock);
		m_orderRef2cycle[m_bidOrderRef] = m_ptradeGrp;
		m_ptradeGrp->m_orderIdList.push_back(m_bidOrderRef);
	}

	m_askOrderRef = m_infra->insertOrder(m_tradeAdapterID, m_productId, m_exchange, ORDER_TYPE_LIMIT,
		ORDER_DIR_SELL, POSITION_EFFECT_OPEN, FLAG_SPECULATION, askprice, m_orderQty,
		bind(&cmMM01::onOrderRtn, this, _1), bind(&cmMM01::onTradeRtn, this, _1));
	if (m_askOrderRef > 0)
	{
		write_lock lock0(m_orderRef2cycleRWlock);
		m_orderRef2cycle[m_askOrderRef] = m_ptradeGrp;
		m_ptradeGrp->m_orderIdList.push_back(m_askOrderRef);
	}

	if (m_bidOrderRef == 0 || m_askOrderRef == 0)
		cout << "debug" << endl;
	m_strategyStatus = STRATEGY_STATUS_ORDER_SENT;
};

void cmMM01::refreshCycle()
{
	CancelOrder(true);
};

void cmMM01::CancelOrder(bool restart)//const boost::system::error_code& error)
{
	if (m_cancelConfirmTimerCancelled)
	{
		LOG(INFO) << m_strategyId << ": cancel order stopped, cycleId: " << m_cycleId << endl;
		return;
	}

	if (m_bidOrderRef == 0 || m_askOrderRef == 0)
		LOG(WARNING) << m_strategyId<<": order ref illegal." << endl;

	//�ȼ�鱨���ر������ٵ��߳�����ȡ��ORDER_CANCEL_ERROR_NOT_FOUND��Ӧ�߼�
	map < int, orderRtnPtr>::iterator bidOrderIter;
	map < int, orderRtnPtr>::iterator askOrderIter;
	{
		read_lock lock(m_orderRtnBuffLock);
		bidOrderIter = m_orderRef2orderRtn.find(m_bidOrderRef);
		askOrderIter = m_orderRef2orderRtn.find(m_askOrderRef);
		if (bidOrderIter == m_orderRef2orderRtn.end() ||
			askOrderIter == m_orderRef2orderRtn.end()){
			if (bidOrderIter == m_orderRef2orderRtn.end())
			{
				m_infra->queryOrder(m_tradeAdapterID, m_bidOrderRef);
				LOG(WARNING) << m_strategyId << ": order not found, querying order, orderRef: " << m_bidOrderRef << endl;
			}
			if (askOrderIter == m_orderRef2orderRtn.end())
			{
				m_infra->queryOrder(m_tradeAdapterID, m_askOrderRef);
				LOG(WARNING) << m_strategyId << ": order not found, querying order, orderRef: " << m_askOrderRef << endl;
			}
			
			m_cancelConfirmTimer.expires_from_now(boost::posix_time::milliseconds(1000 * 10));
			m_cancelConfirmTimer.async_wait(bind(&cmMM01::CancelOrder, this, restart));// , boost::asio::placeholders::error));
			return;
		}
	}

	if (m_cancelBidOrderRC == 0 || m_cancelBidOrderRC == ORDER_CANCEL_ERROR_NOT_FOUND ||
		m_cancelBidOrderRC == ORDER_CANCEL_ERROR_SEND_FAIL)
		m_cancelBidOrderRC = m_infra->cancelOrder(m_tradeAdapterID, m_bidOrderRef,
		bind(&cmMM01::onRspCancel, this, _1));
	if (m_cancelAskOrderRC == 0 || m_cancelAskOrderRC == ORDER_CANCEL_ERROR_NOT_FOUND ||
		m_cancelAskOrderRC == ORDER_CANCEL_ERROR_SEND_FAIL)
		m_cancelAskOrderRC = m_infra->cancelOrder(m_tradeAdapterID, m_askOrderRef,
		bind(&cmMM01::onRspCancel, this, _1));

	if (m_cancelBidOrderRC == ORDER_CANCEL_ERROR_NOT_FOUND || m_cancelAskOrderRC == ORDER_CANCEL_ERROR_NOT_FOUND)
	{
		if (m_cancelBidOrderRC == ORDER_CANCEL_ERROR_NOT_FOUND)
			LOG(WARNING) << m_strategyId << ": order not found in adapter, querying order, orderRef: " << m_bidOrderRef << endl;
	    if (m_cancelAskOrderRC == ORDER_CANCEL_ERROR_NOT_FOUND)
			LOG(WARNING) << m_strategyId << ": order not found in adapter, querying order, orderRef: " << m_askOrderRef << endl;
		m_infra->queryOrder(m_tradeAdapterID);
		m_cancelConfirmTimer.expires_from_now(boost::posix_time::milliseconds(1000*10));
		m_cancelConfirmTimer.async_wait(bind(&cmMM01::CancelOrder, this, restart));// , boost::asio::placeholders::error));
	}
	else if (m_cancelBidOrderRC == ORDER_CANCEL_ERROR_SEND_FAIL || m_cancelAskOrderRC == ORDER_CANCEL_ERROR_SEND_FAIL)
	{
		if (m_cancelBidOrderRC == ORDER_CANCEL_ERROR_SEND_FAIL)
			LOG(WARNING) << m_strategyId << ": send cancel failed, orderRef: " << m_bidOrderRef << endl;
		if (m_cancelAskOrderRC == ORDER_CANCEL_ERROR_SEND_FAIL)
			LOG(WARNING) << m_strategyId << ": send cancel failed, orderRef: " << m_askOrderRef << endl;
		m_cancelConfirmTimer.expires_from_now(boost::posix_time::milliseconds(1000*5));
		m_cancelConfirmTimer.async_wait(bind(&cmMM01::CancelOrder, this, restart)); // , boost::asio::placeholders::error));
	}
	else if (restart)
	{
		boost::recursive_mutex::scoped_lock lock(m_strategyStatusLock);
		if (STRATEGY_STATUS_CLOSING_POSITION == m_strategyStatus)
		{//�����µ�
			m_tradeTP->getDispatcher().post(bind(&cmMM01::registerTrdGrpMap, this, m_cycleId, m_ptradeGrp));
			startCycle();
		}
	}
};

void cmMM01::processOrder(orderRtnPtr pOrder)
{ 
	{
		write_lock lock(m_orderRtnBuffLock);
		m_orderRef2orderRtn[pOrder->m_orderRef] = pOrder;
	}
	tradeGroupBufferPtr ptradeGrp = nullptr; //���ڻ�ȡ��order���ڵıջ�
	{
		read_lock lock1(m_orderRef2cycleRWlock);
		auto iter = m_orderRef2cycle.find(pOrder->m_orderRef);
		if (iter != m_orderRef2cycle.end())
			ptradeGrp = iter->second;
	}
	if (ptradeGrp)
	{
		//��¼��Ч�ҵ�ʱ��
		if (ptradeGrp->m_start_milliSec == 0.0)
		{
			switch (pOrder->m_orderStatus)
			{
			case ORDER_STATUS_PartTradedQueueing:///���ֳɽ����ڶ�����,
			case ORDER_STATUS_NoTradeQueueing:///δ�ɽ����ڶ�����,
			case ORDER_STATUS_Touched:///�Ѵ���,
			{
				ptradeGrp->m_start_milliSec = UTC::GetMilliSecs();
				break; 
			}
			}
		}
		else if (ptradeGrp->m_end_milliSec == 0.0)
		{
			switch (pOrder->m_orderStatus)
			{
			case ORDER_STATUS_Canceled: ///����,
			case ORDER_STATUS_AllTraded:///ȫ���ɽ�,
			case ORDER_STATUS_PartTradedNotQueueing:///���ֳɽ����ڶ�����,
			case ORDER_STATUS_NoTradeNotQueueing:///δ�ɽ����ڶ�����,
			{
				ptradeGrp->m_end_milliSec = UTC::GetMilliSecs();
				break;
			}
			}
		}
	}
};

void cmMM01::logTrade(tradeRtnPtr ptrade)
{
	LOG(INFO) << "," << m_strategyId << ",trade_rtn"
		<< ", orderRef:" << ptrade->m_orderRef
		<< ", tradeDate:" << ptrade->m_tradeDate
		<< ", InstrumentID:" << ptrade->m_instId
		<< ", Direction:" << ptrade->m_orderDir
		<< ", Price:" << ptrade->m_price
		<< ", volume:" << ptrade->m_volume << endl;
};

//�������ĳɽ��ر�
//    1��������״̬����Ϊ TRADED_HEDGING
//    2�������δ�������³���ָ��
//    3���¶Գ嵥
//    4���ȴ�1s�ӣ����öԳ�ָ����� cancelHedgeOrder()
void cmMM01::processTrade(tradeRtnPtr ptrade)
{
	m_tradeTP->getDispatcher().post(boost::bind(&cmMM01::registerTradeRtn, this, ptrade));
	logTrade(ptrade);
	enum_cmMM01_strategy_status status;
	{
		boost::recursive_mutex::scoped_lock lock(m_strategyStatusLock);

		read_lock lock1(m_orderRef2cycleRWlock);
		if (m_cycleId != m_orderRef2cycle[ptrade->m_orderRef]->m_Id) //������ڵ�cycle�Ѿ�����, ��������
			return;

		status = m_strategyStatus;
		m_strategyStatus = STRATEGY_STATUS_TRADED_HEDGING;
		if (STRATEGY_STATUS_ORDER_SENT == status)
		{//����
			m_cancelBidOrderRC = 0;
			m_cancelAskOrderRC = 0;
			m_tradeTP->getDispatcher().post(boost::bind(&cmMM01::CancelOrder, this, false)); // , boost::asio::placeholders::error));
		}
	}

	//ͬ�۶Գ�
	sendHedgeOrder(ptrade);

	//�ȴ�1s
	if (STRATEGY_STATUS_TRADED_HEDGING != status)
	{
		LOG(INFO) << m_strategyId << ": waiting 1s to cancel hedge order!" << endl;
		m_cancelHedgeTimer.expires_from_now(boost::posix_time::milliseconds(1000));
		m_cancelHedgeTimer.async_wait(boost::bind(&cmMM01::cancelHedgeOrder, this)); // , boost::asio::placeholders::error));
	}

};

//������Ӧ����
void cmMM01::processCancelRes(cancelRtnPtr pCancel)
{
	if (pCancel->m_cancelOrderRc == CANCEL_RC_TRADED_OR_CANCELED)
	{
		write_lock lock(m_orderRtnBuffLock);
		auto iter = m_orderRef2orderRtn.find(pCancel->m_originOrderRef);
		if (iter == m_orderRef2orderRtn.end())
			m_orderRef2orderRtn[pCancel->m_originOrderRef] = orderRtnPtr(new  orderRtn_struct());
		m_orderRef2orderRtn[pCancel->m_originOrderRef]->m_orderRef = pCancel->m_originOrderRef;
		m_orderRef2orderRtn[pCancel->m_originOrderRef]->m_orderStatus = ORDER_STATUS_TerminatedFromCancel;
	}
}

void cmMM01::sendHedgeOrder(tradeRtnPtr ptrade)//ͬ�۶Գ�
{
	enum_order_dir_type dir = (ptrade->m_orderDir == ORDER_DIR_BUY) ? ORDER_DIR_SELL : ORDER_DIR_BUY;

	int m_hedgeOrderRef = m_infra->insertOrder(m_tradeAdapterID, m_productId, m_exchange, ORDER_TYPE_LIMIT,
		dir, POSITION_EFFECT_OPEN, FLAG_SPECULATION, ptrade->m_price, ptrade->m_volume,
		bind(&cmMM01::onHedgeOrderRtn, this, _1), bind(&cmMM01::onHedgeTradeRtn, this, _1));

	if (m_hedgeOrderRef > 0)
	{
		write_lock lock0(m_orderRef2cycleRWlock);
		m_orderRef2cycle[m_bidOrderRef] = m_ptradeGrp;
		m_ptradeGrp->m_orderIdList.push_back(m_hedgeOrderRef);

		//��¼�Գ����ͳ�������ɣ�״̬
		write_lock lock1(m_hedgeOrderVolLock);
		m_hedgeOrderVol[m_hedgeOrderRef] = ((dir == ORDER_DIR_BUY) ?
			ptrade->m_volume : (ptrade->m_volume * -1));
		m_hedgeOrderCancelRC[m_hedgeOrderRef] = 0;
	}
};

//�Գ�ɽ�������
void cmMM01::processHedgeTradeRtn(tradeRtnPtr ptrade)
{
	m_tradeTP->getDispatcher().post(boost::bind(&cmMM01::registerTradeRtn, this, ptrade));
	logTrade(ptrade);

	write_lock lock(m_hedgeOrderVolLock);
	m_hedgeOrderVol[ptrade->m_orderRef] -= ((ptrade->m_orderDir == ORDER_DIR_BUY)
		? ptrade->m_volume : (ptrade->m_volume*-1));
	if (0.0 == m_hedgeOrderVol[ptrade->m_orderRef])
		m_hedgeOrderVol.erase(ptrade->m_orderRef); //ɾ���Ѿ���ɵĶԳ�ָ��

	//ͬ�۶Գ嵥ȫ���ɽ�
	if (m_hedgeOrderVol.size() == 0)
	{
		m_hedgeOrderVol.clear();
		m_cancelHedgeTimerCancelled = true;
		m_cancelHedgeTimer.cancel();
		m_cancelConfirmTimerCancelled = true;
		m_cancelConfirmTimer.cancel();

		m_tradeTP->getDispatcher().post(bind(&cmMM01::registerTrdGrpMap, this, m_cycleId, m_ptradeGrp));
		startCycle();
	}

}

void cmMM01::processHedgeOrderRtn(orderRtnPtr pOrder)
{
	write_lock lock(m_orderRtnBuffLock);
	m_orderRef2orderRtn[pOrder->m_orderRef] = pOrder;
}

//����Գ�ָ�����Գ�ָ��δ�ɽ������������첽���������м۶Գ庯�� confirmCancel_hedgeOrder()
void cmMM01::cancelHedgeOrder()//const boost::system::error_code& error){
{
	if (m_cancelHedgeTimerCancelled)
	{
		LOG(INFO) << m_strategyId << ": hedge timer cancelled" << endl;
		return;
	}

	read_lock lock(m_hedgeOrderVolLock);
	if (m_hedgeOrderVol.size() > 0)   //����δ��ɵĶԳ�ָ��
	{
		bool isHedgeOrderConfirmed = true;
		for (auto iter = m_hedgeOrderVol.begin(); iter != m_hedgeOrderVol.end();)
		{
			//�����Գ嵥
			if (m_hedgeOrderCancelRC[iter->first] == 0)
			{
				int cancelOrderRC = m_infra->cancelOrder(m_tradeAdapterID, iter->first,
					bind(&cmMM01::onRspCancel, this, _1));
				m_hedgeOrderCancelRC[iter->first] = cancelOrderRC;
				if (cancelOrderRC < 0)
					isHedgeOrderConfirmed = false;
			}
			else if(m_hedgeOrderCancelRC[iter->first] == ORDER_CANCEL_ERROR_NOT_FOUND)
			{
				m_infra->queryOrder(m_tradeAdapterID, iter->first);
				m_cancelHedgeTimer.expires_from_now(boost::posix_time::milliseconds(1000*10));
				m_cancelHedgeTimer.async_wait(boost::bind(&cmMM01::cancelHedgeOrder, this));// ,boost::asio::placeholders::error));
				return;
			}
			else if (m_hedgeOrderCancelRC[iter->first] == ORDER_CANCEL_ERROR_SEND_FAIL)
			{
				m_hedgeOrderCancelRC[iter->first] = 0;
				m_cancelHedgeTimer.expires_from_now(boost::posix_time::milliseconds(1000*5));
				m_cancelHedgeTimer.async_wait(boost::bind(&cmMM01::cancelHedgeOrder, this)); // , boost::asio::placeholders::error));
				return;
			}
			iter++;
		}
		if (!isHedgeOrderConfirmed)
		{
			m_tradeTP->getDispatcher().post(boost::bind(&cmMM01::cancelHedgeOrder, this)); // ,boost::asio::placeholders::error));
		}
		else
			confirmCancel_hedgeOrder();
	}
};


//�����м۶Գ�
void cmMM01::confirmCancel_hedgeOrder()
{
	//boost::mutex::scoped_lock lock(m_hedgeOrderVolLock); //�� cancelHedgeOrder() �л���
	if (m_hedgeOrderVol.size() > 0)
	{
		{
			boost::recursive_mutex::scoped_lock lock(m_strategyStatusLock);
			m_strategyStatus = STRATEGY_STATUS_TRADED_NET_HEDGING;
		}
		double netHedgeVol = 0.0;
		for (auto iter = m_hedgeOrderVol.begin(); iter != m_hedgeOrderVol.end(); iter++)
			netHedgeVol += iter->second;
		if (0.0 != netHedgeVol)
		{
			//�����м۶Գ�
			sendNetHedgeOrder(netHedgeVol);
		}
		else
		{
			m_cancelConfirmTimerCancelled = true;
			m_cancelConfirmTimer.cancel();

			m_tradeTP->getDispatcher().post(bind(&cmMM01::registerTrdGrpMap, this, m_cycleId, m_ptradeGrp));
			startCycle();
		}
	}
};

void cmMM01::sendNetHedgeOrder(double netHedgeVol)
{
	//�����м۶Գ�
	enum_order_dir_type dir = netHedgeVol > 0.0 ? ORDER_DIR_BUY : ORDER_DIR_SELL;
	double price = (dir == ORDER_DIR_BUY) ?
		m_lastQuotePtr->UpperLimitPrice : m_lastQuotePtr->LowerLimitPrice;
	int netHedgeOrderRef = m_infra->insertOrder(m_tradeAdapterID, m_productId, m_exchange,
		ORDER_TYPE_LIMIT, dir, POSITION_EFFECT_OPEN, FLAG_SPECULATION, price, fabs(netHedgeVol),
		bind(&cmMM01::onNetHedgeOrderRtn, this, _1), bind(&cmMM01::onNetHedgeTradeRtn, this, _1));
	if (netHedgeOrderRef > 0)
	{
		write_lock lock0(m_orderRef2cycleRWlock);
		m_orderRef2cycle[netHedgeOrderRef] = m_ptradeGrp;
		m_ptradeGrp->m_orderIdList.push_back(netHedgeOrderRef);

		//��¼����Գ���
		boost::mutex::scoped_lock lock(m_NetHedgeOrderVolLock);
		m_NetHedgeOrderVol = netHedgeVol;
		m_hedgeOrderVol.clear();
	}
	else
		LOG(INFO) << m_strategyId << " ERROR: send net hedge order failed, rc = " << netHedgeOrderRef << endl;
};

//�����м۳ɽ�������
void cmMM01::processNetHedgeTradeRtn(tradeRtnPtr ptrade)
{
	m_tradeTP->getDispatcher().post(boost::bind(&cmMM01::registerTradeRtn, this, ptrade));
	logTrade(ptrade);

	boost::mutex::scoped_lock lock(m_NetHedgeOrderVolLock);
	m_NetHedgeOrderVol -= ((ptrade->m_orderDir == ORDER_DIR_BUY)
		? ptrade->m_volume : (ptrade->m_volume*-1));

	LOG(INFO) << m_strategyId << ": net hedge order left: " << m_NetHedgeOrderVol << endl;
	if (0.0 == m_NetHedgeOrderVol) //����Գ�ȫ���ɽ�
	{
		m_cancelConfirmTimerCancelled = true;
		m_cancelConfirmTimer.cancel();

		m_tradeTP->getDispatcher().post(bind(&cmMM01::registerTrdGrpMap, this, m_cycleId, m_ptradeGrp));
		startCycle();
	}
}

void cmMM01::processNetHedgeOrderRtn(orderRtnPtr pOrder)
{
	write_lock lock(m_orderRtnBuffLock);
	m_orderRef2orderRtn[pOrder->m_orderRef] = pOrder;
}
