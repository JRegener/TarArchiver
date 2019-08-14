#pragma once
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <sstream>

#include "../TarCommon.h"

typedef std::vector<std::string> VecStr;

class TarPacker
{
private:
	PosixHeader header;
	ContentData emptyBuffer = { 0 }; /* eof */
	ContentData buffer; 


	bool packInternal(std::ofstream & targetFile, const std::string & path, const std::string & name);

public:
	void pack(const std::string & path);

	bool getDirectoryFiles(const std::string & directory, VecStr & files);

	void addExpand(std::ofstream & output);

	void packDirectory(std::ofstream & targetFile, const std::string & path, const struct stat & s);

	bool packRegFile(std::ofstream & targetFile, const std::string & path, const std::string & name, const struct stat & s);

	void packLink(std::ofstream & targetFile, const std::string & path, 
		const std::string & name, const struct stat & s);

	bool packBlockFile(std::ofstream & targetFile, const std::string & path, const std::string & name, const struct stat & s);

	void writeContentToTargetFile(const std::unique_ptr<HeaderInfo> & headerInfo,
		std::ifstream & input, std::ofstream & output);

	std::string extractName(const std::string & path);

	std::string getDirFileName(const std::string & path);

	HeaderInfo * createHeader(const std::string & path, int8_t typeflag, const struct stat & s, const std::string & linkname = "");

	PosixHeader convertHeader(const HeaderInfo & headerInfo);

	int64_t calculateUnsignedCheckSum(const PosixHeader & header);

	void toOctStr(int64_t value, int8_t* res, int32_t n);
};