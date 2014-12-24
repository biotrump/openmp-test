#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>             /* getopt_long() */
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "omp.h"
#include <cv.h>
#include <highgui.h>

#include "genki.h"

char ridFolder[MAX_FILE_PATH_SIZE];//="rid";
char nonObjRidFolder[MAX_FILE_PATH_SIZE];//="nonobj";
int	Verbose;
char genkiFacePath[MAX_FILE_PATH_SIZE];
char nonFacePath[MAX_FILE_PATH_SIZE];

static void usage(FILE *fp, int argc, char **argv)
{
	fprintf(fp,
		 "Usage: %s [options]\n\n"
		 "Options:\n"
		 "-f | --genkiface     genki face path\n"
		 "-h | --help          Print this message\n"
		 "-n | --nonface       non face path\n"
		 "-r | --ridfolder     face rid folder[%s]\n"
		 "-R | --nonobjrid     nonface rid folder[%s]\n"
		 "-v | --verbose       Verbose output[%d]\n"
		 "",
		 argv[0], ridFolder, nonObjRidFolder, Verbose);
}

static const char short_options[] = "f:hn:r:R:v";

static const struct option
long_options[] = {
	{ "help",   no_argument,       NULL, 'h' },
	{ "genkiface",  required_argument, NULL, 'f' },
	{ "nonface",  required_argument, NULL, 'n' },
	{ "ridfolder",  required_argument, NULL, 'r' },
	{ "nonobjrid",  required_argument, NULL, 'R' },
	{ "verbose", 	no_argument,       NULL, 'v' },
	{ 0, 0, 0, 0 }
};

static int option(int argc, char **argv)
{
	int r=0;
	printf("+%s:\n",__func__);

	for (;;) {
		int idx;
		int c;

		c = getopt_long(argc, argv,
				short_options, long_options, &idx);
		printf("c=%d, idx=%d\n", c,idx);
		if(-1 == c){
			break;
		}
		switch (c) {
		case 0: /* getopt_long() flag */
			printf("0\n");
			break;
		case 'f':
			if(optarg && strlen(optarg))
				strncpy(genkiFacePath, optarg, strlen(optarg));
			printf("f:%s\n", genkiFacePath);
			break;
		case 'n':
			if(optarg && strlen(optarg))
				strncpy(nonFacePath, optarg, strlen(optarg));
			printf("n:%s\n", nonFacePath);
			break;
		case 'r':
			if(optarg && strlen(optarg))
				strncpy(ridFolder, optarg, strlen(optarg));
			printf("r:%s\n", ridFolder);
			break;
		case 'R':
			printf("optarg=%s, %zd\n", optarg, strlen(optarg));
			if(optarg && strlen(optarg)){
				strncpy(nonObjRidFolder, optarg, strlen(optarg));
				printf("nonObjRidFolder=%s\n", nonObjRidFolder);
			}
			printf("R:%s\n", nonObjRidFolder);
			//exit(1);
			break;
		case 'h':
			usage(stdout, argc, argv);
			r=-1;
			break;
		case 'v':
			Verbose=1;
			break;
		default:
			printf("default\n");
			usage(stderr, argc, argv);
			r=-1;
		}
	}
	return r;
}

int main(int argc, char **argv)
{
	srand(time(NULL));
	if(option(argc,argv)<0){
		printf("Wrong args!!!\n");
		exit(EXIT_FAILURE);
	}
	genkiFace(genkiFacePath, ridFolder);
	picoNonFace(nonFacePath, nonObjRidFolder );
}
