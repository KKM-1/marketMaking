#pragma once

enum enum_adapterType
{
	ADAPTER_CTP_TRADE,
	ADAPTER_CTP_QUOTE,
	ADAPTER_TAP_TRADE,
	ADAPTER_TAP_QUOTE,
	ADAPTER_ERROR_TYP
};

enum enum_productCategory
{
	CATE_CM_FUTURES,
	CATE_CM_OPTIONS,
};

struct futuresMD_struct
{
	char	TradingDay[9];    ///������
	char	InstrumentID[31]; ///��Լ����
	char	ExchangeID[9];    ///����������
	//char	ExchangeInstID[31];///��Լ�ڽ������Ĵ���
	double	LastPrice; ///���¼�
	/*double	PreSettlementPrice;///�ϴν����
	double	PreClosePrice;///������
	double	PreOpenInterest;///��ֲ���
	double	OpenPrice;///����
	double	HighestPrice;///��߼�
	double	LowestPrice;///��ͼ�*/
	double	Volume;///����
	double	Turnover;///�ɽ����
	double	OpenInterest;///�ֲ���
	/*double	ClosePrice;///������
	double	SettlementPrice;///���ν����*/
	/*double	UpperLimitPrice;///��ͣ���
	double	LowerLimitPrice;///��ͣ���
	double	PreDelta;///����ʵ��
	double	CurrDelta;///����ʵ��
	double	PreIOPV;///���ջ���ֵ
	double	IOPV;///����ֵ
	double	AuctionPrice;///��̬�ο��۸�*/
	char	UpdateTime[9];///����޸�ʱ��
	int	    UpdateMillisec;///����޸ĺ���
	double  bidprice[10];
	double  bidvol[10];
	double  bidCount[10];
	double  askprice[10];
	double  askvol[10];
	double  askCount[10];
	double	AveragePrice;///���վ���
	//char	ActionDay[9];///ҵ������
	//char	TradingPhase;///���׽׶�
	//char	OpenRestriction;///��������
	//double	YieldToMaturity;          ///����������
	//double	TradeCount;         ///�ɽ�����
	//double	TotalTradeVolume;   ///�ɽ�����
	//double	TotalBidVolume;     ///ί����������
	//double	WeightedAvgBidPrice;      ///��Ȩƽ��ί���
	//double	AltWeightedAvgBidPrice;   ///ծȯ��Ȩƽ��ί���
	//double	TotalOfferVolume;         ///ί����������
	//double	WeightedAvgOfferPrice;    ///��Ȩƽ��ί����
	//double	AltWeightedAvgOfferPrice; ///ծȯ��Ȩƽ��ί���۸�
	int	BidPriceLevel;        ///������
	int	OfferPriceLevel;      ///�������
};
typedef boost::shared_ptr<futuresMD_struct> futuresMDPtr;
