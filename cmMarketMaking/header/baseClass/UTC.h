#pragma once
#include <string>

typedef unsigned long long uint64;
typedef long long int64;

namespace athenaUTC
{

	class UTC
		{
		public:
			//��1970��1��1����������׶ص�ǰʱ���΢����(us)
			explicit UTC(int64 v);

			//UTCʱ����ַ�����ʽ����ʽΪ"YYYY-MM-DD HH:MM:SS.xxxxxx"
			explicit UTC(const std::string& str);

			//ȡ��ǰUTCʱ��
			UTC(void);

			//���»�ȡ��ǰʱ��
			UTC& Now(void);

			//UTCʱ��ת�ַ���,��ʽΪ"YYYY-MM-DD HH:MM:SS.xxxxxx"
			std::string ToString(void);

			//UTCʱ��ת����ʱ����ַ���,��ʽΪ"YYYY-MM-DD HH:MM:SS.xxxxxx"
			std::string ToBeiJing(void);

			//UTCʱ��ת����ʱ����ַ���,��ʽΪ"YYYYMMDDHHMMSS"
			std::string ToBeiJing1(void);

			static int Init();
			static double GetMilliSecs();

		public:
			int64 m_Val;       //��1970��1��1����������׶ص�ǰʱ���΢����(us)
		};

	}
