#include <iostream>
#include <time.h>
#include "ctp/tradeAdapter_CTP.h"
#include "baseClass/Utils.h"
#include "glog\logging.h"

using namespace std;

tradeAdapterCTP::tradeAdapterCTP(string adapterID, char * tradeFront, char * broker, char * user, char * pwd,
	athenathreadpoolPtr tp)// :m_onLogin(NULL)
	:m_threadpool(tp), m_lag_Timer(tp->getDispatcher()), m_qryOrder_Timer(tp->getDispatcher())
{
	m_adapterID = adapterID;

	m_pUserApi = CThostFtdcTraderApi::CreateFtdcTraderApi();
	m_pUserApi->RegisterSpi(this);         // ע���¼���
	m_pUserApi->SubscribePublicTopic(THOST_TERT_QUICK);			  // ע�ṫ����
	m_pUserApi->SubscribePrivateTopic(THOST_TERT_QUICK);			  // ע��˽����
	m_pUserApi->RegisterFront(tradeFront);							  // ע�ύ��ǰ�õ�ַ

	memset(&m_loginField, 0, sizeof(m_loginField));
	strncpy(m_loginField.BrokerID, broker, sizeof(m_loginField.BrokerID));
	strncpy(m_loginField.UserID, user, sizeof(m_loginField.UserID));
	strncpy(m_loginField.Password, pwd, sizeof(m_loginField.Password));

	m_needAuthenticate = false;
	m_qryingOrder = false;
	m_cancelQryTimer = false;
};


tradeAdapterCTP::tradeAdapterCTP(string adapterID, char* tradeFront, char* broker, char* user, char* pwd,
	char * userproductID, char * authenticateCode, athenathreadpoolPtr tp)
	:m_threadpool(tp), m_lag_Timer(tp->getDispatcher()), m_qryOrder_Timer(tp->getDispatcher())
{
	m_adapterID = adapterID;

	m_pUserApi = CThostFtdcTraderApi::CreateFtdcTraderApi();
	m_pUserApi->RegisterSpi(this);         // ע���¼���
	m_pUserApi->SubscribePublicTopic(THOST_TERT_QUICK);			  // ע�ṫ����
	m_pUserApi->SubscribePrivateTopic(THOST_TERT_QUICK);			  // ע��˽����
	m_pUserApi->RegisterFront(tradeFront);							  // ע�ύ��ǰ�õ�ַ

	memset(&m_loginField, 0, sizeof(m_loginField));
	strncpy(m_loginField.BrokerID, broker, sizeof(m_loginField.BrokerID));
	strncpy(m_loginField.UserID, user, sizeof(m_loginField.UserID));
	strncpy(m_loginField.Password, pwd, sizeof(m_loginField.Password));

	m_needAuthenticate = true;
	memset(&m_authenticateField, 0, sizeof(m_authenticateField));
	strncpy(m_authenticateField.BrokerID, broker, sizeof(m_authenticateField.BrokerID));
	strncpy(m_authenticateField.UserID, user, sizeof(m_authenticateField.UserID));
	strncpy(m_authenticateField.UserProductInfo, userproductID, sizeof(m_authenticateField.UserProductInfo));
	strncpy(m_authenticateField.AuthCode, authenticateCode, sizeof(m_authenticateField.AuthCode));

	m_qryingOrder = false;
	m_cancelQryTimer = false;
};

void tradeAdapterCTP::destroyAdapter()
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

int tradeAdapterCTP::init()
{
	m_pUserApi->Init();
	m_status = ADAPTER_STATUS_CONNECTING;
	return 0;
};

void tradeAdapterCTP::OnFrontConnected()
{
	LOG(WARNING)  << m_adapterID << ": ctp trade connected!" << endl;
	if (m_needAuthenticate)
	{
		int ret = m_pUserApi->ReqAuthenticate(&m_authenticateField, ++m_requestId);
		LOG(INFO)  << m_adapterID << ":  req | send Authenticate ... " << ((ret == 0) ? "succ" : "fail") << endl;
	}
	else
		login();
};

void tradeAdapterCTP::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField,
	CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (isErrorRespInfo(pRspInfo))
		LOG(INFO)  << m_adapterID << ": Authenticate error | ErrorID: " << pRspInfo->ErrorID <<
		", ErrorMsg: " << pRspInfo->ErrorMsg << endl;
	else
	{
		LOG(INFO)  << m_adapterID << ": Authenticate succ | broker: " << pRspAuthenticateField->BrokerID <<
			", user: " << pRspAuthenticateField->UserID <<
			", productInfo: " << pRspAuthenticateField->UserProductInfo << endl;
		login();
	}
};

void tradeAdapterCTP::OnFrontDisconnected(int nReason)
{
	LOG(INFO)  << m_adapterID << ": trade adapterCTP disconnected!" << endl;
	if (m_status != ADAPTER_STATUS_DISCONNECT && m_OnFrontDisconnected)
	{
		m_OnFrontDisconnected(m_adapterID);
	}
	m_status = ADAPTER_STATUS_DISCONNECT;
};

void tradeAdapterCTP::OnHeartBeatWarning(int nTimeLapse)
{
	LOG(INFO)  << m_adapterID << ": heartbeat warning: " << nTimeLapse << "s." << endl;
};

int tradeAdapterCTP::login()
{
	int ret = m_pUserApi->ReqUserLogin(&m_loginField, ++m_requestId);
	LOG(INFO)  << m_adapterID << ":  req | send login ... " << ((ret == 0) ? "succ" : "fail") << endl;
	return ret;
};

void tradeAdapterCTP::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
	CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (isErrorRespInfo(pRspInfo))
		LOG(INFO)  << m_adapterID << ": trade login error | ErrorID: " << pRspInfo->ErrorID << ", ErrorMsg: " << pRspInfo->ErrorMsg
		<< ", user: " << m_loginField.UserID << ", pwd:" << m_loginField.Password << ", broker: " << m_loginField.BrokerID<<endl;
	else
	{
		LOG(INFO)  << m_adapterID<<": trade login succ!" << endl;

		// ����Ự����    
		//m_frontId = pRspUserLogin->FrontID;
		//m_sessionId = pRspUserLogin->SessionID;

		time_t t;
		tm* local;
		t = time(NULL);
		local = localtime(&t);
		memset(m_orderRef, 0, sizeof(m_orderRef));
		int hourSeq;
		if (21 <= local->tm_hour && local->tm_hour <= 23)
			hourSeq = local->tm_hour - 21;
		else if (0 <= local->tm_hour && local->tm_hour <= 2)
			hourSeq = local->tm_hour + 3;
		else if (9 <= local->tm_hour && local->tm_hour <= 11)
			hourSeq = local->tm_hour - 3;
		else if (13 <= local->tm_hour && local->tm_hour <= 15)
			hourSeq = local->tm_hour - 4;
		else
			hourSeq = local->tm_hour;
		sprintf(m_orderRef, "00%02d%02d%02d0000", hourSeq, local->tm_min, local->tm_sec);

		m_status = ADAPTER_STATUS_LOGIN;
		m_lag_Timer.expires_from_now(boost::posix_time::milliseconds(3000));
		m_lag_Timer.async_wait(boost::bind(&tradeAdapterCTP::queryTradingAccount, this));
	}
};

void tradeAdapterCTP::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	LOG(INFO)  << "tradeAdapterCTP logout!" << endl;

	m_status = ADAPTER_STATUS_LOGOUT;
	if (m_OnUserLogout != NULL)
	{
		m_OnUserLogout(m_adapterID);
	}
}

int tradeAdapterCTP::queryTradingAccount()
{
	CThostFtdcQryTradingAccountField qryTradingAccountField;
	memset(&qryTradingAccountField, 0, sizeof(qryTradingAccountField));
	strncpy(qryTradingAccountField.BrokerID, m_loginField.BrokerID, sizeof(qryTradingAccountField.BrokerID));
	strncpy(qryTradingAccountField.InvestorID, m_loginField.UserID, sizeof(qryTradingAccountField.InvestorID));
	int ret = m_pUserApi->ReqQryTradingAccount(&qryTradingAccountField, ++m_requestId);
	LOG(INFO)  << m_adapterID << ":  req | query trading account ... " << ((ret == 0) ? "succ" : "fail") << endl;
	return ret;
};

void tradeAdapterCTP::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount,
	CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pTradingAccount)
	{
		//LOG(INFO)  << "���͹�˾����	:" << pTradingAccount->BrokerID << endl
		LOG(INFO)  << "Ͷ�����ʺ�:" << pTradingAccount->AccountID << ", "
			//	<< "�ϴ���Ѻ���:" << pTradingAccount->PreMortgage << endl
			//	<< "�ϴ����ö��:" << pTradingAccount->PreCredit << endl
			//	<< "�ϴδ���:" << pTradingAccount->PreDeposit << endl
			//	<< "�ϴν���׼����:" << pTradingAccount->PreBalance << endl
			//	<< "�ϴ�ռ�õı�֤��:" << pTradingAccount->PreMargin << endl
			//	<< "��Ϣ����:" << pTradingAccount->InterestBase << endl
			//	<< "��Ϣ����:" << pTradingAccount->Interest << endl
			//	<< "�����:" << pTradingAccount->Deposit << endl
			//	<< "������:" << pTradingAccount->Withdraw << endl
			//	<< "����ı�֤��:" << pTradingAccount->FrozenMargin << endl
			//	<< "������ʽ�:" << pTradingAccount->FrozenCash << endl
			//	<< "�����������:" << pTradingAccount->FrozenCommission << endl
			//	<< "��ǰ��֤���ܶ�:" << pTradingAccount->CurrMargin << endl
			//	<< "�ʽ���:" << pTradingAccount->CashIn << endl
			//	<< "������:" << pTradingAccount->Commission << endl
			//	<< "ƽ��ӯ��:" << pTradingAccount->CloseProfit << endl
			//	<< "�ֲ�ӯ��:" << pTradingAccount->PositionProfit << endl
			//	<< "�ڻ�����׼����:" << pTradingAccount->Balance << endl
			<< "�����ʽ�: " << pTradingAccount->Available << endl;
		//	<< "��ȡ�ʽ�:" << pTradingAccount->WithdrawQuota << endl
		//	<< "����׼����:" << pTradingAccount->Reserve << endl
		//	<< "������:" << pTradingAccount->TradingDay << endl
		//	<< "������:" << pTradingAccount->SettlementID << endl
		//	<< "���ö��:" << pTradingAccount->Credit << endl
		//	<< "��Ѻ���:" << pTradingAccount->Mortgage << endl
		//	<< "��������֤��:" << pTradingAccount->ExchangeMargin << endl
		//	<< "Ͷ���߽��֤��:" << pTradingAccount->DeliveryMargin << endl
		//	<< "���������֤��:" << pTradingAccount->ExchangeDeliveryMargin << endl
		//	<< "�����ڻ�����׼����:" << pTradingAccount->ReserveBalance << endl
		//	<< "���ִ���:" << pTradingAccount->CurrencyID << endl
		//	<< "�ϴλ���������:" << pTradingAccount->PreFundMortgageIn << endl
		//	<< "�ϴλ����ʳ����:" << pTradingAccount->PreFundMortgageOut << endl
		//	<< "����������:" << pTradingAccount->FundMortgageIn << endl
		//	<< "�����ʳ����:" << pTradingAccount->FundMortgageOut << endl
		//	<< "������Ѻ���:" << pTradingAccount->FundMortgageAvailable << endl
		//	<< "����Ѻ���ҽ��:" << pTradingAccount->MortgageableFund << endl
		//	<< "�����Ʒռ�ñ�֤��:" << pTradingAccount->SpecProductMargin << endl
		//	<< "�����Ʒ���ᱣ֤��:" << pTradingAccount->SpecProductFrozenMargin << endl
		//	<< "�����Ʒ������:" << pTradingAccount->SpecProductCommission << endl
		//	<< "�����Ʒ����������:" << pTradingAccount->SpecProductFrozenCommission << endl
		//	<< "�����Ʒ�ֲ�ӯ��:" << pTradingAccount->SpecProductPositionProfit << endl
		//	<< "�����Ʒƽ��ӯ��:" << pTradingAccount->SpecProductCloseProfit << endl
		//	<< "���ݳֲ�ӯ���㷨����������Ʒ�ֲ�ӯ��:" << pTradingAccount->SpecProductPositionProfitByAlg << endl
		//	<< "�����Ʒ��������֤��:" << pTradingAccount->SpecProductExchangeMargin << endl;

		if (bIsLast)
		{
			LOG(INFO)  << m_adapterID << ": query trading account done." << endl;
			m_lag_Timer.expires_from_now(boost::posix_time::milliseconds(3000));
			m_lag_Timer.async_wait(boost::bind(&tradeAdapterCTP::queryInvestorPosition, this));
		}
	}
	else
	{
		LOG(INFO)  << "resp | query cash fail ";
		if (pRspInfo == nullptr)
			LOG(INFO)  << endl;
		else
			LOG(INFO)  << " ErrorID: " << pRspInfo->ErrorID << ", ErrorMsg : " << pRspInfo->ErrorMsg << endl;
	}
};

int tradeAdapterCTP::queryInvestorPosition()
{
	CThostFtdcQryInvestorPositionField qryInvestorPositionField;
	memset(&qryInvestorPositionField, 0, sizeof(qryInvestorPositionField));
	strncpy(qryInvestorPositionField.BrokerID, m_loginField.BrokerID, sizeof(qryInvestorPositionField.BrokerID));
	strncpy(qryInvestorPositionField.InvestorID, m_loginField.UserID, sizeof(qryInvestorPositionField.InvestorID));
	//LOG(INFO)  << qryInvestorPositionField.BrokerID << endl
	//	<< qryInvestorPositionField.InvestorID << endl
	//	<< qryInvestorPositionField.InstrumentID << endl;
	int ret = m_pUserApi->ReqQryInvestorPosition(&qryInvestorPositionField, ++m_requestId);
	LOG(INFO)  << m_adapterID << ":  req | query position ... " << ((ret == 0) ? "succ" : "fail") << ", ret = " << ret << endl;
	return ret;
};

void tradeAdapterCTP::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition,
	CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pInvestorPosition)
	{
		/*LOG(INFO) << "��Լ����: " << pInvestorPosition->InstrumentID << endl
		<< "���͹�˾����: " << pInvestorPosition->BrokerID << endl
		<< "Ͷ���ߴ���: " << pInvestorPosition->InvestorID << endl
		<< "�ֲֶ�շ���: " << pInvestorPosition->PosiDirection << endl
		<< "Ͷ���ױ���־: " << pInvestorPosition->HedgeFlag << endl
		<< "�ֲ�����: " << pInvestorPosition->PositionDate << endl
		<< "���ճֲ�: " << pInvestorPosition->YdPosition << endl
		<< "���ճֲ�: " << pInvestorPosition->Position << endl
		<< "��ͷ����: " << pInvestorPosition->LongFrozen << endl
		<< "��ͷ����: " << pInvestorPosition->ShortFrozen << endl
		<< "���ֶ�����: " << pInvestorPosition->LongFrozenAmount << endl
		<< "���ֶ�����: " << pInvestorPosition->ShortFrozenAmount << endl
		<< "������: " << pInvestorPosition->OpenVolume << endl
		<< "ƽ����: " << pInvestorPosition->CloseVolume << endl
		<< "���ֽ��: " << pInvestorPosition->OpenAmount << endl
		<< "ƽ�ֽ��: " << pInvestorPosition->CloseAmount << endl
		<< "�ֲֳɱ�: " << pInvestorPosition->PositionCost << endl
		<< "�ϴ�ռ�õı�֤��: " << pInvestorPosition->PreMargin << endl
		<< "ռ�õı�֤��: " << pInvestorPosition->UseMargin << endl
		<< "����ı�֤��: " << pInvestorPosition->FrozenMargin << endl
		<< "������ʽ�: " << pInvestorPosition->FrozenCash << endl
		<< "�����������: " << pInvestorPosition->FrozenCommission << endl
		<< "�ʽ���: " << pInvestorPosition->CashIn << endl
		<< "������: " << pInvestorPosition->Commission << endl
		<< "ƽ��ӯ��: " << pInvestorPosition->CloseProfit << endl
		<< "�ֲ�ӯ��: " << pInvestorPosition->PositionProfit << endl
		<< "�ϴν����: " << pInvestorPosition->PreSettlementPrice << endl
		<< "���ν����: " << pInvestorPosition->SettlementPrice << endl
		<< "������: " << pInvestorPosition->TradingDay << endl
		<< "������: " << pInvestorPosition->SettlementID << endl
		<< "���ֳɱ�: " << pInvestorPosition->OpenCost << endl
		<< "��������֤��: " << pInvestorPosition->ExchangeMargin << endl
		<< "��ϳɽ��γɵĳֲ�: " << pInvestorPosition->CombPosition << endl
		<< "��϶�ͷ����: " << pInvestorPosition->CombLongFrozen << endl
		<< "��Ͽ�ͷ����: " << pInvestorPosition->CombShortFrozen << endl
		<< "���ն���ƽ��ӯ��: " << pInvestorPosition->CloseProfitByDate << endl
		<< "��ʶԳ�ƽ��ӯ��: " << pInvestorPosition->CloseProfitByTrade << endl
		<< "���ճֲ�: " << pInvestorPosition->TodayPosition << endl
		<< "��֤����: " << pInvestorPosition->MarginRateByMoney << endl
		<< "��֤����(������): " << pInvestorPosition->MarginRateByVolume << endl
		<< "ִ�ж���: " << pInvestorPosition->StrikeFrozen << endl
		<< "ִ�ж�����: " << pInvestorPosition->StrikeFrozenAmount << endl
		<< "����ִ�ж���: " << pInvestorPosition->AbandonFrozen << endl;*/
		if (m_OnInvestorPositionRtn)
			m_OnInvestorPositionRtn(pInvestorPosition);

		if (bIsLast)
		{
			LOG(INFO)  << m_adapterID << ": query investor position done." << endl;
			m_lag_Timer.expires_from_now(boost::posix_time::milliseconds(3000));
			m_lag_Timer.async_wait(boost::bind(&tradeAdapterCTP::confirmSettlementInfo, this));
		}
	}
	else
	{
		LOG(INFO)  << m_adapterID << ": resp | query position fail";
		if (pRspInfo == nullptr)
			LOG(INFO)  << ", pRspInfo is nullptr!" << endl;
		else
			LOG(INFO)  << ", ErrorID: " << pRspInfo->ErrorID << ", ErrorMsg : " << pRspInfo->ErrorMsg << endl;
		m_lag_Timer.expires_from_now(boost::posix_time::milliseconds(3000));
		m_lag_Timer.async_wait(boost::bind(&tradeAdapterCTP::confirmSettlementInfo, this));
	}
};
int tradeAdapterCTP::confirmSettlementInfo()
{
	CThostFtdcSettlementInfoConfirmField* pConfirm = new CThostFtdcSettlementInfoConfirmField();
	memset(pConfirm, 0, sizeof(CThostFtdcSettlementInfoConfirmField));
	strncpy(pConfirm->BrokerID, m_loginField.BrokerID, sizeof(pConfirm->BrokerID) - 1);
	strncpy(pConfirm->InvestorID, m_loginField.UserID, sizeof(pConfirm->InvestorID) - 1);
	m_pUserApi->ReqSettlementInfoConfirm(pConfirm, ++m_requestId);
	return 0;
};


///Ͷ���߽�����ȷ����Ӧ
void tradeAdapterCTP::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, 
	CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pSettlementInfoConfirm)
	{
		if (bIsLast)
		{
			LOG(INFO) << m_adapterID << ": resp | confirm Settlement info succ." << endl;
			LOG(WARNING) << "---------- " << m_adapterID << " init done ----------" << endl;
			if (m_OnUserLogin != NULL)
				m_OnUserLogin(m_adapterID);

			//m_lag_Timer.expires_from_now(boost::posix_time::milliseconds(3000));
			//m_lag_Timer.async_wait(boost::bind(&tradeAdapterCTP::queryAllInstrument, this));
		}
	}
	else
	{
		LOG(INFO) << m_adapterID << ": resp | confirm Settlement info fail";
		if (pRspInfo == nullptr)
			LOG(INFO) << ", pRspInfo is nullptr!" << endl;
		else
			LOG(INFO) << ", ErrorID: " << pRspInfo->ErrorID << ", ErrorMsg : " << pRspInfo->ErrorMsg << endl;
		m_lag_Timer.expires_from_now(boost::posix_time::milliseconds(3000));
		m_lag_Timer.async_wait(boost::bind(&tradeAdapterCTP::queryAllInstrument, this));
	}
};

int tradeAdapterCTP::queryAllInstrument()
{
	CThostFtdcQryInstrumentField qryInstrument;
	memset(&qryInstrument, 0, sizeof(CThostFtdcQryInstrumentField));
	int ret = m_pUserApi->ReqQryInstrument(&qryInstrument, ++m_requestId);
	LOG(INFO)  << m_adapterID << ":  req | query all instruments ... " << ((ret == 0) ? "succ" : "fail") << endl;
	return ret;
};

void tradeAdapterCTP::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo,
	int nRequestID, bool bIsLast)
{
	if (pInstrument != nullptr)
	{
		if (m_OnInstrumentsRtn)
			m_OnInstrumentsRtn(m_adapterID, pInstrument);
		if (bIsLast)
		{
			LOG(INFO)  << m_adapterID << ": resp | query instrument done." << endl;
			LOG(WARNING)  << "---------- " << m_adapterID << " init done ----------" << endl;
			if (m_OnUserLogin != NULL)
			{
				m_OnUserLogin(m_adapterID);
			}
		}
		//LOG(INFO)  << "��Լ����" << pInstrument->InstrumentID << endl
		//<< "����������" << pInstrument->ExchangeID << endl
		//<< "��Լ����" << pInstrument->InstrumentName << endl
		//<< "��Լ�ڽ������Ĵ���" << pInstrument->ExchangeInstID << endl
		//<< "��Ʒ����" << pInstrument->ProductID << endl
		//<< "��Ʒ����" << pInstrument->ProductClass << endl
		//<< "�������" << pInstrument->DeliveryYear << endl
		//<< "������" << pInstrument->DeliveryMonth << endl
		//<< "�м۵�����µ���" << pInstrument->MaxMarketOrderVolume << endl
		//<< "�м۵���С�µ���" << pInstrument->MinMarketOrderVolume << endl
		//<< "�޼۵�����µ���" << pInstrument->MaxLimitOrderVolume << endl
		//<< "�޼۵���С�µ���" << pInstrument->MinLimitOrderVolume << endl
		//<< "��Լ��������" << pInstrument->VolumeMultiple << endl
		//<< "��С�䶯��λ" << pInstrument->PriceTick << endl
		//<< "������" << pInstrument->CreateDate << endl
		//<< "������" << pInstrument->OpenDate << endl
		//<< "������" << pInstrument->ExpireDate << endl
		//<< "��ʼ������" << pInstrument->StartDelivDate << endl
		//<< "����������" << pInstrument->EndDelivDate << endl
		//<< "��Լ��������״̬" << pInstrument->InstLifePhase << endl
		//<< "��ǰ�Ƿ���" << pInstrument->IsTrading << endl
		//<< "�ֲ�����" << pInstrument->PositionType << endl
		//<< "�ֲ���������" << pInstrument->PositionDateType << endl
		//<< "��ͷ��֤����" << pInstrument->LongMarginRatio << endl
		//<< "��ͷ��֤����" << pInstrument->ShortMarginRatio << endl
		//<< "�Ƿ�ʹ�ô��߱�֤���㷨" << pInstrument->MaxMarginSideAlgorithm << endl
		//<< "������Ʒ����" << pInstrument->UnderlyingInstrID << endl
		//<< "ִ�м�" << pInstrument->StrikePrice << endl
		//<< "��Ȩ����" << pInstrument->OptionsType << endl
		//<< "��Լ������Ʒ����" << pInstrument->UnderlyingMultiple << endl
		//<< "�������" << pInstrument->CombinationType << endl;
		return;
	}
	else
	{
		LOG(INFO)  << m_adapterID << ": resp | query instrument fail ";
		if (pRspInfo == nullptr)
			LOG(INFO)  << endl;
		else
			LOG(INFO)  << m_adapterID << ": ErrorID: " << pRspInfo->ErrorID << ", ErrorMsg : " << pRspInfo->ErrorMsg << endl;
	}
}

int tradeAdapterCTP::OrderInsert(string instrument, string exchange, char priceType, char dir,
	char ComOffsetFlag, char ComHedgeFlag, double price,
	int volume, char tmCondition, char volCondition, int minVol, char contiCondition,
	double stopPrz, char forceCloseReason)
{
	CThostFtdcInputOrderField * pInputOrder = new CThostFtdcInputOrderField();
	memset(pInputOrder, 0, sizeof(CThostFtdcInputOrderField));
	strncpy(pInputOrder->BrokerID, m_loginField.BrokerID, sizeof(pInputOrder->BrokerID));
	strncpy(pInputOrder->InvestorID, m_loginField.UserID, sizeof(pInputOrder->InvestorID));
	strncpy(pInputOrder->UserID, m_loginField.UserID, sizeof(pInputOrder->UserID));
	int nextOrderRef = -1;
	{
		//LOG(INFO)  << m_adapterID << ": locking m_orderRefLock in OrderInsert." << endl;
		boost::mutex::scoped_lock l0(m_orderRefLock);
		nextOrderRef = updateOrderRef();
		strncpy(pInputOrder->OrderRef, m_orderRef, sizeof(pInputOrder->OrderRef) - 1);  //��������
		//LOG(INFO)  << m_adapterID << ": unlocking m_orderRefLock in OrderInsert." << endl;
	}
	strncpy(pInputOrder->InstrumentID, instrument.c_str(), sizeof(pInputOrder->InstrumentID) - 1);
	strncpy(pInputOrder->ExchangeID, exchange.c_str(), sizeof(pInputOrder->ExchangeID) - 1);
	pInputOrder->OrderPriceType = priceType;///�����۸�����
	pInputOrder->Direction = dir;  ///��������
	pInputOrder->CombOffsetFlag[0] = ComOffsetFlag;///��Ͽ�ƽ��־
	pInputOrder->CombHedgeFlag[0] = ComHedgeFlag;///���Ͷ���ױ���־

	pInputOrder->LimitPrice = price; ///�۸�
	pInputOrder->VolumeTotalOriginal = volume;///����
	pInputOrder->TimeCondition = tmCondition;///��Ч������
	///GTD���� //good till date ����һ����Ч
	///TThostFtdcDateType	GTDDate;
	pInputOrder->VolumeCondition = volCondition;///�ɽ�������
	pInputOrder->MinVolume = minVol;///��С�ɽ���
	pInputOrder->ContingentCondition = contiCondition;///��������
	pInputOrder->StopPrice = stopPrz;///ֹ���
	pInputOrder->ForceCloseReason = forceCloseReason;///ǿƽԭ��

	///TThostFtdcBoolType	IsAutoSuspend;///�Զ������־ Ĭ�� 0
	///TThostFtdcBusinessUnitType	BusinessUnit;///ҵ��Ԫ

	int reqId = ++m_requestId;
	pInputOrder->RequestID = reqId;///req���
	///TThostFtdcBoolType	UserForceClose;///�û�ǿ����־ Ĭ�� 0
	///TThostFtdcBoolType  IsSwapOrder; ///��������־

	int ret = m_pUserApi->ReqOrderInsert(pInputOrder, reqId);

	if (ret == 0)
	{
		LOG(INFO) << m_adapterID << ": req | order insert succ, orderRef: " << nextOrderRef <<
			", inst: " << instrument<< ", price: "<< price << ", volume: "<<volume<< endl;
		return nextOrderRef;
	}
	else
	{
		LOG(INFO) << m_adapterID << ": req | order insert fail! retCode: " << ret << endl;
		return -1;
	}
}

//����ʧ�ܲŷ���
void tradeAdapterCTP::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!isErrorRespInfo(pRspInfo)){
		if (pInputOrder)
			LOG(INFO)  << m_adapterID << ":resp | order insert succ, orderRef: " << pInputOrder->OrderRef << endl;
	}
	else
		LOG(INFO)  << m_adapterID << ":resp | order insert fail, ErrorID: " << pRspInfo->ErrorID << ", ErrorMsg: " << pRspInfo->ErrorMsg << endl;
};

void tradeAdapterCTP::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo)
{
	if (!isErrorRespInfo(pRspInfo)){
		if (pInputOrder)
			LOG(INFO)  << "resp | order insert succ, orderRef: " << pInputOrder->OrderRef << endl;
	}
	else
		LOG(INFO)  << "resp | order insert fail, ErrorID: " << pRspInfo->ErrorID << ", ErrorMsg: " << pRspInfo->ErrorMsg << endl;
};

void tradeAdapterCTP::queryOrder()
{
	boost::mutex::scoped_lock lock(m_qryOrderLock);
	if (!m_qryingOrder)
	{
		CThostFtdcQryOrderField qryOrder;
		memset(&qryOrder, 0, sizeof(CThostFtdcQryOrderField));
		strncpy(qryOrder.BrokerID, m_loginField.BrokerID, sizeof(qryOrder.BrokerID) - 1);
		strncpy(qryOrder.InvestorID, m_loginField.UserID, sizeof(qryOrder.InvestorID) - 1);
		m_pUserApi->ReqQryOrder(&qryOrder, ++m_requestId);
		LOG(WARNING) << m_adapterID << ": Req | query order start ..." << endl;
		closeOrderQrySwitch();
		m_qryOrder_Timer.expires_from_now(boost::posix_time::millisec(1000 * 60 * 3)); //�����Ӻ�򿪲�ѯ
		m_qryOrder_Timer.async_wait(boost::bind(&tradeAdapterCTP::openOrderQrySwitch, this));
	}
	else
		LOG(WARNING) << m_adapterID << ": query order is in process, no more query lunched." << endl;
};

void tradeAdapterCTP::OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, 
	int nRequestID, bool bIsLast)
{
	if (pOrder){
		CThostFtdcOrderFieldPtr orderPtr = CThostFtdcOrderFieldPtr(new CThostFtdcOrderField(*pOrder));
		int orderRef = atoi(pOrder->OrderRef);
		{
			boost::mutex::scoped_lock l(m_ref2order_lock);
			m_ref2order[orderRef] = orderPtr;
		}
		if (m_OnOrderRtn)
			m_OnOrderRtn(m_adapterID, pOrder);
	}
	if (bIsLast) //���ر������
	{
		boost::mutex::scoped_lock lock(m_qryOrderLock);
		openOrderQrySwitch();
		m_cancelQryTimer = true;
		m_qryOrder_Timer.cancel();
	}
};

void tradeAdapterCTP::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
	CThostFtdcOrderFieldPtr orderPtr = CThostFtdcOrderFieldPtr(new CThostFtdcOrderField(*pOrder));
	int orderRef = atoi(pOrder->OrderRef);
	{
		//LOG(INFO)  << m_adapterID << ": locking m_ref2order in OnRtnOrder." << endl;
		boost::mutex::scoped_lock l(m_ref2order_lock);
		m_ref2order[orderRef] = orderPtr;
		//LOG(INFO)  << m_adapterID << ": unlocking m_ref2order in OnRtnOrder." << endl;
	}
	if (m_OnOrderRtn)
		m_OnOrderRtn(m_adapterID, pOrder);
	
	LOG(INFO) << m_adapterID << " Rsp | order Rtn: orderRef: " << pOrder->OrderRef //<< pOrder->BrokerOrderSeq
	<< ", InstrumentID:" << pOrder->InstrumentID
	<< ", Direction:" << pOrder->Direction
	<< ", LimitPrice:" << pOrder->LimitPrice
	<< ", OrderStatus:" << pOrder->OrderStatus
	<< ", StatusMsg:" << pOrder->StatusMsg
	//<< ", CombHedgeFlag:" << pOrder->CombHedgeFlag
	//<< ", CombOffsetFlag:" << pOrder->CombOffsetFlag
	//<< ", MinVolume:" << pOrder->MinVolume
	//<< ", OrderPriceType:" << pOrder->OrderPriceType
	//<< ", orderRef:" << pOrder->OrderRef
	<< endl;
};

void tradeAdapterCTP::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
	if (m_OnTradeRtn)
		m_OnTradeRtn(m_adapterID, pTrade);
	LOG(INFO) << m_adapterID<< " Rsp | trade Rtn: orderRef: " << pTrade->OrderRef //<< pOrder->BrokerOrderSeq
		<< ", tradeTime:" << pTrade->TradeTime
		<< ", InstrumentID:" << pTrade->InstrumentID
		<< ", Direction:" << pTrade->Direction
		<< ", Price:" << pTrade->Price
		<< ", volume:" << pTrade->Volume
		//<< ", CombHedgeFlag:" << pOrder->CombHedgeFlag
		//<< ", CombOffsetFlag:" << pOrder->CombOffsetFlag
		//<< ", MinVolume:" << pOrder->MinVolume
		//<< ", OrderPriceType:" << pOrder->OrderPriceType
		//<< ", orderRef:" << pOrder->OrderRef
		<< endl;
};

int tradeAdapterCTP::cancelOrder(int orderRef)
{
	if (orderRef == 0)
		cout << "debug" << endl;
	auto iter = m_ref2order.begin();
	{
		//LOG(INFO)  << m_adapterID << ": locking m_ref2order in cancelOrder." << endl;
		boost::mutex::scoped_lock l0(m_ref2order_lock);
		iter = m_ref2order.find(orderRef);
		//LOG(INFO)  << m_adapterID << ": unlocking m_ref2order in cancelOrder." << endl;
	}
	if (iter == m_ref2order.end())
	{
		LOG(WARNING) << m_adapterID << ": cancel Order fail | order return not received, orderRef: " << orderRef << endl;
		return ORDER_CANCEL_ERROR_NOT_FOUND;
	}
	CThostFtdcInputOrderActionField actionField;
	memset(&actionField, 0, sizeof(actionField));
	actionField.ActionFlag = THOST_FTDC_AF_Delete;
	actionField.FrontID = iter->second->FrontID;
	actionField.SessionID = iter->second->SessionID;
	int nextOrderRef = -1;
	{
		boost::mutex::scoped_lock l2(m_orderRefLock);
		nextOrderRef = updateOrderRef();
		actionField.OrderActionRef = nextOrderRef;
	}
	sprintf(actionField.OrderRef, "%012d", orderRef);
	strncpy(actionField.BrokerID, m_loginField.BrokerID, sizeof(actionField.BrokerID));
	strncpy(actionField.InvestorID, m_loginField.UserID, sizeof(actionField.InvestorID));
	strncpy(actionField.UserID, m_loginField.UserID, sizeof(actionField.UserID));
	strncpy(actionField.InstrumentID, iter->second->InstrumentID, sizeof(actionField.InstrumentID));
	int ret = m_pUserApi->ReqOrderAction(&actionField, ++m_requestId);
	if (ret == 0)
	{
		LOG(INFO)  << m_adapterID << ": req | cancel order succ, orderRef: " << orderRef << endl;
		return nextOrderRef;
	}
	else
	{
		LOG(INFO)  << m_adapterID << ": req | cancel order fail, orderRef: " << orderRef << endl;
		return ORDER_CANCEL_ERROR_SEND_FAIL;
	}
};

//����ʧ�ܲŻ����
void tradeAdapterCTP::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction,
	CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	/*if (!isErrorRespInfo(pRspInfo)){
		if (pInputOrderAction)
			LOG(INFO)  << m_adapterID << ":resp | send order action succ, orderRef: " << pInputOrderAction->OrderRef << ", orderActionRef: " << pInputOrderAction->OrderActionRef << endl;
	}
	else
		LOG(INFO)  << m_adapterID << ":resp | send order action fail, ErrorID: " << pRspInfo->ErrorID << ", ErrorMsg: " << pRspInfo->ErrorMsg << endl;*/
};

void tradeAdapterCTP::OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo)
{
	if (m_onErrRtnOrderAction)
		m_onErrRtnOrderAction(m_adapterID, pOrderAction, pRspInfo);
	if (isErrorRespInfo(pRspInfo))
		LOG(INFO)  << m_adapterID << ":resp | send order action fail, OrderRef:" << pOrderAction->OrderRef << ", ErrorID: " << pRspInfo->ErrorID << ", ErrorMsg: " << pRspInfo->ErrorMsg << endl;
};

bool tradeAdapterCTP::isErrorRespInfo(CThostFtdcRspInfoField *pRspInfo)
{
	if (pRspInfo == nullptr || pRspInfo->ErrorID != 0)
		return true;
	return false;
};
