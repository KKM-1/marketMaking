#pragma once
#include <iostream>
#include <map>
#include "tap/TapTradeAPI.h"
#include "threadpool/threadpool.h"
#include <boost/shared_ptr.hpp>
#include <boost/smart_ptr/detail/spinlock.hpp>
#include <boost\function.hpp>
#include <boost\asio.hpp>
#include <boost\bind.hpp>
#include "baseClass/adapterBase.h"

using namespace std;

struct orderRefInfo
{
	TAPIINT32					RefInt;							///< ���Ͳο�ֵ
	TAPISTR_50					RefString;						///< �ַ����ο�ֵ
	TAPICHAR					ServerFlag;						///< ��������ʶ
	TAPISTR_20					OrderNo;						///< ί�б���
};

typedef boost::shared_ptr<orderRefInfo> orderRefInfoPtr;

class  tradeAdapter_TAP : public traderAdapterBase, public ITapTradeAPINotify
{

private:
	string m_adapterID;
	int    m_requestId = 0;
	bool   m_isApiReady;
	ITapTradeAPI*         m_pApi;
	TapAPIApplicationInfo m_stAppInfo;
	TapAPITradeLoginAuth  m_stLoginAuth;

	char m_orderRef[13];
	map<int, orderRefInfoPtr> m_ref2order;
	boost::detail::spinlock m_ref2order_lock;

public:
	tradeAdapter_TAP(string adapterID, TAPIAUTHCODE authCode, TAPISTR_300 keyOpLogPath, \
		TAPICHAR * ip_address, TAPIUINT16 port, char * user, char * pwd,  athenathreadpoolPtr tp);
	~tradeAdapter_TAP();
	virtual void destroyAdapter();

	int init();
	int login();
	bool isAdapterReady(){ return m_isApiReady; };

	//int queryTradingAccount();//��ѯ�ʽ�
	//int queryInvestorPosition();//��ѯ�ֲ�
	//int queryAllInstrument();//��ѯȫ����Լ

	//�µ�
	virtual int OrderInsert(string instrument, string exchange, char orderType, char dir,
		char positionEffect, double price, unsigned int volume);
	//����
	virtual int cancelOrder(int orderRef);
private:
	void splitInstId(string instId, char* commodity, char* contract);

private:
	athenathreadpoolPtr m_threadpool;
	athena_lag_timer    m_lag_Timer;

public:
	boost::function<void(string adapterID)> m_OnUserLogin;
	boost::function<void(string adapterID, string adapterType)> m_OnFrontDisconnected;
	boost::function<void(string, TapAPIOrderInfoNotice *)> m_OnOrderRtn;
	boost::function<void(string, TapAPIFillInfo *)> m_OnTradeRtn;
	//boost::function<void(string, CThostFtdcInstrumentField*)> m_OnInstrumentsRtn;
	//boost::function<void(CThostFtdcInvestorPositionField*)> m_OnInvestorPositionRtn;


public:

	/**
	* @brief ���ӳɹ��ص�֪ͨ
	* @ingroup G_T_Login
	*/
	virtual void TAP_CDECL OnConnect();
	/**
	* @brief	ϵͳ��¼���̻ص���
	* @details	�˺���ΪLogin()��¼�����Ļص�������Login()�ɹ���������·���ӣ�Ȼ��API������������͵�¼��֤��Ϣ��
	*			��¼�ڼ�����ݷ�������͵�¼�Ļ�����Ϣ���ݵ��˻ص������С�
	* @param[in] errorCode ���ش�����,0��ʾ�ɹ���
	* @param[in] loginRspInfo ��½Ӧ����Ϣ�����errorCode!=0����loginRspInfo=NULL��
	* @attention	�ûص����سɹ���˵���û���¼�ɹ������ǲ�����API׼����ϡ�
	* @ingroup G_T_Login
	*/
	virtual void TAP_CDECL OnRspLogin(TAPIINT32 errorCode, const TapAPITradeLoginRspInfo *loginRspInfo);
	/**
	* @brief	֪ͨ�û�API׼��������
	* @details	ֻ���û��ص��յ��˾���֪ͨʱ���ܽ��к����ĸ����������ݲ�ѯ������\n
	*			�˻ص�������API�ܷ����������ı�־��
	* @attention ������ſ��Խ��к�����������
	* @ingroup G_T_Login
	*/
	virtual void TAP_CDECL OnAPIReady();
	/**
	* @brief	API�ͷ���ʧȥ���ӵĻص�
	* @details	��APIʹ�ù������������߱��������������ʧȥ���Ӻ󶼻ᴥ���˻ص�֪ͨ�û���������������Ѿ��Ͽ���
	* @param[in] reasonCode �Ͽ�ԭ����롣
	* @ingroup G_T_Disconnect
	*/
	virtual void TAP_CDECL OnDisconnect(TAPIINT32 reasonCode);
	/**
	* @brief ֪ͨ�û������޸Ľ��
	* @param[in] sessionID �޸�����ĻỰID,��ChangePassword���صĻỰID��Ӧ��
	* @param[in] errorCode ���ش����룬0��ʾ�ɹ���
	* @ingroup G_T_UserInfo
	*/
	virtual void TAP_CDECL OnRspChangePassword(TAPIUINT32 sessionID, TAPIINT32 errorCode){};
	/**
	* @brief �����û�Ԥ����Ϣ����
	* @param[in] sessionID �����û�Ԥ����Ϣ�ĻỰID
	* @param[in] errorCode ���ش����룬0��ʾ�ɹ���
	* @param[in] info ָ�򷵻ص���Ϣ�ṹ�塣��errorCode��Ϊ0ʱ��infoΪ�ա�
	* @attention ��Ҫ�޸ĺ�ɾ��info��ָʾ�����ݣ��������ý���������������Ч��
	* @note �ýӿ���δʵ��
	* @ingroup G_T_UserInfo
	*/
	virtual void TAP_CDECL OnRspSetReservedInfo(TAPIUINT32 sessionID, TAPIINT32 errorCode, const TAPISTR_50 info){};
	/**
	* @brief	�����û���Ϣ
	* @details	�˻ص��ӿ����û����ز�ѯ���ʽ��˺ŵ���ϸ��Ϣ���û��б�Ҫ���õ����˺ű�ű���������Ȼ���ں����ĺ���������ʹ�á�
	* @param[in] sessionID ����ĻỰID��
	* @param[in] errorCode �����롣0 ��ʾ�ɹ���
	* @param[in] isLast ��ʾ�Ƿ������һ�����ݣ�
	* @param[in] info ָ�򷵻ص���Ϣ�ṹ�塣��errorCode��Ϊ0ʱ��infoΪ�ա�
	* @attention ��Ҫ�޸ĺ�ɾ��info��ָʾ�����ݣ��������ý���������������Ч��
	* @ingroup G_T_AccountInfo
	*/
	virtual void TAP_CDECL OnRspQryAccount(TAPIUINT32 sessionID, TAPIUINT32 errorCode, TAPIYNFLAG isLast, const TapAPIAccountInfo *info){};
	/**
	* @brief �����ʽ��˻����ʽ���Ϣ
	* @param[in] sessionID ����ĻỰID��
	* @param[in] errorCode �����롣0 ��ʾ�ɹ���
	* @param[in] isLast 	��ʾ�Ƿ������һ�����ݣ�
	* @param[in] info		ָ�򷵻ص���Ϣ�ṹ�塣��errorCode��Ϊ0ʱ��infoΪ�ա�
	* @attention ��Ҫ�޸ĺ�ɾ��info��ָʾ�����ݣ��������ý���������������Ч��
	* @ingroup G_T_AccountDetails
	*/
	virtual void TAP_CDECL OnRspQryFund(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIFundData *info){};
	/**
	* @brief	�û��ʽ�仯֪ͨ
	* @details	�û���ί�гɽ���������ʽ����ݵı仯�������Ҫ���û�ʵʱ������
	* @param[in] info		ָ�򷵻ص���Ϣ�ṹ�塣��errorCode��Ϊ0ʱ��infoΪ�ա�
	* @note �������ע�������ݣ������趨Loginʱ��NoticeIgnoreFlag�����Ρ�
	* @attention ��Ҫ�޸ĺ�ɾ��info��ָʾ�����ݣ��������ý���������������Ч��
	* @ingroup G_T_AccountDetails
	*/
	virtual void TAP_CDECL OnRtnFund(const TapAPIFundData *info){};
	/**
	* @brief ����ϵͳ�еĽ�������Ϣ
	* @param[in] sessionID ����ĻỰID��
	* @param[in] errorCode �����롣0 ��ʾ�ɹ���
	* @param[in] isLast 	��ʾ�Ƿ������һ�����ݣ�
	* @param[in] info		ָ�򷵻ص���Ϣ�ṹ�塣��errorCode��Ϊ0ʱ��infoΪ�ա�
	* @attention ��Ҫ�޸ĺ�ɾ��info��ָʾ�����ݣ��������ý���������������Ч��
	* @ingroup G_T_TradeSystem
	*/
	virtual void TAP_CDECL OnRspQryExchange(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIExchangeInfo *info){};
	/**
	* @brief	����ϵͳ��Ʒ����Ϣ
	* @details	�˻ص��ӿ��������û����صõ�������Ʒ����Ϣ��
	* @param[in] sessionID ����ĻỰID����GetAllCommodities()�������ض�Ӧ��
	* @param[in] errorCode �����롣0 ��ʾ�ɹ���
	* @param[in] isLast 	��ʾ�Ƿ������һ�����ݣ�
	* @param[in] info		ָ�򷵻ص���Ϣ�ṹ�塣��errorCode��Ϊ0ʱ��infoΪ�ա�
	* @attention ��Ҫ�޸ĺ�ɾ��info��ָʾ�����ݣ��������ý���������������Ч��
	* @ingroup G_T_Commodity
	*/
	virtual void TAP_CDECL OnRspQryCommodity(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPICommodityInfo *info){};
	/**
	* @brief ����ϵͳ�к�Լ��Ϣ
	* @param[in] sessionID ����ĻỰID��
	* @param[in] errorCode �����롣0 ��ʾ�ɹ���
	* @param[in] isLast 	��ʾ�Ƿ������һ�����ݣ�
	* @param[in] info		ָ�򷵻ص���Ϣ�ṹ�塣��errorCode��Ϊ0ʱ��infoΪ�ա�
	* @attention ��Ҫ�޸ĺ�ɾ��info��ָʾ�����ݣ��������ý���������������Ч��
	* @ingroup G_T_Contract
	*/
	virtual void TAP_CDECL OnRspQryContract(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPITradeContractInfo *info){};
	/**
	* @brief	����������Լ��Ϣ
	* @details	���û������µĺ�Լ����Ҫ���������ڽ���ʱ����з�����������º�Լʱ�����û����������Լ����Ϣ��
	* @param[in] info		ָ�򷵻ص���Ϣ�ṹ�塣��errorCode��Ϊ0ʱ��infoΪ�ա�
	* @attention ��Ҫ�޸ĺ�ɾ��info��ָʾ�����ݣ��������ý���������������Ч��
	* @ingroup G_T_Contract
	*/
	virtual void TAP_CDECL OnRtnContract(const TapAPITradeContractInfo *info){};
	/**
	* @brief ������ί�С����µĻ��������ط��µ����͹����ġ�
	* @details	���������յ��ͻ��µ�ί�����ݺ�ͻᱣ�������ȴ�������ͬʱ���û�����һ��
	*			��ί����Ϣ˵����������ȷ�������û������󣬷��ص���Ϣ�а�����ȫ����ί����Ϣ��
	*			ͬʱ��һ��������ʾ��ί�е�ί�кš�
	* @param[in] info ָ�򷵻ص���Ϣ�ṹ�塣��errorCode��Ϊ0ʱ��infoΪ�ա�
	* @note �������ע�������ݣ������趨Loginʱ��NoticeIgnoreFlag�����Ρ�
	* @attention ��Ҫ�޸ĺ�ɾ��info��ָʾ�����ݣ��������ý���������������Ч��
	* @ingroup G_T_TradeActions
	*/
	virtual void TAP_CDECL OnRtnOrder(const TapAPIOrderInfoNotice *info);
	/**
	* @brief	���ضԱ����������������
	* @details	���µ��������Ȳ����Ľ����
	* @param[in] sessionID ����ĻỰID��
	* @param[in] errorCode �����롣0 ��ʾ�ɹ���
	* @param[in] info �����ľ�����Ϣ����errorCode��Ϊ0ʱ��infoΪ�ա�
	* @attention ��Ҫ�޸ĺ�ɾ��info��ָʾ�����ݣ��������ý���������������Ч��
	* @note �ýӿ�Ŀǰû���õ������в������ͨ��OnRtnOrder����
	* @ingroup G_T_TradeActions
	*/
	virtual void TAP_CDECL OnRspOrderAction(TAPIUINT32 sessionID, TAPIUINT32 errorCode, const TapAPIOrderActionRsp *info);
	/**
	* @brief	���ز�ѯ��ί����Ϣ
	* @details	�����û���ѯ��ί�еľ�����Ϣ��
	* @param[in] sessionID ����ĻỰID��
	* @param[in] errorCode �����롣0 ��ʾ�ɹ���
	* @param[in] isLast ��ʾ�Ƿ������һ�����ݣ�
	* @param[in] info ָ�򷵻ص���Ϣ�ṹ�塣��errorCode��Ϊ0ʱ��infoΪ�ա�
	* @attention ��Ҫ�޸ĺ�ɾ��info��ָʾ�����ݣ��������ý���������������Ч��
	* @ingroup G_T_TradeInfo
	*/
	virtual void TAP_CDECL OnRspQryOrder(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIOrderInfo *info){};
	/**
	* @brief ���ز�ѯ��ί�б仯������Ϣ
	* @param[in] sessionID ����ĻỰID��
	* @param[in] errorCode �����룬��errorCode==0ʱ��infoָ�򷵻ص�ί�б仯���̽ṹ�壬��ȻΪNULL��
	* @param[in] isLast ��ʾ�Ƿ������һ�����ݣ�
	* @param[in] info ���ص�ί�б仯����ָ�롣
	* @attention ��Ҫ�޸ĺ�ɾ��info��ָʾ�����ݣ��������ý���������������Ч��
	* @ingroup G_T_TradeInfo
	*/
	virtual void TAP_CDECL OnRspQryOrderProcess(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIOrderInfo *info){};
	/**
	* @brief ���ز�ѯ�ĳɽ���Ϣ
	* @param[in] sessionID ����ĻỰID��
	* @param[in] errorCode �����롣0 ��ʾ�ɹ���
	* @param[in] isLast 	��ʾ�Ƿ������һ�����ݣ�
	* @param[in] info		ָ�򷵻ص���Ϣ�ṹ�塣��errorCode��Ϊ0ʱ��infoΪ�ա�
	* @attention ��Ҫ�޸ĺ�ɾ��info��ָʾ�����ݣ��������ý���������������Ч��
	* @ingroup G_T_TradeInfo
	*/
	virtual void TAP_CDECL OnRspQryFill(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIFillInfo *info){};
	/**
	* @brief	�������ĳɽ���Ϣ
	* @details	�û���ί�гɽ������û����ͳɽ���Ϣ��
	* @param[in] info		ָ�򷵻ص���Ϣ�ṹ�塣��errorCode��Ϊ0ʱ��infoΪ�ա�
	* @note �������ע�������ݣ������趨Loginʱ��NoticeIgnoreFlag�����Ρ�
	* @attention ��Ҫ�޸ĺ�ɾ��info��ָʾ�����ݣ��������ý���������������Ч��
	* @ingroup G_T_TradeActions
	*/
	virtual void TAP_CDECL OnRtnFill(const TapAPIFillInfo *info);
	/**
	* @brief ���ز�ѯ�ĳֲ�
	* @param[in] sessionID ����ĻỰID��
	* @param[in] errorCode �����롣0 ��ʾ�ɹ���
	* @param[in] isLast 	��ʾ�Ƿ������һ�����ݣ�
	* @param[in] info		ָ�򷵻ص���Ϣ�ṹ�塣��errorCode��Ϊ0ʱ��infoΪ�ա�
	* @attention ��Ҫ�޸ĺ�ɾ��info��ָʾ�����ݣ��������ý���������������Ч��
	* @ingroup G_T_TradeInfo
	*/
	virtual void TAP_CDECL OnRspQryPosition(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIPositionInfo *info){};
	/**
	* @brief �ֱֲ仯����֪ͨ
	* @param[in] info		ָ�򷵻ص���Ϣ�ṹ�塣��errorCode��Ϊ0ʱ��infoΪ�ա�
	* @note �������ע�������ݣ������趨Loginʱ��NoticeIgnoreFlag�����Ρ�
	* @attention ��Ҫ�޸ĺ�ɾ��info��ָʾ�����ݣ��������ý���������������Ч��
	* @ingroup G_T_TradeActions
	*/
	virtual void TAP_CDECL OnRtnPosition(const TapAPIPositionInfo *info){};
	/**
	* @brief ���ز�ѯ��ƽ��
	* @param[in] sessionID ����ĻỰID��
	* @param[in] errorCode �����롣0 ��ʾ�ɹ���
	* @param[in] isLast 	��ʾ�Ƿ������һ�����ݣ�
	* @param[in] info		ָ�򷵻ص���Ϣ�ṹ�塣��errorCode��Ϊ0ʱ��infoΪ�ա�
	* @attention ��Ҫ�޸ĺ�ɾ��info��ָʾ�����ݣ��������ý���������������Ч��
	* @ingroup G_T_TradeInfo
	*/
	virtual void TAP_CDECL OnRspQryClose(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPICloseInfo *info){};
	/**
	* @brief ƽ�����ݱ仯����
	* @param[in] info		ָ�򷵻ص���Ϣ�ṹ�塣��errorCode��Ϊ0ʱ��infoΪ�ա�
	* @note �������ע�������ݣ������趨Loginʱ��NoticeIgnoreFlag�����Ρ�
	* @attention ��Ҫ�޸ĺ�ɾ��info��ָʾ�����ݣ��������ý���������������Ч��
	* @ingroup G_T_TradeActions
	*/
	virtual void TAP_CDECL OnRtnClose(const TapAPICloseInfo *info){};
	/**
	* @brief �ֲ�ӯ��֪ͨ
	* @param[in] info		ָ�򷵻ص���Ϣ�ṹ�塣��errorCode��Ϊ0ʱ��infoΪ�ա�
	* @note �������ע�������ݣ������趨Loginʱ��NoticeIgnoreFlag�����Ρ�
	* @attention ��Ҫ�޸ĺ�ɾ��info��ָʾ�����ݣ��������ý���������������Ч��
	* @ingroup G_T_TradeActions
	*/
	virtual void TAP_CDECL OnRtnPositionProfit(const TapAPIPositionProfitNotice *info){};
	/**
	* @brief ��������ѯӦ��
	* @param[in] sessionID ����ĻỰID��
	* @param[in] errorCode �����롣0 ��ʾ�ɹ���
	* @param[in] isLast 	��ʾ�Ƿ������һ�����ݣ�
	* @param[in] info	  ָ�򷵻ص����������Ϣ�ṹ�塣��errorCode��Ϊ0ʱ��infoΪ�ա�
	* @attention ��Ҫ�޸ĺ�ɾ��info��ָʾ�����ݣ��������ý���������������Ч��
	* @ingroup G_T_DeepQuote
	*/
	virtual void TAP_CDECL OnRspQryDeepQuote(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIDeepQuoteQryRsp *info){};
	/**
	* @brief ������ʱ��״̬��Ϣ��ѯӦ��
	* @param[in] sessionID ����ĻỰID��
	* @param[in] errorCode �����롣0 ��ʾ�ɹ���
	* @param[in] isLast 	��ʾ�Ƿ������һ������
	* @param[in] info		ָ�򷵻ص���Ϣ�ṹ�塣��errorCode��Ϊ0ʱ��infoΪ�ա�
	* @attention  ��Ҫ�޸ĺ�ɾ��info��ָʾ�����ݣ��������ý���������������Ч��
	* @ingroup G_T_TradeSystem
	*/
	virtual void TAP_CDECL OnRspQryExchangeStateInfo(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIExchangeStateInfo * info){};
	/**
	* @brief ������ʱ��״̬��Ϣ֪ͨ
	* @param[in] info		ָ�򷵻ص���Ϣ�ṹ�塣��errorCode��Ϊ0ʱ��infoΪ�ա�
	* @attention  ��Ҫ�޸ĺ�ɾ��info��ָʾ�����ݣ��������ý���������������Ч��
	* @ingroup G_T_TradeSystem
	*/
	virtual void TAP_CDECL OnRtnExchangeStateInfo(const TapAPIExchangeStateInfoNotice * info){};
	/**
	* @brief ѯ��֪ͨ
	* @param[in] info		ָ�򷵻ص���Ϣ�ṹ�塣��errorCode��Ϊ0ʱ��infoΪ�ա�
	* @attention ��Ҫ�޸ĺ�ɾ��info��ָʾ�����ݣ��������ý���������������Ч��
	* @ingroup G_T_TradeActions
	*/
	virtual void TAP_CDECL OnRtnReqQuoteNotice(const TapAPIReqQuoteNotice *info){};

	/**
	* @brief ������Ϣ��ѯӦ��
	* @param[in] sessionID ����ĻỰID��
	* @param[in] errorCode �����롣0 ��ʾ�ɹ���
	* @param[in] isLast 	��ʾ�Ƿ������һ������
	* @param[in] info		ָ�򷵻ص���Ϣ�ṹ�塣��errorCode��Ϊ0ʱ��infoΪ�ա�
	* @attention  ��Ҫ�޸ĺ�ɾ��info��ָʾ�����ݣ��������ý���������������Ч��
	* @ingroup G_T_UpperChannelInfo
	*/
	virtual void TAP_CDECL OnRspUpperChannelInfo(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIUpperChannelInfo * info){};
	/**
	* @brief �ͻ����շ���Ӧ��
	* @details   ��֤��������㷽ʽ������*ÿ�ֳ���*�������*�۸�\n
	*             ��֤�𶨶���㷽ʽ������*�������\n
	*             �����Ѿ��Է�ʽ���㷽ʽ������*�������������+����*ÿ�ֳ���*�۸�*�����������
	* @param[in] sessionID ����ĻỰID��
	* @param[in] errorCode �����롣0 ��ʾ�ɹ���
	* @param[in] isLast 	��ʾ�Ƿ������һ������
	* @param[in] info		ָ�򷵻ص���Ϣ�ṹ�塣��errorCode��Ϊ0ʱ��infoΪ�ա�
	* @attention  ��Ҫ�޸ĺ�ɾ��info��ָʾ�����ݣ��������ý���������������Ч��
	* @ingroup G_T_AccountRentInfo
	*/
	virtual void TAP_CDECL OnRspAccountRentInfo(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIAccountRentInfo * info){};
};
