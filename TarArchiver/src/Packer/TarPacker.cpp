#include "TarPacker.h"

void 
TarPacker::pack(const std::string & targetPath)
{
	std::ofstream targetFile;

	std::string targetFilename = extractName(targetPath) + ".tar";
	std::string name = getDirFileName(targetPath);
	std::string basePath = targetPath.substr(0, targetPath.length() - name.length());

	targetFile.open(targetFilename, std::ios::binary);
	if (!targetFile.is_open())
	{
		return;
	}

	if (!packInternal(targetFile, basePath, name))
	{
		targetFile.close();
		//remove
	}
	else
	{
		/* eof */
		targetFile.write((char*)&emptyBuffer, BLOCK_SIZE);
		targetFile.write((char*)&emptyBuffer, BLOCK_SIZE);

		targetFile.flush();
		targetFile.close();
	}
}

bool
TarPacker::packInternal(std::ofstream & targetFile, const std::string & path, const std::string & name)
{
	struct stat s;
	if (lstat((path + name).c_str(), &s))
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

	switch (s.st_mode & S_IFMT)
	{
	case S_IFDIR:
	{
		VecStr files;
		if (!getDirectoryFiles(path + name, files))
		{
			return false;
		}
		packDirectory(targetFile, name, s);
		for (auto it = files.begin(); it != files.end(); ++it)
		{
			if (*it == ".." || *it == ".")
			{
				continue;
			}

			if (!packInternal(targetFile, path, name + '/' + (*it)))
			{
				return false;
			}
		}
	}
	break;

	case S_IFREG:
	{
		if (!packRegFile(targetFile, path, name, s))
		{
			return false;
		}
	}
	break;

	case S_IFLNK:
	{
		packLink(targetFile, path, name, s);
	}
	break;

	case S_IFCHR:
	{

	}
	break;

	case S_IFBLK:
	{
#if 0
		if (!packBlockFile(targetFile, path, name, s))
		{
			return false;
		}
#endif
	}
	break;

	case S_IFIFO:
	{

	}
	break;

	case S_IFSOCK:
	{

	}
	break;

	default:
		break;
	}

	return true;
}

bool
TarPacker::getDirectoryFiles(const std::string & directory, VecStr & files)
{
	DIR* curDir = opendir(directory.c_str());
	if (curDir)
	{
		struct dirent* ent;
		while ((ent = readdir(curDir)) != nullptr)
		{
			files.emplace_back(ent->d_name);
		}
		closedir(curDir);

		return true;
	}

	return false;
}

void
TarPacker::packDirectory(std::ofstream & targetFile, const std::string & name, const struct stat & s)
{
	HeaderInfo * h = createHeader(name + '/', DIRTYPE, s);
	std::unique_ptr<HeaderInfo> headerInfo(h);
	convertHeader(*headerInfo);

	targetFile.write((char*)&header, BLOCK_SIZE);
}

bool 
TarPacker::packRegFile(std::ofstream & targetFile, const std::string & path, 
	const std::string & name, const struct stat & s)
{
	std::ifstream fileInput;

	HeaderInfo * h = createHeader(name, REGTYPE, s);
	std::unique_ptr<HeaderInfo> headerInfo(h);
	convertHeader(*headerInfo);

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
TarPacker::packLink(std::ofstream & targetFile, const std::string & path,
	const std::string & name, const struct stat & s)
{
	char buf[100];
	ssize_t len;

	if ((len = readlink((path + name).c_str(), buf, sizeof(buf) - 1)) != -1) {
		buf[len] = '\0';
	}

	/* LNK or SYM ??? */
	HeaderInfo * h = createHeader(name, SYMTYPE, s, std::string(buf));
	std::unique_ptr<HeaderInfo> headerInfo(h);
	convertHeader(*headerInfo);

	targetFile.write((char*)&header, BLOCK_SIZE);
}

bool 
TarPacker::packBlockFile(std::ofstream & targetFile, const std::string & path,
	const std::string & name, const struct stat & s)
{
#if 0
	std::ifstream fileInput;

	HeaderInfo * h = createHeader(name, BLKTYPE, s);
	std::unique_ptr<HeaderInfo> headerInfo(h);
	convertHeader(*headerInfo);

	fileInput.open(path + name, std::ios::binary);
	if (!fileInput.is_open())
	{
		return false;
	}

	targetFile.write((char*)&header, BLOCK_SIZE);
	writeContentToTargetFile(headerInfo, fileInput, targetFile);
	fileInput.close();

	return true;
#endif
	return false;
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

	return name;
}

HeaderInfo *
TarPacker::createHeader(const std::string & name, int8_t typeflag, 
	const struct stat & s, const std::string & linkname)
{
	HeaderInfo * headerInfo = new HeaderInfo();
	struct passwd *pw;
	struct group *gr;

	pw = getpwuid(s.st_uid);
	gr = getgrgid(s.st_gid);

	headerInfo->name = name;
	
	headerInfo->mode = s.st_mode & RWX;
	headerInfo->uid = s.st_uid;
	headerInfo->gid = s.st_gid;

	headerInfo->mtime = s.st_mtime;
	headerInfo->checksum = 0;
	headerInfo->typeflag = typeflag;

	headerInfo->linkname = linkname;
	headerInfo->magic = TMAGIC;
	headerInfo->version = TVERSION;
	headerInfo->uname = pw->pw_name;
	headerInfo->gname = gr->gr_name;

	//headerInfo->prefix = ;

	switch (typeflag)
	{
	case AREGTYPE:
	case REGTYPE:
	{
		headerInfo->size = s.st_size;
	}
	break;
	
	case SYMTYPE:
	case LNKTYPE:
	case DIRTYPE:
	{
		headerInfo->size = 0;
	}
	break;

	case CHRTYPE:
	case BLKTYPE:
	{
		headerInfo->devmajor = major(s.st_dev);
		headerInfo->devminor = minor(s.st_dev);
	}
	break;

	case FIFOTYPE:
	{

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
	int8_t res[12];
	std::memset(&header, 0, BLOCK_SIZE);

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

	if (headerInfo.typeflag == BLKTYPE || headerInfo.typeflag == CHRTYPE)
	{
		toOctStr(headerInfo.devmajor, res, sizeof(header.devmajor));
		std::memcpy(header.devmajor, res, sizeof(header.devmajor));
		toOctStr(headerInfo.devminor, res, sizeof(header.devminor));
		std::memcpy(header.devminor, res, sizeof(header.devminor));
	}

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
