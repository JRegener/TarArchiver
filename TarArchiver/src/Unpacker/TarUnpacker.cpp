#include "TarUnpacker.h"

void 
TarUnpacker::unpack(const std::string & path)
{
	std::ifstream inputFile;
	std::ofstream targetFile;
	std::streampos sizeOfContent;
	

	inputFile.open(path, std::ios::binary);
	if (!inputFile.is_open())
	{
		return;
	}

	/* check for correct tar eof */
	if (!checkExpand(inputFile, sizeOfContent))
	{
		inputFile.close();
		return;
	}

	while (inputFile.tellg() != sizeOfContent)
	{
		/* get header */
		inputFile.read((char*)&header, BLOCK_SIZE);
		HeaderInfo * h = convertHeader(header);
		std::unique_ptr<HeaderInfo> headerInfo(h);

		if (!checkHeader(*headerInfo, header))
		{
			/* stop reading */
			break;
		}

		if (!createFileType(*headerInfo, inputFile))
		{
			/* can't create file */
			/* stop */
			break;
		}
	}

	std::cout << inputFile.tellg() << std::endl;


	inputFile.close();
}

bool
TarUnpacker::checkExpand(std::ifstream & finput, std::streampos & sizeOfContent)
{
	finput.seekg(-2 * BLOCK_SIZE, std::ios::end);
	sizeOfContent = finput.tellg();

	finput.read((char*)&workBuffer, BLOCK_SIZE);
	finput.read((char*)&additionalBuffer, BLOCK_SIZE);

	finput.seekg(0, std::ios::beg);

	if (std::memcmp(&workBuffer, &emptyBuffer, BLOCK_SIZE) == 0 &&
		std::memcmp(&additionalBuffer, &emptyBuffer, BLOCK_SIZE) == 0)
	{
		return true;
	}

	return false;
}

HeaderInfo *
TarUnpacker::convertHeader(const PosixHeader & header)
{
	HeaderInfo * headerInfo = new HeaderInfo();

	headerInfo->name = (char*)header.name;
	headerInfo->mode = strtol((char*)header.mode, nullptr, 8);
	headerInfo->uid = strtol((char*)header.uid, nullptr, 8);
	headerInfo->gid = strtol((char*)header.gid, nullptr, 8);
	headerInfo->size = strtol((char*)header.size, nullptr, 8);
	headerInfo->mtime = strtoll((char*)header.mtime, nullptr, 8);
	headerInfo->checksum = strtoll((char*)header.chksum, nullptr, 8);
	headerInfo->typeflag = header.typeflag;
	headerInfo->linkname = (char*)header.linkname;
	headerInfo->magic = (char*)header.magic;
	headerInfo->version = (char*)header.version;
	headerInfo->uname = (char*)header.uname;
	headerInfo->gname = (char*)header.gname;
	headerInfo->devmajor = strtol((char*)header.devmajor, nullptr, 8);
	headerInfo->devminor = strtol((char*)header.devminor, nullptr, 8);
	headerInfo->prefix = (char*)header.prefix;

	headerInfo->blockCount = headerInfo->size / BLOCK_SIZE;
	headerInfo->reminderBytes = headerInfo->size % BLOCK_SIZE;
	if (headerInfo->reminderBytes)
	{
		headerInfo->blockCount++;
	}

	return headerInfo;
}

int64_t
TarUnpacker::calculateUnsignedCheckSum(PosixHeader & header) const
{
	std::memset(header.chksum, 32, 7);
	int64_t checksum = 0;
	const uint8_t * bytes = reinterpret_cast<const uint8_t *>(&header);
	for (size_t i = 0; i < BLOCK_SIZE; ++i) {
		checksum += bytes[i];
	}
	return checksum;
}

bool
TarUnpacker::checkHeader(const HeaderInfo & headerInfo, PosixHeader & header)
{
	if (headerInfo.magic != TMAGIC &&
		headerInfo.checksum != calculateUnsignedCheckSum(header))
	{
		return false;
	}
	return true;
}

bool
TarUnpacker::createFileType(const HeaderInfo & header, std::ifstream & finput)
{
	/* in windows label (ÿנכûך) is regular file which contains all info from base file */
	switch (header.typeflag)
	{
	case REGTYPE:	/* regular file */
	case AREGTYPE:	/* regular file */
	{
		std::ofstream targetFile(header.name, std::ios::binary);
		if (!targetFile.is_open())
		{
			return false;
		}

		writeContentToTargetFile(header, finput, targetFile);

		targetFile.flush();
		targetFile.close();

		chmod(("./" + header.name).c_str(), header.mode);
	}
	break;
	case LNKTYPE:	/* link */
	{

	}
	break;
	case SYMTYPE:	/* reserved */
	{

	}
	break;
	case CHRTYPE:	/* character special */
	{

	}
	break;
	case BLKTYPE:	/* block special */
	{

	}
	break;
	case DIRTYPE:	/* directory */
	{
		/* enum error creation for linux and windows */
		/* create dir */
		Error errorType;
		createDir(header, errorType);
		if (errorType != Error::SUCCESS)
		{
			return false;
		}
	}
	break;
	case FIFOTYPE:	/* FIFO special */
	{

	}
	break;
	case CONTTYPE:	/* reserved */
	{

	}
	break;
	default:
	{
		/* something wrong */
	}
	break;
	}

	return true;
}

bool 
TarUnpacker::createDir(const HeaderInfo & header, Error & errorType)
{
	const int32_t dir_err = mkdir(header.name.c_str(), 0);
	if (dir_err)
	{
		/* error creating file */
		switch (errno)
		{
		case EACCES:
			errorType = Error::NO_ACCESS;
			break;
		case ENOENT:
			errorType = Error::BAD_PATH;
			break;
		default:
			errorType = Error::UNKNOWN_ERROR;
		}
		
		return false;
	}

	chmod(("./" + header.name).c_str(), header.mode);
	errorType = Error::SUCCESS;

	return true;
}

void 
TarUnpacker::writeContentToTargetFile(const HeaderInfo & header, std::ifstream & input, std::ofstream & target)
{
	/* read/write content */
	for (size_t i = 0; i < header.blockCount - 1; ++i)
	{
		input.read((char*)&workBuffer, BLOCK_SIZE);
		target.write((char*)&workBuffer, BLOCK_SIZE);
	}

	/* last block */
	input.read((char*)&workBuffer, BLOCK_SIZE);
	if (header.reminderBytes)
	{
		target.write((char*)&workBuffer, header.reminderBytes);
	}
	else
	{
		target.write((char*)&workBuffer, BLOCK_SIZE);
	}
}
