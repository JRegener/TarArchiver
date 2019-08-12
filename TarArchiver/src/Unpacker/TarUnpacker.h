#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../TarCommon.h"

/*
		чтение:
		при разархивировании проверяем все файлы на уже существующие и спрашиваем про замену

		идем по порядку если есть ошибка в заголовке перрываем чтение
		сигнализируем об ошибке так как будут проблемы с созданием последующих директорий

		открыть файл
		проверить eof

		считать header
		конвертировать header
		проверить magic
		проверить checksum
		пропустить если плохо

		определить тип файла
		создать файл
		установить права доступа
		записать блоки данных
		закрыть файл

		определить иерархию файла

		повторить до конца
	*/
class TarUnpacker
{
private:
	PosixHeader header;
	ContentData emptyBuffer = { NULL }; 		/* eof init */
	ContentData workBuffer;
	ContentData additionalBuffer;

public:
	TarUnpacker() {};

	void unpack(const std::string & path);

	/* Check end of file. It must contains 2 blocks size of 512 bytes at the end of file */
	bool checkExpand(std::ifstream & finput, std::streampos & sizeOfContent);

	HeaderInfo * convertHeader(const PosixHeader & header);

	/* to calculate checksum must summarizes all bytes of header */
	/* but in field checksum first seven bytes must set to whitespace ((char)32)  */
	/* the last byte eight set to trailing zero (\0) */
	int64_t calculateUnsignedCheckSum(PosixHeader & header) const;

	bool checkHeader(const HeaderInfo & headerInfo, PosixHeader & header);

	bool createFileType(const HeaderInfo & headerInfo, std::ifstream & finput);

	bool createDir(const HeaderInfo & header, Error & errorType);

	void writeContentToTargetFile(const HeaderInfo & header, std::ifstream & input,
		std::ofstream & target);

};