/******************************************
 * Copyright 2017
 * Sigma Designs, Inc. All Rights Reserved
 * Proprietary and Confidential
 *
 * This file describes a tool for create/dump PMAN Secure Table (PST)
 * that is used to pass PMAN Protection Settings to firmware.
 *
 * Author:  Tony He
 * Date:    2017/04/05
 *
 ******************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h> /* strerror */
#include <errno.h>  /* errno */
#include <unistd.h> /* getopt_long */
#include <getopt.h> /* getopt */
#include <assert.h> /* assert */

#include "crc32.h"

#ifndef SD_PMAN_NR_GRPS		/*i.e. for genpst tool*/
# define SD_PMAN_NR_GRPS	1	/* at least 1 group */
#endif
#ifndef SD_PMAN_NR_RGNS
# define SD_PMAN_NR_RGNS	32	/* max 32 regions per group */
#endif

/*
 * attributes
 */
#define PMAN_RGN_ATTR_WRITE	(1 << 16)
#define PMAN_RGN_ATTR_EXEC	(1 << 12)
#define PMAN_RGN_ATTR_READ	(1 << 8)
#define PMAN_RGN_ATTR_CONTENT	(1 << 4)
#define PMAN_RGN_ATTR_VALID	(1 << 0)

#define LOG2_PMAN_REGION_GRANULARITY	12
#define PMAN_REGION_GRANULARITY		(1 << LOG2_PMAN_REGION_GRANULARITY)

#include <pman_sec_tab.h>

#define ASSERT(expr) do {			\
	if (!(expr)) {				\
		MY_ERROR("ASSERT(%s)\n", #expr);\
		assert(expr);			\
	}					\
}while(0)

#define _MY_LOG(...)	\
	fprintf(stderr, __VA_ARGS__)

#define MY_ERROR(...) do {		\
	_MY_LOG("%d: ", __LINE__);	\
	_MY_LOG(__VA_ARGS__);		\
}while(0)

#define MY_LOG(...) do {		\
	if (b_verbose)			\
		_MY_LOG(__VA_ARGS__);	\
}while(0)

#define LOG_TAG	"GEN  "

#define PTAB_HDR_TRACE(hdr) do {			\
	if (b_verbose)					\
		_PTAB_HDR_TRACE(hdr, LOG_TAG, _MY_LOG);	\
}while(0)

#define PTAB_REGION_TRACE(rgn)	do {			\
	if (b_verbose)					\
		_PTAB_REGION_TRACE(rgn, LOG_TAG, _MY_LOG);\
}while(0)

#define BOOT_SIZE	0x10000	/* 64kB */

#define FALSE	0
#define TRUE	!FALSE
typedef int bool;

/*
 * struct option {
 *	const char *name;
 *	int has_arg;
 *	int *flag;
 *	int val;
 * };
 */
static struct option long_options[] = {
	{"dump",	0, NULL, 'd'},
	{"output",	1, NULL, 'o'},
	{"binary",	0, NULL, 'b'},
	{"text",	0, NULL, 't'},
	{"help",	0, NULL, 'h'},
	{"version",	0, NULL, 'v'},
	{"verbose",	0, NULL, 'V'},
};

static bool b_verbose = FALSE;

static void print_usage(void)
{
	fprintf(stderr, "\n"
	"PMAN Secure Table (PST) create/dump tool v%d.%d\n\n"
	"Usage:\n"
	"Create PST from <in-1>..<in-n> and output to <out>:\n"
	"    genpst [OPTIONS] <in-1> ... <in-n> -o <out>\n"
	"Dump PST from <in-1>..<in-n> to <out> file (or stdout if <out> is omitted):\n"
	"    genpst [OPTIONS] -d <in-1> ... <in-n> [-o <out>]\n"
	"Default to create mode.\n"
	"\n"
	"OPTIONS:\n"
	"       -d,--dump          enable dump mode, default off\n"
	"       -o,--output <out>  specify output file\n"
	"       -b,--binary        render PST in binary form when dump, default off\n"
	"       -t,--text          render PST in plaintext form when dump, default on\n"
	"       -V,--verbose       verbose for debug purpose, default off\n"
	"       -v,--version       display version and exit\n"
	"       -h,--help          display this help and exit\n"
	"\n", VERSION_MAJOR, VERSION_MINOR);
}

static void print_usage_and_exit(int rc)
{
	print_usage();
	exit(rc);
}

static bool validate_pst(struct ptbl_hdr *pst)
{
	struct ptbl_hdr header;
	uint32_t tmpcrc;
	memcpy(&header, pst, PTAB_HDR_LENGTH);
	header.hcrc = 0;	/* reset for recalculating */
	tmpcrc = crc32(0, &header, PTAB_HDR_LENGTH);
	if (tmpcrc == pst->hcrc) {
		tmpcrc = crc32(0, pst + 1, pst->dlen);
		if (tmpcrc == pst->dcrc)
			return TRUE;
	}
	return FALSE;
}

static int locate_pst(const char *fn, size_t *pos, bool is_dump)
{
	int i, ret;
	FILE *fp;
	size_t flen = 0, tmp;
	void *buf = NULL;

	ASSERT(pos != NULL);

	/* no exist, just set pos to 0 in case not dump */
	if (!is_dump && access(fn, F_OK) != 0) {
		*pos = 0;
		return 0;
	}

	if ((fp = fopen(fn, "rb")) == NULL) {
		MY_ERROR("fopen: %s\n", strerror(errno));
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	flen = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if ((buf = calloc(flen + PTAB_ALIGN, sizeof(unsigned char))) == NULL) {
		MY_ERROR("calloc: %s\n", strerror(errno));
		ret = -2;
		goto OUT;
	}

	tmp = fread(buf, 1, flen, fp);
	if (tmp < 0) {
		MY_ERROR("fread: %s\n", strerror(errno));
		ret = -3;
		goto OUT;
	}

	/*
	 * search for magic code
	 * do full scan util the end of bootloader (BOOT_SIZE)
	 * TODO:
	 *   1. search at some special positions firstly.
	 */
	ret = -9;
	for (i = 0; i < tmp; i += PTAB_ALIGN) {
		if (i >= BOOT_SIZE) {
			MY_ERROR("No PST magic code in 0x%x bytes\n", BOOT_SIZE);
			ret = -4;
			break;
		}

		if (!strncmp((char*)buf+i, PTAB_MAGIC, strlen(PTAB_MAGIC))) {
			struct ptbl_hdr* hdr = (struct ptbl_hdr*)(buf + i);
			//MY_LOG("PST magic code detected in file '%s' offset %d\n", fn, i);
			if ((tmp >= i + hdr->tlen) &&
			    validate_pst(hdr)) {
				*pos = i;
				ret = 0;
				break;
			} else {
				MY_LOG("Magic detected but integrity check failed. Invalid PST!?\n");
				continue;
			}
		}
	}

OUT:
	if (buf != NULL) {
		free(buf);
		buf = NULL;
	}
	if (fp != NULL) {
		fclose(fp);
		fp = NULL;
	}
	return ret;
}

/************************************************************/
/* Dumper */
/************************************************************/
static void dump_rgn_to_file(struct ptbl_rgn_body* rgn, void* param)
{
	FILE *fp = (FILE*) param;
	ASSERT(fp != NULL);

	/* p <id> <addr> <size> <sec> <attr> */
	fprintf(fp, "p %d 0x%08x 0x%08x 0x%08x 0x%08x\n",
		rgn->lsb.bits.id, PTAB2PHYADDR(rgn->lsb.bits.start),
		rgn->size, rgn->sec, rgn->attr);
}

static int dump_one_file(const char* fin, FILE* fpout, bool btext)
{
	int ret;
	size_t pos;
	FILE *fpin = NULL;
	struct ptbl_hdr *pst = NULL;

	/* debug info */
	MY_LOG ("PST Dump '%s'\n", fin);

	ret = locate_pst(fin, &pos, TRUE);
	if (0 == ret) {
		int tmp = 0;
		if ((pst = (struct ptbl_hdr*)calloc(PTAB_MAX_LENGTH, 1)) == NULL) {
			MY_ERROR("calloc: %s\n", strerror(errno));
			ret = -100;
			goto OUT;
		}

		if ((fpin = fopen(fin, "rb")) == NULL) {
			MY_ERROR("fopen: %s\n", strerror(errno));
			ret = -101;
			goto OUT;
		}

		fseek(fpin, pos, SEEK_SET);
		tmp = fread(pst, 1, PTAB_MAX_LENGTH, fpin);
		if (tmp < 0) {
			MY_ERROR("fread: %s\n", strerror(errno));
			ret = -102;
			goto OUT;
		}

		if (pst->ver > PST_VERSION_CODE) {
			MY_ERROR("Unsupported PST (ver=0x%02x). Have to update tool\n", pst->ver);
			ret = -103;
			goto OUT;
		} else if (pst->ver < PST_VERSION_CODE) {
			MY_LOG("Legacy ");
		}
		MY_LOG("PST (ver=0x%02x) found at pos 0x%x\n", pst->ver, (int)pos);

		ASSERT(fpout != NULL);
		if (btext) {
			/* dump in text mode */
			fprintf(fpout, "# PMAN Secure Table dumped from '%s'\n", fin);
			fprintf(fpout, "# syntax: p <id> <addr> <len> <sec> <attr>\n");
			DO_FOR_EACH_PTAB_REGION(pst, dump_rgn_to_file, fpout);
			ret = pst->dlen + PTAB_HDR_LENGTH;
		} else {
			/* dump in binary mode */
			tmp = fwrite(pst, 1, PTAB_MAX_LENGTH, fpout);
			if (tmp < 0) {
				MY_ERROR("fwrite: %s\n", strerror(errno));
				ret = -104;
				goto OUT;
			}
			ret = tmp;
		}
	} else {
		MY_ERROR("No PST found in file '%s', ret %d\n", fin, ret);
	}

OUT:
	if (pst != NULL) {
		free(pst);
		pst = NULL;
	}
	if (fpin != NULL) {
		fclose(fpin);
		fpin = NULL;
	}
	return ret;
}

static int dump_pst(int fc, char* fv[], const char* fout, bool btext)
{
#define DUMP_STD_FP	stdout
	int i, ret;
	FILE *fpout = NULL;

	if (fc < 1) {
		MY_ERROR("error inputs\n");
		print_usage_and_exit(1);
	}

	/* debug info */
	MY_LOG("PST Dump\n\tinputs: ");
	for (i = 0; i < fc; i++)
		MY_LOG("'%s' ", fv[i]);
	MY_LOG("\n\toutput: '%s'\n", (fout != NULL) ? fout : "stdout");

	/* setup output file pointer */
	if (fout != NULL) {
		const char *fmode;
		if (btext)
			fmode = "w";
		else
			fmode = "w+b";
		if ((fpout = fopen(fout, fmode)) == NULL) {
			MY_ERROR("fopen: %s\n", strerror(errno));
			ret = -10;
			goto OUT;
		}
	} else {
		fpout = DUMP_STD_FP;
	}

	ret = 0;
	/* dump files */
	for (i = 0; i < fc; i++) {
		dump_one_file(fv[i], fpout, btext);
	}

OUT:
	if (fpout != NULL && fpout != DUMP_STD_FP) {
		fclose(fpout);
		fpout = NULL;
	}
	return ret;
}

/************************************************************/
/* Creator */
/************************************************************/
static int pman_compr_regions(const void* A, const void* B) {
	struct ptbl_rgn_body* rgnA = (struct ptbl_rgn_body*)A;
	struct ptbl_rgn_body* rgnB = (struct ptbl_rgn_body*)B;
	if (rgnA->lsb.bits.id != rgnB->lsb.bits.id)
		return ((int)rgnA->lsb.bits.id - (int)rgnB->lsb.bits.id);
	else if (rgnA->lsb.bits.start != rgnB->lsb.bits.start)
		return ((int)rgnA->lsb.bits.start - (int)rgnB->lsb.bits.start);
	else
		return ((int)rgnA->size - (int)rgnB->size);
}

static int pman_add_one_region(void *body, size_t ofs, const struct ptbl_rgn_body* prgn) {
	int i, n;
	struct ptbl_rgn_body *rgns = NULL;

	ASSERT(body != NULL && !(ofs % sizeof(struct ptbl_rgn_body)));
	if (PTAB_REGION_SANITY_CHECK(prgn)) {
		MY_ERROR("error: invalid region!\n");
		PTAB_REGION_TRACE(prgn);
		return 0;
	}

	rgns = (struct ptbl_rgn_body*)body;
	n = ofs / sizeof(struct ptbl_rgn_body);
	if (n > PTAB_RGN_MAX - 1) {
		MY_ERROR("error: small pman table!!\n");
		return 0;
	}

	/* in ascending order */
	for (i = 0; i < n; i++) {
		if (pman_compr_regions(prgn, rgns + i) < 0)
			break;
	}

	if (n > i)
		memmove(rgns + i + 1, rgns + i, (n - i) * sizeof(struct ptbl_rgn_body));

	memcpy(rgns + i, prgn, sizeof(struct ptbl_rgn_body));
	return sizeof(struct ptbl_rgn_body);
}

static char* strtrim(char* str) {
	char* p = str;
	char* q;

	if ((q = strstr(str, "//"))) *q = '\0';

	while ((*p == ' ') || (*p == '\n') || (*p == '\r') || (*p == '\t')) p++;

	q = p + strlen(p) - 1;
	while (q > p) {
		if ((*q == ' ') || (*q == '\n') || (*q == '\r') || (*q == '\t'))
			q--;
		else
			break;
	}
	*(q + 1) = '\0';
	return p;
}

static int parse_one_file(const char* fn, void *body, size_t ofs)
{
	int lineno = 0;
	int len = 0;
	FILE *fp = NULL;
	struct ptbl_rgn_body rgn;

	if ((fp = fopen(fn, "r")) == NULL) {
		MY_ERROR("fopen: %s\n", strerror(errno));
		goto OUT;
	}

	while (!feof(fp)) {
		#define MAX_LINE_LENGTH 1024
		char buf[MAX_LINE_LENGTH], *p;
		if (NULL == fgets(buf, sizeof(buf), fp)) {
			MY_LOG("fgets: %s\n", strerror(errno));
			break;
		}

		lineno++;
		p = strtrim(buf);
		if (strlen(p) == 0) continue;
		if (p[0] == '#') continue;

		/*
		 * pman setting entry shall be given in format:
		 *   p <id> <addr> <size> <sec> <attr>
		 */
		if (('p' == p[0]) || ('P') == p[0]) {
			int n, tid, taddr;
			char *str = p++;
			p = strtrim(p);
			memset(&rgn, 0, sizeof(struct ptbl_rgn_body));
			n = sscanf(p, "%x %x %x %x %x", &tid, &taddr, &rgn.size, &rgn.sec, &rgn.attr);
			if (n != 5) {
				MY_ERROR("%s:%d: invalid entry:'%s', ignored\n", fn, lineno, str);
				continue;
			}
			rgn.lsb.bits.id = tid;
			rgn.lsb.bits.start = PHY2PTABADDR(taddr);

			if (PTAB_REGION_SANITY_CHECK(&rgn)) {
				MY_ERROR("%s:%d: invalid region:'%s', ignored\n", fn, lineno, str);
				continue;
			}

			PTAB_REGION_TRACE(&rgn);
			len += pman_add_one_region(body, ofs + len, &rgn);
		} else {
			MY_ERROR("%s:%d: unknown entry:'%s', ignored\n", fn, lineno, p);
		}
	}

	fclose(fp);
	fp = NULL;
OUT:
	return len;
}

static int gen_pst(int fc, char* fv[], void *pst)
{
	int i;
	size_t dlen = 0;
	struct ptbl_hdr * hdr = NULL;
	struct ptbl_rgn_body * body = NULL;

	ASSERT(pst != NULL);
	hdr = (struct ptbl_hdr*)pst;

	/* fill in regions */
	body = (struct ptbl_rgn_body*)(hdr + 1);
	for (i = 0; i < fc; i++) {
		int tmp;
		tmp = parse_one_file(fv[i], body, dlen);
		dlen += tmp;
	}
	/* fill in the header */
	memset(hdr, 0, sizeof(struct ptbl_hdr));
	memcpy(hdr->magic, PTAB_MAGIC, sizeof(hdr->magic));
	hdr->ver  = PST_VERSION_CODE;
	hdr->tlen = PTAB_MAX_LENGTH;
	hdr->dlen = dlen;
	hdr->dcrc = crc32(0, body, dlen);
	hdr->hcrc = crc32(0, hdr, sizeof(struct ptbl_hdr));
	PTAB_HDR_TRACE(hdr);

	return dlen;
}

static int write_pst_to_file(void *pst, const char *fn)
{
	int ret;
	size_t pos = 0;
	FILE *fp = NULL;
	void *buf = NULL;
	ret = locate_pst(fn, &pos, FALSE);
	if (0 == ret) {
		if (access(fn, F_OK)) {
			/* create */
			if ((fp = fopen(fn, "wb")) == NULL) {
				MY_ERROR("fopen: %s\n", strerror(errno));
				ret = -1;
				goto OUT;
			}

			ret = fwrite(pst, 1, PTAB_MAX_LENGTH, fp);
			if (ret < 0) {
				MY_ERROR("fwrite: %s\n", strerror(errno));
			}
		} else {
			/* modify */
			size_t flen;
			struct ptbl_hdr *old;
			if ((fp = fopen(fn, "r+b")) == NULL) {
				MY_ERROR("fopen: %s\n", strerror(errno));
				ret = -1;
				goto OUT;
			}

			fseek(fp, 0, SEEK_END);
			flen = ftell(fp);
			rewind(fp);

			if ((buf = calloc(flen + PTAB_MAX_LENGTH, 1)) == NULL) {
				MY_ERROR("calloc: %s\n", strerror(errno));
				ret = -2;
				goto OUT;
			}

			flen = fread(buf, 1, flen, fp);
			if (flen < 0) {
				MY_ERROR("fread: %s\n", strerror(errno));
				ret = -3;
				goto OUT;
			}
			old = (struct ptbl_hdr*)(buf + pos);
			PTAB_HDR_TRACE(old);
			if (old->tlen < PTAB_MAX_LENGTH && ((flen - pos) > old->tlen)) {
				MY_ERROR("fail write pst to file '%s' for buffer overflow!\n", fn);
				ret = -4;
				goto OUT;
			}
			memcpy(buf + pos, pst, PTAB_MAX_LENGTH);
			flen = (flen > (pos + PTAB_MAX_LENGTH)) ? flen : (pos + PTAB_MAX_LENGTH);
			rewind(fp);
			ret = fwrite(buf, 1, flen, fp);
			if (ret < 0) {
				MY_ERROR("fwrite: %s\n", strerror(errno));
			}
		}
	}

OUT:
	if (buf != NULL) {
		free(buf);
		buf = NULL;
	}
	if (fp != NULL) {
		fclose(fp);
		fp = NULL;
	}
	return ret;
}

static int create_pst(int fc, char* fv[], const char* fout)
{
	int i, ret = 0;
	void *pst = NULL;

	if (fc < 1) {
		MY_ERROR("error inputs\n");
		print_usage_and_exit(1);
	}

	if (NULL == fout) {
		MY_ERROR("output missed\n");
		print_usage_and_exit(2);
	}

	/* debug info */
	MY_LOG("PST Creator\n\tinputs: ");
	for (i = 0; i < fc; i++)
		MY_LOG("'%s' ", fv[i]);
	MY_LOG("\n\toutput: '%s'\n", fout);

	/* allocate memory once for all */
	pst = malloc(PTAB_MAX_LENGTH);
	if (NULL == pst) {
		MY_ERROR("malloc: %s\n", strerror(errno));
		ret = 3;
		goto OUT;
	}

	/* generate table */
	if (gen_pst(fc, fv, pst) < 0) {
		MY_ERROR("got error when try generating PST!\n");
		ret = 4;
		goto OUT;
	}

	/* write table to file */
	if (write_pst_to_file(pst, fout) < 0) {
		MY_ERROR("writing pst error!\n");
		ret = 5;
		goto OUT;
	}

OUT:
	/* free memory */
	if (pst != NULL) {
		free(pst);
		pst = NULL;
	}
	return ret;
}

int main(int argc, char* argv[])
{
	int opt, ret = 0, nfiles = 0;
	const char* fout = NULL;
	bool b_dump, b_text;
	b_dump = FALSE, b_text = TRUE;

	while ((opt = getopt_long(argc, argv, "do:bthvV", long_options, NULL)) != -1) {
		switch (opt) {
		case 'd':
			b_dump = TRUE;
			break;
		case 'o':
			fout = optarg;
			break;
		case 'b':
			b_text = FALSE;
			break;
		case 't':
			b_text = TRUE;
			break;
		case 'h':
			print_usage_and_exit(0);
			break;
		case 'v':
			fprintf(stderr, "PST tool v%d.%d (%s - %s)\n",
				VERSION_MAJOR, VERSION_MINOR, __DATE__, __TIME__);
			exit(0);
			break;
		case 'V':
			b_verbose = TRUE;
			break;
		case '?':
			break;
		case ':':
			break;
		default:
			fprintf(stderr, "?? getopt returned character code 0%o ??\n", opt);
			break;
		}
	}

	/* Show banner */
	MY_LOG("PST tool v%d.%d (%s - %s)\n",
		VERSION_MAJOR, VERSION_MINOR,
		__DATE__, __TIME__);

	if (optind < argc) {
		nfiles = argc - optind;
	} else {
		nfiles = 0;
	}

	if (b_dump) {
		ret = dump_pst(nfiles, argv + optind, fout, b_text);
	} else {
		ret = create_pst(nfiles, argv + optind, fout);
	}
	return ret;
}
