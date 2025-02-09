/// mini7z - minimalistic 7z.dll client written in C ///
/// multibyte filenames are not tested ///

#include "../lib/lzma.h"
#include "../lib/xutil.h" // ismatchwildcard

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

static int guessed7zVersion = 0;

static WORD LastByte(LPCSTR lpszStr){
	int n=strlen(lpszStr);
	return n ? lpszStr[n-1] : 0;
}

static int match(const char *name, int argc, const char **argv){
  for(int i=0;i<argc;i++){
	if(matchwildcard(argv[i],name))return 0;
  }
  return 1;
}

typedef struct{
	IArchiveExtractCallback22_vt *vt;
	u32 refs;
	SCryptoGetTextPasswordFixed22 setpassword;
	IInArchive22_ *archiver;
	u32 lastIndex;
	int argc;
	const char **argv;
	const char *dir;
} SArchiveExtractCallbackFileList22;

static HRESULT WINAPI SArchiveExtractCallbackFileList22_GetStream(void* _self, u32 index, /*ISequentialOutStream22_*/IOutStream22_ **outStream, s32 askExtractMode){
	SArchiveExtractCallbackFileList22 *self = (SArchiveExtractCallbackFileList22*)_self;
	*outStream = NULL;
	PROPVARIANT path;
	memset(&path,0,sizeof(PROPVARIANT));
	self->lastIndex = index;
	lzmaGetArchiveFileProperty(self->archiver, index, kpidPath, &path);
	int len = SysStringLen(path.bstrVal);
	int dirlen = strlen(self->dir);
	char *fname = (char*)calloc(1,dirlen+len*3);
	strcpy(fname,self->dir);
	if(path.bstrVal){
		wcstombs(fname+dirlen,path.bstrVal,len*3);
		if(!match(fname+dirlen,self->argc,self->argv)){
			printf("Extracting %s...\n",fname+dirlen);
			makedir(fname);
			IOutStream22_* stream = (IOutStream22_*)calloc(1,sizeof(SOutStreamFile22));
			MakeSOutStreamFile22((SOutStreamFile22*)stream,fname,false);
			*outStream = stream; // WILL BE RELEASED AUTOMATICALLY.
		}
	}
	PropVariantClear(&path);
	free(fname);
	return S_OK;
}

typedef struct{
	IArchiveExtractCallback23_vt *vt;
	u32 refs;
	SCryptoGetTextPasswordFixed23 setpassword;
	IInArchive23_ *archiver;
	u32 lastIndex;
	int argc;
	const char **argv;
	const char *dir;
} SArchiveExtractCallbackFileList23;

static HRESULT WINAPI SArchiveExtractCallbackFileList23_GetStream(void* _self, u32 index, /*ISequentialOutStream23_*/IOutStream23_ **outStream, s32 askExtractMode){
	SArchiveExtractCallbackFileList23 *self = (SArchiveExtractCallbackFileList23*)_self;
	*outStream = NULL;
	PROPVARIANT path;
	memset(&path,0,sizeof(PROPVARIANT));
	self->lastIndex = index;
	lzmaGetArchiveFileProperty(self->archiver, index, kpidPath, &path);
	int len = SysStringLen(path.bstrVal);
	int dirlen = strlen(self->dir);
	char *fname = (char*)calloc(1,dirlen+len*3);
	strcpy(fname,self->dir);
	if(path.bstrVal){
		wcstombs(fname+dirlen,path.bstrVal,len*3);
		if(!match(fname+dirlen,self->argc,self->argv)){
			printf("Extracting %s...\n",fname+dirlen);
			makedir(fname);
			IOutStream23_* stream = (IOutStream23_*)calloc(1,sizeof(SOutStreamFile23));
			MakeSOutStreamFile23((SOutStreamFile23*)stream,fname,false);
			*outStream = stream; // WILL BE RELEASED AUTOMATICALLY.
		}
	}
	PropVariantClear(&path);
	free(fname);
	return S_OK;
}

typedef struct{
	IArchiveUpdateCallback22_vt *vt;
	u32 refs;
	SCryptoGetTextPassword2Fixed22 setpassword;
	IOutArchive22_ *archiver;
	u32 lastIndex;
	int argc;
	const char **argv;
} SArchiveUpdateCallbackFileList22;

static HRESULT WINAPI SArchiveUpdateCallbackFileList22_GetStream(void* _self, u32 index, /*ISequentialInStream22_*/IInStream22_ **inStream){
	SArchiveUpdateCallbackFileList22 *self = (SArchiveUpdateCallbackFileList22*)_self;
	*inStream = NULL;
	IInStream22_* stream = (IInStream22_*)calloc(1,sizeof(SInStreamFile22));
	if(!MakeSInStreamFile22((SInStreamFile22*)stream,self->argv[index])){free(stream);return E_FAIL;}
	printf("Compressing %s...\n",self->argv[index]);
	*inStream = stream; // WILL BE RELEASED AUTOMATICALLY.
	return S_OK;
}

typedef struct{
	IArchiveUpdateCallback23_vt *vt;
	u32 refs;
	SCryptoGetTextPassword2Fixed23 setpassword;
	IOutArchive23_ *archiver;
	u32 lastIndex;
	int argc;
	const char **argv;
} SArchiveUpdateCallbackFileList23;

static HRESULT WINAPI SArchiveUpdateCallbackFileList23_GetStream(void* _self, u32 index, /*ISequentialInStream23_*/IInStream23_ **inStream){
	SArchiveUpdateCallbackFileList23 *self = (SArchiveUpdateCallbackFileList23*)_self;
	*inStream = NULL;
	IInStream23_* stream = (IInStream23_*)calloc(1,sizeof(SInStreamFile23));
	if(!MakeSInStreamFile23((SInStreamFile23*)stream,self->argv[index])){free(stream);return E_FAIL;}
	printf("Compressing %s...\n",self->argv[index]);
	*inStream = stream; // WILL BE RELEASED AUTOMATICALLY.
	return S_OK;
}

static HRESULT WINAPI SArchiveUpdateCallbackFileList_GetProperty(void* _self, u32 index, PROPID propID, PROPVARIANT *value){
	SArchiveUpdateCallbackFileList23 *self = (SArchiveUpdateCallbackFileList23*)_self;
	//printf("%d %d\n",index,propID);
	if(propID == kpidPath){
		int len=strlen(self->argv[index]);
		value->vt = VT_BSTR;
		value->bstrVal = SysAllocStringLen(NULL,len);
		mbstowcs(value->bstrVal, self->argv[index], len);
	}else if(propID == kpidIsDir || propID == kpidIsAnti){
		value->vt = VT_BOOL;
		value->boolVal = VARIANT_FALSE;
	}else if(propID == kpidSize){
		value->vt = VT_UI8;
		struct stat st;
		stat(self->argv[index], &st);
		//value->ulVal = 1; ///
		value->uhVal.QuadPart = st.st_size;
	}else if(propID == kpidMTime){
		value->vt = VT_FILETIME;
		struct stat st;
		stat(self->argv[index], &st);
		value->filetime = UTCToFileTime(st.st_mtime);
	}else if(propID == kpidAttrib){
		value->vt = VT_UI4;
		value->uintVal = 0x20;
	}
	return S_OK;
}

#define extract extract23
#define list list23
#define add add23
#define IInArchive_ IInArchive23_
#define IArchiveExtractCallback_ IArchiveExtractCallback23_
#define IOutArchive_ IOutArchive23_
#define SInStreamFile SInStreamFile23
#define SArchiveExtractCallbackFileList SArchiveExtractCallbackFileList23
#define SArchiveExtractCallbackBare SArchiveExtractCallbackBare23
#define SOutStreamFile SOutStreamFile23
#define SArchiveUpdateCallbackFileList SArchiveUpdateCallbackFileList23
#define SArchiveUpdateCallbackBare SArchiveUpdateCallbackBare23
#define MakeSInStreamFile MakeSInStreamFile23
#define MakeSArchiveExtractCallbackBare MakeSArchiveExtractCallbackBare23
#define MakeSOutStreamFile MakeSOutStreamFile23
#define MakeSArchiveUpdateCallbackBare MakeSArchiveUpdateCallbackBare23
#define SArchiveExtractCallbackFileList_GetStream SArchiveExtractCallbackFileList23_GetStream
#define SArchiveUpdateCallbackFileList_GetStream SArchiveUpdateCallbackFileList23_GetStream
#include "mini7z_23.h"
#undef extract
#undef list
#undef add
#undef IInArchive_
#undef IArchiveExtractCallback_
#undef IOutArchive_
#undef SInStreamFile
#undef SArchiveExtractCallbackFileList
#undef SArchiveExtractCallbackBare
#undef SOutStreamFile
#undef SArchiveUpdateCallbackFileList
#undef SArchiveUpdateCallbackBare
#undef MakeSInStreamFile
#undef MakeSArchiveExtractCallbackBare
#undef MakeSOutStreamFile
#undef MakeSArchiveUpdateCallbackBare
#undef SArchiveExtractCallbackFileList_GetStream
#undef SArchiveUpdateCallbackFileList_GetStream

#define extract extract22
#define list list22
#define add add22
#define IInArchive_ IInArchive22_
#define IArchiveExtractCallback_ IArchiveExtractCallback22_
#define IOutArchive_ IOutArchive22_
#define SInStreamFile SInStreamFile22
#define SArchiveExtractCallbackFileList SArchiveExtractCallbackFileList22
#define SArchiveExtractCallbackBare SArchiveExtractCallbackBare22
#define SOutStreamFile SOutStreamFile22
#define SArchiveUpdateCallbackFileList SArchiveUpdateCallbackFileList22
#define SArchiveUpdateCallbackBare SArchiveUpdateCallbackBare22
#define MakeSInStreamFile MakeSInStreamFile22
#define MakeSArchiveExtractCallbackBare MakeSArchiveExtractCallbackBare22
#define MakeSOutStreamFile MakeSOutStreamFile22
#define MakeSArchiveUpdateCallbackBare MakeSArchiveUpdateCallbackBare22
#define SArchiveExtractCallbackFileList_GetStream SArchiveExtractCallbackFileList22_GetStream
#define SArchiveUpdateCallbackFileList_GetStream SArchiveUpdateCallbackFileList22_GetStream
#include "mini7z_23.h"
#undef extract
#undef list
#undef add
#undef IInArchive_
#undef IArchiveExtractCallback_
#undef IOutArchive_
#undef SInStreamFile
#undef SArchiveExtractCallbackFileList
#undef SArchiveExtractCallbackBare
#undef SOutStreamFile
#undef SArchiveUpdateCallbackFileList
#undef SArchiveUpdateCallbackBare
#undef MakeSInStreamFile
#undef MakeSArchiveExtractCallbackBare
#undef MakeSOutStreamFile
#undef MakeSArchiveUpdateCallbackBare
#undef SArchiveExtractCallbackFileList_GetStream
#undef SArchiveUpdateCallbackFileList_GetStream

static int extract(const char *password,const char *arc, const char *dir, int argc, const char **argv){
	return guessed7zVersion>=2300 ? extract23(password, arc, dir, argc, argv) : extract22(password, arc, dir, argc, argv);
}

static int list(const char *password,const char *arc, int argc, const char **argv){
	return guessed7zVersion>=2300 ? list23(password, arc, argc, argv) : list22(password, arc, argc, argv);
}

static int add(const char *password,unsigned char arctype,int level,const char *arc, int argc, const char **argv){
	return guessed7zVersion>=2300 ? add23(password, arctype, level, arc, argc, argv) : add22(password, arctype, level, arc, argc, argv);
}

// 7zip/Guid.txt
// ruby -nae 'puts "\"0x%s %3d %s\\n\""%[$F[0],$F[0].to_i(16),$F[1]] if $F.size==2'

#if 0
static int info(){
	fprintf(stderr,
"known archive types:\n"
"0x01   1 Zip\n"
"0x02   2 BZip2\n"
"0x03   3 Rar\n"
"0x04   4 Arj\n"
"0x05   5 Z\n"
"0x06   6 Lzh\n"
"0x07   7 7z\n"
"0x08   8 Cab\n"
"0x09   9 Nsis\n"
"0x0A  10 lzma\n"
"0x0B  11 lzma86\n"
"0x0C  12 xz\n"
"0x0D  13 ppmd\n"
"0xC6 198 COFF\n"
"0xC7 199 Ext\n"
"0xC8 200 VMDK\n"
"0xC9 201 VDI\n"
"0xCA 202 Qcow\n"
"0xCB 203 GPT\n"
"0xCC 204 Rar5\n"
"0xCD 205 IHex\n"
"0xCE 206 Hxs\n"
"0xCF 207 TE\n"
"0xD0 208 UEFIc\n"
"0xD1 209 UEFIs\n"
"0xD2 210 SquashFS\n"
"0xD3 211 CramFS\n"
"0xD4 212 APM\n"
"0xD5 213 Mslz\n"
"0xD6 214 Flv\n"
"0xD7 215 Swf\n"
"0xD8 216 Swfc\n"
"0xD9 217 Ntfs\n"
"0xDA 218 Fat\n"
"0xDB 219 Mbr\n"
"0xDC 220 Vhd\n"
"0xDD 221 Pe\n"
"0xDE 222 Elf\n"
"0xDF 223 Mach-O\n"
"0xE0 224 Udf\n"
"0xE1 225 Xar\n"
"0xE2 226 Mub\n"
"0xE3 227 Hfs\n"
"0xE4 228 Dmg\n"
"0xE5 229 Compound\n"
"0xE6 230 Wim\n"
"0xE7 231 Iso\n"
"0xE9 233 Chm\n"
"0xEA 234 Split\n"
"0xEB 235 Rpm\n"
"0xEC 236 Deb\n"
"0xED 237 Cpio\n"
"0xEE 238 Tar\n"
"0xEF 239 GZip\n"
	);
	return 0;
}
#endif

#ifdef STANDALONE
unsigned char buf[BUFLEN];
int main(int argc, const char **argv){
#else
int mini7z(int argc, const char **argv){
#endif
  printf(
  	"7z Extractor\n"
  	"Usage:\n"
  	"mini7z i\n"
  	"mini7z [xl][PASSWORD] arc.7z [extract_dir] [filespec]\n"
	"mini7z a[PASSWORD] TYPE LEVEL(-1) arc.7z [filespec] (cannot handle wildcard nor directories)\n"
	"(possibly) find filespec -type f | xargs mini7z a 7 9 arc.7z\n"
  	"\n"
  );
  if(argc<3 && !(argc==2&&argv[1][0]=='i'))return -1;
  const char *w="*";

  if(lzmaOpen7z()){
    fprintf(stderr,"cannot load 7z.so.\n");
    return -1;
  }
  guessed7zVersion = lzmaGuessVersion();
  lzmaLoadExternalCodecs();

  int ret = -1;
  switch(argv[1][0]){
	case 'x':{
		const char *password=argv[1]+1;
		const char *arc=argv[2];
		const char *dir="./";
		argv+=3;argc-=3;
		if(argv[0]&&(LastByte(argv[0])=='/'||LastByte(argv[0])=='\\')){makedir(argv[0]);dir=argv[0];argv++;argc--;}
		ret = extract(password,arc,dir,argc?argc:1,argc?argv:&w);
		break;
	}
    case 'l':{
		ret = list(argv[1]+1,argv[2],argc-3?argc-3:1,argc-3?argv+3:&w);
		break;
	}
	case 'a':{
		if(argc<5){
			ret = -1;
		}else{
		    ret = add(argv[1]+1,strtol(argv[2],NULL,0),strtol(argv[3],NULL,10),argv[4],argc-5,argv+5);
		}
		break;
	}
	case 'i':{
		ret = lzmaShowInfos(stderr);//info();
		break;
	}
	default:{
		ret = -1;
	}
  }
  lzmaUnloadExternalCodecs();
  lzmaClose7z();
  return ret;
}
