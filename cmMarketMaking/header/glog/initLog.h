#pragma once

#include <string>
#include "glog\logging.h"

inline bool initLog(char * pathprefix, const int severity, int maxSize)
{
	char fullpath[100];
	google::InitGoogleLogging("");
	google::SetStderrLogging(severity); // google::GLOG_WARNING); //���ü������ google::INFO ����־ͬʱ�������Ļ
	FLAGS_colorlogtostderr = true; //�����������Ļ����־��ʾ��Ӧ��ɫ
	memset(fullpath, 0, sizeof(fullpath));
	strncpy(fullpath, pathprefix, sizeof(fullpath));
	strcat(fullpath, "log_fatal_");
	google::SetLogDestination(google::GLOG_FATAL, fullpath); // ���� google::FATAL �������־�洢·�����ļ���ǰ׺
	memset(fullpath, 0, sizeof(fullpath));
	strncpy(fullpath, pathprefix, sizeof(fullpath));
	strcat(fullpath, "log_error_");
	google::SetLogDestination(google::GLOG_ERROR, fullpath); //���� google::ERROR �������־�洢·�����ļ���ǰ׺
	memset(fullpath, 0, sizeof(fullpath));
	strncpy(fullpath, pathprefix, sizeof(fullpath));
	strcat(fullpath, "log_warn_");
	google::SetLogDestination(google::GLOG_WARNING, fullpath); //���� google::WARNING �������־�洢·�����ļ���ǰ׺
	memset(fullpath, 0, sizeof(fullpath));
	strncpy(fullpath, pathprefix, sizeof(fullpath));
	strcat(fullpath, "log_info_");
	google::SetLogDestination(google::GLOG_INFO, fullpath); //���� google::INFO �������־�洢·�����ļ���ǰ׺
	FLAGS_logbufsecs = 0; //������־�����Ĭ��Ϊ30�룬�˴���Ϊ�������
	FLAGS_max_log_size = 100; //�����־��СΪ 100MB
	FLAGS_stop_logging_if_full_disk = true; //�����̱�д��ʱ��ֹͣ��־���

	return true;
}
