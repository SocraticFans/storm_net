#ifndef _MFW_UTIL_STRING_
#define _MFW_UTIL_STRING_

#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iomanip>
#include <limits>
#include <cmath>
using namespace std;

namespace storm
{

class UtilString
{
public:
	static string trim(const string &s);
	static string trimLeft(const string &s);
	static string trimRight(const string &s);
	static bool isTrimEmpty(const string &s);

	static string toupper(const string &s);
	static string tolower(const string &s);
	static void toupperInplace(string &s);
	static void tolowerInplace(string &s);

	static bool isContains(const string &s, const string &match);
	static bool isBeginsWith(const string &s, const string &match);
	static bool isEndsWith(const string &s, const string &match);
	static bool isdigit(const string &s);
	static bool isxdigit(const string &s);

	static string replace(const string &s, const string &from, const string &to);
	static string format(const char *fmt, ...);

	struct streamstr
	{
		ostringstream os;

		template <typename T>
		streamstr &operator<< (const T &t) { os << t; return *this; }

		typedef ostream &(*manip)(ostream &);
		streamstr &operator<< (manip pf) { os << pf; return *this; }

		operator string () const { return os.str(); }
	};

	template <typename T>
	static string tostr(const T &t);

	template <typename T>
	static T strto(const string &s);

	template <typename T>
	static T strto(const string &s, const T &def)
	{
		if (s.empty()) return def;
		return strto<T>(s);
	}

	static void splitString(const string &str, const string &sep, bool bStrip, vector<string> &vResult);
	static vector<string> splitString(const string &str, const string &sep, bool bStrip = true);

	template <typename T> static void splitString(const string &str, const string &sep, bool bStrip, vector<T> &vResult);
	template <typename T> static vector<T> splitString(const string &str, const string &sep, bool bStrip = true);

	static string joinString(const vector<string> &v, const string &sep);
	template <typename T> static string joinString(const vector<T> &v, const string &sep);

	static void splitString2(const string &str, const string &sep1, const string &sep2, vector<pair<string, string> > &vResult);
	static void splitString2(const string &str, const string &sep1, const string &sep2, map<string, string> &mResult);
	static map<string, string> splitString2(const string &str, const string &sep1, const string &sep2);

	template <typename T1, typename T2>
	static void splitString2(const string &str, const string &sep1, const string &sep2, vector<pair<T1, T2> > &vResult);
	template <typename T1, typename T2>
	static void splitString2(const string &str, const string &sep1, const string &sep2, map<T1, T2> &mResult);
	template <typename T1, typename T2>
	static map<T1, T2> splitString2(const string &str, const string &sep1, const string &sep2);

	static string joinString2(const vector<pair<string, string> > &v, const string &sep1, const string &sep2);
	template <typename T1, typename T2>
	static string joinString2(const vector<pair<T1, T2> > &v, const string &sep1, const string &sep2);
	template <typename T1, typename T2>
	static string joinString2(const map<T1, T2> &v, const string &sep1, const string &sep2);

	static string repeat(const string &s, uint32_t n);
	static string repeat(const string &s, const string &sep, uint32_t n);

	static const string &getEmptyString();

	static string gbk2utf8(const string &s);
	static string utf82gbk(const string &s);

	static uint64_t parseHumanReadableSize(const string &s);

	enum REPACE_MODE
	{
		REPLACE_MODE_ALL_MATCH,
		REPLACE_MODE_KEEP_VARIABLE_ON_MISS,
		REPLACE_MODE_EMPTY_VARIABLE_ON_MISS,
		REPLACE_MODE_EMPTY_RESULT_ON_MISS,
	};

	static string replaceVariable(const string &s, const map<string, string> &mVariable, REPACE_MODE mode);

	static string joinURLParam(const map<string, string> &mParam, const string &sep1 = "&", const string &sep2 = "=");
	static void splitURLParam(const string &sParam, map<string, string> &mParam, const string &sep1 = "&", const string &sep2 = "=");
};

namespace impl
{
	template <typename T>
	struct tostr_helper
	{
		static string tostr(const T &t)
		{
			ostringstream ss;
			ss << t;
			return ss.str();
		}
	};

	template <>
	struct tostr_helper <float>
	{
		static string tostr(float t)
		{
			ostringstream ss;
			float r = (uint32_t)1 << numeric_limits<float>::digits;
			if (t <= r && t >= -r && floor(t) == t)
			{
				// 不会失真的整数
				ss << (int32_t)t;
			}
			else
			{
				ss << setprecision(8) << t;
			}
			return ss.str();
		}
	};

	template <>
	struct tostr_helper <double>
	{
		static string tostr(double t)
		{
			ostringstream ss;
			double r = (uint64_t)1 << numeric_limits<double>::digits;
			if (t <= r && t >= -r && floor(t) == t)
			{
				// 不会失真的整数
				ss << (int64_t)t;
			}
			else
			{
				ss << setprecision(15) << t;
			}
			return ss.str();
		}
	};

	template <typename T>
	struct lexical_castor
	{
		static T cast(const string &s)
		{
			istringstream ss(s);
			T t;
			ss >> t;
			return t;
		}
	};

	template<> struct lexical_castor <uint64_t>
	{
		static uint64_t cast(const string &s)
		{
			return strtoull(s.c_str(), NULL, 0);
		}
	};

	template<> struct lexical_castor <bool>
	{
		static bool cast(const string &s) { return lexical_castor<uint64_t>::cast(s) ? true : false; }
	};

	template<> struct lexical_castor <int8_t>
	{
		static int8_t cast(const string &s) { return static_cast<int8_t>(lexical_castor<uint64_t>::cast(s)); }
	};

	template<> struct lexical_castor <uint8_t>
	{
		static uint8_t cast(const string &s) { return static_cast<uint8_t>(lexical_castor<uint64_t>::cast(s)); }
	};

	template<> struct lexical_castor <int16_t>
	{
		static int16_t cast(const string &s) { return static_cast<int16_t>(lexical_castor<uint64_t>::cast(s)); }
	};

	template<> struct lexical_castor <uint16_t>
	{
		static uint16_t cast(const string &s) { return static_cast<uint16_t>(lexical_castor<uint64_t>::cast(s)); }
	};

	template<> struct lexical_castor <int32_t>
	{
		static int32_t cast(const string &s) { return static_cast<int32_t>(lexical_castor<uint64_t>::cast(s)); }
	};

	template<> struct lexical_castor <uint32_t>
	{
		static uint32_t cast(const string &s) { return static_cast<uint32_t>(lexical_castor<uint64_t>::cast(s)); }
	};

	template<> struct lexical_castor <int64_t>
	{
		static int64_t cast(const string &s) { return static_cast<int64_t>(lexical_castor<uint64_t>::cast(s)); }
	};

	template<> struct lexical_castor <float>
	{
		static float cast(const string &s)
		{
			return strtof(s.c_str(), NULL);
		}
	};

	template<> struct lexical_castor <double>
	{
		static double cast(const string &s)
		{
			return strtod(s.c_str(), NULL);
		}
	};

	template<> struct lexical_castor <string>
	{
		static const string &cast(const string &s)
		{
			return s;
		}
	};

}

template <typename T>
string UtilString::tostr(const T &t)
{
	return impl::tostr_helper<T>::tostr(t);
}

template <typename T>
T UtilString::strto(const string &s)
{
	return impl::lexical_castor<T>::cast(s);
}

template <typename T>
void UtilString::splitString(const string &str, const string &sep, bool bStrip, vector<T> &vResult)
{
	vector<string> vStrResult;
	splitString(str, sep, bStrip, vStrResult);
	for (unsigned i = 0; i < vStrResult.size(); ++i)
	{
		T tmp = strto<T>(vStrResult[i]);
		vResult.push_back(tmp);
	}
}

template <typename T>
vector<T> UtilString::splitString(const string &str, const string &sep, bool bStrip)
{
	vector<T> vResult;
	splitString<T>(str, sep, bStrip, vResult);
	return vResult;
}

template <typename T1, typename T2>
void UtilString::splitString2(const string &str, const string &sep1, const string &sep2, vector< pair<T1, T2> > &vResult)
{
	vector<pair<string, string> > vStrResult;
	splitString2(str, sep1, sep2, vStrResult);
	for (unsigned i = 0; i < vStrResult.size(); ++i)
	{
		pair<string, string> &kv = vStrResult[i];
		vResult.push_back(make_pair(strto<T1>(kv.first), strto<T2>(kv.second)));
	}
}

template <typename T1, typename T2>
void UtilString::splitString2(const string &str, const string &sep1, const string &sep2, map<T1, T2> &mResult)
{
	vector<pair<string, string> > vStrResult;
	splitString2(str, sep1, sep2, vStrResult);
	for (unsigned i = 0; i < vStrResult.size(); ++i)
	{
		pair<string, string> &kv = vStrResult[i];
		mResult[strto<T1>(kv.first)] = strto<T2>(kv.second);
	}
}

template <typename T1, typename T2>
map<T1, T2> UtilString::splitString2(const string &str, const string &sep1, const string &sep2)
{
	map<T1, T2> mResult;
	splitString2<T1, T2>(str, sep1, sep2, mResult);
	return mResult;
}

template <typename T>
string UtilString::joinString(const vector<T> &v, const string &sep)
{
	string sResult;
	for (unsigned i = 0; i < v.size(); ++i)
	{
		if (!sResult.empty())
		{
			sResult.append(sep);
		}
		sResult.append(tostr(v[i]));
	}
	return sResult;
}

template <typename T1, typename T2>
string UtilString::joinString2(const vector<pair<T1, T2> > &v, const string &sep1, const string &sep2)
{
	string sResult;
	for (unsigned i = 0; i < v.size(); ++i)
	{
		if (!sResult.empty())
		{
			sResult.append(sep1);
		}

		const pair<T1, T2> &kv = v[i];
		sResult.append(tostr(kv.first));
		sResult.append(sep2);
		sResult.append(tostr(kv.second));
	}
	return sResult;
}

template <typename T1, typename T2>
string UtilString::joinString2(const map<T1, T2> &v, const string &sep1, const string &sep2)
{
	string sResult;
	for (typename map<T1, T2>::const_iterator first = v.begin(), last = v.end(); first != last; ++first)
	{
		if (!sResult.empty())
		{
			sResult.append(sep1);
		}

		sResult.append(tostr(first->first));
		sResult.append(sep2);
		sResult.append(tostr(first->second));
	}
	return sResult;
}

}

#endif
