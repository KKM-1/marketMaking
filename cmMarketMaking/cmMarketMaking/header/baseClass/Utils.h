#pragma once
#include <boost\asio.hpp>
#include <boost\thread.hpp>

class athenaUtils
{
public:
	static std::string&  Rtrim(std::string& s);    //ɾ���ַ����Ҷ˵Ŀո�
	static std::string&  Ltrim(std::string& s);    //ɾ���ַ�����˵Ŀո�
	static std::string&  Trim(std::string& s);     //ɾ���ַ����������˵Ŀո�
	static bool Equals(const double& d1, const double& d2);    //�ж�d1��d2�Ƿ�������
	static int  Compare(const double& d1, const double& d2);   //�Ƚϴ�С,-1 d1С��d2; 0 d1����d2;+1 d1����d2
	static bool Greater(const double& d1, const double& d2);
	static bool GreaterOrEqual(const double& d1, const double& d2);
	static bool Less(const double& d1, const double& d2);
	static bool LessOrEqual(const double& d1, const double& d2);
	static bool IsInvalid(const double& d);    //�ж�dΪ��Чֵ 
	static double GetInvalidValue();     //��Чֵ����Ϊdouble���͵����ֵ1.7976931348623158e+308
	static int  gcd(const int integer1, const int integer2); //�������Լ��
	///@brief        ��ָ���ķָ��ַ�������һ���ַ���         
	///@param[in]    src ��������ַ����������� 
	///@param[in]    delimit    ָ���ķָ��ַ���
	///@param[in]    null_subst �������ָ��ַ���֮�������Ϊ�գ����� @p null_subst �����
	///                       ��� @p null_subst Ҳ�ǿգ������
	///@param[out] v ���ֵĽ������,vector, list, set
	///@return        ���ֵĽ������Ĵ�С
	template <class resultType>
	static inline unsigned int  Split(const std::string& src, const std::string& delimit, resultType& v, const std::string& null_subst = "")
	{
		v.clear();

		if (src.empty() || delimit.empty())
			return 0;

		bool substIsNull = null_subst.empty() ? true : false;
		std::string::size_type deli_len = delimit.size();
		size_t index = std::string::npos, last_search_position = 0;
		while ((index = src.find(delimit, last_search_position)) != std::string::npos)
		{
			if (index == last_search_position)
			{
				if (!substIsNull)
					v.insert(v.end(), null_subst);
			}
			else
			{
				std::string temp = src.substr(last_search_position, index - last_search_position);
				Trim(temp);
				if (!temp.empty())
					v.insert(v.end(), temp);
				else if (!substIsNull)
					v.insert(v.end(), null_subst);
			}

			last_search_position = index + deli_len;
		}

		std::string last_one = src.substr(last_search_position);
		Trim(last_one);
		if (!last_one.empty())
			v.insert(v.end(), last_one);
		else if (!substIsNull)
			v.insert(v.end(), null_subst);

		return v.size();
	};
};

