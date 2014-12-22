#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "omp.h"
#include <cv.h>
#include <highgui.h>


#define	MAX_OBJ_FILENAME_LEN	(81)
#define	MAX_OBJ_FILES		(1000000)
#define	MAX_FILE_PATH_SIZE	(1024)

#define	CV_DATASET_GENKI_IMAGES		"/Subsets/GENKI-SZSL/GENKI-SZSL_Images.txt"
#define	CV_DATASET_GENKI_LABELS	   	"/Subsets/GENKI-SZSL/GENKI-SZSL_labels.txt"

#define max( a, b ) ((a) > (b) ? (a) : (b) )
#define min( a, b ) ((a) < (b) ? (a) : (b) )
//inline int max ( int a, int b ) { return a > b ? a : b; }

typedef struct _genki_bounding_box{
int	center_col;	//X of the box center
int center_row;	//Y of the box center
int diameter;	//diameter of the box
} GENKI_FACE_BBOX, *PGENKI_FACE_BBOX;

int TotalListFiles;
int MaxFileNameLen;
void *pGenkiImgList;
void *pGenkiLabelList;

/*
 * raw intensity data
 *
 * im is an array/list from a cropped image around the face bounding box:
 * im dim : row * col
 * rid file : 4 bytes width, 4 bytes height , image data (row-wise)
 */
int saveAsRID(char *path, IplImage* src,  CvRect roi)
{
	//assert(path && src);
	int fd = open(path, O_RDWR|O_CREAT|O_TRUNC, 0664 );
	printf("%s:roi(x,y,w,h)=(%d,%d,%d,%d)\n",__func__, roi.x, roi.y,
		   roi.width, roi.height);
	printf("src(w,h,s)=(%d,%d,%d)\n", src->width, src->height, src->imageSize);
	if(fd > 0){
		// Must have dimensions of output image
		IplImage* cropped = cvCreateImage( cvSize(roi.width,roi.height), src->depth, src->nChannels );
		if(cropped){
			// Say what the source region is
			cvSetImageROI( src, roi );
			// Do the copy
			//cvCopy( src, cropped, NULL );
			printf("crop:%d x %d : %d bytes\n", cropped->width, cropped->height,
				   cropped->imageSize);
			cvResetImageROI( src );
			//cvSaveImage (path , cropped);
			write(fd, &roi.width,sizeof(int));	//4 bytes w
			write(fd, &roi.height,sizeof(int));	//4 bytes h
			write(fd, cropped->imageData, cropped->imageSize);	//body
			printf("%d x %d : %d bytes\n", roi.width, roi.height, cropped->imageSize);
			cvReleaseImage(&cropped);
			close(fd);
		}else{
			printf("[%d,%d,%d,%d] crop fails to create.\n", roi.width,roi.height,
				src->depth, src->nChannels);
		}
	}else{
		printf("[%s] fails to open:%d\n", path,errno);
	}
	return errno;
}


/*
 * write 7 random boxes to list.txt for each rid.
 *  1 face0.rid
 *  2     7	==> 7 random boxs
 *  3     54.128180 54.193473 61.633318
 *  4     50.617332 47.980850 62.638414
 *  5     54.102598 52.237606 61.709570
 *  6     53.919454 51.292225 71.041327
 *  7     52.621050 51.479368 70.888629
 *  8     50.815017 53.567457 62.003842
 *  9     51.473735 49.503986 67.789544
 *
 */
void export(IplImage *image, GENKI_FACE_BBOX bbox, int id, FILE *fList, CvRNG rng)
{
	assert(image);
	int nrows = image->height;
	int ncols = image->width;
	int r, c, s;
	r = bbox.center_row;
	c = bbox.center_col;
	s = bbox.diameter;

	/* The new cropped area is bounded by +-0.75diameter around box center (r,c).
	 * The new cropped area is bigger than the original face bounding box, +-0.5diameter,
	 * around box center (r,c)
	 */
	int r0 = max((r - 0.75*s), 0);
	int r1 = min(r + 0.75*s, nrows);
	int c0 = max((c - 0.75*s), 0);
	int c1 = min(c + 0.75*s, ncols);

	//the new cropped image containing the original face bounding box.
	//the new dim of the cropped image
	nrows = r1- r0 + 1;
	ncols = c1- c0 + 1;

	//the new bounding box center in the new cropped image
	r = r - r0;
	c = c - c0;

	//resize, if needed
	float maxwsize = 192.0;
	int wsize = max(nrows, ncols);
	float ratio = maxwsize/wsize;

	if (ratio<1.0){//resize the pic because it's > maxwsize
		//im = numpy.asarray( Image.fromarray(im).resize((int(ratio*ncols), int(ratio*nrows))) )

		r = ratio*r;
		c = ratio*c;
		s = ratio*s;
	}
	#pragma omp critical
	{
		fprintf(fList, "face%d.rid\n",id);

		//creating 7 randomized face bounding boxes from the original bounding box
		int nrands = 7;
		//write '7' to list.txt
		fprintf(fList, "\t%d\n",nrands);
		int i;
		for(i= 0; i < nrands; i ++){
			float stmp, rtmp, ctmp;
			//uniformly randomize size (diameter of the bounding box) ratio 0.9-1.1, 10% random
			//stmp = s*random.uniform(0.9, 1.1)
			stmp = s * (0.9 + 0.2 * cvRandReal(&rng));
			//uniformly randomize row and column, ratio +-5% random
			//rtmp = r + s*random.uniform(-0.05, 0.05)
			rtmp = r + s*(0.05 - 0.1 *cvRandReal(&rng));
			//ctmp = c + s*random.uniform(-0.05, 0.05)
			ctmp = c + s*(0.05 - 0.1 *cvRandReal(&rng));
			//write the randomized row, column and size (diameter of bounding box)
			fprintf(fList, "\t%f %f %f\n",rtmp, ctmp, stmp);
		}
		fputc('\n',fList);
	}
}

void exportmirrored(IplImage *image, GENKI_FACE_BBOX bbox, int id, FILE *fList, CvRNG rng)
{
	/*
	 * exploit mirror symmetry of the face
	 *
	 * flip image, horizontal , so the dim is not changed!
	 */
	//im = numpy.asarray(ImageOps.mirror(Image.fromarray(im)))
	cvFlip(image, image, 1);
	// flip column, so the face bounding box is mirrored.
	// the center of the bounding box is shifted in x-axis after the mirror, too.
	//c = im.shape[1] - c
	bbox.center_col = image->width - bbox.center_col ;
	// export
	export(image, bbox, id, fList, rng);
}


/*
 * read the object file list and store the list
 */
int readGenkiImgFileList(char *path)
{
	FILE *f;
	char t[1024], filename[MAX_FILE_PATH_SIZE];
	int ret=-1, len=1;
	assert(path);
	snprintf(filename, MAX_FILE_PATH_SIZE, "%s%s", path, CV_DATASET_GENKI_IMAGES);

	//counting the lines in the file.
	if((f=fopen(filename, "rt"))!=NULL){
		while(fgets(t,1024,f) != NULL){
		TotalListFiles++;
		if(strnlen(t, 1024) > len) len = strnlen(t, 1024);
	}
	MaxFileNameLen=len+1;//plus null
	rewind(f);
	printf("total %d lines, max strlen=%d\n", TotalListFiles, MaxFileNameLen);

	if(TotalListFiles && MaxFileNameLen){
	  int i = 0;
	  pGenkiImgList = calloc(TotalListFiles, MaxFileNameLen);
	  char (*p)[MaxFileNameLen] = pGenkiImgList;
	  while((i < TotalListFiles) &&
			fgets(p[i],MAX_OBJ_FILENAME_LEN,f) != NULL){
		//remove \n or \r
		int l=strnlen(p[i], 1024);
		if(l){
		  if( (p[i][l-1] == '\n') || (p[i][l-1] == '\r'))
			p[i][l-1]=0;
		  //printf("%d:%s\n",i, p[i]);
		}
	  	i++;
	  }
	  printf("total %d files in the list:%s\n",i, p[i-1]);
	}
	fclose(f);
	ret = 0;
  }
  return ret;
}

/*
 * GENKI/GENKI-R2009a/Subsets/GENKI-SZSL/GENKI-SZSL_labels.txt
 * Each line is a bounding box of a face in the same line number of GENKI-SZSL_Images.txt.
 * For example: the first line of GENKI-SZSL_Images.txt is "file0000000000003987.jpg",
 * so the first line of GENKI-SZSL_labels.txt is the bounding box of the file file0000000000003987.jpg.
 *
 * read the face bounding box (x,y,diameter) mapped to each file listed in GENKI-SZSL_Images.txt
 */
int readGenkiLabelList(char *path)
{
	FILE *f;
	char str[MAX_FILE_PATH_SIZE], filename[MAX_FILE_PATH_SIZE];
	int ret=-1, bc=0;
	assert(path);
	snprintf(filename, MAX_FILE_PATH_SIZE, "%s%s", path, CV_DATASET_GENKI_LABELS);

	if((f=fopen(filename, "rt"))!=NULL){
		while(fgets(str,MAX_FILE_PATH_SIZE,f) != NULL){
			bc++;
		}
		rewind(f);
		printf("total %d box lines\n", bc);
		if(bc){
			int i = 0;
			pGenkiLabelList = calloc(bc, sizeof(GENKI_FACE_BBOX));
			PGENKI_FACE_BBOX p = (PGENKI_FACE_BBOX)pGenkiLabelList;
			while(	fscanf(f, "%d %d %d", &(p[i].center_col), &(p[i].center_row),
						&(p[i].diameter)) != EOF){
				//printf("%d:(x,y,d)=(%d, %d, %d)\n",i, p[i].center_col,p[i].center_row,p[i].diameter );
				i++;
			}
			printf("total %d files in the list:(%d,%d,%d)\n",bc,
				p[i-1].center_col, p[i-1].center_row, p[i-1].diameter);
		}
		fclose(f);
		ret = 0;
	}else{
		printf("%s opening failure\n", filename);
	}
	return ret;
}

void memdump(void)
{
	struct sysinfo info;
	sysinfo( &info );
	printf("mem_unit=%lu\n", (size_t)info.mem_unit);
	printf("total RAM=%lu\n", (size_t)info.totalram * (size_t)info.mem_unit);
	printf("total Available RAM=%lu\n", (size_t)info.freeram * (size_t)info.mem_unit);
}

int main(int argc, char **argv)
{
	unsigned long ulUsed=0;
	int curObjIndex=0;
	int totalNonObjFiles=0;
	CvRNG rng;
	rng = cvRNG(cvGetTickCount());

	memdump();
	if( (argc >= 3) && argv[1]){
		char listFileName[MAX_FILE_PATH_SIZE];
		char *srcfolder,*dstfolder;
		srcfolder = argv[1];
		dstfolder = argv[2];
		readGenkiImgFileList(srcfolder);
		readGenkiLabelList(srcfolder);

		snprintf(listFileName, MAX_FILE_PATH_SIZE, "%s/list.txt",dstfolder);
		FILE *fList = fopen(listFileName, "w" );
		if(fList > 0){
		#pragma omp parallel
		{
			int id = omp_get_thread_num();//local to this thread
			int counts=0,i=0;
			int cont=1;	//local to this thread stack
			char (*list)[MaxFileNameLen] = pGenkiImgList;
			char imgFileName[MAX_FILE_PATH_SIZE];
			char ridFileName[MAX_FILE_PATH_SIZE];
			IplImage *image=NULL;
			PGENKI_FACE_BBOX fb=NULL;
			CvRect roi;

			while(cont){
				memset(imgFileName,0,MAX_FILE_PATH_SIZE);
				#pragma omp critical
				{//get an id to the file list, so the file can be opened later.
					if(curObjIndex < TotalListFiles){
						snprintf(imgFileName, MAX_FILE_PATH_SIZE, "%s/files/%s",srcfolder, list[curObjIndex]);
						i=curObjIndex++;
						printf(">>>CR:%d:%d:%s\n", id, i, imgFileName);
					}
					if(curObjIndex >= TotalListFiles){
						printf("%d is done\n", id);
						cont=0;
					}
				}
				if(imgFileName[0]){//load the image file
					fb = (PGENKI_FACE_BBOX)pGenkiLabelList;
					printf("i->%d\n", i);
					printf("<<<TID[%d]:%d:(%d,%d,%d):%s\n", id, i,
						   fb[i].center_col, fb[i].center_row, fb[i].diameter, imgFileName);
					image= cvLoadImage(imgFileName, CV_LOAD_IMAGE_GRAYSCALE);
					if(image){
						roi = cvRect(fb[i].center_col - fb[i].diameter/2,//x
												fb[i].center_row - fb[i].diameter/2,//y
												fb[i].diameter,fb[i].diameter);//w, h
						//printf(">>>i->%d\n", i);
						//printf("depth:%d, channel=%d, (%d,%d)\n", image->depth, image->nChannels,
						//								image->width, image->height);
						snprintf(ridFileName, MAX_FILE_PATH_SIZE, "%s/face%d.rid",dstfolder,i*2);
						printf("ridFileName=%s\n", ridFileName);
						export(image, fb[i] , i*2, fList, rng);
						saveAsRID(ridFileName, image, roi);
						exportmirrored(image, fb[i] , i*2+1, fList, rng);
						snprintf(ridFileName, MAX_FILE_PATH_SIZE, "%s/face%d.rid",dstfolder,i*2+1);
						printf("ridFileName=%s\n", ridFileName);
						saveAsRID(ridFileName, image, roi);
						cvReleaseImage(&image);
					}else{
						printf("%s not found\n", imgFileName);
						cont=0;
					}
				}else{
					printf("TID:%d, null image\n", id);
				}
			}
		}
			fclose(fList);
		}else{
			printf("[%s]opening failure : %d\n", listFileName, errno);
		}
	}else{
		printf("genki_path dst_path\n");
	}
	if(pGenkiImgList){
		free(pGenkiImgList);
		pGenkiImgList=NULL;
	}
	if(pGenkiLabelList){
		free(pGenkiLabelList);
		pGenkiLabelList=NULL;
	}
	memdump();
}