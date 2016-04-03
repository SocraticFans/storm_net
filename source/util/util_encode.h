#ifndef _MFW_UTIL_ENCODE_
#define _MFW_UTIL_ENCODE_

#include <stdint.h>
#include <string>
using namespace std;

namespace storm
{

class UtilEncode
{
public:
	static string toHex(const string &s);
	static string fromHex(const string &s);

	static string c_unescape(const string &s);
	static string c_escape(const string &s);

	static string urlencode(const string &s); // 空格替换为+,~替换为%7e
	static string urldecode(const string &s);
	static string rawurlencode(const string &s); // 空格替换为%20,不转义~
	static string rawurldecode(const string &s);
};

}

#endif
