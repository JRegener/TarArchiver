#include "Packer/TarPacker.h"
#include "Unpacker/TarUnpacker.h"


int main()
{
	TarPacker packer;
	packer.pack("/home/osboxes/projects/TarArchiveLinux/bin/x64/Debug/drr");
	return 0;
}