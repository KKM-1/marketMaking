#pragma once
#include <string>
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

struct orderRtn_struct
{
};
typedef boost::shared_ptr<orderRtn_struct> orderRtnPtr;

struct tradeRtn_struct
{
};
typedef boost::shared_ptr<tradeRtn_struct> tradeRtnPtr;
