#pragma once
#include <string>
#include <boost/shared_ptr.hpp>
using namespace std;


enum enum_order_type
{
	ORDER_TYPE_MARKET,
	ORDER_TYPE_LIMIT,

};

//! ��ƽ����
enum enum_position_effect_type
{
	//! ���ֿ�ƽ
	POSITION_EFFECT_NONE,
	//! ����
	POSITION_EFFECT_OPEN,
	//! ƽ��
	POSITION_EFFECT_CLOSE,
	//! ƽ����
	POSITION_EFFECT_CLOSE_TODAY,
};

enum enum_order_dir_type
{
	ORDER_DIR_BUY,
	ORDER_DIR_SELL,
};

enum enum_hedge_flag
{
	///Ͷ��
	FLAG_SPECULATION,
	///����
	FLAG_ARBITRAGE,
	///�ױ�
	FLAG_HEDGE,
	///������
	FLAG_MARKETMAKER,
};

enum enum_order_error
{
	ORDER_SEND_ERROR_TO_DEFINE = -1000,
	ORDER_CANCEL_ERROR_NOT_FOUND,
};

struct orderRtn_struct
{
};
typedef boost::shared_ptr<orderRtn_struct> orderRtnPtr;

struct tradeRtn_struct
{
	int m_orderRef;
	string m_exchange;
	string m_instId;
	//enum_order_type           m_orderTyp;
	//enum_position_effect_type m_positionEffectTyp;
	enum_order_dir_type       m_orderDir;
	double m_price;
	double m_volume;
};
typedef boost::shared_ptr<tradeRtn_struct> tradeRtnPtr;

struct cancelRtn_struct
{
	int m_cancelOrderRef;
	int m_originOrderRef;
	bool m_isCancelSucc;
};
typedef boost::shared_ptr<cancelRtn_struct> cancelRtnPtr;