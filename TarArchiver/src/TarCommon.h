#pragma once
#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>
#include <memory>
#include <iostream>

#define BLOCK_SIZE 512

#define TMAGIC   "ustar "        /* ustar and a null */
#define TMAGLEN  6
#define TVERSION ' ' + '\0'           /* 00 and no null */
#define TVERSLEN 2

/* Values used in typeflag field.  */
#define REGTYPE  '0'            /* regular file */
#define AREGTYPE '\0'           /* regular file */
#define LNKTYPE  '1'            /* link */
#define SYMTYPE  '2'            /* reserved */
#define CHRTYPE  '3'            /* character special */
#define BLKTYPE  '4'            /* block special */
#define DIRTYPE  '5'            /* directory */
#define FIFOTYPE '6'            /* FIFO special */
#define CONTTYPE '7'            /* reserved */

#define XHDTYPE  'x'            /* Extended header referring to the
								   next file in the archive */
#define XGLTYPE  'g'            /* Global extended header */

/* Bits used in the mode field, values in octal.  */
#define TSUID    04000          /* set UID on execution */
#define TSGID    02000          /* set GID on execution */
#define TSVTX    01000          /* reserved */

/* file permissions */
#define TUREAD   00400          /* read by owner */
#define TUWRITE  00200          /* write by owner */
#define TUEXEC   00100          /* execute/search by owner */
#define TGREAD   00040          /* read by group */
#define TGWRITE  00020          /* write by group */
#define TGEXEC   00010          /* execute/search by group */
#define TOREAD   00004          /* read by other */
#define TOWRITE  00002          /* write by other */
#define TOEXEC   00001          /* execute/search by other */

/* i don't know why st_mode return this value (0100655) */
#define RWX 0777

struct PosixHeader
{
	int8_t name[100];               /*   0 */
	int8_t mode[8];                 /* 100 */
	int8_t uid[8];                  /* 108 */
	int8_t gid[8];                  /* 116 */
	int8_t size[12];                /* 124 */
	int8_t mtime[12];               /* 136 */
	int8_t chksum[8];               /* 148 */
	int8_t typeflag;                /* 156 */
	int8_t linkname[100];           /* 157 */
	int8_t magic[6];                /* 257 */
	int8_t version[2];              /* 263 */
	int8_t uname[32];               /* 265 */
	int8_t gname[32];               /* 297 */
	int8_t devmajor[8];             /* 329 */
	int8_t devminor[8];             /* 337 */
	int8_t prefix[155];             /* 345 */
	int8_t padding[12];				/* 500 */
};

struct ContentData
{
	int8_t buffer[BLOCK_SIZE];
};

struct HeaderInfo
{
	std::string name;
	uint16_t mode;
	int16_t uid;
	int16_t gid;
	int32_t size;
	time_t mtime;
	size_t checksum;
	int8_t typeflag;
	std::string linkname;	/* for UNIX link and symlink */
	std::string magic;
	std::string version;
	std::string uname;
	std::string gname;
	uint32_t devmajor;		/* for blk files (only for UNIX?) */
	uint32_t devminor;		/* for blk files (only for UNIX?) */
	std::string prefix;

	size_t blockCount;
	size_t reminderBytes;

	HeaderInfo() {}
	HeaderInfo(HeaderInfo &&) = delete;
	HeaderInfo(const HeaderInfo &) = delete;
};

enum class Error
{
	NO_ACCESS = 0,
	BAD_PATH,
	SUCCESS,
	UNKNOWN_ERROR
};
