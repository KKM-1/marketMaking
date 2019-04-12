#include <iostream>
#include "glog/initLog.h"
#include "json/configloader.h"
#include "threadpool/threadpool.h"
#include "infrastructure.h"
#include "strategyEngine.h"

using namespace std;

int main()
{

	initLog("D://mm_log//", //��־�ļ�λ��
		    1,              // ��Ļ�������: GLOG_INFO = 0, GLOG_WARNING = 1, GLOG_ERROR = 2, GLOG_FATAL = 3
		    100);           //��־�ļ���С
			
	auto global_config = loadconfig(".\\resource\\config.json");

	infrastructure* pInfra = new infrastructure(global_config);
	pInfra->init();

	strategyEngine* pStrategy = new strategyEngine(global_config, pInfra);
	pStrategy->init();
	pStrategy->commandProcess();

	return 0;
}
