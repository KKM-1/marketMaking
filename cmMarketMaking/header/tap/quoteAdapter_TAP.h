#pragma once
#include <vector>
#include "tap/TapQuoteAPI.h"
#include "baseClass/adapterBase.h"
#include <boost\thread\mutex.hpp>
#include <boost\bind.hpp>
#include <boost\function.hpp>

using namespace std;

class quoteAdapter_TAP : public quoteAdapterBase, public ITapQuoteAPINotify
{
private:
	string m_adapterID;
	int    m_requestId = 0;
	bool   m_isApiReady;
	ITapQuoteAPI*         m_pApi;
	TapAPIApplicationInfo m_stAppInfo;
	TapAPIQuoteLoginAuth  m_stLoginAuth;

public:
	//quoteAdapter_TAP();
	quoteAdapter_TAP(string adapterID, TAPIAUTHCODE authCode, TAPISTR_300 keyOpLogPath, \
		TAPICHAR * ip_address, TAPIUINT16 port, char * user, char * pwd);
	~quoteAdapter_TAP();
	virtual void destroyAdapter();
	int init();
	int login();
	bool isAdapterReady(){ return m_isApiReady; };

public:
	void splitInstId(string instId, char* commodity, char* contract);
	virtual void Subscribe(string instId, string exchange);
	virtual void UnSubscribe(string instId, string exchange);

public:

	//boost::function<void()> m_onLogin;
	boost::function<void(string, TapAPIQuoteWhole *)> m_onRtnMarketData;
	boost::function<void(string adapterID)> m_OnUserLogin;
	boost::function<void(string adapterID)> m_OnUserLogout;
	boost::function<void(string adapterID, string adapterType)> m_OnFrontDisconnected;

public:
	/**
	* @brief	ϵͳ��¼���̻ص���
	* @details	�˺���ΪLogin()��¼�����Ļص�������Login()�ɹ���������·���ӣ�Ȼ��API������������͵�¼��֤��Ϣ��
	*			��¼�ڼ�����ݷ�������͵�¼�Ļ�����Ϣ���ݵ��˻ص������С�
	* @param[in] errorCode ���ش�����,0��ʾ�ɹ���
	* @param[in] info ��½Ӧ����Ϣ�����errorCode!=0����info=NULL��
	* @attention	�ûص����سɹ���˵���û���¼�ɹ������ǲ�����API׼����ϡ���Ҫ�ȵ�OnAPIReady���ܽ��в�ѯ�붩������
	* @ingroup G_Q_Login
	*/
	virtual void TAP_CDECL OnRspLogin(TAPIINT32 errorCode, const TapAPIQuotLoginRspInfo *info);
	/**
	* @brief	֪ͨ�û�API׼��������
	* @details	ֻ���û��ص��յ��˾���֪ͨʱ���ܽ��к����ĸ����������ݲ�ѯ������\n
	*			�˻ص�������API�ܷ����������ı�־��
	* @attention  ������ſ��Խ��к�����������
	* @ingroup G_Q_Login
	*/
	virtual void TAP_CDECL OnAPIReady();
	/**
	* @brief	API�ͷ���ʧȥ���ӵĻص�
	* @details	��APIʹ�ù������������߱��������������ʧȥ���Ӻ󶼻ᴥ���˻ص�֪ͨ�û���������������Ѿ��Ͽ���
	* @param[in] reasonCode �Ͽ�ԭ����롣����ԭ����μ��������б� \n
	* @ingroup G_Q_Disconnect
	*/
	virtual void TAP_CDECL OnDisconnect(TAPIINT32 reasonCode);
	/**
	* @brief	��������Ʒ����Ϣ��
	* @details	�˻ص��ӿ��������û����صõ�������Ʒ����Ϣ��
	* @param[in] sessionID ����ĻỰID
	* @param[in] errorCode �����룬��errorCode!=0ʱ,infoΪNULL��
	* @param[in] isLast ��ʾ�Ƿ������һ�����ݣ�
	* @param[in] info ���ص���Ϣ�������ʼָ�롣
	* @attention  ��Ҫ�޸ĺ�ɾ��info��ָʾ�����ݣ��������ý���������������Ч��
	* @ingroup G_Q_Commodity
	*/
	virtual void TAP_CDECL OnRspQryCommodity(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIQuoteCommodityInfo *info){};
	/**
	* @brief ����ϵͳ�к�Լ��Ϣ
	* @param[in] sessionID ����ĻỰID��
	* @param[in] errorCode �����룬��errorCode!=0ʱ,infoΪNULL��
	* @param[in] isLast ��ʾ�Ƿ������һ�����ݣ�
	* @param[in] info		ָ�򷵻ص���Ϣ�ṹ�塣��errorCode��Ϊ0ʱ��infoΪ�ա�
	* @attention ��Ҫ�޸ĺ�ɾ��info��ָʾ�����ݣ��������ý���������������Ч��
	* @ingroup G_Q_Contract
	*/
	virtual void TAP_CDECL OnRspQryContract(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIQuoteContractInfo *info){};
	/**
	* @brief	���ض��������ȫ�ġ�
	* @details	�˻ص��ӿ��������ض��������ȫ�ġ�ȫ��Ϊ��ǰʱ���������Ϣ��
	* @param[in] sessionID ����ĻỰID��
	* @param[in] isLast ��ʾ�Ƿ������һ�����ݣ�
	* @param[in] errorCode �����룬��errorCode!=0ʱ,infoΪNULL��
	* @param[in] info		ָ�򷵻ص���Ϣ�ṹ�塣��errorCode��Ϊ0ʱ��infoΪ�ա�
	* @attention  ��Ҫ�޸ĺ�ɾ��info��ָʾ�����ݣ��������ý���������������Ч��
	* @ingroup G_Q_Quote
	*/
	virtual void TAP_CDECL OnRspSubscribeQuote(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIQuoteWhole *info);
	/**
	* @brief �˶�ָ����Լ������Ľ���ص�
	* @param[in] sessionID ����ĻỰID��
	* @param[in] errorCode �����룬��errorCode!=0ʱ,infoΪNULL��
	* @param[in] isLast ��ʾ�Ƿ������һ�����ݣ�
	* @param[in] info		ָ�򷵻ص���Ϣ�ṹ�塣��errorCode��Ϊ0ʱ��infoΪ�ա�
	* @attention  ��Ҫ�޸ĺ�ɾ��info��ָʾ�����ݣ��������ý���������������Ч��
	* @ingroup G_Q_Quote
	*/
	virtual void TAP_CDECL OnRspUnSubscribeQuote(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIContract *info);
	/**
	* @brief	���ض�������ı仯���ݡ�
	* @details	�˻ص��ӿ�����֪ͨ�û�������Ϣ�����˱仯�������û��ύ�µ�����ȫ�ġ�
	* @param[in] info ���µ�����ȫ������
	* @attention ��Ҫ�޸ĺ�ɾ��Quoteָʾ�����ݣ��������ý���������������Ч��
	* @ingroup G_Q_Quote
	*/
	virtual void TAP_CDECL OnRtnQuote(const TapAPIQuoteWhole *info);


};
