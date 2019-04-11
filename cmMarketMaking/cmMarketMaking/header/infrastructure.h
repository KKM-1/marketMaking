#pragma once
#include <vector>
#include <hash_map>
#include <list>
#include <iostream>
#include <boost\shared_ptr.hpp>
#include <boost\unordered_map.hpp>
#include <boost\bind.hpp>
#include <boost\thread\mutex.hpp>
#include <boost\smart_ptr\detail\spinlock.hpp>
#include "infrastructureStruct.h"
#include "baseClass\orderBase.h"
#include "baseClass\adapterBase.h"
#include "threadpool\threadpool.h"
#include "ctp\tradeAdapter_CTP.h"
#include "ctp\quoteAdapter_CTP.h"
#include "tap\tradeAdapter_TAP.h"
#include "tap\quoteAdapter_TAP.h"
#include "json\json.h"
//#include "adapterConfig.h"

using namespace std;

struct futuresContractDetail
{
public:
	///��Լ����
	string m_code;
	///����������
	string m_exchange;
	///��Լ����
	string m_InstrumentName;
	///��Լ�ڽ������Ĵ���
	string m_ExchangeInstID;
	///��Ʒ����
	string m_ProductID;
	///��Ʒ����
	char m_ProductClass;
	///�������
	int m_DeliveryYear;
	///������
	int m_DeliveryMonth;
	///�м۵�����µ���
	int m_MaxMarketOrderVolume;
	///�м۵���С�µ���
	int m_MinMarketOrderVolume;
	///�޼۵�����µ���
	int m_MaxLimitOrderVolume;
	///�޼۵���С�µ���
	int m_MinLimitOrderVolume;
	///��Լ��������
	int m_VolumeMultiple;
	///��С�䶯��λ
	double m_PriceTick;
	///������
	string m_CreateDate;
	///������
	string m_OpenDate;
	///������
	string m_ExpireDate;
	///��ʼ������
	string m_StartDelivDate;
	///����������
	string m_EndDelivDate;
	///��Լ��������״̬
	char m_InstLifePhase;
	///��ǰ�Ƿ���
	int m_IsTrading;
	///�ֲ�����
	char m_PositionType;
	///�ֲ���������
	char m_PositionDateType;
	///��ͷ��֤����
	double m_LongMarginRatio;
	///��ͷ��֤����
	double m_ShortMarginRatio;
	///�Ƿ�ʹ�ô��߱�֤���㷨
	char m_MaxMarginSideAlgorithm;
	///������Ʒ����
	string m_UnderlyingInstrID;
	///ִ�м�
	double m_StrikePrice;
	///��Լ������Ʒ����
	double m_UnderlyingMultiple;
	///�������
	char m_CombinationType;

public:
	futuresContractDetail(){};

	futuresContractDetail(CThostFtdcInstrumentField* inst):m_code(string(inst->InstrumentID)), m_exchange(string(inst->ExchangeID)),
		m_InstrumentName(string(inst->InstrumentName)), m_ExchangeInstID(string(inst->ExchangeInstID)),
		m_ProductID(string(inst->ProductID)), m_ProductClass(inst->ProductClass), m_DeliveryYear(inst->DeliveryYear),
		m_DeliveryMonth(inst->DeliveryMonth), m_MaxMarketOrderVolume(inst->MaxMarketOrderVolume),
		m_MinMarketOrderVolume(inst->MinMarketOrderVolume), m_MaxLimitOrderVolume(inst->MaxLimitOrderVolume),
		m_MinLimitOrderVolume(inst->MinLimitOrderVolume), m_VolumeMultiple(inst->VolumeMultiple), m_PriceTick(inst->PriceTick),
		m_CreateDate(string(inst->CreateDate)), m_OpenDate(string(inst->OpenDate)),	m_ExpireDate(string(inst->ExpireDate)),
		m_StartDelivDate(string(inst->StartDelivDate)), m_EndDelivDate(string(inst->EndDelivDate)), m_InstLifePhase(inst->InstLifePhase),
		m_IsTrading(inst->IsTrading), m_PositionType(inst->PositionType), m_PositionDateType(inst->PositionDateType),
		m_LongMarginRatio(inst->LongMarginRatio), m_ShortMarginRatio(inst->ShortMarginRatio),
		m_MaxMarginSideAlgorithm(inst->MaxMarginSideAlgorithm), m_UnderlyingInstrID(string(inst->UnderlyingInstrID)),
		m_StrikePrice(inst->StrikePrice), m_UnderlyingMultiple(inst->UnderlyingMultiple), m_CombinationType(inst->CombinationType)
	{};

	friend ostream& operator<<(ostream& out, const futuresContractDetail& s)
	{
		out << "��Լ����: " << s.m_code << endl	<< "����������: " << s.m_exchange << endl	<< "��Լ����: " << s.m_InstrumentName << endl
			<< "��Լ�ڽ������Ĵ���: " << s.m_ExchangeInstID << endl	<< "��Ʒ����: " << s.m_ProductID << endl
			<< "��Ʒ����: " << s.m_ProductClass << endl	<< "�������: " << s.m_DeliveryYear << endl	<< "������: " << s.m_DeliveryMonth << endl
			<< "�м۵�����µ���: " << s.m_MaxMarketOrderVolume << endl	<< "�м۵���С�µ���: " << s.m_MinMarketOrderVolume << endl
			<< "�޼۵�����µ���: " << s.m_MaxLimitOrderVolume << endl	<< "�޼۵���С�µ���: " << s.m_MinLimitOrderVolume << endl
			<< "��Լ��������: " << s.m_VolumeMultiple << endl	<< "��С�䶯��λ: " << s.m_PriceTick << endl
			<< "������: " << s.m_CreateDate << endl	<< "������: " << s.m_OpenDate << endl	<< "������: " << s.m_ExpireDate << endl
			<< "��ʼ������: " << s.m_StartDelivDate << endl	<< "����������: " << s.m_EndDelivDate << endl	<< "��Լ��������״̬: " << s.m_InstLifePhase << endl
			<< "��ǰ�Ƿ���: " << s.m_IsTrading << endl	<< "�ֲ�����: " << s.m_PositionType << endl	<< "�ֲ���������: " << s.m_PositionDateType << endl
			<< "��ͷ��֤����: " << s.m_LongMarginRatio << endl	<< "��ͷ��֤����: " << s.m_ShortMarginRatio << endl
			<< "�Ƿ�ʹ�ô��߱�֤���㷨: " << s.m_MaxMarginSideAlgorithm << endl	<< "������Ʒ����: " << s.m_UnderlyingInstrID << endl
			<< "ִ�м�: " << s.m_StrikePrice << endl << "��Լ������Ʒ����: " << s.m_UnderlyingMultiple << endl
			<< "�������: " << s.m_CombinationType << endl;
		return out;
	}

};
typedef boost::shared_ptr<futuresContractDetail> futuresContractDetailPtr;

class infrastructure
{
private:
	string      m_date;
	Json::Value m_config;
	athenathreadpoolPtr m_quoteTP;
	athenathreadpoolPtr m_tradeTP;

public:
	infrastructure(Json::Value config);
	void init();
	//bool isInfrastructureReady();

	//for adapter logout or disconnection.
private:
	//boost::unordered_map<string, list<boost::function<void(string, bool)> > > m_adapterMonitorList;
	//boost::mutex m_adapterMonitorListLock;
	//void callAdapterMonitor(string adapterID, bool isAdapterAlive);
	//bool isAdapterReady(string adapterID);
public:
	//bool registerAdapterMonitor(string adapterID, boost::function<void(string, bool)> adapterMonitor);

	//��Լ
private: //futures contract
	//map<string,  // adapterID
	//	map<string, futuresContractDetailPtr> > m_futuresContracts0; // contract code -> contract detail
	//boost::unordered_map<string,        //adapterID
	//	boost::unordered_map<string,    //exchange
	//	boost::unordered_map<string,    //productID
	//	boost::unordered_map<string, futuresContractDetailPtr> > > > m_futuresContracts1;
	//map<string,        //adapterID
	//	map<string,    //exchange
	//	map<string,    //productID
	//	map<string, futuresContractDetailPtr> > > > m_futuresContracts1; // contract code -> contract detail

public:
	void onRtnCtpInstruments(string adapterID, CThostFtdcInstrumentField* inst);
	void onRtnCtpOrder(string adapterID, CThostFtdcOrderField *pOrder);
	void onRtnCtpTrade(string adapterID, CThostFtdcTradeField *pTrade);
	void onRtnTapOrder(string adapterID, TapAPIOrderInfoNotice *pOrder);
	void onRtnTapTrade(string adapterID, TapAPIFillInfo *pTrade);
/*	map<string, map<string, futuresContractDetailPtr> > & getFuturesContracts(){ return m_futuresContracts0; };
	map<string,map<string,map<string,
		map<string, futuresContractDetailPtr> > > >& getFuturesContracts1(){ return m_futuresContracts1; }*/;

private: // adapter

	map<string, enum_adapterType> m_adapterTypeMap; // adapterID -> adapterType
	void registerAdapterType(string, string);
	//map<string, queryAdapterCfgStru> m_queryAdapterCfg;  // adapterID -> config
	//map<string, quoteAdapterCfgStru> m_quoteAdapterCfg;  // adapterID -> config

	boost::unordered_map<string, adapterBase*> m_adapters; // adapterID -> adapterBase*
	//boost::unordered_map<string, quoteAdapterBase*> m_quoteAdapters; // adapterID -> quoteAdapterBase*
	//boost::unordered_map<string, traderAdapterBase*> m_tradeAdapters; // adapterID-> tradeAdapterBase*
	//map<string, adapter_status> m_adapterStatus; //adapterID->adapter status
	//boost::mutex m_adapterStatusLock;

private:
	//void loadAdapterConfig();
	//void createTradeAdapter(string adapterID, string tradeFront, string broker, string user,
	//	string pwd, string userproductID, string authenticateCode, athenathreadpoolPtr tp);
	//void createQuoteAdapter(string adapterID, string mdFront, string broker, string user, string pwd);
	//void deleteTradeAdapter(string adapterID, bool reCreate);
	//void deleteQuoteAdapter(string adapterID, bool reCreate);

public:
	void onAdapterLogin(string adapterID);
	void onAdapterLogout(string adapterID);
	void onFrontDisconnected(string adapterID, string adapterType);
	void deleteAdapter(string adapterID, string adapterType, bool reCreate);

	//����
private:
	// quoteAdapterID -> list of <exchange, code>
	//map <string, list<pair<string, string> > >  m_delayedSubscribeInstruments;  
	//boost::mutex m_delayedSubscribeInstrumentsLock;
	map < string, //exchange
		map < string, //instrument
		enum_productCategory > > m_productCategory;
	map< string, //adapterID
		//map<string, exchange
		map<string, //instrumentID
		list<boost::function<void(futuresMDPtr)> > > >  m_futuresMDHandler;
	boost::mutex m_futuresMDHandlerLock;
	void registerFuturesQuoteHandler(string adapterID, string exchange, string instList, boost::function<void(futuresMDPtr)> handler);
	void onFuturesTick(string adapterID, futuresMDPtr pQuote);

	//list<boost::function<void(quoteDetailPtr)> > m_AllInstrHandler;
	//boost::mutex m_AllInstrHandlerLock;

public:
	void subscribeFutures(string adapterID, string exchange, string instList, boost::function<void(futuresMDPtr)> handler);
	//void registerAllInstrHandler(boost::function<void(quoteDetailPtr)> handler);
	void onRtnCtpQuote(string adapterID, CThostFtdcDepthMarketDataField*); // call onCtpTick
	void onRtnTapQuote(string adapterID, TapAPIQuoteWhole *);              // call onCtpTick

	//�µ�
public:
	int insertOrder(string adapterID, string instrument, string exchange, enum_order_type orderType, 
		enum_order_dir_type dir, enum_position_effect_type positionEffect, enum_hedge_flag hedgeflag,
		double price, unsigned int volume,
		boost::function<void(orderRtnPtr)> orderRtnhandler, boost::function<void(tradeRtnPtr)> tradeRtnhandler);
	int cancelOrder(string adapterID, int orderRef, boost::function<void(cancelRtnPtr)> cancelRtnhandler);
	void onRespCtpCancel(string adapterID, CThostFtdcInputOrderActionField *pInputOrderAction, 
		CThostFtdcRspInfoField *pRspInfo);

private:
	map<enum_adapterType, map<enum_order_type, char> > m_orderTypeMap;
	map<enum_adapterType, map<enum_order_dir_type, char> > m_orderDirMap;
	map<enum_adapterType, map<enum_position_effect_type, char> > m_positinEffectMap;
	map<enum_adapterType, map<enum_hedge_flag, char> > m_hedgeFlagMap;
	void genOrderParmMap();

	map<string, //adapterID
		map<int, boost::function<void(orderRtnPtr)> > > m_orderRtnHandlers;
	map<string, //adapterID
		map< int, boost::function<void(tradeRtnPtr)> > > m_tradeRtnHandlers;
	map < string, //adapterID
		map<int, boost::function<void(cancelRtnPtr)> > > m_cancelRtnHandlers;

private:
	void initAdapters();
};
