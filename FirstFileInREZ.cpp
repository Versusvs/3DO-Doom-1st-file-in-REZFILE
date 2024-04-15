#include "stdafx.h"

#include <cstdlib>
#include <iostream>

#include "burger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

using namespace std;

typedef struct {
	Word Type;
	Word RezNum;
	LongWord Length;
	LongWord Offset;
} RezEntryInternal;

typedef struct {
	Word PicWidth;
	Word PicHeight;
	Word Blank;
} PicBlock;

typedef struct {
	Word TexCounter;
	Word TexPos;			// Always == 2 (Rezfile structure)
	Word FlatCounter;
	Word FlatPos;
} Header;

static Word LineNum;               /* Line being executed from the script */
static Word RezType;               /* Resource type */
static Word RezIDNum;              /* Resource ID Number */
static Word RezCount;              /* Number of resources loaded */
static Word RezFixed;				/* Fixed memory flag */
static char Delimiters[] = " \t\n";        /* Token delimiters */
static char NumDelimiters[] = " ,\t\n";    /* Value delimiters */
static char InputLine[256];        /* Input line from script */
static char RezFileName[32] = "FIRSTFILE";
static char ScratchFileName[32] = "Scratch.TMP";
static FILE *tmpfp;                /* Temp data output file */
static LongWord TempFileLength;    /* Length of the data output */
//static Boolean SwapEndian;
typedef int Boolean;         /* True if I should swap the endian */
Boolean SwapEndian;
#define TRUE  1	
#define FALSE 0	
static void LoadTexture(Word Config);
static void LoadFlat(Word Config);
static LongWord Offset_to_Flats;

static Header *VSHeader;

static Word Height;
static Word Width;
//LongWord SwapULong(LongWord val);
#define SwapULong(val) val = ( (((val) >> 24) & 0x000000FF) | (((val) >> 8) & 0x0000FF00) | (((val) <<  8) & 0x00FF0000) | (((val) << 24) & 0xFF000000) )
//#define SwapULong(val) (val << 24 | (val << 8 & 0xFF0000) | (val >> 8 & 0xFF00) | val >> 24 & 0xFF)
#define Swap8Bytes(val) ( (((val) >> 56) & 0x00000000000000FF) | (((val) >> 40) & 0x000000000000FF00) | (((val) >> 24) & 0x0000000000FF0000) | (((val) >>  8) & 0x00000000FF000000) | (((val) <<  8) & 0x000000FF00000000) | (((val) << 24) & 0x0000FF0000000000) | (((val) << 40) & 0x00FF000000000000) | (((val) << 56) & 0xFF00000000000000) )
#define Swap2Bytes(val) ( (((val) >> 8) & 0x00FF) | (((val) << 8) & 0xFF00) )

static Word LastRezNum;			/* Step value for resources */

#define CommandCount 6      /* Number of commands */
static char *Commands[] = {
	"TYPE","LOAD","ENDIAN","LOADNEXT","LOADFIXED","LOADHANDLE"
};

#define PicBlock_SIZE (0x1800*sizeof(PicBlock))
#define BUFFER_SIZE 0x40000UL
#define ENTRY_SIZE (0x2000*sizeof(RezEntryInternal))
#define Header_SIZE (0x2000*sizeof(VSHeader))

static Byte *Buffer;         /* File buffer for data transfer */
static RezEntryInternal *EntArray;     /* Resource headers */
static MyRezHeader MyHeader;		/* Header to save to disk */

static PicBlock *PicArray;
static Word Counter_textures;
static Word Counter_flats;
static Word TexData; // Quantity of bytes occupied by textures PicBlock
static Word FlatData; 

static Byte ErrorStr[80];

#if 1;
/**********************************

	Print out a script error

**********************************/

static void PrintError(char *Error)
{
	printf("# Error in Line %d, %s\n",LineNum,Error);
}

/**********************************

	Init the resource file and set up the defaults

**********************************/
#endif;

static void InitLoadingPics(void)
{
Offset_to_Flats = 0;
Counter_textures = 0;
Counter_flats = 0;

VSHeader->TexCounter = 0;
VSHeader->TexPos = 2;			// Always == 2 (Rezfile structure)
VSHeader->FlatCounter = 0;
VSHeader->FlatPos = 0;

//Height = 0;
//Width = 0;

TempFileLength = 0;     /* No data saved */
}



static void MakeTex(FILE* fp)
{
	char *TextPtr;  /* Pointer to string token */
	Word i;         /* Index */

	InitLoadingPics();    /* Init the output file and other variables */
	
	while (fgets(InputLine,sizeof(InputLine),fp)) { /* Get a string */
		++LineNum;          /* Adjust the line # */
		TextPtr = strtok(InputLine,Delimiters); /* Get the first token */
		if (!TextPtr) {
			continue;
		}
		i = 0;      /* Check for the first command */
		if (isalnum(TextPtr[0])) {  /* Comment? */
			do {
				if (!strcmp(TextPtr,Commands[i])) { /* Match? */
					switch (i) {        /* Execute the command */
					case 0:
//						SetType();   /* Target machine */
						break;
					case 1:
						LoadTexture(0); 
//						LoadRezFile(0);   /* Input art file type */
						break;
//					case 2:
//						SetEndian();    /* Swap endian if needed */
//						break;
//					case 3:
//						LoadRezFile(1);	/* Load the next sequential record */
//						break;
//					case 4:
//						RezFixed = 1;	/* Movable memory */
//						break;
//					case 5:
//						RezFixed = 0;

					}
					break;      /* Don't parse anymore! */
				}
			} while (++i<CommandCount); /* Keep checking */
		}
		if (i==CommandCount) {      /* Didn't find it? */
			printf("# Command %s not implemented\n",TextPtr);
		}
	}
///	WrapUpMakeRez();

}


static void MakeFlats(FILE* fp)
{
	char *TextPtr;  /* Pointer to string token */
	Word i;         /* Index */

//	InitLoadingPics();    /* Init the output file and other variables */
	
	while (fgets(InputLine,sizeof(InputLine),fp)) { /* Get a string */
		++LineNum;          /* Adjust the line # */
		TextPtr = strtok(InputLine,Delimiters); /* Get the first token */
		if (!TextPtr) {
			continue;
		}
		i = 0;      /* Check for the first command */
		if (isalnum(TextPtr[0])) {  /* Comment? */
			do {
				if (!strcmp(TextPtr,Commands[i])) { /* Match? */
					switch (i) {        /* Execute the command */
					case 0:
//						SetType();   /* Target machine */
						break;
					case 1:
						LoadFlat(0); 
//						LoadRezFile(0);   /* Input art file type */
						break;
//					case 2:
//						SetEndian();    /* Swap endian if needed */
//						break;
//					case 3:
//						LoadRezFile(1);	/* Load the next sequential record */
//						break;
//					case 4:
//						RezFixed = 1;	/* Movable memory */
//						break;
//					case 5:
//						RezFixed = 0;

					}
					break;      /* Don't parse anymore! */
				}
			} while (++i<CommandCount); /* Keep checking */
		}
		if (i==CommandCount) {      /* Didn't find it? */
			printf("# Command %s not implemented\n",TextPtr);
		}
	}
///	WrapUpMakeRez();

}




static void MakeFinalFile(void)
{
	FILE *outfp, *tempfile;
	LongWord Length;
//	Header *VSHeader;

////	outfp = fopen(RezFileName,"wb");    /* Open the final file */

#if 0;
	VSHeader->TexCounter = 1;
	VSHeader->TexPos = 2;
	VSHeader->FlatCounter = 1;
	VSHeader->FlatPos = 1;
#endif;

#if 1; 	
	VSHeader->TexCounter = Counter_textures;
	VSHeader->TexPos = 2;
	VSHeader->FlatCounter = Counter_flats;
///	VSHeader->FlatPos = Offset_to_Flats;
	VSHeader->FlatPos = Counter_textures+2;

	VSHeader->TexCounter = SwapULong(VSHeader->TexCounter);
	VSHeader->TexPos = SwapULong(VSHeader->TexPos);
	VSHeader->FlatCounter = SwapULong(VSHeader->FlatCounter);
	VSHeader->FlatPos = SwapULong(VSHeader->FlatPos);

///	fwrite(MyHeader,1,0x4,outfp);
#endif;

////	fseek(outfp,0,SEEK_SET );
////	fwrite(VSHeader,4,sizeof(VSHeader),outfp);

////	fclose(outfp);

#if 0;
outfp = fopen(RezFileName,"ab"); // Append to this file
// Здесь приклеиваем временный файл
fclose(outfp);
#endif;

outfp = fopen("r_TexData","wb"); // Перезапишем файл. Создадим файл и обнулим его.
fwrite(0,0,0,outfp);
fclose(outfp);


outfp = fopen("r_TexData","ab"); // Append to this file
fwrite(VSHeader,4,sizeof(VSHeader),outfp);

tempfile = fopen("TempFile","rb"); 
fseek(tempfile, 0, SEEK_END);  
Length = ftell(tempfile);

// Здесь читаем временный файл в буфер для записи в финальный файл
//	Length = TempFileLength;      /* How large is the output file? */
	fseek(tempfile,0,SEEK_SET);       /* Reset to the beginning */
	if (Length) {		/* Any data to copy? */
		do {
			LongWord Chunk;
			if (Length>PicBlock_SIZE*Counter_textures) {	/* Get the smaller, the length */
				Chunk = PicBlock_SIZE*Counter_textures;		/* or the buffer size */
			} else {
				Chunk = Length;
			}
			fread(Buffer,1,Chunk,tempfile);
			fwrite(Buffer,1,Chunk,outfp);
			Length -= Chunk;
		} while (Length);
	}

// Добавляем в финальный файл прочитанный буфер.


fclose(tempfile);
fclose(outfp);


	remove("Tempfile");	// REMOVE FOR FINAL BUILD!!!
}






static void LoadTexture(Word Config)
{
	char *TextPtr;
	FILE *outfp;
	FILE *infile;
	LongWord Offset, Chunk;
	PicBlock *PicPtr;
	Word TmpWidth, TmpHeight;
	int i;

	Chunk = 0x10-4;
	PicPtr = &PicArray[Chunk];

	TextPtr = strtok(0,NumDelimiters);
	if (!TextPtr) {
		PrintError("Not enough parms for LOAD");
		return;
	}

	TextPtr = strtok(0,NumDelimiters);
	if (!TextPtr) {
		PrintError("Not enough parms for LOAD");
		return;
	}
	
	Offset = TempFileLength;

	infile = fopen(TextPtr,"rb");


//fseek(infile,0x0F,SEEK_SET);
//fread(PicPtr->PicHeight,0x4,1,infile);
//PicPtr->PicHeight = fgetc(infile);

//fseek(infile,0x13,SEEK_SET);
//fread(PicPtr->PicWidth,0x4,1,infile);
//PicPtr->PicWidth = fgetc(infile);
#if 0;
for (i = 0x0F; i <= 0x13; i++) // задаем начальное значение 1, конечное 0x18 и задаем шаг цикла - 1.
	{
		fseek(infile,i,SEEK_SET);
		fread(PicPtr,1,1,infile);
		TmpWidth = PicPtr->PicHeight;
		TmpHeight = PicPtr->PicWidth;

		PicPtr->PicHeight = TmpHeight;
		PicPtr->PicWidth = TmpWidth;
	}
#endif;

#if 1
fseek(infile,0x12,SEEK_SET);
fread(PicPtr,0x8,1,infile);

TmpWidth = PicPtr->PicHeight;
TmpHeight = PicPtr->PicWidth;

PicPtr->PicHeight = TmpHeight;
PicPtr->PicWidth = TmpWidth;

//PicPtr->PicHeight = Swap8Bytes(PicPtr->PicHeight);
//PicPtr->PicWidth = Swap8Bytes(PicPtr->PicWidth);

//	PicPtr->PicWidth = 0x80;
//	PicPtr->PicHeight = 0x70;
	PicPtr->Blank = 0x0;

	PicPtr->PicWidth = SwapULong(PicPtr->PicWidth);
	PicPtr->PicHeight = SwapULong(PicPtr->PicHeight);
#endif

	printf("# Loading texture #");
	outfp = fopen("TempFile","ab");
	
	
	fseek(outfp,0,SEEK_END);
	Offset = ftell(outfp);
	TempFileLength += Offset;

	fwrite(PicPtr,1,Chunk,outfp);
	fclose(outfp);
	
	fclose(infile);
		++Counter_textures;
	
	cout << Counter_textures << "\n";

	printf("# Parsing file: ");
	cout << TextPtr << "\n";

	printf("Offset: ");
	cout << TempFileLength << "\n";
	
	Offset_to_Flats = TempFileLength;

//	printf("\n");
//	printf("Counter_textures = ");
//	cout << Counter_textures << "\n";

//	VSHeader->TexCounter = Counter_textures;
//	VSHeader->TexPos = 2;			// Always == 2 (Rezfile structure)
//	VSHeader->FlatPos = Offset_to_Flats;


}



static void LoadFlat(Word Config)
{
	char *TextPtr;
	FILE *outfp;
	FILE *infile;
	LongWord Offset, Chunk;
	PicBlock *PicPtr;
	Word TmpWidth, TmpHeight;


	Chunk = 0x10-4;
	PicPtr = &PicArray[Chunk];

	TextPtr = strtok(0,NumDelimiters);
	if (!TextPtr) {
		PrintError("Not enough parms for LOAD");
		return;
	}

	TextPtr = strtok(0,NumDelimiters);
	if (!TextPtr) {
		PrintError("Not enough parms for LOAD");
		return;
	}
	
	Offset = TempFileLength;

	infile = fopen(TextPtr,"rb");

fseek(infile,0x0F,SEEK_SET);
fread(PicPtr,0x8,1,infile);
	
TmpWidth = PicPtr->PicHeight;
TmpHeight = PicPtr->PicWidth;

PicPtr->PicHeight = TmpHeight;
PicPtr->PicWidth = TmpWidth;

//	PicPtr->PicWidth = 0x80;
//	PicPtr->PicHeight = 0x70;
	PicPtr->Blank = 0x0;

//	PicPtr->PicWidth = SwapULong(PicPtr->PicWidth);
//	PicPtr->PicHeight = SwapULong(PicPtr->PicHeight);
#if 1;	
	printf("# Loading flat #");
	outfp = fopen("TempFile","ab");
	
	fseek(outfp,0,SEEK_END);
	Offset = ftell(outfp);
	TempFileLength += Offset;

//	fwrite(PicPtr,1,Chunk,outfp); // Писать инфу о flats не нужно!
	fclose(outfp);
	
	fclose(infile);
		++Counter_flats;
	
	cout << Counter_flats << "\n";

	printf("# Parsing file: ");
	cout << TextPtr << "\n";

	printf("Offset: ");
	cout << TempFileLength << "\n";
#endif;
//	VSHeader->FlatCounter = Counter_flats;

}



int _tmain(int argc, _TCHAR* argv[])
{
	FILE *fp;               /* Input file */

argc=2;

	if (argc!=2) {          /* Gotta have input and output arguments */
		printf("# Copyright 1995 by LogicWare\n"
			"# This program will create a resource data file using a script\n"
			"# Usage: MakeRez Infile\n");
		return 1;
	}
	Buffer = (Byte *)malloc(BUFFER_SIZE);
	if (!Buffer) {
		printf("# Not enough memory for buffer!\n");
		return 1;
	}
	EntArray = (RezEntryInternal *)malloc(ENTRY_SIZE);
	if (!EntArray) {
		free(Buffer);
		printf("# Not enough memory for resource entries!\n");
		return 1;
	}
	
	PicArray = (PicBlock *)malloc(PicBlock_SIZE);
		if (!PicArray) {
		free(Buffer);
		printf("# Not enough memory for picture entries!\n");
		return 1;
	}


	VSHeader = (Header *)malloc(Header_SIZE);
		if (!VSHeader) {
		free(Buffer);
		printf("# Not enough memory for Header entries!\n");
		return 1;
	}

    fp = fopen("LoadTexturesBMP.txt","r");    /* Read the ASCII script */
	if (!fp) {
		printf("# Can't open Textures script file %s.\n",argv[1]);    /* Oh oh */
		free(Buffer);
		free(EntArray);
		free(PicArray);
		return 1;
	}


///	MakeRez(fp);     /* Slice it up! */
//	LoadTexture();
	
	MakeTex(fp);
	fclose(fp);     /* Close the file */
///	free(Buffer);
///	free(EntArray);
///	free(PicArray);
	
   
	fp = fopen("LoadFlatsBMP.txt","r");    /* Read the ASCII script */
	if (!fp) {
		printf("# Can't open Flats script file %s.\n",argv[1]);    /* Oh oh */
		free(Buffer);
		free(EntArray);
		free(PicArray);
		return 1;
	}


	MakeFlats(fp);
	
	MakeFinalFile();
#if 0;
	cout << "\n";
	cout << "\n";
	cout << "\n";
	cout << "TexCounter = ";
	cout << VSHeader->TexCounter;
		cout << "\n";
	cout << "TexPos = ";
	cout << VSHeader->TexPos;
		cout << "\n";
	cout << "FlatCounter = ";
	cout << VSHeader->FlatCounter;
		cout << "\n";
	cout << "FlatPos = ";
	cout << VSHeader->FlatPos;
		cout << "\n";
#endif;

	fclose(fp);     /* Close the file */
	free(Buffer);
	free(EntArray);
	free(PicArray);

	cout << "Press the enter key to continue ...";
    cin.get();
    return 0;
}
