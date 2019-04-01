#include "baseClass\Utils.h"
#include "infrastructure.h"

void infrastructure::genOrderParmMap()
{
	//委托类型: 限价单、市价单
	m_orderTypeMap[ADAPTER_CTP_TRADE][ORDER_TYPE_MARKET] = THOST_FTDC_OPT_AnyPrice;
	m_orderTypeMap[ADAPTER_CTP_TRADE][ORDER_TYPE_LIMIT] = THOST_FTDC_OPT_LimitPrice;
	m_orderTypeMap[ADAPTER_TAP_TRADE][ORDER_TYPE_MARKET] = TAPI_ORDER_TYPE_MARKET;
	m_orderTypeMap[ADAPTER_TAP_TRADE][ORDER_TYPE_LIMIT] = TAPI_ORDER_TYPE_LIMIT;

	//委托方向: 买、卖
	m_orderDirMap[ADAPTER_CTP_TRADE][ORDER_DIR_BUY] = THOST_FTDC_D_Buy;
	m_orderDirMap[ADAPTER_CTP_TRADE][ORDER_DIR_SELL] = THOST_FTDC_D_Sell;
	m_orderDirMap[ADAPTER_TAP_TRADE][ORDER_DIR_BUY] = TAPI_SIDE_BUY;
	m_orderDirMap[ADAPTER_TAP_TRADE][ORDER_DIR_SELL] = TAPI_SIDE_SELL;

	//开平标志
	m_positinEffectMap[ADAPTER_CTP_TRADE][POSITION_EFFECT_OPEN] = THOST_FTDC_OFEN_Open;
	m_positinEffectMap[ADAPTER_CTP_TRADE][POSITION_EFFECT_CLOSE] = THOST_FTDC_OFEN_Close;
	m_positinEffectMap[ADAPTER_TAP_TRADE][POSITION_EFFECT_OPEN] = TAPI_PositionEffect_OPEN;
	m_positinEffectMap[ADAPTER_TAP_TRADE][POSITION_EFFECT_CLOSE] = TAPI_PositionEffect_COVER;

	//投机套保标志
	m_hedgeFlagMap[ADAPTER_CTP_TRADE][FLAG_SPECULATION] = THOST_FTDC_HF_Speculation;
	m_hedgeFlagMap[ADAPTER_CTP_TRADE][FLAG_MARKETMAKER] = THOST_FTDC_HF_MarketMaker;

};

int infrastructure::insertOrder(string adapterID, string instrument, string exchange, 
	enum_order_type orderType, enum_order_dir_type dir, enum_position_effect_type positionEffect, 
	enum_hedge_flag hedgeflag, double price, unsigned int volume,
	boost::function<void(orderRtnPtr)> orderRtnhandler, boost::function<void(tradeRtnPtr)> tradeRtnhandler)
{
	switch (m_adapterTypeMap[adapterID])
	{
	case ADAPTER_CTP_TRADE:
	{
		tradeAdapterCTP * pTradeAdapter = (tradeAdapterCTP *)m_adapters[adapterID];
		int orderRef = pTradeAdapter->OrderInsert(instrument,
			m_orderTypeMap[ADAPTER_CTP_TRADE][orderType],
			m_orderDirMap[ADAPTER_CTP_TRADE][dir],
			m_positinEffectMap[ADAPTER_CTP_TRADE][positionEffect],
			m_hedgeFlagMap[ADAPTER_CTP_TRADE][hedgeflag],
			price, volume, 
			THOST_FTDC_TC_GFD, //当日有效
			THOST_FTDC_VC_AV, //任何数量
			1, THOST_FTDC_CC_Immediately, //立即成交
			0.0, THOST_FTDC_FCC_NotForceClose //非强平
			);
		if (orderRef != -1) //下单成功
		{
			m_orderRtnHandlers[adapterID][orderRef] = orderRtnhandler;
			m_tradeRtnHandlers[adapterID][orderRef] = tradeRtnhandler;
			cout << "infra: order sent succ." << endl;
			return orderRef;
		}
		break;
	}
	case ADAPTER_TAP_TRADE:
	{
		tradeAdapter_TAP * pTradeAdapter = (tradeAdapter_TAP *)m_adapters[adapterID];
		int orderRef = pTradeAdapter->OrderInsert(instrument, exchange,
			m_orderTypeMap[ADAPTER_TAP_TRADE][orderType],
			m_orderDirMap[ADAPTER_TAP_TRADE][dir],
			m_positinEffectMap[ADAPTER_TAP_TRADE][positionEffect],
			price, volume);
		if (orderRef != -1) //下单成功
		{
			m_orderRtnHandlers[adapterID][orderRef] = orderRtnhandler;
			m_tradeRtnHandlers[adapterID][orderRef] = tradeRtnhandler;
			cout << "infra: order sent succ." << endl;
			return orderRef;
		}
		break;
	}
	}
	return -1;
};

void infrastructure::cancelOrder(string adapterID, int orderRef)
{
	switch (m_adapterTypeMap[adapterID])
	{
	case ADAPTER_CTP_TRADE:
	{
		tradeAdapterCTP * pTradeAdapter = (tradeAdapterCTP *)m_adapters[adapterID];
		pTradeAdapter->cancelOrder(orderRef);
		break;
	}
	case ADAPTER_TAP_TRADE:
	{
		tradeAdapter_TAP * pTradeAdapter = (tradeAdapter_TAP *)m_adapters[adapterID];
		pTradeAdapter->cancelOrder(orderRef);
		break;
	}
	}
};

void infrastructure::onRtnCtpOrder(string adapterID, CThostFtdcOrderField *pOrder)
{
	orderRtnPtr orderPtr = orderRtnPtr(new orderRtn_struct());
	int orderRef = atoi(pOrder->OrderRef);
	auto iter1 = m_orderRtnHandlers.find(adapterID);
	if (iter1 != m_orderRtnHandlers.end())
	{
		auto iter2 = iter1->second.find(orderRef);
		if (iter2 != iter1->second.end())
			m_tradeTP->getDispatcher().post(bind((m_orderRtnHandlers[adapterID][orderRef]), orderPtr));
	}
};
void infrastructure::onRtnCtpTrade(string adapterID, CThostFtdcTradeField *pTrade)
{
	tradeRtnPtr tradePtr = tradeRtnPtr(new tradeRtn_struct());
	int orderRef = atoi(pTrade->OrderRef);
	auto iter = m_tradeRtnHandlers.find(adapterID);
	if (iter != m_tradeRtnHandlers.end())
	{
		auto iter2 = iter->second.find(orderRef);
		if (iter2 != iter->second.end())
			m_tradeTP->getDispatcher().post(bind((m_tradeRtnHandlers[adapterID][orderRef]), tradePtr));
	}
};
void infrastructure::onRtnTapOrder(string adapterID, TapAPIOrderInfoNotice *pOrder)
{
	orderRtnPtr orderPtr = orderRtnPtr(new orderRtn_struct());
	int orderRef = pOrder->OrderInfo->RefInt;
	auto iter1 = m_orderRtnHandlers.find(adapterID);
	if (iter1 != m_orderRtnHandlers.end())
	{
		auto iter2 = iter1->second.find(orderRef);
		if (iter2 != iter1->second.end())
			m_tradeTP->getDispatcher().post(bind((m_orderRtnHandlers[adapterID][orderRef]), orderPtr));
	}
};
void infrastructure::onRtnTapTrade(string adapterID, TapAPIFillInfo *pTrade)
{
	tradeRtnPtr tradePtr = tradeRtnPtr(new tradeRtn_struct());
	//int orderRef = atoi(pTrade->ref);
	//auto iter = m_tradeRtnHandlers.find(orderRef);
	//if (iter != m_tradeRtnHandlers.end())
	//	m_tradeRtnHandlers[orderRef](tradePtr);
};