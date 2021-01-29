#include "common.h"

int
main(int argc, char** argv)
{
	for(int count = 1; count < argc; count++){
	  printf("%s\n", argv[count]);	
	}

	return 0;
}
