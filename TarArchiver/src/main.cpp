#include "Packer/TarPacker.h"
#include "Unpacker/TarUnpacker.h"


int main()
{
	TarPacker packer;
	packer.pack("/home/osboxes/projects");
	return 0;
}