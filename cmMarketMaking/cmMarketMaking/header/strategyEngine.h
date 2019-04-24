#pragma once
#include <iostream>
#include "infrastructure.h"
#include "threadpool\threadpool.h"
#include "json\json.h"
#include "glog\logging.h"
#include "strategy\cmMM01.h"
#include "strategy\cmSpec01.h"

struct IpauseStrategy
{
	void plainVanilla(){ cout << "strategyEngine: processing pause" << endl; };
};

class strategyEngine
{
private:
	Json::Value     m_config; //���ڱ���config�ļ�����
	infrastructure* m_infra;  //���ڷ���infrastructure
	athenathreadpoolPtr m_quoteTP; // ���������̳߳�
	athenathreadpoolPtr m_tradeTP; // ������ί���̳߳�

public:
	strategyEngine(Json::Value  config, infrastructure* infra);
	void init();
	void commandProcess();

	IpauseStrategy m_pauseInterface;

private:
	map<string, strategyBase*>      m_strategies;
	map<string, enum_strategy_type> m_strategyTypeMap;
	void registerStrategyType(string strategyID, string strategyType);
};