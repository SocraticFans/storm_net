#ifndef _STORM_UTIL_FILE_
#define _STORM_UTIL_FILE_

#include <string>
#include <vector>
#include <stdint.h>
using namespace std;

namespace storm
{

class UtilFile
{
public:
	static string getHomeDir();
	static string getCurrentDir();
	static string getSimplifiedPath(const string &sPath);
	static string getAbsolutePath(const string &sPath, const string &sBasePath = ".");
	static string getCanonicalPath(const string &sPath);

	static string getFileBasename(const string &sPath);
	static string getFileDirname(const string &sPath);
	static string getFileExt(const string &sPath, bool bWithDot = false);
	static string replaceFileExt(const string &sPath, const string &sReplace, bool bWithDot = false);

	static string loadFromFile(const string &sFileName);
	static void saveToFile(const string &sFileName, const string &sContent);

	static bool removeFile(const string &sPath);
	static bool moveFile(const string &sPathFrom, const string &sPathTo);
	static bool makeDirectory(const string &sPath);
	static bool makeDirectoryRecursive(const string &sPath);
	static bool isFileExists(const string &sPath);
	static bool isDirectoryExists(const string &sPath);

	static void listDirectory(const string &sPath, vector<string> &vFiles, bool bRecursive, bool bWithDirectory = true);

	static int32_t doFile(const string &sFileName);
};

}

#endif
