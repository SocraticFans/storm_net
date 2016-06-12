#include "util_config.h"
#include "util_file.h"
#include "util_string.h"
#include <stack>
#include <stdexcept>

namespace storm
{

void CConfig::parseFile(const string &sFile)
{
	string sData = UtilFile::loadFromFile(sFile);
	parseString(sData);
}

void CConfig::parseString(const string &sData)
{
	vector<string> vLines = UtilString::splitString(sData, "\r\n");

	stack<CConfig *> stk;
	stk.push(this);
	for (unsigned i = 0; i < vLines.size(); ++i)
	{
		string s = UtilString::trim(vLines[i]);
		if (s.empty() || s[0] == '#')
		{
			continue;
		}
		if (s[0] == '<')
		{
			if (s.size() <= 1 || s.find('>') != s.size() - 1)
			{
				throw std::runtime_error("[util_config]parse error: " + s);
			}
			if (s[1] == '/')
			{
				string sKey = s.substr(2, s.size() - 3);
				if (stk.size() <= 1 || stk.top()->m_sNodeName != sKey)
				{
					throw std::runtime_error("[util_config]key mismatch: " + sKey);
				}
				stk.pop();
			}
			else
			{
				string sKey = s.substr(1, s.size() - 2);
				CConfig &cfg = *stk.top();
				if (cfg.m_mKeyValue.find(sKey) != cfg.m_mKeyValue.end())
				{
					throw std::runtime_error("[util_config]cannot override key: " + sKey);
				}

				map<string, CConfig>::iterator it = cfg.m_mSubConfig.find(sKey);
				if (it == cfg.m_mSubConfig.end())
				{
					cfg.m_vItem.push_back(sKey);
					CConfig &subcfg = cfg.m_mSubConfig[sKey];
					subcfg.m_sNodeName = sKey;
					it = cfg.m_mSubConfig.find(sKey);
				}
				CConfig &subcfg = it->second;
				stk.push(&subcfg);
			}
		}
		else
		{
			string sKey, sValue;
			string::size_type pos = s.find('=');
			if (pos != string::npos)
			{
				sKey = UtilString::trim(s.substr(0, pos));
				sValue = UtilString::trim(s.substr(pos + 1));
			}
			else
			{
				sKey = s;
			}
			CConfig &cfg = *stk.top();
			if (cfg.m_mSubConfig.find(sKey) != cfg.m_mSubConfig.end())
			{
				throw std::runtime_error("[util_config]cannot override subconf: " + sKey);
			}
			if (cfg.m_mKeyValue.find(sKey) == cfg.m_mKeyValue.end())
			{
				cfg.m_vItem.push_back(sKey);
			}
			cfg.m_mKeyValue[sKey] = sValue;
		}
	}
}

string CConfig::toString(uint32_t iIndent) const
{
	string sData;
	toString(sData, iIndent);
	return sData;
}

void CConfig::toString(string &sData, uint32_t iIndent) const
{
	string sTab = UtilString::repeat("  ", iIndent);
	for (unsigned i = 0; i < m_vItem.size(); ++i)
	{
		const string &sKey = m_vItem[i];
		map<string, string>::const_iterator it1 = m_mKeyValue.find(sKey);
		if (it1 != m_mKeyValue.end())
		{
			sData += sTab + sKey + " = " + it1->second + "\n";
			continue;
		}

		map<string, CConfig>::const_iterator it2 = m_mSubConfig.find(sKey);
		if (it2 != m_mSubConfig.end())
		{
			sData += sTab + "<" + sKey + ">\n";
			it2->second.toString(sData, iIndent + 1);
			sData += sTab + "</" + sKey + ">\n";
		}
	}
}

bool CConfig::hasConfigKey(const string &sPath) const
{
	return getConfigKeyPtr(sPath) == NULL ? false : true;
}

bool CConfig::hasSubConfig(const string &sPath) const
{
	return getSubConfigPtr(sPath) == NULL ? false : true;
}

const string &CConfig::getCfg(const string &sPath) const
{
	const string *pvalue = getConfigKeyPtr(sPath);
	if (pvalue == NULL)
	{
		throw std::runtime_error("[util_config]config key missing: " + sPath);
	}
	return *pvalue;
}

const string &CConfig::getCfg(const string &sPath, const string &sDefault) const
{
	const string *pvalue = getConfigKeyPtr(sPath);
	if (pvalue == NULL)
	{
		return sDefault;
	}
	return *pvalue;
}

CConfig &CConfig::getSubConfig(const string &sPath)
{
	const CConfig &cfg = const_cast<const CConfig *>(this)->getSubConfig(sPath);
	return const_cast<CConfig &>(cfg);
}

const CConfig &CConfig::getSubConfig(const string &sPath) const
{
	const CConfig *pcfg = getSubConfigPtr(sPath);
	if (pcfg == NULL)
	{
		throw std::runtime_error("[util_config]config subconf missing: " + sPath);
	}
	return *pcfg;
}

void CConfig::joinConfig(const CConfig &config)
{
	m_vItem.clear();
	m_mKeyValue.clear();
	m_mSubConfig.clear();
	parseString(toString() + config.toString());
}

// TODO
static string transformPath(const string &sPath)
{
	string s = UtilString::replace(sPath, "<", "/");
	s = UtilString::replace(s, ">", "");
	return s;
}

const string *CConfig::getConfigKeyPtr(const string &sPath_) const
{
	string sPath = transformPath(sPath_);
	string::size_type pos = sPath.rfind('/');

	const CConfig *pcfg = NULL;
	string sKey;
	if (pos == string::npos)
	{
		pcfg = this;
		sKey = sPath;
	}
	else
	{
		pcfg = getSubConfigPtr(sPath.substr(0, pos));
		sKey = sPath.substr(pos + 1);
	}
	if (pcfg == NULL)
	{
		return NULL;
	}
	map<string, string>::const_iterator it = pcfg->m_mKeyValue.find(sKey);
	if (it == pcfg->m_mKeyValue.end())
	{
		return NULL;
	}
	return &it->second;
}

const CConfig *CConfig::getSubConfigPtr(const string &sPath_) const
{
	string sPath = transformPath(sPath_);
	vector<string> vPath = UtilString::splitString(sPath, "/");
	const CConfig *pcfg = this;
	for (unsigned i = 0; i < vPath.size(); ++i)
	{
		const string &sKey = vPath[i];
		map<string, CConfig>::const_iterator it = pcfg->m_mSubConfig.find(sKey);
		if (it == pcfg->m_mSubConfig.end())
		{
			return NULL;
		}
		pcfg = &it->second;
	}
	return pcfg;
}

}
