/// mini7z - lzmasdk.c example to extract archives supported by 7z.so ///
/// multibyte filenames are not tested ///
/// cannot handle multivolume archives ///

#include "../lib/lzma.h"
#include "../lib/xutil.h" // ismatchwildcard

#include <stdio.h>
//#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

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
	IArchiveExtractCallback_vt *vt;
	u32 refs;
	SCryptoGetTextPasswordFixed setpassword;
	IInArchive_ *archiver;
	u32 lastIndex;
	int argc;
	const char **argv;
	const char *dir;
} SArchiveExtractCallbackFileList;

static HRESULT WINAPI SArchiveExtractCallbackFileList_GetStream(void* _self, u32 index, /*ISequentialOutStream_*/IOutStream_ **outStream, s32 askExtractMode){
	SArchiveExtractCallbackFileList *self = (SArchiveExtractCallbackFileList*)_self;
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
			IOutStream_* stream = (IOutStream_*)calloc(1,sizeof(SOutStreamFile));
			MakeSOutStreamFile((SOutStreamFile*)stream,fname,false);
			*outStream = stream; // WILL BE RELEASED AUTOMATICALLY.
		}
	}
	PropVariantClear(&path);
	free(fname);
	return S_OK;
}

static int extract(const char *password,const char *arc, const char *dir, int argc, const char **argv){
	if(lzmaOpen7z()){
		fprintf(stderr,"cannot load 7z.so.\n");
		return -1;
	}
	lzmaLoadUnrar();
	SInStreamFile sin;
	MakeSInStreamFile(&sin,arc);
	void *archiver=NULL;
	unsigned char arctype=0;
	for(;arctype<0xff;arctype++){
		if(!lzmaCreateArchiver(&archiver,arctype,0,0)){
			sin.vt->Seek(&sin,0,SEEK_SET,NULL); // archiver should do this automatically lol lol lol
			if(!lzmaOpenArchive(archiver,&sin,password))break;
			lzmaDestroyArchiver(&archiver,0);
		}
	}
	if(!archiver){
		fprintf(stderr,"7z.so could not open the file as any archive (possibly encrypted).\n");
		sin.vt->Release(&sin);
		lzmaClose7z();
		return 1;
	}
	printf("arctype = %d .\n\n",arctype);

	SArchiveExtractCallbackFileList sextract;
	MakeSArchiveExtractCallbackBare((SArchiveExtractCallbackBare*)&sextract,(IInArchive_*)archiver,password);
	sextract.argc = argc;
	sextract.argv = argv;
	sextract.dir = dir;
	sextract.vt->GetStream = SArchiveExtractCallbackFileList_GetStream;
	lzmaExtractArchive(archiver, NULL, -1, 0, (IArchiveExtractCallback_*)&sextract);

	lzmaDestroyArchiver(&archiver,0);
	sin.vt->Release(&sin);
	lzmaUnloadUnrar();
	lzmaClose7z();

	return 0;
}

static int list(const char *password,const char *arc, int argc, const char **argv){
	if(lzmaOpen7z()){
		fprintf(stderr,"cannot load 7z.so.\n");
		return -1;
	}
	SInStreamFile sin;
	MakeSInStreamFile(&sin,arc);
	void *archiver=NULL;
	unsigned char arctype=0;
	for(;arctype<0xff;arctype++){
		if(!lzmaCreateArchiver(&archiver,arctype,0,0)){
			sin.vt->Seek(&sin,0,SEEK_SET,NULL); // archiver should do this automatically lol lol lol
			if(!lzmaOpenArchive(archiver,&sin,password))break;
			lzmaDestroyArchiver(&archiver,0);
		}
	}
	if(!archiver){
		fprintf(stderr,"7z.so could not open the file as any archive (possibly encrypted).\n");
		sin.vt->Release(&sin);
		lzmaClose7z();
		return 1;
	}
	printf("arctype = %d .\n\n",arctype);
	int num_items;
	lzmaGetArchiveFileNum(archiver,(u32*)&num_items); // to deal with empty archive...
	printf("Name                                     PackedSize Size       Time                Method              \n");
	printf("---------------------------------------- ---------- ---------- ------------------- --------------------\n");
	for(int i=0;i<num_items;i++){
		PROPVARIANT propPath,propMethod,propPackedSize,propSize,propMTime;
		memset(&propPath,0,sizeof(PROPVARIANT));
		memset(&propMethod,0,sizeof(PROPVARIANT));
		memset(&propPackedSize,0,sizeof(PROPVARIANT));
		memset(&propSize,0,sizeof(PROPVARIANT));
		memset(&propMTime,0,sizeof(PROPVARIANT));
		lzmaGetArchiveFileProperty(archiver,i,kpidPath,&propPath);
		lzmaGetArchiveFileProperty(archiver,i,kpidMethod,&propMethod);
		lzmaGetArchiveFileProperty(archiver,i,kpidPackSize,&propPackedSize);
		lzmaGetArchiveFileProperty(archiver,i,kpidSize,&propSize);
		lzmaGetArchiveFileProperty(archiver,i,kpidMTime,&propMTime);
		cbuf[0]=0;
		if(propPath.bstrVal)wcstombs(cbuf,propPath.bstrVal,BUFLEN);
		if(!match(cbuf,argc,argv)){
			time_t t = FileTimeToUTC(propMTime.filetime);
			struct tm tt;
#if defined(_WIN32) || (!defined(__GNUC__) && !defined(__clang__))
			localtime_s(&tt,&t);
#else
			localtime_r(&t,&tt);
#endif
			strftime(cbuf,99,"%Y-%m-%d %H:%M:%S",&tt);
			printf("%-40ls %10llu %10llu %s %-20ls\n",propPath.bstrVal,propPackedSize.uhVal,propSize.uhVal,cbuf,propMethod.bstrVal);
		}
		PropVariantClear(&propPath);
		PropVariantClear(&propMethod);
	}
	printf("---------------------------------------- ---------- ---------- ------------------- --------------------\n");
	lzmaDestroyArchiver(&archiver,0);
	sin.vt->Release(&sin);
	lzmaClose7z();

	return 0;
}

typedef struct{
	IArchiveUpdateCallback_vt *vt;
	u32 refs;
	SCryptoGetTextPassword2Fixed setpassword;
	IOutArchive_ *archiver;
	u32 lastIndex;
	int argc;
	const char **argv;
} SArchiveUpdateCallbackFileList;

static HRESULT WINAPI SArchiveUpdateCallbackFileList_GetStream(void* _self, u32 index, /*ISequentialInStream_*/IInStream_ **inStream){
	SArchiveUpdateCallbackFileList *self = (SArchiveUpdateCallbackFileList*)_self;
	*inStream = NULL;
	IOutStream_* stream = (IOutStream_*)calloc(1,sizeof(SOutStreamFile));
	MakeSInStreamFile((SInStreamFile*)stream,self->argv[index]);
	printf("Compressing %s...\n",self->argv[index]);
	*inStream = stream; // WILL BE RELEASED AUTOMATICALLY.
	return S_OK;
}

static HRESULT WINAPI SArchiveUpdateCallbackFileList_GetProperty(void* _self, u32 index, PROPID propID, PROPVARIANT *value){
	SArchiveUpdateCallbackFileList *self = (SArchiveUpdateCallbackFileList*)_self;
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
		value->ulVal = st.st_size;
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

static int add(const char *password,unsigned char arctype,int level,const char *arc, int argc, const char **argv){
	// todo implement password
	if(lzmaOpen7z()){
		fprintf(stderr,"cannot load 7z.so.\n");
		return -1;
	}
	void *archiver=NULL;
	if(lzmaCreateArchiver(&archiver,arctype,1,level)){
		fprintf(stderr,"7z.so does not have the encoder for this file type.\n");
		lzmaClose7z();
		return 1;
	}
	SOutStreamFile sout;
	MakeSOutStreamFile(&sout,arc,false); //true); /// todo implement GetUpdateItemInfo properly
	SArchiveUpdateCallbackFileList supdate;
	MakeSArchiveUpdateCallbackBare((SArchiveUpdateCallbackBare*)&supdate,(IOutArchive_*)archiver,password&&*password?password:NULL);
	supdate.argc = argc;
	supdate.argv = argv;
	supdate.vt->GetStream = SArchiveUpdateCallbackFileList_GetStream;
	supdate.vt->GetProperty = SArchiveUpdateCallbackFileList_GetProperty;
	int x=lzmaUpdateArchive(archiver,&sout,argc,&supdate);
	//printf("finished %llx.\n",x);
	lzmaDestroyArchiver(&archiver,1);
	sout.vt->Release(&sout);
	lzmaClose7z();

	return 0;
}

#ifdef STANDALONE
unsigned char buf[BUFLEN];
int main(int argc, const char **argv){
#else
int mini7z(int argc, const char **argv){
#endif
  printf(
  	"7z Extractor\n"
  	"Usage:\n"
  	"mini7z [xl][PASSWORD] arc.7z [extract_dir] [filespec]\n"
	"mini7z a[PASSWORD] TYPE LEVEL(-1) arc.7z [filespec] (cannot handle wildcard nor directories)\n"
	"(possibly) find filespec -type f | xargs mini7z a 7 9 arc.7z\n"
  	"\n"
  );
  if(argc<3)return -1;
  const char *w="*";
  
  switch(argv[1][0]){
	case 'x':{
		const char *password=argv[1]+1;
		const char *arc=argv[2];
		const char *dir="./";
		argv+=3;argc-=3;
		if(argv[0]&&(LastByte(argv[0])=='/'||LastByte(argv[0])=='\\')){makedir(argv[0]);dir=argv[0];argv++;argc--;}
		return extract(password,arc,dir,argc?argc:1,argc?argv:&w);
	}
	case 'l':return list(argv[1]+1,argv[2],argc-3?argc-3:1,argc-3?argv+3:&w);
	case 'a':{
		if(argc<5)return -1;
		return add(argv[1]+1,strtol(argv[2],NULL,0),strtol(argv[3],NULL,10),argv[4],argc-5,argv+5);
	}
	default:return -1;
  }
}
