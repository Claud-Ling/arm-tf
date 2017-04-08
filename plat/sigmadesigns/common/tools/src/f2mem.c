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
	long offset=0;

	if (argc != 4) {
		printf("f2mem infile outfile offset(hex)\n");
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

	fout = fopen(argv[2], "r+");
	if (!fout) {
		printf("outfile open failed,%s\n", strerror(errno));
		exit(1);
	}

	//write to 'offset'
	offset = strtol(argv[3],(char **)NULL,16);
	fseek(fout, offset, SEEK_SET);
	while (!feof(fin)) {
		memset(ch,0,4);
		readb = fread(ch, sizeof (unsigned char), 4, fin);
		if(readb == 0)
			break;
		fwrite(ch,4,1,fout);
	}
	fclose(fin);
	fclose(fout);
	return 0;
}
