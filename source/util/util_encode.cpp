#include "util_encode.h"
#include <cstdio>

namespace storm
{

static const char g_hexcode[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

static char getHex(uint8_t hi, uint8_t lo)
{
	hi = hi >= 'a' ? (hi - 'a' + 10) : (hi >= 'A' ? (hi - 'A' + 10) : hi - '0');
	lo = lo >= 'a' ? (lo - 'a' + 10) : (lo >= 'A' ? (lo - 'A' + 10) : lo - '0');
	return (hi << 4) | lo;
}

string UtilEncode::toHex(const string &s)
{
	string r;
	r.resize(s.size() * 2);
	for (string::size_type i = 0; i < s.size(); ++i)
	{
		uint8_t c = s[i];
		r[i * 2] = g_hexcode[c >> 4];
		r[i * 2 + 1] = g_hexcode[c & 0xf];
	}
	return r;
}

string UtilEncode::fromHex(const string &s)
{
	string r;
	string::size_type n = s.size() / 2;
	r.resize(n);
	for (string::size_type i = 0; i < n; ++i)
	{
		uint8_t hi = s[i * 2];
		uint8_t lo = s[i * 2 + 1];
		r[i] = getHex(hi, lo);
	}
	return r;
}

string UtilEncode::c_unescape(const string &s)
{
	string r;
	for (unsigned i = 0; i < s.size(); ++i)
	{
		if (s[i] != '\\' || i + 1 == s.size())
		{
			r.push_back(s[i]);
			continue;
		}

		char c = s[++i];
		switch(c)
		{
		case 'a': r.push_back('\a'); break;
		case 'b': r.push_back('\b'); break;
		case 'f': r.push_back('\f'); break;
		case 'n': r.push_back('\n'); break;
		case 'r': r.push_back('\r'); break;
		case 't': r.push_back('\t'); break;
		case 'v': r.push_back('\v'); break;
		case '\\': r.push_back('\\'); break;
		case '\'': r.push_back('\''); break;
		case '"': r.push_back('\"'); break;
		case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
			{
				unsigned char val = c - '0';
				if (i + 1 < s.size() && s[i + 1] >= '0' && s[i + 1] <= '7')
				{
					c = s[++i];
					val = val * 8 + (c - '0');
					if (val <= 037 && i + 1 < s.size() && s[i + 1] >= '0' && s[i + 1] <= '7')
					{
						c = s[++i];
						val = val * 8 + (c - '0');
					}
				}
				r.push_back(val);
			}
			break;
		case 'x':
			{
				if (i + 1 < s.size() && ::isxdigit(s[i + 1]))
				{
					c = ::tolower(s[++i]);
					unsigned char val = c >= 'a' ? (c - 'a' + 10) : (c - '0');
					if (i + 1 < s.size() && ::isxdigit(s[i + 1]))
					{
						c = ::tolower(s[++i]);
						val = val * 16 + (c >= 'a' ? (c - 'a' + 10) : (c - '0'));
					}
					r.push_back(val);
				}
				else
				{
					r.push_back('\\');
					r.push_back(c);
				}
			}
			break;
		default:
			r.push_back(c);
			break;
		}
	}
	return r;
}

string UtilEncode::c_escape(const string &s)
{
	string r;
	for (unsigned i = 0; i < s.size(); ++i)
	{
		char c = s[i];
		switch(c)
		{
		case '\a': r.append("\\a"); break;
		case '\b': r.append("\\b"); break;
		case '\f': r.append("\\f"); break;
		case '\n': r.append("\\n"); break;
		case '\r': r.append("\\r"); break;
		case '\t': r.append("\\t"); break;
		case '\v': r.append("\\v"); break;
		case '\\': r.append("\\\\"); break;
		case '\'': r.append("\\\'"); break;
		case '"': r.append("\\\""); break;
		default:
			{
				if (isprint(c) || (c & 0x80))
				{
					r.push_back(c);
				}
				else
				{
					r.append("\\x");
					char buf[4];
					snprintf(buf, sizeof(buf), "%02x", (unsigned)(unsigned char)c);
					r.append(buf);
				}
			}
			break;
		}
	}
	return r;
}

string UtilEncode::urlencode(const string &s)
{
	string r;
	for (unsigned i = 0; i < s.size(); ++i)
	{
		char c = s[i];
		if (c == ' ')
		{
			r.push_back('+');
		}
		else if ((c < '0' && c != '-' && c != '.') ||
				   (c < 'A' && c > '9') ||
				   (c > 'Z' && c < 'a' && c != '_') ||
				   (c > 'z'))
		{
			r.push_back('%');
			r.push_back(g_hexcode[(uint8_t)c >> 4]);
			r.push_back(g_hexcode[(uint8_t)c & 15]);
		}
		else
		{
			r.push_back(c);
		}
	}
	return r;
}

string UtilEncode::urldecode(const string &s)
{
	string r;
	for (unsigned i = 0; i < s.size(); ++i)
	{
		char c = s[i];
		if (c == '+')
		{
			r.push_back(' ');
		}
		else if (c == '%'
				&& ((int)s.size() - 1 - i) >= 2
				&& ::isxdigit(s[i + 1])
				&& ::isxdigit(s[i + 2]))
		{
			r.push_back(getHex(s[i + 1], s[i + 2]));
			i += 2;
		}
		else
		{
			r.push_back(c);
		}
	}
	return r;
}

string UtilEncode::rawurlencode(const string &s)
{
	string r;
	for (unsigned i = 0; i < s.size(); ++i)
	{
		char c = s[i];
		if ((c < '0' && c != '-' && c != '.') ||
			(c < 'A' && c > '9') ||
			(c > 'Z' && c < 'a' && c != '_') ||
			(c > 'z' && c != '~'))
		{
			r.push_back('%');
			r.push_back(g_hexcode[(uint8_t)c >> 4]);
			r.push_back(g_hexcode[(uint8_t)c & 15]);
		}
		else
		{
			r.push_back(c);
		}
	}
	return r;
}

string UtilEncode::rawurldecode(const string &s)
{
	string r;
	for (unsigned i = 0; i < s.size(); ++i)
	{
		char c = s[i];
		if (c == '%'
				&& ((int)s.size() - 1 - i) >= 2
				&& ::isxdigit(s[i + 1])
				&& ::isxdigit(s[i + 2]))
		{
			r.push_back(getHex(s[i + 1], s[i + 2]));
			i += 2;
		}
		else
		{
			r.push_back(c);
		}
	}
	return r;
}

}
