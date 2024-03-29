// LZMA SDK C interface
// basic idea from:
// https://gist.github.com/harvimt/9461046
// https://github.com/harvimt/pylib7zip

// STDMETHOD is virtual __stdcall (or __cdecl), so it can be called from C if the interface is properly aligned.

#include "lzma.h"
#include "scv.h"
#include <string.h>
#include <stdlib.h> // getenv
#include <dirent.h>

#ifndef wchar_t
#include <wchar.h>
#endif

#ifdef __cplusplus
extern "C"{
#define EXTERN extern
#else
#define EXTERN
#endif

unsigned int read32(const void *p){
	const unsigned char *x=(const unsigned char*)p;
	return x[0]|(x[1]<<8)|(x[2]<<16)|((unsigned int)x[3]<<24);
}

void write32(void *p, const unsigned int n){
	unsigned char *x=(unsigned char*)p;
	x[0]=n&0xff,x[1]=(n>>8)&0xff,x[2]=(n>>16)&0xff,x[3]=(n>>24)&0xff;
}

EXTERN const IID IID_IUnknown_={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

EXTERN const IID IID_ISequentialInStream_={0x23170F69,0x40C1,0x278A,{0x00,0x00,0x00,0x03,0x00,0x01,0x00,0x00}};
EXTERN const IID IID_ISequentialOutStream_={0x23170F69,0x40C1,0x278A,{0x00,0x00,0x00,0x03,0x00,0x02,0x00,0x00}};
EXTERN const IID IID_IInStream_={0x23170F69,0x40C1,0x278A,{0x00,0x00,0x00,0x03,0x00,0x03,0x00,0x00}};
EXTERN const IID IID_IOutStream_={0x23170F69,0x40C1,0x278A,{0x00,0x00,0x00,0x03,0x00,0x04,0x00,0x00}};

EXTERN const IID IID_ICompressCoder_={0x23170F69,0x40C1,0x278A,{0x00,0x00,0x00,0x04,0x00,0x05,0x00,0x00}};
EXTERN const IID IID_ICompressSetCoderProperties_={0x23170F69,0x40C1,0x278A,{0x00,0x00,0x00,0x04,0x00,0x20,0x00,0x00}};
EXTERN const IID IID_ICompressCodecsInfo_={0x23170F69,0x40C1,0x278A,{0x00,0x00,0x00,0x04,0x00,0x60,0x00,0x00}};

EXTERN const IID IID_ICryptoGetTextPassword_={0x23170F69,0x40C1,0x278A,{0x00,0x00,0x00,0x05,0x00,0x10,0x00,0x00}};
EXTERN const IID IID_ICryptoGetTextPassword2_={0x23170F69,0x40C1,0x278A,{0x00,0x00,0x00,0x05,0x00,0x11,0x00,0x00}};

EXTERN const IID IID_IArchiveExtractCallback_={0x23170F69,0x40C1,0x278A,{0x00,0x00,0x00,0x06,0x00,0x20,0x00,0x00}};
EXTERN const IID IID_IArchiveOpenVolumeCallback_={0x23170F69,0x40C1,0x278A,{0x00,0x00,0x00,0x06,0x00,0x30,0x00,0x00}};
EXTERN const IID IID_IInArchive_={0x23170F69,0x40C1,0x278A,{0x00,0x00,0x00,0x06,0x00,0x60,0x00,0x00}};
EXTERN const IID IID_IArchiveUpdateCallback_={0x23170F69,0x40C1,0x278A,{0x00,0x00,0x00,0x06,0x00,0x80,0x00,0x00}};
EXTERN const IID IID_IOutArchive_={0x23170F69,0x40C1,0x278A,{0x00,0x00,0x00,0x06,0x00,0xA0,0x00,0x00}};
EXTERN const IID IID_ISetProperties_={0x23170F69,0x40C1,0x278A,{0x00,0x00,0x00,0x06,0x00,0x03,0x00,0x00}};

static HRESULT WINAPI SInStreamFile_QueryInterface(void* _self, const GUID* iid, void** out_obj){
	LZMA_UNUSED SInStreamFile* self = (SInStreamFile*)_self;
	if(!memcmp(iid,&IID_IInStream_,sizeof(GUID))){
		*out_obj = self;
		self->vt->AddRef(self);
		return S_OK;
	}
	*out_obj = NULL;
	return E_NOINTERFACE;
}

static u32 WINAPI SInStreamFile_AddRef(void* _self){
	LZMA_UNUSED SInStreamFile* self = (SInStreamFile*)_self;
	return ++self->refs;
}

static u32 WINAPI SInStreamFile_Release(void* _self){
	LZMA_UNUSED SInStreamFile* self = (SInStreamFile*)_self;
	if(--self->refs==0){
		free(self->vt);
		self->vt=NULL;
		if(self->f)fclose(self->f);
		self->f=NULL;
	}
	return self->refs;
}

static HRESULT WINAPI SInStreamFile_Read(void* _self, void *data, u32 size, u32 *processedSize){
	LZMA_UNUSED SInStreamFile* self = (SInStreamFile*)_self;
	u32 readlen = fread(data, 1, size, self->f);
	if(processedSize)*processedSize = readlen;
	return S_OK;
}

static HRESULT WINAPI SInStreamFile_Seek(void* _self, s64 offset, u32 seekOrigin, u64 *newPosition){
	LZMA_UNUSED SInStreamFile* self = (SInStreamFile*)_self;
	fseeko(self->f, offset, seekOrigin);
	if(newPosition)*newPosition = ftello(self->f);
	return S_OK;
}

bool WINAPI MakeSInStreamFile(SInStreamFile *self, const char *fname){
	self->f = fopen(fname,"rb");
	if(!self->f)return false;
	self->vt = (IInStream_vt*)calloc(1,sizeof(IInStream_vt));
	if(!self->vt){
		if(self->f)fclose(self->f);
		self->f=NULL;
		return false;
	}
	self->vt->QueryInterface = SInStreamFile_QueryInterface;
	self->vt->AddRef = SInStreamFile_AddRef;
	self->vt->Release = SInStreamFile_Release;
	self->vt->Read = SInStreamFile_Read;
	self->vt->Seek = SInStreamFile_Seek;
	self->refs = 1;
	return true;
}

static HRESULT WINAPI SInStreamMem_QueryInterface(void* _self, const GUID* iid, void** out_obj){
	LZMA_UNUSED SInStreamMem* self = (SInStreamMem*)_self;
	if(!memcmp(iid,&IID_IInStream_,sizeof(GUID))){
		*out_obj = self;
		self->vt->AddRef(self);
		return S_OK;
	}
	*out_obj = NULL;
	return E_NOINTERFACE;
}

static u32 WINAPI SInStreamMem_AddRef(void* _self){
	LZMA_UNUSED SInStreamMem* self = (SInStreamMem*)_self;
	return ++self->refs;
}

static u32 WINAPI SInStreamMem_Release(void* _self){
	LZMA_UNUSED SInStreamMem* self = (SInStreamMem*)_self;
	if(--self->refs==0){
		free(self->vt);
		self->vt=NULL;
		mclose(self->m);
		self->m=NULL;
	}
	return self->refs;
}

static HRESULT WINAPI SInStreamMem_Read(void* _self, void *data, u32 size, u32 *processedSize){
	LZMA_UNUSED SInStreamMem* self = (SInStreamMem*)_self;
	u32 readlen = mread(data, size, self->m);
	if(processedSize)*processedSize = readlen;
	return S_OK;
}

static HRESULT WINAPI SInStreamMem_Seek(void* _self, s64 offset, u32 seekOrigin, u64 *newPosition){
	LZMA_UNUSED SInStreamMem* self = (SInStreamMem*)_self;
	u64 pos = mseek(self->m, offset, seekOrigin);
	if(newPosition)*newPosition = pos;
	return S_OK;
}

bool MakeSInStreamMem(SInStreamMem *self, void *p, const unsigned int size){
	self->m = mopen(p,size,NULL);
	if(!self->m)return false;
	self->vt = (IInStream_vt*)calloc(1,sizeof(IInStream_vt));
	if(!self->vt){
		mclose(self->m);
		self->m=NULL;
		return false;
	}
	self->vt->QueryInterface = SInStreamMem_QueryInterface;
	self->vt->AddRef = SInStreamMem_AddRef;
	self->vt->Release = SInStreamMem_Release;
	self->vt->Read = SInStreamMem_Read;
	self->vt->Seek = SInStreamMem_Seek;
	self->refs = 1;
	return true;
}

static HRESULT WINAPI SInStreamGeneric_QueryInterface(void* _self, const GUID* iid, void** out_obj){
	LZMA_UNUSED SInStreamGeneric* self = (SInStreamGeneric*)_self;
	if(!memcmp(iid,&IID_IInStream_,sizeof(GUID))){
		*out_obj = self;
		self->vt->AddRef(self);
		return S_OK;
	}
	*out_obj = NULL;
	return E_NOINTERFACE;
}

static u32 WINAPI SInStreamGeneric_AddRef(void* _self){
	LZMA_UNUSED SInStreamGeneric* self = (SInStreamGeneric*)_self;
	return ++self->refs;
}

static u32 WINAPI SInStreamGeneric_Release(void* _self){
	LZMA_UNUSED SInStreamGeneric* self = (SInStreamGeneric*)_self;
	if(--self->refs==0){
		free(self->vt);
		self->vt=NULL;
		if(self->pClose)self->pClose(self->h);
		self->h=NULL;
	}
	return self->refs;
}

static HRESULT WINAPI SInStreamGeneric_Read(void* _self, void *data, u32 size, u32 *processedSize){
	LZMA_UNUSED SInStreamGeneric* self = (SInStreamGeneric*)_self;
	if(!self->h||!self->pRead)return E_FAIL;
	int readlen=self->pRead(self->h,(char*)data,size);
	if(processedSize)*processedSize=readlen;
	return S_OK;
}

static HRESULT WINAPI SInStreamGeneric_Seek(void* _self, s64 offset, u32 seekOrigin, u64 *newPosition){
	LZMA_UNUSED SInStreamGeneric* self = (SInStreamGeneric*)_self;
	if(!self->h||!self->pSeek)return E_FAIL;
	if(self->pSeek(self->h,offset,seekOrigin))return E_FAIL;//errno;
	if(newPosition){
		if(!self->pTell)return E_FAIL;
		*newPosition=self->pTell(self->h);
	}
	return S_OK;
}

bool MakeSInStreamGeneric(SInStreamGeneric *self, void *h, tRead pRead, tClose pClose, tSeek pSeek, tTell pTell){
	self->h = h;
	if(!self->h)return false;
	self->pRead = pRead;
	self->pSeek = pSeek;
	self->pClose = pClose;
	self->pTell = pTell;
	self->vt = (IInStream_vt*)calloc(1,sizeof(IInStream_vt));
	if(!self->vt)return false;
	self->vt->QueryInterface = SInStreamGeneric_QueryInterface;
	self->vt->AddRef = SInStreamGeneric_AddRef;
	self->vt->Release = SInStreamGeneric_Release;
	self->vt->Read = SInStreamGeneric_Read;
	self->vt->Seek = SInStreamGeneric_Seek;
	self->refs = 1;
	return true;
}

static HRESULT WINAPI SOutStreamFile_QueryInterface(void* _self, const GUID* iid, void** out_obj){
	LZMA_UNUSED SOutStreamFile* self = (SOutStreamFile*)_self;
	if(!memcmp(iid,&IID_IOutStream_,sizeof(GUID))){
		*out_obj = self;
		self->vt->AddRef(self);
		return S_OK;
	}
	*out_obj = NULL;
	return E_NOINTERFACE;
}

static u32 WINAPI SOutStreamFile_AddRef(void* _self){
	LZMA_UNUSED SOutStreamFile* self = (SOutStreamFile*)_self;
	return ++self->refs;
}

static u32 WINAPI SOutStreamFile_Release(void* _self){
	LZMA_UNUSED SOutStreamFile* self = (SOutStreamFile*)_self;
	if(--self->refs==0){
		free(self->vt);
		self->vt=NULL;
		if(self->f)fclose(self->f);
		self->f=NULL;
	}
	return self->refs;
}

static HRESULT WINAPI SOutStreamFile_Write(void* _self, const void *data, u32 size, u32 *processedSize){
	LZMA_UNUSED SOutStreamFile* self = (SOutStreamFile*)_self;
	u32 writelen = fwrite(data, 1, size, self->f);
	if(processedSize)*processedSize = writelen;
	if(size&&!writelen)return E_FAIL;
	return S_OK;
}

static HRESULT WINAPI SOutStreamFile_Seek(void* _self, s64 offset, u32 seekOrigin, u64 *newPosition){
	LZMA_UNUSED SOutStreamFile* self = (SOutStreamFile*)_self;
	fseeko(self->f, offset, seekOrigin);
	if(newPosition)*newPosition = ftello(self->f);
	return S_OK;
}

static HRESULT WINAPI SOutStreamFile_SetSize(void* _self, u64 newSize){
	LZMA_UNUSED SOutStreamFile* self = (SOutStreamFile*)_self;
	ftruncate(fileno(self->f),newSize);
	return S_OK;
}

bool MakeSOutStreamFile(SOutStreamFile *self, const char *fname, bool readable){
	self->f = NULL;
	if(readable){
		self->f = fopen(fname,"r+b");
	}
	if(!self->f){
		self->f = fopen(fname,"wb");
	}
	if(!self->f)return false;
	self->vt = (IOutStream_vt*)calloc(1,sizeof(IOutStream_vt));
	if(!self->vt){
		if(self->f)fclose(self->f);
		self->f=NULL;
		return false;
	}
	self->vt->QueryInterface = SOutStreamFile_QueryInterface;
	self->vt->AddRef = SOutStreamFile_AddRef;
	self->vt->Release = SOutStreamFile_Release;
	self->vt->Write = SOutStreamFile_Write;
	self->vt->Seek = SOutStreamFile_Seek;
	self->vt->SetSize = SOutStreamFile_SetSize;
	self->refs = 1;
	return true;
}

static HRESULT WINAPI SSequentialOutStreamMem_QueryInterface(void* _self, const GUID* iid, void** out_obj){
	LZMA_UNUSED SSequentialOutStreamMem* self = (SSequentialOutStreamMem*)_self;
	*out_obj = NULL;
	return E_NOINTERFACE;
}

static u32 WINAPI SSequentialOutStreamMem_AddRef(void* _self){
	LZMA_UNUSED SSequentialOutStreamMem* self = (SSequentialOutStreamMem*)_self;
	return ++self->refs;
}

static u32 WINAPI SSequentialOutStreamMem_Release(void* _self){
	LZMA_UNUSED SSequentialOutStreamMem* self = (SSequentialOutStreamMem*)_self;
	if(--self->refs==0){
		free(self->vt);
		self->vt=NULL;
		mclose(self->m);
		self->m=NULL;
	}
	return self->refs;
}

static HRESULT WINAPI SSequentialOutStreamMem_Write(void* _self, const void *data, u32 size, u32 *processedSize){
	LZMA_UNUSED SSequentialOutStreamMem* self = (SSequentialOutStreamMem*)_self;
	u32 writelen = mwrite(data, size, self->m);
	if(processedSize)*processedSize = writelen;
	if(size&&!writelen)return E_FAIL;
	return S_OK;
}

static HRESULT WINAPI SSequentialOutStreamMem_Seek(void* _self, s64 offset, u32 seekOrigin, u64 *newPosition){
	LZMA_UNUSED SSequentialOutStreamMem* self = (SSequentialOutStreamMem*)_self;
	u64 pos = mseek(self->m, offset, seekOrigin);
	if(newPosition)*newPosition = pos;
	return S_OK;
}

static HRESULT WINAPI SSequentialOutStreamMem_SetSize(void* _self, u64 newSize){
	LZMA_UNUSED SSequentialOutStreamMem* self = (SSequentialOutStreamMem*)_self;
	return E_FAIL; // this is why declared as Sequential.
}

bool MakeSSequentialOutStreamMem(SSequentialOutStreamMem *self, void *p, const unsigned int size){
	self->m = mopen(p,size,NULL);
	if(!self->m)return false;
	self->vt = (IOutStream_vt*)calloc(1,sizeof(IOutStream_vt));
	if(!self->vt){
		mclose(self->m);
		self->m=NULL;
		return false;
	}
	self->vt->QueryInterface = SSequentialOutStreamMem_QueryInterface;
	self->vt->AddRef = SSequentialOutStreamMem_AddRef;
	self->vt->Release = SSequentialOutStreamMem_Release;
	self->vt->Write = SSequentialOutStreamMem_Write;
	self->vt->Seek = SSequentialOutStreamMem_Seek;
	self->vt->SetSize = SSequentialOutStreamMem_SetSize;
	self->refs = 1;
	return true;
}

static HRESULT WINAPI SSequentialOutStreamGeneric_QueryInterface(void* _self, const GUID* iid, void** out_obj){
	LZMA_UNUSED SSequentialOutStreamGeneric* self = (SSequentialOutStreamGeneric*)_self;
	*out_obj = NULL;
	return E_NOINTERFACE;
}

static u32 WINAPI SSequentialOutStreamGeneric_AddRef(void* _self){
	LZMA_UNUSED SSequentialOutStreamGeneric* self = (SSequentialOutStreamGeneric*)_self;
	return ++self->refs;
}

static u32 WINAPI SSequentialOutStreamGeneric_Release(void* _self){
	LZMA_UNUSED SSequentialOutStreamGeneric* self = (SSequentialOutStreamGeneric*)_self;
	if(--self->refs==0){
		free(self->vt);
		self->vt=NULL;
		if(self->pClose)self->pClose(self->h);
		self->h=NULL;
	}
	return self->refs;
}

static HRESULT WINAPI SSequentialOutStreamGeneric_Write(void* _self, const void *data, u32 size, u32 *processedSize){
	LZMA_UNUSED SSequentialOutStreamGeneric* self = (SSequentialOutStreamGeneric*)_self;
	if(!self->h||!self->pWrite)return E_FAIL;
	int writelen=self->pWrite(self->h,(char*)data,size);
	if(processedSize)*processedSize=writelen;
	if(size&&!writelen)return E_FAIL;
	return S_OK;
}

static HRESULT WINAPI SSequentialOutStreamGeneric_Seek(void* _self, s64 offset, u32 seekOrigin, u64 *newPosition){
	LZMA_UNUSED SSequentialOutStreamGeneric* self = (SSequentialOutStreamGeneric*)_self;
	return E_FAIL;
#if 0
	if(!self->h||!self->pSeek)return E_FAIL;
	if(self->pSeek(self->h,offset,seekOrigin))return E_FAIL;//errno;
	if(newPosition){
		if(!self->pTell)return E_FAIL;
		*newPosition=self->pTell(self->h);
	}
	return S_OK;
#endif
}

static HRESULT WINAPI SSequentialOutStreamGeneric_SetSize(void* _self, u64 newSize){
	LZMA_UNUSED SSequentialOutStreamGeneric* self = (SSequentialOutStreamGeneric*)_self;
	return E_FAIL;
}

bool MakeSSequentialOutStreamGeneric(SSequentialOutStreamGeneric *self, void *h, tWrite pWrite, tClose pClose){
	self->h = h;
	if(!self->h)return false;
	self->pWrite = pWrite;
	self->pClose = pClose;
	self->vt = (IOutStream_vt*)calloc(1,sizeof(IOutStream_vt));
	if(!self->vt)return false;
	self->vt->QueryInterface = SSequentialOutStreamGeneric_QueryInterface;
	self->vt->AddRef = SSequentialOutStreamGeneric_AddRef;
	self->vt->Release = SSequentialOutStreamGeneric_Release;
	self->vt->Write = SSequentialOutStreamGeneric_Write;
	self->vt->Seek = SSequentialOutStreamGeneric_Seek;
	self->vt->SetSize = SSequentialOutStreamGeneric_SetSize;
	self->refs = 1;
	return true;
}

static HRESULT WINAPI SCryptoGetTextPasswordFixed_QueryInterface(void* _self, const GUID* iid, void** out_obj){
	LZMA_UNUSED SCryptoGetTextPasswordFixed *self = (SCryptoGetTextPasswordFixed*)_self;
	*out_obj = NULL;
	return E_NOINTERFACE;
}

static u32 WINAPI SCryptoGetTextPasswordFixed_AddRef(void* _self){
	LZMA_UNUSED SCryptoGetTextPasswordFixed *self = (SCryptoGetTextPasswordFixed*)_self;
	return ++self->refs;
}

static u32 WINAPI SCryptoGetTextPasswordFixed_Release(void* _self){
	LZMA_UNUSED SCryptoGetTextPasswordFixed *self = (SCryptoGetTextPasswordFixed*)_self;
	if(--self->refs==0){
		free(self->vt);
		self->vt=NULL;
		free(self->password);
		self->password=NULL;
	}
	return self->refs;
}

static HRESULT WINAPI SCryptoGetTextPasswordFixed_CryptoGetTextPassword(void *_self, BSTR* password){
	LZMA_UNUSED SCryptoGetTextPasswordFixed *self = (SCryptoGetTextPasswordFixed*)_self;
	if(self->password){
		int passwordlen = strlen(self->password);
		*password = SysAllocStringLen(NULL,passwordlen+1);
		u8* passwordStartAddr = ((u8*)*password)-4;
		write32(passwordStartAddr, mbstowcs(*password, self->password, passwordlen+1));
	}
	return S_OK;
}

bool MakeSCryptoGetTextPasswordFixed(SCryptoGetTextPasswordFixed *self, const char *password){
	self->vt = (ICryptoGetTextPassword_vt*)calloc(1,sizeof(ICryptoGetTextPassword_vt));
	if(!self->vt)return false;
	self->password = NULL;
	if(password){
		self->password = (char*)malloc(strlen(password)+1);
		strcpy(self->password, password);
	}
	self->vt->QueryInterface = SCryptoGetTextPasswordFixed_QueryInterface;
	self->vt->AddRef = SCryptoGetTextPasswordFixed_AddRef;
	self->vt->Release = SCryptoGetTextPasswordFixed_Release;
	self->vt->CryptoGetTextPassword = SCryptoGetTextPasswordFixed_CryptoGetTextPassword;
	self->refs = 1;
	return true;
}

static HRESULT WINAPI SCryptoGetTextPassword2Fixed_QueryInterface(void* _self, const GUID* iid, void** out_obj){
	LZMA_UNUSED SCryptoGetTextPassword2Fixed *self = (SCryptoGetTextPassword2Fixed*)_self;
	*out_obj = NULL;
	return E_NOINTERFACE;
}

static u32 WINAPI SCryptoGetTextPassword2Fixed_AddRef(void* _self){
	LZMA_UNUSED SCryptoGetTextPassword2Fixed *self = (SCryptoGetTextPassword2Fixed*)_self;
	return ++self->refs;
}

static u32 WINAPI SCryptoGetTextPassword2Fixed_Release(void* _self){
	LZMA_UNUSED SCryptoGetTextPassword2Fixed *self = (SCryptoGetTextPassword2Fixed*)_self;
	if(--self->refs==0){
		free(self->vt);
		self->vt=NULL;
		free(self->password);
		self->password=NULL;
	}
	return self->refs;
}

static HRESULT WINAPI SCryptoGetTextPassword2Fixed_CryptoGetTextPassword2(void *_self, s32 *passwordIsDefined, BSTR* password){
	LZMA_UNUSED SCryptoGetTextPassword2Fixed *self = (SCryptoGetTextPassword2Fixed*)_self;
	*passwordIsDefined = 0;
	if(self->password){
		*passwordIsDefined = 1;
		int passwordlen = strlen(self->password);
		*password = SysAllocStringLen(NULL,passwordlen+1);
		u8* passwordStartAddr = ((u8*)*password)-4;
		write32(passwordStartAddr, mbstowcs(*password, self->password, passwordlen+1));
	}
	return S_OK;
}

bool MakeSCryptoGetTextPassword2Fixed(SCryptoGetTextPassword2Fixed *self, const char *password){
	self->vt = (ICryptoGetTextPassword2_vt*)calloc(1,sizeof(ICryptoGetTextPassword2_vt));
	if(!self->vt)return false;
	self->password = NULL;
	if(password){
		self->password = (char*)malloc(strlen(password)+1);
		strcpy(self->password, password);
	}
	self->vt->QueryInterface = SCryptoGetTextPassword2Fixed_QueryInterface;
	self->vt->AddRef = SCryptoGetTextPassword2Fixed_AddRef;
	self->vt->Release = SCryptoGetTextPassword2Fixed_Release;
	self->vt->CryptoGetTextPassword2 = SCryptoGetTextPassword2Fixed_CryptoGetTextPassword2;
	self->refs = 1;
	return true;
}

static HRESULT WINAPI SArchiveOpenVolumeCallback_QueryInterface(void* _self, const GUID* iid, void** out_obj){
	LZMA_UNUSED SArchiveOpenVolumeCallback *self = (SArchiveOpenVolumeCallback*)_self;
	*out_obj = NULL;
	return E_NOINTERFACE;
}

static u32 WINAPI SArchiveOpenVolumeCallback_AddRef(void* _self){
	LZMA_UNUSED SArchiveOpenVolumeCallback *self = (SArchiveOpenVolumeCallback*)_self;
	return ++self->refs;
}

static u32 WINAPI SArchiveOpenVolumeCallback_Release(void* _self){
	LZMA_UNUSED SArchiveOpenVolumeCallback *self = (SArchiveOpenVolumeCallback*)_self;
	if(--self->refs==0){
		free(self->vt);
		self->vt=NULL;
		free(self->fname);
		self->fname=NULL;
	}
	return self->refs;
}

static HRESULT WINAPI SArchiveOpenVolumeCallback_GetProperty(void* _self, PROPID propID, PROPVARIANT *value){
	LZMA_UNUSED SArchiveOpenVolumeCallback *self = (SArchiveOpenVolumeCallback*)_self;
	if(propID == kpidName || propID==kpidPath){
		if(self->fname){
			value->vt = VT_BSTR;
			int fnamelen = strlen(self->fname);
			value->bstrVal = SysAllocStringLen(NULL,fnamelen+1);
			u8* startAddr = ((u8*)value->bstrVal)-4;
			write32(startAddr, mbstowcs(value->bstrVal, self->fname, fnamelen+1));
		}
	}
	return S_OK;
}

static HRESULT WINAPI SArchiveOpenVolumeCallback_GetStream(void* _self, const wchar_t *name, IInStream_ **inStream){
	LZMA_UNUSED SArchiveOpenVolumeCallback *self = (SArchiveOpenVolumeCallback*)_self;
	IInStream_* stream = (IInStream_*)calloc(1,sizeof(SInStreamFile));
	size_t mbnamelen = wcslen(name)*4;
	char *mbname = (char*)malloc(mbnamelen);
	if(!mbname)return E_FAIL;
	wcstombs(mbname,name,mbnamelen);
	if(!MakeSInStreamFile((SInStreamFile*)stream,mbname)){
		free(mbname);
		stream->vt->Release(&stream);
		return E_FAIL;
	}
	*inStream = stream; // WILL BE RELEASED AUTOMATICALLY.
	free(mbname);
	return S_OK;
}

bool MakeSArchiveOpenVolumeCallback(SArchiveOpenVolumeCallback *self, const char *fname){
	self->vt = (IArchiveOpenVolumeCallback_vt*)calloc(1,sizeof(IArchiveOpenVolumeCallback_vt));
	if(!self->vt)return false;
	self->fname = NULL;
	if(fname){
		self->fname = (char*)malloc(strlen(fname)+1);
		strcpy(self->fname, fname);
	}
	self->vt->QueryInterface = SArchiveOpenVolumeCallback_QueryInterface;
	self->vt->AddRef = SArchiveOpenVolumeCallback_AddRef;
	self->vt->Release = SArchiveOpenVolumeCallback_Release;
	self->vt->GetProperty = SArchiveOpenVolumeCallback_GetProperty;
	self->vt->GetStream = SArchiveOpenVolumeCallback_GetStream;
	self->refs = 1;
	return true;
}

static HRESULT WINAPI SArchiveOpenCallbackPassword_QueryInterface(void* _self, const GUID* iid, void** out_obj){
	LZMA_UNUSED SArchiveOpenCallbackPassword *self = (SArchiveOpenCallbackPassword*)_self;
	if(!memcmp(iid,&IID_ICryptoGetTextPassword_,sizeof(GUID))){
		*out_obj = &self->setpassword;
		self->setpassword.vt->AddRef(&self->setpassword);
		return S_OK;
	}
	if(!memcmp(iid,&IID_IArchiveOpenVolumeCallback_,sizeof(GUID))){
		*out_obj = &self->openvolume;
		self->openvolume.vt->AddRef(&self->openvolume);
		return S_OK;
	}
	*out_obj = NULL;
	return E_NOINTERFACE;
}

static u32 WINAPI SArchiveOpenCallbackPassword_AddRef(void* _self){
	LZMA_UNUSED SArchiveOpenCallbackPassword *self = (SArchiveOpenCallbackPassword*)_self;
	return ++self->refs;
}

static u32 WINAPI SArchiveOpenCallbackPassword_Release(void* _self){
	LZMA_UNUSED SArchiveOpenCallbackPassword *self = (SArchiveOpenCallbackPassword*)_self;
	if(--self->refs==0){
		free(self->vt);
		self->vt=NULL;
		self->setpassword.vt->Release(&self->setpassword);
		self->openvolume.vt->Release(&self->openvolume);
	}
	return self->refs;
}

static HRESULT WINAPI SArchiveOpenCallbackPassword_SetTotal(void* _self, const u64 *files, const u64 *bytes){
	LZMA_UNUSED SArchiveOpenCallbackPassword *self = (SArchiveOpenCallbackPassword*)_self;
	return S_OK;
}

static HRESULT WINAPI SArchiveOpenCallbackPassword_SetCompleted(void* _self, const u64 *files, const u64 *bytes){
	LZMA_UNUSED SArchiveOpenCallbackPassword *self = (SArchiveOpenCallbackPassword*)_self;
	return S_OK;
}

bool MakeSArchiveOpenCallbackPassword(SArchiveOpenCallbackPassword *self, const char *password, const char *fname){
	self->vt = (IArchiveOpenCallback_vt*)calloc(1,sizeof(IArchiveOpenCallback_vt));
	if(!self->vt)return false;
	MakeSCryptoGetTextPasswordFixed(&self->setpassword,password);
	MakeSArchiveOpenVolumeCallback(&self->openvolume,fname);
	self->vt->QueryInterface = SArchiveOpenCallbackPassword_QueryInterface;
	self->vt->AddRef = SArchiveOpenCallbackPassword_AddRef;
	self->vt->Release = SArchiveOpenCallbackPassword_Release;
	self->vt->SetTotal = SArchiveOpenCallbackPassword_SetTotal;
	self->vt->SetCompleted = SArchiveOpenCallbackPassword_SetCompleted;
	self->refs = 1;
	return true;
}

static HRESULT WINAPI SArchiveExtractCallbackBare_QueryInterface(void* _self, const GUID* iid, void** out_obj){
	LZMA_UNUSED SArchiveExtractCallbackBare *self = (SArchiveExtractCallbackBare*)_self;
	if(!memcmp(iid,&IID_ICryptoGetTextPassword_,sizeof(GUID))){
		*out_obj = &self->setpassword;
		self->setpassword.vt->AddRef(&self->setpassword);
		return S_OK;
	}
	*out_obj = NULL;
	return E_NOINTERFACE;
}

static u32 WINAPI SArchiveExtractCallbackBare_AddRef(void* _self){
	LZMA_UNUSED SArchiveExtractCallbackBare *self = (SArchiveExtractCallbackBare*)_self;
	return ++self->refs;
}

static u32 WINAPI SArchiveExtractCallbackBare_Release(void* _self){
	LZMA_UNUSED SArchiveExtractCallbackBare *self = (SArchiveExtractCallbackBare*)_self;
	if(--self->refs==0){
		free(self->vt);
		self->vt=NULL;
	}
	return self->refs;
}

static HRESULT WINAPI SArchiveExtractCallbackBare_SetTotal(void* _self, const u64 total){
	LZMA_UNUSED SArchiveExtractCallbackBare *self = (SArchiveExtractCallbackBare*)_self;
	return S_OK;
}

static HRESULT WINAPI SArchiveExtractCallbackBare_SetCompleted(void* _self, const u64 *completedValue){
	LZMA_UNUSED SArchiveExtractCallbackBare *self = (SArchiveExtractCallbackBare*)_self;
	return S_OK;
}

static HRESULT WINAPI SArchiveExtractCallbackBare_GetStream(void* _self, u32 index, /*ISequentialOutStream_*/IOutStream_ **outStream, s32 askExtractMode){
	LZMA_UNUSED SArchiveExtractCallbackBare *self = (SArchiveExtractCallbackBare*)_self;
	*outStream = NULL;
	PROPVARIANT path;
	memset(&path,0,sizeof(PROPVARIANT));
	self->lastIndex = index;
	lzmaGetArchiveFileProperty(self->archiver, index, kpidPath, &path);
	//printf("%d\t%ls\n",index,path.bstrVal);
	IOutStream_* stream = (IOutStream_*)calloc(1,sizeof(SOutStreamFile));
	MakeSOutStreamFile((SOutStreamFile*)stream,"/dev/null",false);
	*outStream = stream; // WILL BE RELEASED AUTOMATICALLY.
	PropVariantClear(&path);
	return S_OK;
}

static HRESULT WINAPI SArchiveExtractCallbackBare_PrepareOperation(void* _self, s32 askExtractMode){
	LZMA_UNUSED SArchiveExtractCallbackBare *self = (SArchiveExtractCallbackBare*)_self;
	return S_OK;
}

static HRESULT WINAPI SArchiveExtractCallbackBare_SetOperationResult(void* _self, s32 opRes){
	LZMA_UNUSED SArchiveExtractCallbackBare *self = (SArchiveExtractCallbackBare*)_self;
	return S_OK;
}

bool MakeSArchiveExtractCallbackBare(SArchiveExtractCallbackBare *self, IInArchive_ *archiver, const char *password){
	self->vt = (IArchiveExtractCallback_vt*)calloc(1,sizeof(IArchiveExtractCallback_vt));
	if(!self->vt)return false;
	MakeSCryptoGetTextPasswordFixed(&self->setpassword,password);
	self->archiver = archiver;
	self->lastIndex = -1;
	self->vt->QueryInterface = SArchiveExtractCallbackBare_QueryInterface;
	self->vt->AddRef = SArchiveExtractCallbackBare_AddRef;
	self->vt->Release = SArchiveExtractCallbackBare_Release;
	self->vt->SetTotal = SArchiveExtractCallbackBare_SetTotal;
	self->vt->SetCompleted = SArchiveExtractCallbackBare_SetCompleted;
	self->vt->GetStream = SArchiveExtractCallbackBare_GetStream;
	self->vt->PrepareOperation = SArchiveExtractCallbackBare_PrepareOperation;
	self->vt->SetOperationResult = SArchiveExtractCallbackBare_SetOperationResult;
	self->refs = 1;
	return true;
}

static HRESULT WINAPI SArchiveUpdateCallbackBare_QueryInterface(void* _self, const GUID* iid, void** out_obj){
	LZMA_UNUSED SArchiveUpdateCallbackBare *self = (SArchiveUpdateCallbackBare*)_self;
	if(!memcmp(iid,&IID_ICryptoGetTextPassword2_,sizeof(GUID))){
		*out_obj = &self->setpassword;
		self->setpassword.vt->AddRef(&self->setpassword);
		return S_OK;
	}
	*out_obj = NULL;
	return E_NOINTERFACE;
}

static u32 WINAPI SArchiveUpdateCallbackBare_AddRef(void* _self){
	LZMA_UNUSED SArchiveUpdateCallbackBare *self = (SArchiveUpdateCallbackBare*)_self;
	return ++self->refs;
}

static u32 WINAPI SArchiveUpdateCallbackBare_Release(void* _self){
	LZMA_UNUSED SArchiveUpdateCallbackBare *self = (SArchiveUpdateCallbackBare*)_self;
	if(--self->refs==0){
		free(self->vt);
		self->vt=NULL;
	}
	return self->refs;
}

static HRESULT WINAPI SArchiveUpdateCallbackBare_SetTotal(void* _self, const u64 total){
	LZMA_UNUSED SArchiveExtractCallbackBare *self = (SArchiveExtractCallbackBare*)_self;
	return S_OK;
}

static HRESULT WINAPI SArchiveUpdateCallbackBare_SetCompleted(void* _self, const u64 *completedValue){
	LZMA_UNUSED SArchiveUpdateCallbackBare *self = (SArchiveUpdateCallbackBare*)_self;
	return S_OK;
}

static HRESULT WINAPI SArchiveUpdateCallbackBare_GetUpdateItemInfo(void* _self, u32 index, s32 *newData, s32 *newProps, u32 *indexInArchive){
	LZMA_UNUSED SArchiveUpdateCallbackBare *self = (SArchiveUpdateCallbackBare*)_self;
	*newData = 1;
	*newProps = 1;
	*indexInArchive = -1;
	return S_OK;
}

static HRESULT WINAPI SArchiveUpdateCallbackBare_GetProperty(void* _self, u32 index, PROPID propID, PROPVARIANT *value){
	LZMA_UNUSED SArchiveUpdateCallbackBare *self = (SArchiveUpdateCallbackBare*)_self;
	//printf("%d %d\n",index,propID);
	if(propID == kpidPath){
		value->vt = VT_BSTR;
		value->bstrVal = SysAllocStringLen(NULL,1);
		wcscpy(value->bstrVal,L"$");
	}else if(propID == kpidIsDir || propID == kpidIsAnti){
		value->vt = VT_BOOL;
		value->boolVal = VARIANT_FALSE;
	}else if(propID == kpidSize){
		value->vt = VT_UI8;
		value->ulVal = 1; ///
	}else if(propID == kpidMTime){
		value->vt = VT_FILETIME;
	}else if(propID == kpidAttrib){
		value->vt = VT_UI4;
	}
	return S_OK;
}

static HRESULT WINAPI SArchiveUpdateCallbackBare_GetStream(void* _self, u32 index, /*ISequentialInStream_*/IInStream_ **inStream){
	LZMA_UNUSED SArchiveExtractCallbackBare *self = (SArchiveExtractCallbackBare*)_self;
	*inStream = NULL;
	//PROPVARIANT path;
	//memset(&path,0,sizeof(PROPVARIANT));
	self->lastIndex = index;
	IInStream_* stream = (IInStream_*)calloc(1,sizeof(SInStreamFile));
	MakeSInStreamFile((SInStreamFile*)stream,"/dev/null");
	*inStream = stream; // WILL BE RELEASED AUTOMATICALLY.
	//PropVariantClear(&path);
	return S_OK;
}

static HRESULT WINAPI SArchiveUpdateCallbackBare_SetOperationResult(void* _self, s32 opRes){
	LZMA_UNUSED SArchiveUpdateCallbackBare *self = (SArchiveUpdateCallbackBare*)_self;
	return S_OK;
}

bool MakeSArchiveUpdateCallbackBare(SArchiveUpdateCallbackBare *self, IOutArchive_ *archiver, const char *password){
	self->vt = (IArchiveUpdateCallback_vt*)calloc(1,sizeof(IArchiveUpdateCallback_vt));
	if(!self->vt)return false;
	MakeSCryptoGetTextPassword2Fixed(&self->setpassword,password);
	self->archiver = archiver;
	self->lastIndex = -1;
	self->vt->QueryInterface = SArchiveUpdateCallbackBare_QueryInterface;
	self->vt->AddRef = SArchiveUpdateCallbackBare_AddRef;
	self->vt->Release = SArchiveUpdateCallbackBare_Release;
	self->vt->SetTotal = SArchiveUpdateCallbackBare_SetTotal;
	self->vt->SetCompleted = SArchiveUpdateCallbackBare_SetCompleted;
	self->vt->GetUpdateItemInfo = SArchiveUpdateCallbackBare_GetUpdateItemInfo;
	self->vt->GetProperty = SArchiveUpdateCallbackBare_GetProperty;
	self->vt->GetStream = SArchiveUpdateCallbackBare_GetStream;
	self->vt->SetOperationResult = SArchiveUpdateCallbackBare_SetOperationResult;
	self->refs = 1;
	return true;
}

//Loading 7z.so
typedef HRESULT (WINAPI*funcCreateObject)(const GUID*,const GUID*,void**);
static funcCreateObject pCreateArchiver,pCreateCoder;
typedef HRESULT (WINAPI*funcSetCodecs)(ICompressCodecsInfo_ *compressCodecsInfo);
static funcSetCodecs pSetCodecs;
typedef HRESULT (WINAPI*funcGetNumMethods)(u32 *numMethods);
static funcGetNumMethods pGetNumMethods;
typedef HRESULT (WINAPI*funcGetNumFormats)(u32 *numFormats);
static funcGetNumFormats pGetNumFormats;
typedef HRESULT (WINAPI*funcGetProperty)(u32 index, PROPID propID, PROPVARIANT *value);
static funcGetProperty pGetMethodProperty;
static funcGetProperty pGetHandlerProperty2;
typedef HRESULT (WINAPI*funcCreateEncoder)(u32 index, const GUID *iid, void **coder);
typedef HRESULT (WINAPI*funcCreateDecoder)(u32 index, const GUID *iid, void **coder);

#if defined(_WIN32) || (!defined(__GNUC__) && !defined(__clang__))
	static HMODULE h7z=NULL;
#else
	static void *h7z=NULL;
	#include <pwd.h>
	#include <unistd.h>
#endif

typedef struct{
	ICompressCodecsInfo_vt *vt;
	u32 refs;

	struct scv_vector *vCodecs;
	struct scv_vector *vNumMethods;
	struct scv_vector *vGetMethodProperty;
	struct scv_vector *vCreateDecoder;
	struct scv_vector *vCreateEncoder;
} SCompressCodecsInfoExternal_;
static SCompressCodecsInfoExternal_ scoder = {NULL, 0, NULL, NULL, NULL, NULL, NULL};
static HRESULT WINAPI SCompressCodecsInfoExternal_GetNumMethods(void* _self, u32 *numMethods);
static HRESULT WINAPI SCompressCodecsInfoExternal_GetProperty(void* _self, u32 index, PROPID propID, PROPVARIANT *value);

int lzmaOpen7z(){
	if(lzma7zAlive())return 0;

	h7z=NULL;
#ifndef NODLOPEN
	//getenv is useful but it can cause security matter
	//if(!h7z && getenv("MINI7Z_IMPL"))h7z=LoadLibraryA(getenv("MINI7Z_IMPL"));
#if defined(_WIN32) || (!defined(__GNUC__) && !defined(__clang__))
	if(!h7z)h7z=LoadLibraryA("C:\\Program Files\\7-Zip\\7z.dll");
	if(!h7z)h7z=LoadLibraryA("C:\\Program Files (x86)\\7-Zip\\7z.dll");
	if(!h7z)h7z=LoadLibraryA("7z.dll"); // last resort using PATH
	if(!h7z)h7z=LoadLibraryA("7z64.dll"); // lol... (for example, could be useful for Wine testing)
	if(!h7z)h7z=LoadLibraryA("7z32.dll"); // lol...
#else
	// 7-zip-full
	if(!h7z)h7z=LoadLibraryA("/usr/local/opt/7-zip-full/lib/7-zip-full/7z.so"); //Homebrew
	if(!h7z)h7z=LoadLibraryA("/opt/homebrew/opt/7-zip-full/lib/7-zip-full/7z.so");
	if(!h7z)h7z=LoadLibraryA("/home/linuxbrew/.linuxbrew/lib/7-zip-full/7z.so");
	// if(!h7z)h7z=LoadLibraryA("/opt/local/lib/7-zip-full/7z.so"); //MacPorts
	// if(!h7z)h7z=LoadLibraryA("/sw/lib/7-zip-full/7z.so"); //Fink
	if(!h7z){
#if !defined(FEOS) && !defined(__ANDROID__)
		size_t buflen = sysconf(_SC_GETPW_R_SIZE_MAX)+128;
		char *buf = malloc(buflen);
		struct passwd pwd, *r=NULL;
		getpwuid_r(getuid(), &pwd, buf, buflen, &r);
		char *home = pwd.pw_dir;
		//char *home = getenv("HOME");
		if(home){
			char buf[256];
			sprintf(buf,"%s/.linuxbrew/lib/7-zip-full/7z.so",home);
			h7z=LoadLibraryA(buf);
		}
		free(buf);
#endif
	}
	if(!h7z)h7z=LoadLibraryA("/usr/lib/7-zip-full/7z.so"); //Generic
	if(!h7z)h7z=LoadLibraryA("/usr/local/lib/7-zip-full/7z.so");
	if(!h7z)h7z=LoadLibraryA("/usr/libexec/7-zip-full/7z.so");
	// p7zip
	if(!h7z)h7z=LoadLibraryA("/usr/local/opt/p7zip/lib/p7zip/7z.so"); //Homebrew
	if(!h7z)h7z=LoadLibraryA("/opt/homebrew/opt/p7zip/lib/p7zip/7z.so");
	if(!h7z)h7z=LoadLibraryA("/opt/local/lib/p7zip/7z.so"); //MacPorts
	if(!h7z)h7z=LoadLibraryA("/sw/lib/p7zip/7z.so"); //Fink
	if(!h7z)h7z=LoadLibraryA("/home/linuxbrew/.linuxbrew/lib/p7zip/7z.so");
	if(!h7z){
#if !defined(FEOS) && !defined(__ANDROID__)
		size_t buflen = sysconf(_SC_GETPW_R_SIZE_MAX)+128;
		char *buf = malloc(buflen);
		struct passwd pwd, *r=NULL;
		getpwuid_r(getuid(), &pwd, buf, buflen, &r);
		char *home = pwd.pw_dir;
		//char *home = getenv("HOME");
		if(home){
			char buf[256];
			sprintf(buf,"%s/.linuxbrew/lib/p7zip/7z.so",home);
			h7z=LoadLibraryA(buf);
		}
		free(buf);
#endif
	}
	if(!h7z)h7z=LoadLibraryA("/usr/lib/p7zip/7z.so"); //Generic
	if(!h7z)h7z=LoadLibraryA("/usr/local/lib/p7zip/7z.so");
	if(!h7z)h7z=LoadLibraryA("/usr/libexec/p7zip/7z.so");
	if(!h7z)h7z=LoadLibraryA("7z.so"); // last resort using LD_LIBRARY_PATH
	if(!h7z)h7z=LoadLibraryA("7z.dylib");
#endif
#endif
	if(!h7z)return 1;

#ifndef NODLOPEN
#if defined(_WIN32) || (!defined(__GNUC__) && !defined(__clang__))
	pCreateArchiver=(funcCreateObject)GetProcAddress(h7z,"CreateObject");
	pCreateCoder=(funcCreateObject)GetProcAddress(h7z,"CreateObject");
#else
	pCreateArchiver=(funcCreateObject)GetProcAddress(h7z,"CreateArchiver");
	pCreateCoder=(funcCreateObject)GetProcAddress(h7z,"CreateCoder");
#endif
	pSetCodecs=(funcSetCodecs)GetProcAddress(h7z,"SetCodecs");
	pGetNumFormats=(funcGetNumFormats)GetProcAddress(h7z,"GetNumberOfFormats");
	pGetNumMethods=(funcGetNumMethods)GetProcAddress(h7z,"GetNumberOfMethods");
	pGetHandlerProperty2 = (funcGetProperty)GetProcAddress(h7z,"GetHandlerProperty2");
	pGetMethodProperty = (funcGetProperty)GetProcAddress(h7z,"GetMethodProperty");
#endif
	if(!lzma7zAlive()){
#ifndef NODLOPEN
		FreeLibrary(h7z);
#endif
		h7z=NULL;
		return 2;
	}

	return 0; //now you can call 7z.so.
}
bool lzma7zAlive(){
	return h7z&&pCreateArchiver&&pCreateCoder&&pSetCodecs&&pGetNumFormats&&pGetNumMethods&&pGetHandlerProperty2&&pGetMethodProperty;
}
int lzmaGet7zFileName(char* path, int siz){
	if(!lzma7zAlive())return 0;
#if 0
//defined(DL_ANDROID)
	return GetModuleFileNameA(pCreateArchiver,path,siz);
#else
	return GetModuleFileNameA(h7z,path,siz);
#endif
}
int lzmaClose7z(){
	if(!lzma7zAlive())return 1;
#ifndef NODLOPEN
	FreeLibrary(h7z);
#endif
	h7z=NULL;
	return 0;
}

int lzmaShowInfos(){
	if(!lzma7zAlive())return 1;
	FILE *out = stderr;
	char strbuf[768];

	lzmaGet7zFileName(strbuf, 768);
	fprintf(out,"7-zip implementation: %s\n\n", strbuf);

	fprintf(out, "Formats:\n");
	u32 numFormats;
	pGetNumFormats(&numFormats);
	for(u32 i=0;i<numFormats;i++){
		PROPVARIANT value;
		pGetHandlerProperty2(i, NHandlerPropID_kName, &value);
		wcstombs(strbuf, value.bstrVal, 768);
		fprintf(out, "%s\t", strbuf);
		if(strlen(strbuf)<8)fputc('\t', out);
		// PropVariantClear(&value); // cleared externally
		pGetHandlerProperty2(i, NHandlerPropID_kClassID, &value);
		fprintf(out, "%d (0x%02x)\t", ((u8*)value.bstrVal)[13], ((u8*)value.bstrVal)[13]);
		pGetHandlerProperty2(i, NHandlerPropID_kUpdate, &value);
		fputc(value.boolVal ? 'C' : '-', out);
		pGetHandlerProperty2(i, NHandlerPropID_kFlags, &value);
		// CPP/7zip/UI/Console/Main.cpp
		for(u32 j=0;j<14;j++)fputc((value.uintVal&(1<<j)) ? "KSNFMGOPBELHXC"[j] : '-', out);
		pGetHandlerProperty2(i, NHandlerPropID_kExtension, &value);
		wcstombs(strbuf, value.bstrVal, 768);
		fprintf(out, "\t%s", strbuf);
		//pGetHandlerProperty2(i, NHandlerPropID_kSignature, &value);
		//wcstombs(strbuf, value.bstrVal, 768);
		//fprintf(out, "\t%s", strbuf);
		fprintf(out, "\n");
	}

	fprintf(out, "Codecs:\n");
	u32 numMethods;
	pGetNumMethods(&numMethods);
	for(u32 i=0;i<numMethods;i++){
		PROPVARIANT value;
		pGetMethodProperty(i, NMethodPropID_kName, &value);
		wcstombs(strbuf, value.bstrVal, 768);
		fprintf(out, "%s\t", strbuf);
		if(strlen(strbuf)<8)fputc('\t', out);
		pGetMethodProperty(i, NMethodPropID_kID, &value);
		fprintf(out, "0x%08x\t", value.uintVal);
		pGetMethodProperty(i, NMethodPropID_kEncoderIsAssigned, &value);
		fputc(value.boolVal ? 'E' : '-', out);
		pGetMethodProperty(i, NMethodPropID_kDecoderIsAssigned, &value);
		fputc(value.boolVal ? 'D' : '-', out);
		HRESULT r = pGetMethodProperty(i, NMethodPropID_kIsFilter, &value);
		fputc(r ? '*' : value.boolVal ? 'F' : '-', out);
		//r = pGetMethodProperty(i, NMethodPropID_kDescription, &value);
		//if(!r){
		//	wcstombs(strbuf, value.bstrVal, 768);
		//	fprintf(out, "\t%s", strbuf);
		//}
		fprintf(out, "\n");
	}
	if(scoder.vt){
		SCompressCodecsInfoExternal_GetNumMethods(&scoder, &numMethods);
		for(u32 i=0;i<numMethods;i++){
			PROPVARIANT value;
			SCompressCodecsInfoExternal_GetProperty(&scoder, i, NMethodPropID_kName, &value);
			wcstombs(strbuf, value.bstrVal, 768);
			fprintf(out, "%s\t", strbuf);
			if(strlen(strbuf)<8)fputc('\t', out);
			SCompressCodecsInfoExternal_GetProperty(&scoder, i, NMethodPropID_kID, &value);
			fprintf(out, "0x%08x\t", value.uintVal);
			SCompressCodecsInfoExternal_GetProperty(&scoder, i, NMethodPropID_kEncoderIsAssigned, &value);
			fputc(value.boolVal ? 'E' : '-', out);
			SCompressCodecsInfoExternal_GetProperty(&scoder, i, NMethodPropID_kDecoderIsAssigned, &value);
			fputc(value.boolVal ? 'D' : '-', out);
			HRESULT r = SCompressCodecsInfoExternal_GetProperty(&scoder, i, NMethodPropID_kIsFilter, &value);
			fputc(r ? '*' : value.boolVal ? 'F' : '-', out);
			fprintf(out, "\n");
		}
	}
	return 0;
}

#if 0
//call with CreateArchiver
GUID CLSID_CArchiveHandler=
	{0x23170F69,0x40C1,0x278A,
	{0x10,0x00,0x00,0x01,0x10,/*archive type*/0x07,0x00,0x00}};

//call with CreateCodec
GUID CLSID_CCodec=
	{0x23170F69,0x40C1,/*decoder:0x2790 encoder:0x2791*/0x2791,
	{0x08,0x01,0x04,0x00,0x00,0x00,0x00,0x00}}; //Codec ID in little endian
	//{0x01,0x01,0x03,0x00,0x00,0x00,0x00,0x00}}; //LZMA
	//{0x02,0x02,0x04,0x00,0x00,0x00,0x00,0x00}}; //BZip2
#endif

static int lzmaGUIDSetArchiver(void *g,unsigned char arctype){
	if(!g)return 1;
	GUID *guid=(GUID*)g;
	guid->Data1=0x23170F69;
	guid->Data2=0x40C1;
	guid->Data3=0x278A;
	guid->Data4[0]=0x10;
	guid->Data4[1]=0x00;
	guid->Data4[2]=0x00;
	guid->Data4[3]=0x01;
	guid->Data4[4]=0x10;
	guid->Data4[5]=arctype;
	guid->Data4[6]=0x00;
	guid->Data4[7]=0x00;
	return 0;
}

static int lzmaGUIDSetCoder(void *g,unsigned long long int codecid,int encode){
	if(!g||(encode!=0&&encode!=1))return 1;
	GUID *guid=(GUID*)g;
	guid->Data1=0x23170F69;
	guid->Data2=0x40C1;
	guid->Data3=0x2790|encode;
	int i=0;
	for(;i<8;i++,codecid>>=8)guid->Data4[i]=codecid&0xff;
	return 0;
}

//Archive API
int lzmaCreateArchiver(void **archiver,unsigned char arctype,int encode,int level){
	if(!h7z||!archiver)return -1;
	GUID CLSID_CArchiveHandler;
	lzmaGUIDSetArchiver(&CLSID_CArchiveHandler,arctype);
	if(pCreateArchiver(&CLSID_CArchiveHandler, encode?&IID_IOutArchive_:&IID_IInArchive_, archiver)!=S_OK){*archiver=NULL;return 1;}
	if(encode && level>=0){
		// there are massive options in 7-zip; just support -mx=N for now.
		ISetProperties_ *setprop;
		(*(IOutArchive_**)archiver)->vt->QueryInterface(*archiver, &IID_ISetProperties_, (void**)&setprop);
		const wchar_t* OPTS[]={L"x"};
		PROPVARIANT VAR[]={{VT_UI4,0,0,0,{0}}};
		VAR[0].uintVal=level;
		setprop->vt->SetProperties(setprop,OPTS,VAR,1);
	}
	return 0;
}
int lzmaDestroyArchiver(void **archiver,int encode){
	if(!h7z||!archiver||!*archiver)return -1;
	if(encode)(*(IOutArchive_**)archiver)->vt->Release(*archiver);
	else (*(IInArchive_**)archiver)->vt->Release(*archiver);
	*archiver=NULL;
	return 0;
}

int lzmaOpenArchive(void *archiver,void *reader,const char *password,const char *fname){
	if(!h7z||!archiver)return -1;
	u64 maxCheckStartPosition=0;
	SArchiveOpenCallbackPassword scallback;
	MakeSArchiveOpenCallbackPassword(&scallback,password,fname);
	int r=((IInArchive_*)archiver)->vt->Open(archiver, (IInStream_*)reader, &maxCheckStartPosition, (IArchiveOpenCallback_*)&scallback);
	scallback.vt->Release(&scallback);
	return r;
}
int lzmaCloseArchive(void *archiver){
	if(!h7z||!archiver)return -1;
	return ((IInArchive_*)archiver)->vt->Close(archiver);
}
int lzmaGetArchiveFileNum(void *archiver,unsigned int *numItems){
	if(!h7z||!archiver)return -1;
	return ((IInArchive_*)archiver)->vt->GetNumberOfItems(archiver, numItems);
}
int lzmaGetArchiveFileProperty(void *archiver,unsigned int index,int kpid,PROPVARIANT *prop){
	if(!h7z||!archiver)return -1;
	return ((IInArchive_*)archiver)->vt->GetProperty(archiver, index, kpid, prop);
}
int lzmaExtractArchive(void *archiver,const unsigned int* indices, unsigned int numItems, int testMode, void *callback){
	if(!h7z||!archiver)return -1;
	return ((IInArchive_*)archiver)->vt->Extract(archiver,indices,numItems,testMode,(IArchiveExtractCallback_*)callback);
}
int lzmaUpdateArchive(void *archiver,void *writer,u32 numItems,void *callback){
	if(!h7z||!archiver)return -1;
	return ((IOutArchive_*)archiver)->vt->UpdateItems(archiver,(IOutStream_*)writer,numItems,(IArchiveUpdateCallback_*)callback);
}

//Coder API
int lzmaCreateCoder(void **coder,unsigned long long int id,int encode,int level){
	if(!h7z||!coder)return -1;
	if(level<1)level=1;
	if(level>9)level=9;

	GUID CLSID_CCodec;
	lzmaGUIDSetCoder(&CLSID_CCodec,id,encode);
	if(pCreateCoder(&CLSID_CCodec, &IID_ICompressCoder_, coder)!=S_OK){*coder=NULL;return 1;}
	if(!encode)return 0;

	ICompressSetCoderProperties_ *coderprop;
	(*(ICompressCoder_**)coder)->vt->QueryInterface(*coder, &IID_ICompressSetCoderProperties_, (void**)&coderprop);

#ifndef LZMA_SPECIAL_COMPRESSION_LEVEL
	PROPID ID[]={NCoderPropID_kLevel,NCoderPropID_kEndMarker,NCoderPropID_kDictionarySize};
	PROPVARIANT VAR[]={{VT_UI4,0,0,0,{0}},{VT_BOOL,0,0,0,{0}},{VT_UI4,0,0,0,{0}}};
	VAR[0].uintVal=level;
	VAR[1].boolVal=VARIANT_TRUE;
	VAR[2].uintVal=1<<24;
	coderprop->vt->SetCoderProperties(coderprop,ID,VAR,id==0x030101 ? 3 : 1);
#else
	//Deflate(64)
	if(id==0x040108||id==0x040109){
		//VT_UI4 NCoderPropID_kNumPasses
		//VT_UI4 NCoderPropID_kNumFastBytes (3 .. 255+2)
		//VT_UI4 NCoderPropID_kMatchFinderCycles
		//VT_UI4 NCoderPropID_kAlgorithm
		PROPID ID[]={NCoderPropID_kAlgorithm,NCoderPropID_kNumPasses,NCoderPropID_kNumFastBytes};
		PROPVARIANT VAR[]={{VT_UI4,0,0,0,{0}},{VT_UI4,0,0,0,{0}},{VT_UI4,0,0,0,{0}}};
		switch(level){
			case 9: VAR[0].ulVal=1,VAR[1].ulVal=10,VAR[2].ulVal=256;break;
			case 8: VAR[0].ulVal=1,VAR[1].ulVal=10,VAR[2].ulVal=128;break;
			case 7: VAR[0].ulVal=1,VAR[1].ulVal=3,VAR[2].ulVal=128;break;
			case 6: VAR[0].ulVal=1,VAR[1].ulVal=3,VAR[2].ulVal=64;break;
			case 5: VAR[0].ulVal=1,VAR[1].ulVal=1,VAR[2].ulVal=64;break;
			case 4: VAR[0].ulVal=0,VAR[1].ulVal=1,VAR[2].ulVal=32;break;
			case 3: VAR[0].ulVal=0,VAR[1].ulVal=1,VAR[2].ulVal=32;break;
			case 2: VAR[0].ulVal=0,VAR[1].ulVal=1,VAR[2].ulVal=32;break;
			case 1: VAR[0].ulVal=0,VAR[1].ulVal=1,VAR[2].ulVal=32;break;
		}
		coderprop->vt->SetCoderProperties(coderprop,ID,VAR,3);
	}
	//BZip2
	if(id==0x040202){
		//VT_UI4 NCoderPropID_kNumPasses
		//VT_UI4 NCoderPropID_kDictionarySize
		//VT_UI4 NCoderPropID_kNumThreads
		PROPID ID[]={NCoderPropID_kNumPasses,NCoderPropID_kDictionarySize};
		PROPVARIANT VAR[]={{VT_UI4,0,0,0,{0}},{VT_UI4,0,0,0,{0}}};
		switch(level){
			case 9: VAR[0].ulVal=9,VAR[1].ulVal=900000;break;
			case 8: VAR[0].ulVal=7,VAR[1].ulVal=900000;break;
			case 7: VAR[0].ulVal=2,VAR[1].ulVal=900000;break;
			case 6: VAR[0].ulVal=1,VAR[1].ulVal=900000;break;
			case 5: VAR[0].ulVal=1,VAR[1].ulVal=900000;break;
			case 4: VAR[0].ulVal=1,VAR[1].ulVal=700000;break;
			case 3: VAR[0].ulVal=1,VAR[1].ulVal=500000;break;
			case 2: VAR[0].ulVal=1,VAR[1].ulVal=300000;break;
			case 1: VAR[0].ulVal=1,VAR[1].ulVal=100000;break;
		}
		coderprop->vt->SetCoderProperties(coderprop,ID,VAR,2);
	}
	//PPMD
	if(id==0x030401){
		//VT_UI4 NCoderPropID_kUsedMemorySize
		//VT_UI4 NCoderPropID_kOrder
		PROPID ID[]={NCoderPropID_kUsedMemorySize,NCoderPropID_kOrder};
		PROPVARIANT VAR[]={{VT_UI4,0,0,0,{0}},{VT_UI4,0,0,0,{0}}};
		VAR[0].ulVal=1 << (19 + (level > 8 ? 8 : level));
		VAR[1].ulVal=3 + level;
		coderprop->vt->SetCoderProperties(coderprop,ID,VAR,2);
	}
	//LZMA
	if(id==0x030101){
		//VT_BSTR NCoderPropID_kMatchFinder
		//VT_UI4  NCoderPropID_kNumFastBytes (5 .. 2+8+8+256-1)
		//VT_UI4  NCoderPropID_kMatchFinderCycles
		//VT_UI4  NCoderPropID_kAlgorithm
		//VT_UI4  NCoderPropID_kDictionarySize
		//VT_UI4  NCoderPropID_kPosStateBits
		//VT_UI4  NCoderPropID_kLitPosBits
		//VT_UI4  NCoderPropID_kLitContextBits
		//VT_UI4  NCoderPropID_kNumThreads
		//VT_BOOL NCoderPropID_kEndMarker (not available in LZMA2)
		PROPID ID[]={
			NCoderPropID_kEndMarker,
			NCoderPropID_kMatchFinder,
			NCoderPropID_kAlgorithm,
			NCoderPropID_kNumFastBytes,
			NCoderPropID_kDictionarySize
		};
		PROPVARIANT VAR[]={{VT_BOOL,0,0,0,{0}},{VT_BSTR,0,0,0,{0}},{VT_UI4,0,0,0,{0}},{VT_UI4,0,0,0,{0}},{VT_UI4,0,0,0,{0}}};
		VAR[0].boolVal=VARIANT_TRUE;
		switch(level){
			case 9: VAR[1].bstrVal=L"BT4",VAR[2].ulVal=1,VAR[3].ulVal=256,VAR[4].ulVal=1<<24;break;//26
			case 8: VAR[1].bstrVal=L"BT4",VAR[2].ulVal=1,VAR[3].ulVal=128,VAR[4].ulVal=1<<24;break;//25
			case 7: VAR[1].bstrVal=L"BT4",VAR[2].ulVal=1,VAR[3].ulVal=64,VAR[4].ulVal=1<<24;break;//25
			case 6: VAR[1].bstrVal=L"BT4",VAR[2].ulVal=1,VAR[3].ulVal=64,VAR[4].ulVal=1<<24;break;
			case 5: VAR[1].bstrVal=L"BT4",VAR[2].ulVal=1,VAR[3].ulVal=32,VAR[4].ulVal=1<<24;break;
			case 4: VAR[1].bstrVal=L"HC4",VAR[2].ulVal=0,VAR[3].ulVal=32,VAR[4].ulVal=1<<24;break;//20
			case 3: VAR[1].bstrVal=L"HC4",VAR[2].ulVal=0,VAR[3].ulVal=32,VAR[4].ulVal=1<<24;break;//20
			case 2: VAR[1].bstrVal=L"HC4",VAR[2].ulVal=0,VAR[3].ulVal=32,VAR[4].ulVal=1<<24;break;//16
			case 1: VAR[1].bstrVal=L"HC4",VAR[2].ulVal=0,VAR[3].ulVal=32,VAR[4].ulVal=1<<24;break;//16
		}
		coderprop->vt->SetCoderProperties(coderprop,ID,VAR,5);
	}
	//LZMA2
	if(id==0x21){
		//VT_UI4 NCoderPropID_kBlockSize
		//left blank intentionally.
	}
#endif
	return 0;
}
int lzmaDestroyCoder(void **coder){
	if(!h7z||!coder||!*coder)return -1;
	(*(ICompressCoder_**)coder)->vt->Release(*coder);
	*coder=NULL;
	return 0;
}

//static int mread2(void *h, char *p, int n){return mread(p,n,(memstream*)h);}
//static int mwrite2(void *h, const char *p, int n){return mwrite(p,n,(memstream*)h);}
//static int mclose2(void *h){return 0;}
int lzmaCodeOneshot(void *coder, const unsigned char *in, size_t isize, unsigned char *out, size_t *osize){
		if(!h7z||!in||!out||!osize||!*osize)return -1;
		//unsigned long long int dummy=0;
		SInStreamMem sin;
		MakeSInStreamMem(&sin,(void*)in,isize);
		SSequentialOutStreamMem sout;
		MakeSSequentialOutStreamMem(&sout,out,*osize);
		HRESULT r=((ICompressCoder_*)coder)->vt->Code(coder,(IInStream_*)&sin, (IOutStream_*)&sout, (UINT64*)&isize, (UINT64*)osize, NULL);
		sout.vt->Seek(&sout,0,SEEK_CUR,(u64*)osize);
		sin.vt->Release(&sin);
		sout.vt->Release(&sout);
		if(r!=S_OK)return r;
		return 0;
}
int lzmaCodeCallback(void *coder, void *hin, tRead pRead_in, tClose pClose_in, void *hout, tWrite pWrite_out, tClose pClose_out){
//decoder isn't working here...
		if(!h7z||!hin||!hout)return -1;
		unsigned long long int isize=0xffffffffUL,osize=0xffffffffUL; //some_big_size... lol
		SInStreamGeneric sin;
		MakeSInStreamGeneric(&sin,hin,pRead_in,pClose_in,NULL,NULL);
		SSequentialOutStreamGeneric sout;
		MakeSSequentialOutStreamGeneric(&sout,hout,pWrite_out,pClose_out);
		HRESULT r=((ICompressCoder_*)coder)->vt->Code(coder,(IInStream_*)&sin, (IOutStream_*)&sout, (UINT64*)&isize, (UINT64*)&osize, NULL);
		sin.vt->Release(&sin);
		sout.vt->Release(&sout);
		if(r!=S_OK)return r;
		return 0;
}

#if defined(_WIN32) || (!defined(__GNUC__) && !defined(__clang__))
#else
typedef void* HMODULE;
#endif

static HRESULT SCompressCodecsInfoExternal_QueryInterface(void* _self, const GUID* iid, void** out_obj){
	LZMA_UNUSED SCompressCodecsInfoExternal_ *self = (SCompressCodecsInfoExternal_*)_self;
	*out_obj = NULL;
	return E_NOINTERFACE;
}

static u32 WINAPI SCompressCodecsInfoExternal_AddRef(void* _self){
	LZMA_UNUSED SCompressCodecsInfoExternal_ *self = (SCompressCodecsInfoExternal_*)_self;
	return ++self->refs;
}

static u32 WINAPI SCompressCodecsInfoExternal_Release(void* _self){
	LZMA_UNUSED SCompressCodecsInfoExternal_ *self = (SCompressCodecsInfoExternal_*)_self;
	if(--self->refs==0){
		pSetCodecs(NULL);
		free(self->vt);
		self->vt=NULL;
		if(self->vCodecs){
			size_t n = scv_size(self->vCodecs);
			for(size_t i=0; i<n; i++){
				FreeLibrary(*(HMODULE*)scv_at(self->vCodecs, i));
			}
			scv_delete(self->vCodecs);
			self->vCodecs = NULL;
		}
		if(self->vNumMethods)scv_delete(self->vNumMethods);
		self->vNumMethods=NULL;
		if(self->vGetMethodProperty)scv_delete(self->vGetMethodProperty);
		self->vGetMethodProperty=NULL;
		if(self->vCreateEncoder)scv_delete(self->vCreateEncoder);
		self->vCreateEncoder=NULL;
		if(self->vCreateDecoder)scv_delete(self->vCreateDecoder);
		self->vCreateDecoder=NULL;
	}
	return self->refs;
}

static HRESULT WINAPI SCompressCodecsInfoExternal_GetNumMethods(void* _self, u32 *numMethods){
	LZMA_UNUSED SCompressCodecsInfoExternal_ *self = (SCompressCodecsInfoExternal_*)_self;
	size_t n = scv_size(self->vNumMethods);
	*numMethods = 0;
	for(size_t i=0; i<n; i++){
		*numMethods += *(u32*)scv_at(self->vNumMethods, i);
	}
	return S_OK;
}

static HRESULT WINAPI SCompressCodecsInfoExternal_GetProperty(void* _self, u32 index, PROPID propID, PROPVARIANT *value){
	LZMA_UNUSED SCompressCodecsInfoExternal_ *self = (SCompressCodecsInfoExternal_*)_self;
	size_t n = scv_size(self->vNumMethods);
	for(size_t i=0; i<n; i++){
		u32 numMethods = *(u32*)scv_at(self->vNumMethods, i);
		if(numMethods>index){
			return (*(funcGetProperty*)scv_at(self->vGetMethodProperty, i))(index,propID,value);
		}
		index -= numMethods;
	}
	return E_FAIL;
}

static HRESULT WINAPI SCompressCodecsInfoExternal_CreateDecoder(void* _self, u32 index, const GUID *iid, void **coder){
	LZMA_UNUSED SCompressCodecsInfoExternal_ *self = (SCompressCodecsInfoExternal_*)_self;
	size_t n = scv_size(self->vNumMethods);
	for(size_t i=0; i<n; i++){
		u32 numMethods = *(u32*)scv_at(self->vNumMethods, i);
		if(numMethods>index){
			return (*(funcCreateDecoder*)scv_at(self->vCreateDecoder, i))(index,iid,coder);
		}
		index -= numMethods;
	}
	return E_FAIL;
}

static HRESULT WINAPI SCompressCodecsInfoExternal_CreateEncoder(void* _self, u32 index, const GUID *iid, void **coder){
	LZMA_UNUSED SCompressCodecsInfoExternal_ *self = (SCompressCodecsInfoExternal_*)_self;
	size_t n = scv_size(self->vNumMethods);
	for(size_t i=0; i<n; i++){
		u32 numMethods = *(u32*)scv_at(self->vNumMethods, i);
		if(numMethods>index){
			return (*(funcCreateEncoder*)scv_at(self->vCreateEncoder, i))(index,iid,coder);
		}
		index -= numMethods;
	}
	return E_FAIL;
}

int lzmaLoadExternalCodecs(){
	if(!lzma7zAlive())return E_FAIL;
	if(!pSetCodecs)return E_FAIL;
	if(!scoder.refs){
#ifdef NODLOPEN
		return E_FAIL;
#else
		scoder.vCodecs = scv_new(sizeof(HMODULE), 0);
		scoder.vNumMethods = scv_new(sizeof(u32), 0);
		scoder.vGetMethodProperty = scv_new(sizeof(void*), 0);
		scoder.vCreateDecoder = scv_new(sizeof(void*), 0);
		scoder.vCreateEncoder = scv_new(sizeof(void*), 0);
		{
			char path[768];
#if 0
//defined(DL_ANDROID)
			GetModuleFileNameA(pCreateArchiver,path,768);
#else
			GetModuleFileNameA(h7z,path,768);
#endif
			int i=strlen(path)-1;
			for(;i>=0;i--)if(path[i]=='/'||path[i]=='\\')break;
			i+=1;
			strcpy(path+i,"Codecs/");
			char *basename = path+strlen(path);
			DIR *d=opendir(path);
			if(d){
				struct dirent *ent;
				while(ent=readdir(d)){
					strcpy(basename,ent->d_name);
					HMODULE h=LoadLibraryA(path);
					if(h){
						funcGetNumMethods pGetNumMethods = (funcGetNumMethods)GetProcAddress(h,"GetNumberOfMethods");
						funcGetProperty pGetMethodProperty = (funcGetProperty)GetProcAddress(h,"GetMethodProperty");
						funcCreateDecoder pCreateDecoder = (funcCreateDecoder)GetProcAddress(h,"CreateDecoder");
						funcCreateEncoder pCreateEncoder = (funcCreateEncoder)GetProcAddress(h,"CreateEncoder");
						if(pGetNumMethods && pGetMethodProperty && pCreateDecoder && pCreateEncoder){
							u32 numMethods = 0;
							HRESULT ret = pGetNumMethods(&numMethods);
							if(ret == S_OK){
								scv_push_back(scoder.vCodecs, &h);
								scv_push_back(scoder.vNumMethods, &numMethods);
								scv_push_back(scoder.vGetMethodProperty, &pGetMethodProperty);
								scv_push_back(scoder.vCreateDecoder, &pCreateDecoder);
								scv_push_back(scoder.vCreateEncoder, &pCreateEncoder);
							}else{
								FreeLibrary(h);
							}
						}else{
							FreeLibrary(h);
						}
					}
				}
			}
		}
		scoder.vt = (ICompressCodecsInfo_vt*)calloc(1,sizeof(ICompressCodecsInfo_vt));
		scoder.vt->QueryInterface = SCompressCodecsInfoExternal_QueryInterface;
		scoder.vt->AddRef = SCompressCodecsInfoExternal_AddRef;
		scoder.vt->Release = SCompressCodecsInfoExternal_Release;
		scoder.vt->GetNumMethods = SCompressCodecsInfoExternal_GetNumMethods;
		scoder.vt->GetProperty = SCompressCodecsInfoExternal_GetProperty;
		scoder.vt->CreateDecoder = SCompressCodecsInfoExternal_CreateDecoder;
		scoder.vt->CreateEncoder = SCompressCodecsInfoExternal_CreateEncoder;
		pSetCodecs((ICompressCodecsInfo_*)&scoder);
#endif
	}
	scoder.refs++;
	return S_OK;
}

int lzmaUnloadExternalCodecs(){
	if(!scoder.refs)return 1;
	scoder.vt->Release(&scoder);
	return 0;
}

unsigned long long FileTimeToUTC(const FILETIME in){
	return ((unsigned long long)in.dwHighDateTime*0x100000000+in.dwLowDateTime-0x19DB1DED53E8000)/10000000;
}

FILETIME UTCToFileTime(const unsigned long long UTC){
	FILETIME ftime;
	unsigned long long ll = UTC*10000000+0x19DB1DED53E8000;
	ftime.dwLowDateTime  = (DWORD) ll;
	ftime.dwHighDateTime = (DWORD)(ll >>32);
	return ftime;
}

#if defined(_WIN32) || (!defined(__GNUC__) && !defined(__clang__))
#else
HRESULT PropVariantClear(PROPVARIANT *pvar){
	if(!pvar)return E_INVALIDARG;
	if(pvar->vt == VT_BSTR)SysFreeString(pvar->bstrVal);
	memset(pvar,0,sizeof(PROPVARIANT));
	return S_OK;
}
BSTR SysAllocString(const OLECHAR *str){
	u32 len = wcslen(str);
	return SysAllocStringLen(str,len);
}
BSTR SysAllocStringLen(const OLECHAR *str,u32 len){
	u8 *x = (u8*)calloc(1,4+(len+1)*sizeof(OLECHAR)); // needs to be calloc.
	write32(x,len);
	BSTR ret = (BSTR)(x+4);
	if(str){
		int i=0;
		for(;i<len;i++)ret[i]=str[i];
	}
	return ret;
}
void SysFreeString(BSTR str){
	if(str)free(((u8*)str)-4);
}
u32 SysStringLen(BSTR str){
	if(!str)return 0;
	return read32(((u8*)str)-4);
}
#endif

#ifdef __cplusplus
}
#endif
