#ifndef	_H_GENKI_HEADER_H
#define	_H_GENKI_HEADER_H

#define	MAX_OBJ_FILENAME_LEN	(81)
#define	MAX_OBJ_FILES		(1000000)
#define	MAX_FILE_PATH_SIZE	(1024)

#define	CV_DATASET_GENKI_IMAGES		"/Subsets/GENKI-SZSL/GENKI-SZSL_Images.txt"
#define	CV_DATASET_GENKI_LABELS	   	"/Subsets/GENKI-SZSL/GENKI-SZSL_labels.txt"

#define max( a, b ) ((a) > (b) ? (a) : (b) )
#define min( a, b ) ((a) < (b) ? (a) : (b) )
#define MAX_WIN_SIZE	(192.0)

typedef struct _genki_bounding_box{
int	center_col;	//X of the box center
int center_row;	//Y of the box center
int diameter;	//diameter of the box
} GENKI_FACE_BBOX, *PGENKI_FACE_BBOX;

extern int genki(char *facepath, char *nonfacepath, char *dst);

extern int TotalListFiles;
extern int MaxFileNameLen;
extern void *pGenkiImgList;
extern void *pGenkiLabelList;

#endif
