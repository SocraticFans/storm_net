#include "util_log.h"

#include "util_file.h"
#include "util_thread.h"

#include <string.h>
#include <stdio.h>
#include <thread>
#include <unistd.h>

namespace storm {

#define FILE_REOPEN_INTERVAL 10

const char *briefLogFileName(const char *name) {
	/*
	if (name[0] != '/') {
		return name;
	}
	*/
	const char* p = strrchr(name, '/');
	if (p != NULL) {
		return p + 1;
	}
	return name;
}

class LogBase {
public:
	LogBase(const string& path, const string fileNamePrefix, const string& fileName)
		:m_path(path), m_fileName(fileName), m_fileSize(0), m_openTime(0), m_lastTime(0), m_fp(NULL) {

		if (m_path.empty()) {
			m_path = ".";
		}
		if (m_fileName.empty()) {
			m_fullFileName = m_path + "/" + fileNamePrefix;
		} else {
			m_fullFileName = m_path + "/" + fileNamePrefix + "_" + m_fileName;
		}
	}
	virtual ~LogBase() {
		if (m_fp != NULL) {
			fclose(m_fp);
			m_fp = NULL;
		}
	}

	void writeLog(const LogData& data) {
		if (prepareLogFile(data.logTime)) {
			uint64_t n = 0;
			n += fwrite(data.content.c_str(), 1, data.content.size(), m_fp);
			m_fileSize += n;
			fflush(m_fp);
		}
		m_lastTime = data.logTime;
	}

	virtual void closeExpiredFile() {}
	virtual bool prepareLogFile(uint32_t logTime) = 0;

	const string& getLogFullName() const { return m_fullFileName; }

protected:
	string m_path;
	string m_fileName;
	string m_fullFileName;
	uint64_t m_fileSize;
	uint32_t m_openTime;
	uint32_t m_lastTime;
	FILE* m_fp;
};


static uint32_t g_maxFileNum = 10;
static uint64_t g_maxFileSize = 100 * 1024 * 1024;

class RollLog : public LogBase {
public:
	RollLog(const string& path, const string& fileNamePrefix, const string& fileName)
		:LogBase(path, fileNamePrefix, fileName),
		 m_maxFile(g_maxFileNum), m_maxSize(g_maxFileSize) {
	}

	void setInfo(uint32_t maxFileNum, uint64_t maxFileSize) {
		m_maxFile = maxFileNum;
		m_maxSize = maxFileSize;
	}

protected:
	virtual bool prepareLogFile(uint32_t logTime) {
		bool reOpen = false;
		if (m_fp == NULL) {
			reOpen = true;
			m_openTime = logTime;
		} else if (logTime - FILE_REOPEN_INTERVAL > m_openTime) {
			reOpen = true;
			fclose(m_fp);
			m_fp = NULL;
			m_openTime = logTime;
		}

		if (reOpen) {
			string fileName = getRollLogFileName(0);
			m_fp = fopen(fileName.c_str(), "a+");
			if (m_fp == NULL) {
				return false;
			}
			fseeko(m_fp, 0, SEEK_END);
			m_fileSize = ftello(m_fp);
		}

		if (m_fileSize < m_maxSize) {
			return true;
		}

		// close
		fclose(m_fp);
		m_fp = NULL;

		//roll
		UtilFile::removeFile(getRollLogFileName(m_maxFile - 1));
		for (int i = m_maxFile - 2; i >= 0; --i) {
			UtilFile::moveFile(getRollLogFileName(i), getRollLogFileName(i + 1));
		}

		return prepareLogFile(logTime);
	}

	string getRollLogFileName(uint32_t index) {
		if (index == 0) {
			return getLogFullName() + ".log";
		}
		return getLogFullName() + UtilString::tostr(index) + ".log";
	}

protected:
	uint32_t m_maxFile;
	uint64_t m_maxSize;
};

class DayLog : public LogBase {
public:
	DayLog(const string& path, const string& fileNamePrefix, const string& fileName)
		:LogBase(path, fileNamePrefix, fileName) {
	}

protected:
	virtual bool prepareLogFile(uint32_t logTime) {
		bool reOpen = false;
		uint32_t nowDate = UtilTime::getDate(logTime);
		uint32_t lastDate = UtilTime::getDate(m_lastTime);
		if (m_fp == NULL) {
			reOpen = true;
			m_openTime = logTime;
		} else if (nowDate != lastDate || (logTime - FILE_REOPEN_INTERVAL > m_openTime)) {
			reOpen = true;
			fclose(m_fp);
			m_fp = NULL;
			m_openTime = logTime;
		}

		if (reOpen) {
			string fileName = getLogFullName() + "_" + UtilString::tostr(nowDate) + ".log";
			m_fp = fopen(fileName.c_str(), "a+");
			if (m_fp == NULL) {
				return false;
			}
			fseeko(m_fp, 0, SEEK_END);
			m_fileSize = ftello(m_fp);
		}
		return true;
	}
};

class HourLog : public LogBase {
public:
	HourLog(const string& path, const string& fileNamePrefix, const string& fileName)
		:LogBase(path, fileNamePrefix, fileName) {
	}

protected:
	virtual bool prepareLogFile(uint32_t logTime) {
		bool reOpen = false;
		uint32_t nowDateHour = UtilTime::getDateHour(logTime);
		uint32_t lastDateHour = UtilTime::getDateHour(m_lastTime);
		if (m_fp == NULL) {
			reOpen = true;
			m_openTime = logTime;
		} else if (nowDateHour != lastDateHour || (logTime - FILE_REOPEN_INTERVAL > m_openTime)) {
			reOpen = true;
			fclose(m_fp);
			m_fp = NULL;
			m_openTime = logTime;
		}

		if (reOpen) {
			char buf[32];
			snprintf(buf, sizeof(buf), "%d_%d",  nowDateHour / 100, nowDateHour % 100);
			string fileName = getLogFullName() + "_" + buf + ".log";
			m_fp = fopen(fileName.c_str(), "a+");
			if (m_fp == NULL) {
				return false;
			}
			fseeko(m_fp, 0, SEEK_END);
			m_fileSize = ftello(m_fp);
		}
		return true;
	}
};

static string g_logPath = ".";
static string g_logPrefix;
static bool g_syncLog = true;
static bool g_logExit = false;
static std::thread g_logThread;
static Mutex g_logMutex;
static list<LogData::ptr> g_logList;
static map<string, LogBase*> g_logs;

LogLevel g_level = LogLevel_Debug;
LogLevel g_stormLevel = LogLevel_Debug;

void LogManager::initLog(const string& path, const string fileNamePrefix) {
	ScopeMutex<Mutex> lock(g_logMutex);
	g_logPath = UtilFile::getAbsolutePath(path);
	UtilFile::makeDirectoryRecursive(g_logPath);
	g_logPrefix = fileNamePrefix;

	atexit(LogManager::finish);

	lock.unlock();
	startAsyncThread();
}

void LogManager::setLogSync(bool sync) {
	ScopeMutex<Mutex> lock(g_logMutex);
	g_syncLog = sync;
}

void LogManager::setLogLevel(LogLevel level) {
	g_level = level;
}

void LogManager::setStormLogLevel(LogLevel level) {
	g_stormLevel = level;
}

void LogManager::setRollLogInfo(const string& logName, uint32_t maxFileNum, uint64_t maxFileSize) {
	ScopeMutex<Mutex> lock(g_logMutex);
	LogBase* log = getLog(logName, LogType_Roll);
	RollLog* r = dynamic_cast<RollLog*>(log);
	if (r != NULL) {
		r->setInfo(maxFileNum, maxFileSize);
	}
}

LogLevel LogManager::parseLevel(const string& levelStr) {
	LogLevel level = LogLevel_Debug;
	string s = UtilString::toupper(levelStr);
	if (s == "DEBUG") {
		level = LogLevel_Debug;
	} else if (s == "INFO") {
		level = LogLevel_Info;
	} else if (s == "ERROR") {
		level = LogLevel_Error;
	} else if (s == "EXCEPTION") {
		level = LogLevel_Exception;
	} else if (s == "NONE") {
		level = LogLevel_None;
	}
	return level;	
}

void LogManager::finish() {
	ScopeMutex<Mutex> lock(g_logMutex);
	if (g_logExit == false) {
		g_logExit = true;
		lock.unlock();
		g_logThread.join();
	}
}

void LogManager::doLog(LogData::ptr logData) {
	if (g_syncLog == true) {
		realDoLog(logData);
	} else {
		ScopeMutex<Mutex> lock(g_logMutex);
		g_logList.push_back(logData);
	}
}

void LogManager::realDoLog(LogData::ptr logData) {
	LogBase* l = getLog(logData->logName, logData->logType);
	l->writeLog(*logData);
}

LogBase* LogManager::getLog(const string& logName, LogType logType) {
	string key = logName;
	switch (logType) {
		case LogType_Roll:
			key.append("-R");
			break;
		case LogType_Day:
			key.append("-D");
			break;
		case LogType_Hour:
			key.append("-H");
			break;
	}
	auto it = g_logs.find(key);
	if (it != g_logs.end()) {
		return it->second;
	}

	LogBase* log = NULL;
	switch (logType) {
		case LogType_Roll:
			log = new RollLog(g_logPath, g_logPrefix, logName);
			break;
		case LogType_Day:
			log = new DayLog(g_logPath, g_logPrefix, logName);
			break;
		case LogType_Hour:
			log = new HourLog(g_logPath, g_logPrefix, logName);
			break;
	}
	g_logs.insert(make_pair(key, log));

	return log;
}

static void logThreadEntry() {
	while (1) {
		ScopeMutex<Mutex> lock(g_logMutex);
		list<LogData::ptr> allLogs;
		swap(allLogs, g_logList);
		lock.unlock();

		if (allLogs.empty()) {
			if (g_logExit) {
				break;
			}
			usleep(10 * 1000);
			continue;
		}
		for (auto l : allLogs) {
			LogManager::realDoLog(l);
		}
	}

	ScopeMutex<Mutex> lock(g_logMutex);
	for (auto l : g_logs) {
		delete l.second;
	}
}

void LogManager::startAsyncThread() {
	g_logThread = std::thread(logThreadEntry);
}

const char* g_logLevelName[LogLevel_None] = {
	"DEBUG|",
	"INFO|",
	"ERROR|",
	"EXCEPTION|"
};

__thread ostringstream* t_oss = NULL;
__thread char t_timeStr[32];
__thread uint32_t t_lastTime = 0;

ostringstream& getOss() {
	if (__builtin_expect(t_oss == NULL, 0)) {
		t_oss = new ostringstream;
	}
	return *t_oss;
}

static void prepareTimeStr(uint32_t now) {
	if (now != t_lastTime) {
		struct tm tmnow;
		time_t t = now;
		localtime_r(&t, &tmnow);

		strftime(t_timeStr, sizeof(t_timeStr), "%Y-%m-%d %H:%M:%S", &tmnow);

		t_lastTime = now;
	}
}

LogStream::LogStream(const string& logName, LogType logType)
	:m_logName(logName), m_logType(logType) {

	ostringstream& oss = getOss();
	oss.clear();
	oss.str("");
	uint64_t nowUs = UtilTime::getNowUs();
	m_logTime = nowUs / MICROSEC_PER_SEC;

	uint32_t us = nowUs % MICROSEC_PER_SEC;
	char buf[8];
	snprintf(buf, sizeof(buf), ".%06d", us);

	prepareTimeStr(m_logTime);
	oss << t_timeStr << buf << "|";
}

LogStream::LogStream(const string& logName, LogType logType, LogLevel logLevel, bool isFrameWork,
					 const string& fileName, int line, const string& func)
	:m_logName(logName), m_logType(logType) {

	ostringstream& oss = getOss();
	oss.clear();
	oss.str("");
	uint64_t nowUs = UtilTime::getNowUs();
	m_logTime = nowUs / MICROSEC_PER_SEC;

	uint32_t us = nowUs % MICROSEC_PER_SEC;
	char buf[8];
	snprintf(buf, sizeof(buf), ".%06d", us);

	prepareTimeStr(m_logTime);
	oss << t_timeStr << buf << "|" << getThreadIdStr() << "|" << g_logLevelName[logLevel];
	if (isFrameWork) {
		oss << "[STORM]|";
	}
	oss << briefLogFileName(fileName.c_str()) << "(" << line << "):" << func << "|";
}

LogStream::~LogStream() {
	ostringstream& oss = getOss();
	oss << endl;

	LogData::ptr logData(new LogData());
	logData->logTime = m_logTime;
	logData->logName = m_logName;
	logData->logType = m_logType;
	logData->content = oss.str();

	LogManager::doLog(logData);
}

ostringstream& LogStream::stream() {
	return getOss();
}

RollDayStream::RollDayStream(const string& logName, LogLevel logLevel, const string& fileName, int line, const string& func)
	:m_logName(logName) {
	ostringstream& oss = getOss();
	oss.clear();
	oss.str("");
	uint64_t nowUs = UtilTime::getNowUs();
	m_logTime = nowUs / MICROSEC_PER_SEC;

	uint32_t us = nowUs % MICROSEC_PER_SEC;
	char buf[8];
	snprintf(buf, sizeof(buf), ".%06d", us);

	prepareTimeStr(m_logTime);
	oss << t_timeStr << buf << "|" << getThreadIdStr() << "|" << g_logLevelName[logLevel];
	oss << briefLogFileName(fileName.c_str()) << "(" << line << "):" << func << "|";
}

RollDayStream::~RollDayStream() {
	ostringstream& oss = getOss();
	oss << endl;

	//RollLog
	LogData::ptr logData(new LogData());
	logData->logTime = m_logTime;
	logData->logName = string("");
	logData->logType = LogType_Roll;
	logData->content = oss.str();
	LogManager::doLog(logData);

	//DayLog
	LogData::ptr logData2(new LogData());
	logData2->logTime = m_logTime;
	logData2->logName = m_logName;
	logData2->logType = LogType_Day;
	logData2->content = oss.str();
	LogManager::doLog(logData2);
}

ostringstream& RollDayStream::stream() {
	return getOss();
}

}
