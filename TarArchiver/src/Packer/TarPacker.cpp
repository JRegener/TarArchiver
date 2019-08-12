#include "TarPacker.h"

void 
TarPacker::pack(const std::string & targetPath)
{
	std::ofstream targetFile;

	std::string targetFilename = extractName(targetPath) + ".tar";
	targetFile.open(targetFilename, std::ios::binary);
	if (!targetFile.is_open())
	{
		return;
	}

	std::string name = getDirFileName(targetPath);
	std::string basePath = targetPath.substr(0, targetPath.length() - name.length());

	if (!pack(targetFile, basePath, name))
	{
		targetFile.close();
		//remove
	}
	else
	{
		addExpand(targetFile);
		targetFile.flush();
		targetFile.close();
	}
}

bool
TarPacker::pack(std::ofstream & targetFile, const std::string & path, const std::string & name)
{
	struct stat s;
	if (stat((path + name).c_str(), &s) == 0)
	{
		if (s.st_mode & S_IFDIR)
		{
			VectorString files = getDirectoryFiles(path + name);
			packDirectory(targetFile, name, s);
			for (auto it = files.begin(); it != files.end(); ++it)
			{
				if (*it == ".." || *it == ".")
				{
					continue;
				}

				if (!pack(targetFile, path, name + '/' + (*it)))
				{
					return false;
				}
			}
		}
		else if (s.st_mode & S_IFREG)
		{
			if (!packRegFile(targetFile, path, name, s))
			{
				return false;
			}
		}
		else if (s.st_mode & S_IFCHR)
		{
			
		}
		else if (s.st_mode & S_IFBLK)
		{

		}
		else if (s.st_mode & S_IFIFO)
		{

		}
		else if (s.st_mode & S_IFLNK)
		{

		}
		else
		{
			
		}
	}
	else
	{
		switch (errno)
		{
		case ENOENT:
			printf("File %s not found.\n", path);
			break;
		case EINVAL:
			printf("Invalid parameter to _stat.\n");
			break;
		default:
			/* Should never be reached. */
			printf("Unexpected error in _stat.\n");
			break;
		}
	}

	return true;
}

VectorString
TarPacker::getDirectoryFiles(const std::string & directory)
{
	VectorString paths;
	
	DIR* curDir = opendir(directory.c_str());
	struct dirent* ent;
	while ((ent = readdir(curDir)) != nullptr)
	{
		paths.emplace_back(ent->d_name);
	}
	closedir(curDir);

	return paths;
}

void 
TarPacker::addExpand(std::ofstream & output)
{
	output.write((char*)&emptyBuffer, BLOCK_SIZE);
	output.write((char*)&emptyBuffer, BLOCK_SIZE);
}

void
TarPacker::packDirectory(std::ofstream & targetFile, const std::string & name, struct stat & s)
{
	HeaderInfo * h = createHeader(name + '/', DIRTYPE, s);
	std::unique_ptr<HeaderInfo> headerInfo(h);
	header = convertHeader(*headerInfo);

	targetFile.write((char*)&header, BLOCK_SIZE);
}

bool 
TarPacker::packRegFile(std::ofstream & targetFile, const std::string & path, const std::string & name, struct stat & s)
{
	std::ifstream fileInput;

	HeaderInfo * h = createHeader(name, REGTYPE, s);
	std::unique_ptr<HeaderInfo> headerInfo(h);
	header = convertHeader(*headerInfo);

	fileInput.open(path + name, std::ios::binary);
	if (!fileInput.is_open())
	{
		return false;
	}

	targetFile.write((char*)&header, BLOCK_SIZE);
	writeContentToTargetFile(headerInfo, fileInput, targetFile);
	fileInput.close();

	return true;
}

void 
TarPacker::writeContentToTargetFile(const std::unique_ptr<HeaderInfo>& headerInfo,
	std::ifstream & input, std::ofstream & output)
{
	/* clear buffer */
	std::memset(&buffer, '\0', BLOCK_SIZE);

	while (input.tellg() != headerInfo->size)
	{
		if (headerInfo->size - input.tellg() >= BLOCK_SIZE)
		{
			input.read((char*)&buffer, BLOCK_SIZE);
		}
		else
		{
			input.read((char*)&buffer, headerInfo->size - input.tellg());
		}

		output.write((char*)&buffer, BLOCK_SIZE);
	}
}

std::string 
TarPacker::extractName(const std::string & path)
{
	std::string name = std::string(path);
	const size_t last_slash_idx = name.find_last_of("\\/");
	if (std::string::npos != last_slash_idx)
	{
		name.erase(0, last_slash_idx + 1);
	}

	/* remove extension */
	const size_t period_idx = name.rfind('.');
	if (std::string::npos != period_idx)
	{
		name.erase(period_idx);
	}

	return name;
}

std::string 
TarPacker::getDirFileName(const std::string & path)
{
	std::string name = std::string(path);
	const size_t last_slash_idx = name.find_last_of("\\/");
	if (std::string::npos != last_slash_idx)
	{
		name.erase(0, last_slash_idx + 1);
	}

	/* if not exist extension add reverce slash '/' */
	//const size_t period_idx = name.rfind('.');
	//if (std::string::npos == period_idx)
	//{
	//	name += '/';
	//}

	return name;
}

HeaderInfo *
TarPacker::createHeader(const std::string & name, int8_t typeflag, struct stat & s)
{
	/* in future change to switch case for create correct type of file */
	
	HeaderInfo * headerInfo = new HeaderInfo();
	struct passwd *pw;
	struct group *gr;

	pw = getpwuid(s.st_uid);
	gr = getgrgid(s.st_gid);

	/**/
	headerInfo->name = name;
	
	headerInfo->mode = s.st_mode & RWX;
	headerInfo->uid = s.st_uid;
	headerInfo->gid = s.st_gid;

	headerInfo->mtime = s.st_mtime;
	headerInfo->checksum = 0;
	headerInfo->typeflag = typeflag;
	
	/**/
	headerInfo->linkname = "";

	headerInfo->magic = TMAGIC;
	headerInfo->version = TVERSION;
	headerInfo->uname = pw->pw_name;
	headerInfo->gname = gr->gr_name;
	//headerInfo->devmajor = ;
	//headerInfo->devminor = ;
	//headerInfo->prefix = ;

	switch (typeflag)
	{
	case AREGTYPE:
	case REGTYPE:
	{
		headerInfo->size = s.st_size;
	}
	break;
	
	case DIRTYPE:
	{
		headerInfo->size = 0;
	}
	break;

	default:
		break;
	}

	return headerInfo;
}


PosixHeader
TarPacker::convertHeader(const HeaderInfo & headerInfo)
{
	PosixHeader header = { 0 };
	int8_t res[12];

	std::memcpy(header.name, headerInfo.name.c_str(), headerInfo.name.length());
	
	toOctStr(headerInfo.mode, res, sizeof(header.mode));
	std::memcpy(header.mode, res, sizeof(header.mode));

	toOctStr(headerInfo.uid, res, sizeof(header.uid));
	std::memcpy(header.uid, res, sizeof(header.uid));
	
	toOctStr(headerInfo.gid, res, sizeof(header.gid));
	std::memcpy(header.gid, res, sizeof(header.gid));

	toOctStr(headerInfo.size, res, sizeof(header.size));
	std::memcpy(header.size, res, sizeof(header.size));

	toOctStr(headerInfo.mtime, res, sizeof(header.mtime));
	std::memcpy(header.mtime, res, sizeof(header.mtime));

	std:memset(header.chksum, 0x20, sizeof(header.chksum));
	
	header.typeflag = headerInfo.typeflag;
	std::memcpy(header.linkname, headerInfo.linkname.c_str(), headerInfo.linkname.length());
	std::memcpy(header.magic, headerInfo.magic.c_str(), headerInfo.magic.length());
	std::memcpy(header.version, headerInfo.version.c_str(), headerInfo.version.length());
	std::memcpy(header.uname, headerInfo.uname.c_str(), headerInfo.uname.length());
	std::memcpy(header.gname, headerInfo.gname.c_str(), headerInfo.gname.length());
	// devmajor
	// devminor

	/* only at the end */
	int64_t checkSum = calculateUnsignedCheckSum(header);
	toOctStr(checkSum, res, 7);
	std::memcpy(header.chksum, res, 6);

	return header;
}

int64_t
TarPacker::calculateUnsignedCheckSum(const PosixHeader & header) 
{
	int64_t checksum = 0;
	const uint8_t * bytes = reinterpret_cast<const uint8_t *>(&header);
	for (size_t i = 0; i < BLOCK_SIZE; ++i) {
		checksum += bytes[i];
	}
	return checksum;
}

void
TarPacker::toOctStr(int64_t value, int8_t * res, int32_t n)
{
	uint32_t radix = 8;

	int32_t i = n - 2;
	while (value > 0)
	{
		res[i] = (value % radix) + '0';
		value /= radix;
		i--;
	}

	while (i >= 0)
	{
		res[i] = '0';
		i--;
	}

	res[n - 1] = '\0';
}
