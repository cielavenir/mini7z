#include <cstring>
#include "lzma_iostream.h"

extern const IID IID_IInStream_;

static HRESULT WINAPI SInStreamIS22_QueryInterface(void* _self, const GUID* iid, void** out_obj){
	LZMA_UNUSED SInStreamIS22* self = (SInStreamIS22*)_self;
	if(!memcmp(iid,&IID_IInStream_,sizeof(GUID))){
		*out_obj = self;
		self->vt->AddRef(self);
		return S_OK;
	}
	*out_obj = NULL;
	return E_NOINTERFACE;
}

static HRESULT WINAPI SInStreamIS23_QueryInterface(void* _self, const GUID* iid, void** out_obj){
	LZMA_UNUSED SInStreamIS23* self = (SInStreamIS23*)_self;
	if(!memcmp(iid,&IID_IInStream_,sizeof(GUID))){
		*out_obj = self;
		self->vt->AddRef(self);
		return S_OK;
	}
	*out_obj = NULL;
	return E_NOINTERFACE;
}

static u32 WINAPI SInStreamIS_AddRef(void* _self){
	LZMA_UNUSED SInStreamIS23* self = (SInStreamIS23*)_self;
	return ++self->refs;
}

static u32 WINAPI SInStreamIS_Release(void* _self){
	LZMA_UNUSED SInStreamIS23* self = (SInStreamIS23*)_self;
	if(--self->refs==0){
		free(self->vt);
		self->vt=NULL;
		//self->f.close();
	}
	return self->refs;
}

static HRESULT WINAPI SInStreamIS_Read(void* _self, void *data, u32 size, u32 *processedSize){
	LZMA_UNUSED SInStreamIS23* self = (SInStreamIS23*)_self;
	self->f->read((char*)data, size);
	if(processedSize)*processedSize = self->f->gcount();
	return S_OK;
}

static HRESULT WINAPI SInStreamIS_Seek(void* _self, s64 offset, u32 seekOrigin, u64 *newPosition){
	LZMA_UNUSED SInStreamIS23* self = (SInStreamIS23*)_self;
	self->f->clear();
	self->f->seekg(offset, (std::ios_base::seekdir)seekOrigin);
	if(newPosition)*newPosition = self->f->tellg();
	return S_OK;
}

bool WINAPI MakeSInStreamIS22(SInStreamIS22 *self, std::istream *f){
	self->f = f;
	self->vt = (IInStream22_vt*)calloc(1,sizeof(IInStream22_vt));
	if(!self->vt)return false;
	self->vt->QueryInterface = SInStreamIS22_QueryInterface;
	self->vt->AddRef = SInStreamIS_AddRef;
	self->vt->Release = SInStreamIS_Release;
	self->vt->Read = SInStreamIS_Read;
	self->vt->Seek = SInStreamIS_Seek;
	self->refs = 1;
	return true;
}

bool WINAPI MakeSInStreamIS23(SInStreamIS23 *self, std::istream *f){
	self->f = f;
	self->vt = (IInStream23_vt*)calloc(1,sizeof(IInStream23_vt));
	if(!self->vt)return false;
	self->vt->QueryInterface = SInStreamIS23_QueryInterface;
	self->vt->AddRef = SInStreamIS_AddRef;
	self->vt->Release = SInStreamIS_Release;
	self->vt->Read = SInStreamIS_Read;
	self->vt->Seek = SInStreamIS_Seek;
	self->refs = 1;
	return true;
}

static HRESULT WINAPI SOutStreamOS22_QueryInterface(void* _self, const GUID* iid, void** out_obj){
	LZMA_UNUSED SOutStreamOS22* self = (SOutStreamOS22*)_self;
	*out_obj = NULL;
	return E_NOINTERFACE;
}

static HRESULT WINAPI SOutStreamOS23_QueryInterface(void* _self, const GUID* iid, void** out_obj){
	LZMA_UNUSED SOutStreamOS23* self = (SOutStreamOS23*)_self;
	*out_obj = NULL;
	return E_NOINTERFACE;
}

static u32 WINAPI SOutStreamOS_AddRef(void* _self){
	LZMA_UNUSED SOutStreamOS23* self = (SOutStreamOS23*)_self;
	return ++self->refs;
}

static u32 WINAPI SOutStreamOS_Release(void* _self){
	LZMA_UNUSED SOutStreamOS23* self = (SOutStreamOS23*)_self;
	if(--self->refs==0){
		free(self->vt);
		self->vt=NULL;
		//self->f->close();
	}
	return self->refs;
}

static HRESULT WINAPI SOutStreamOS_Write(void* _self, const void *data, u32 size, u32 *processedSize){
	LZMA_UNUSED SOutStreamOS23* self = (SOutStreamOS23*)_self;
	u64 pos = self->f->tellp();
	self->f->write((const char*)data, size);
	u32 writelen = (std::streamoff)self->f->tellp() - pos; // pcount does not exist in ostream... lol?
	if(size&&!writelen)return E_FAIL;
	if(processedSize)*processedSize = writelen;
	return S_OK;
}

static HRESULT WINAPI SOutStreamOS_Seek(void* _self, s64 offset, u32 seekOrigin, u64 *newPosition){
	LZMA_UNUSED SOutStreamOS23* self = (SOutStreamOS23*)_self;
	self->f->seekp(offset, (std::ios_base::seekdir)seekOrigin);
	if(newPosition)*newPosition = self->f->tellp();
	return S_OK;
}

static HRESULT WINAPI SOutStreamOS_SetSize(void* _self, u64 newSize){
	LZMA_UNUSED SOutStreamOS23* self = (SOutStreamOS23*)_self;
	return E_FAIL;
}

bool MakeSOutStreamOS22(SOutStreamOS22 *self, std::ostream *f){
	self->f = f;
	self->vt = (IOutStream22_vt*)calloc(1,sizeof(IOutStream22_vt));
	if(!self->vt)return false;
	self->vt->QueryInterface = SOutStreamOS22_QueryInterface;
	self->vt->AddRef = SOutStreamOS_AddRef;
	self->vt->Release = SOutStreamOS_Release;
	self->vt->Write = SOutStreamOS_Write;
	self->vt->Seek = SOutStreamOS_Seek;
	self->vt->SetSize = SOutStreamOS_SetSize;
	self->refs = 1;
	return true;
}

bool MakeSOutStreamOS23(SOutStreamOS23 *self, std::ostream *f){
	self->f = f;
	self->vt = (IOutStream23_vt*)calloc(1,sizeof(IOutStream23_vt));
	if(!self->vt)return false;
	self->vt->QueryInterface = SOutStreamOS23_QueryInterface;
	self->vt->AddRef = SOutStreamOS_AddRef;
	self->vt->Release = SOutStreamOS_Release;
	self->vt->Write = SOutStreamOS_Write;
	self->vt->Seek = SOutStreamOS_Seek;
	self->vt->SetSize = SOutStreamOS_SetSize;
	self->refs = 1;
	return true;
}

static HRESULT WINAPI SArchiveExtractCallbackOS22_GetStream(void* _self, u32 index, /*ISequentialOutStream22_*/IOutStream22_ **outStream, s32 askExtractMode){
	SArchiveExtractCallbackOS22 *self = (SArchiveExtractCallbackOS22*)_self;
	IOutStream22_ *stream = (IOutStream22_*)calloc(1,sizeof(SOutStreamOS22));
	MakeSOutStreamOS22((SOutStreamOS22*)stream,self->f);
	*outStream = stream;
	return S_OK;
}

static HRESULT WINAPI SArchiveExtractCallbackOS23_GetStream(void* _self, u32 index, /*ISequentialOutStream23_*/IOutStream23_ **outStream, s32 askExtractMode){
	SArchiveExtractCallbackOS23 *self = (SArchiveExtractCallbackOS23*)_self;
	IOutStream23_ *stream = (IOutStream23_*)calloc(1,sizeof(SOutStreamOS23));
	MakeSOutStreamOS23((SOutStreamOS23*)stream,self->f);
	*outStream = stream;
	return S_OK;
}

bool MakeSArchiveExtractCallbackOS22(SArchiveExtractCallbackOS22 *self, IInArchive22_ *archiver, const char *password){
	bool b = MakeSArchiveExtractCallbackBare22((SArchiveExtractCallbackBare22*)self,(IInArchive22_*)archiver,password);
	if(!b)return false;
	self->vt->GetStream = SArchiveExtractCallbackOS22_GetStream;
	return true;
}

bool MakeSArchiveExtractCallbackOS23(SArchiveExtractCallbackOS23 *self, IInArchive23_ *archiver, const char *password){
	bool b = MakeSArchiveExtractCallbackBare23((SArchiveExtractCallbackBare23*)self,(IInArchive23_*)archiver,password);
	if(!b)return false;
	self->vt->GetStream = SArchiveExtractCallbackOS23_GetStream;
	return true;
}
