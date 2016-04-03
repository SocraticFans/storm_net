#include "util_time.h"
#include <ctime>
#include <sys/time.h>
#include <string.h>
#include <vector>
#include <stdexcept>
#include "util_string.h"
using namespace std;

namespace storm
{

uint32_t UtilTime::getNow() {
	return time(0);
}

uint64_t UtilTime::getNowMS() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
}

uint64_t UtilTime::getNowUs() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint64_t)tv.tv_sec * MICROSEC_PER_SEC + (uint64_t)tv.tv_usec;
}

string UtilTime::formatTime(uint32_t iTime, const char *fmt)
{
	struct tm tmnow;
	time_t t = iTime;
	localtime_r(&t, &tmnow);

	char buf[256];
	size_t len = strftime(buf, sizeof(buf), fmt, &tmnow);
	buf[len >= sizeof(buf) ? sizeof(buf) - 1 : len] = 0;
	return buf;
}

uint32_t UtilTime::parseTime(const string &sTime, const char *fmt)
{
	struct tm stTm;
	parseTime(sTime, stTm, fmt);
	return mktime(&stTm);
}

void UtilTime::parseTime(const string &sTime, struct tm &stTm, const char *fmt)
{
	memset(&stTm, 0, sizeof(stTm));
	if (strptime(sTime.c_str(), fmt, &stTm) == NULL)
	{
		throw std::runtime_error("invald time: " + sTime + ", format: " + fmt);
	}
}

uint32_t UtilTime::getDate(uint32_t iTime, int32_t offset)
{
	time_t now_time = iTime + (offset * 86400);
	struct tm tmnow;
	localtime_r(&now_time, &tmnow);
	return (tmnow.tm_year + 1900) * 10000 + (tmnow.tm_mon + 1) * 100 + tmnow.tm_mday;
}

uint32_t UtilTime::getHour(uint32_t iTime, int32_t offset) {
	time_t now_time = iTime + (offset * 3600);
	struct tm tmnow;
	localtime_r(&now_time, &tmnow);
	return tmnow.tm_hour;
}

uint32_t UtilTime::getDateHour(uint32_t iTime, int32_t offset) {
	time_t now_time = iTime + (offset * 3600);
	struct tm tmnow;
	localtime_r(&now_time, &tmnow);
	return ((tmnow.tm_year + 1900) * 10000 + (tmnow.tm_mon + 1) * 100 + tmnow.tm_mday) * 100 + tmnow.tm_hour;
}

uint32_t UtilTime::fromDate(uint32_t iDate, uint32_t iHour)
{
	struct tm tmnow;
	memset(&tmnow, 0, sizeof(tmnow));
	tmnow.tm_year = iDate / 10000 - 1900;
	tmnow.tm_mon = iDate / 100 % 100 - 1;
	tmnow.tm_mday = iDate % 100;
	tmnow.tm_hour = iHour;
	return mktime(&tmnow);
}

uint32_t UtilTime::parseDayTime(const string &sTime)
{
	if (sTime.empty())
	{
		return 0;
	}
	vector<uint32_t> vItem = UtilString::splitString<uint32_t>(sTime, ":");
	if (vItem.size() != 3
			|| vItem[0] > 23
			|| vItem[1] > 59
			|| vItem[2] > 59
			)
	{
		throw std::runtime_error("invald day time format: " + sTime);
		return 0;
	}

	return vItem[2] + vItem[1]*100 + vItem[0]*10000;
}

uint32_t UtilTime::getDayTime(uint32_t iTime)
{
	time_t now_time = iTime;
	struct tm tmnow;
	localtime_r(&now_time, &tmnow);
	return tmnow.tm_hour*10000 + tmnow.tm_min*100 + tmnow.tm_sec;
}

bool UtilTime::isInDayTimeRange(uint32_t iTime, uint32_t from, uint32_t end)
{
	return (iTime >= from) && (iTime <= end);
}

static bool findDst(uint32_t iBeginTime, uint32_t iEndTime, uint32_t &iFoundTime)
{
	time_t a = iBeginTime, b = iEndTime;
	struct tm tma, tmb;
	localtime_r(&a, &tma);
	localtime_r(&b, &tmb);
	if (tma.tm_isdst == tmb.tm_isdst)
	{
		return false;
	}

	while (true)
	{
		time_t diff = b - a;
		if (diff < 2)
		{
			break;
		}

		time_t t = a + diff / 2;
		struct tm tmt;
		localtime_r(&t, &tmt);
		if (tma.tm_isdst == tmt.tm_isdst)
		{
			a = t;
			tma = tmt;
		}
		else
		{
			b = t;
		}
	}
	iFoundTime = b;
	return true;
}

bool UtilTime::getLocalDstTime(uint32_t iYear, uint32_t &iDst1, uint32_t &iDst2)
{
	uint32_t a = fromDate(iYear * 10000 + 101);
	uint32_t b = fromDate(iYear * 10000 + 701) - 1;
	bool bFound = findDst(a, b, iDst1);
	if (!bFound)
	{
		return false;
	}

	a = b + 1;
	b = fromDate((iYear + 1) * 10000 + 101) - 1;
	bFound = findDst(a, b, iDst2);
	if (!bFound)
	{
		return false;
	}
	return true;
}

void UtilTimer::reset(uint64_t iInterval, uint64_t iCurrTime)
{
	m_iInterval = iInterval;
	m_iLastTriggerTime = iCurrTime;
}

bool UtilTimer::operator()(uint64_t iCurrTime)
{
	return isReady(iCurrTime);
}

bool UtilTimer::isReady(uint64_t iCurrTime)
{
	if (m_iLastTriggerTime + m_iInterval > iCurrTime)
	{
		return false;
	}

	// 避免超时时间较长时连续触发多次
	if(iCurrTime > m_iLastTriggerTime + m_iInterval + m_iInterval)
	{
		m_iLastTriggerTime = iCurrTime;
	}
	else
	{
		m_iLastTriggerTime += m_iInterval;
	}
	return true;

}

}
