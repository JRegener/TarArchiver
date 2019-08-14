#include "Packer/TarPacker.h"
#include "Unpacker/TarUnpacker.h"


int main()
{
	TarUnpacker packer;
	packer.unpack("/home/osboxes/projects/TarArchiveLinux/bin/x64/Debug/soft.tar");
	return 0;
}