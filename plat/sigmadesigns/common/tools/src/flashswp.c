#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

extern int errno;

int
main(int argc, char *argv[])
{
	int i;
	FILE *fin, *fout;
	unsigned char ch[4];
	int readb;
	long filesize;

	if (argc != 3) {
		printf("flashswp infile outfile\n");
		return 0;
	}

	fin = fopen(argv[1], "rb");
	if (!fin) {
		printf("infile open failed,%s\n", strerror(errno));
		exit(1);
	}
	fseek(fin, 0L, SEEK_END);
	filesize = ftell(fin);
	fseek(fin, 0L, SEEK_SET);

	/*
	if (filesize%4) {
		printf("This is not a MIPS complied code\n");
		return;
	}
	 */

	fout = fopen(argv[2], "wb");
	if (!fout) {
		printf("outfile open failed,%s\n", strerror(errno));
		exit(1);
	}

	while (!feof(fin)) {
		//memset to be sure ch[0~~3] is zero
		memset(ch,0,4);
		readb = fread(ch, sizeof (unsigned char), 4, fin);
		if(readb == 0)
			break;
		for (i = 0; i < 4; i++){
			fwrite(&ch[3-i], sizeof (unsigned char), 1, fout);
		}
	}
	fclose(fin);
	fclose(fout);
	return 0;
}
