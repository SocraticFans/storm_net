#ifndef _MFW_UTIL_CONFIG_
#define _MFW_UTIL_CONFIG_

#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include "util_string.h"
using namespace std;

namespace storm
{

class CConfig
{
public:
	void parseFile(const string &sFile);
	void parseString(const string &sData);
	string toString(uint32_t iIndent = 0) const;
	void toString(string &sData, uint32_t iIndent = 0) const;

	bool hasConfigKey(const string &sPath) const;
	bool hasSubConfig(const string &sPath) const;

	//const string &operator[] (const string &sPath) const;
	const string &getCfg(const string &sPath) const;
	const string &getCfg(const string &sPath, const string &sDefault) const;

	template <typename T>
	T getCfg(const string &sPath) const
	{
		return UtilString::strto<T>(getCfg(sPath));
	}

	template <typename T>
	T getCfg(const string &sPath, const T &def) const
	{
		const string *pvalue = getConfigKeyPtr(sPath);
		if (pvalue == NULL)
		{
			return def;
		}
		return UtilString::strto<T>(*pvalue);
	}

	CConfig &getSubConfig(const string &sPath);
	const CConfig &getSubConfig(const string &sPath) const;
	void joinConfig(const CConfig &config);

	const vector<string> &getAllItem() const { return m_vItem; }
	const vector<string> &getAllItem(const string &sPath) const { return getSubConfig(sPath).getAllItem(); }
	const map<string, string> &getAllKeyValue() const { return m_mKeyValue; }
	const map<string, string> &getAllKeyValue(const string &sPath) const { return getSubConfig(sPath).getAllKeyValue(); }
	const map<string, CConfig> &getAllSubConfig() const { return m_mSubConfig; }
	const map<string, CConfig> &getAllSubConfig(const string &sPath) const { return getSubConfig(sPath).getAllSubConfig(); }

protected:
	const string *getConfigKeyPtr(const string &sPath) const;
	const CConfig *getSubConfigPtr(const string &sPath) const;

protected:
	string m_sNodeName;
	vector<string> m_vItem;
	map<string, string> m_mKeyValue;
	map<string, CConfig> m_mSubConfig;
};

}

#endif
