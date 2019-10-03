#include <cstring>
#include "lzma_iostream.h"

extern const IID IID_IInStream_;

static HRESULT WINAPI SInStreamIS_QueryInterface(void* _self, const GUID* iid, void** out_obj){
	LZMA_UNUSED SInStreamIS* self = (SInStreamIS*)_self;
	if(!memcmp(iid,&IID_IInStream_,sizeof(GUID))){
		*out_obj = self;
		self->vt->AddRef(self);
		return S_OK;
	}
	*out_obj = NULL;
	return E_NOINTERFACE;
}

static u32 WINAPI SInStreamIS_AddRef(void* _self){
	LZMA_UNUSED SInStreamIS* self = (SInStreamIS*)_self;
	return ++self->refs;
}

static u32 WINAPI SInStreamIS_Release(void* _self){
	LZMA_UNUSED SInStreamIS* self = (SInStreamIS*)_self;
	if(--self->refs==0){
		free(self->vt);
		self->vt=NULL;
		//self->f.close();
	}
	return self->refs;
}

static HRESULT WINAPI SInStreamIS_Read(void* _self, void *data, u32 size, u32 *processedSize){
	LZMA_UNUSED SInStreamIS* self = (SInStreamIS*)_self;
	self->f->read((char*)data, size);
	if(processedSize)*processedSize = self->f->gcount();
	return S_OK;
}

static HRESULT WINAPI SInStreamIS_Seek(void* _self, s64 offset, u32 seekOrigin, u64 *newPosition){
	LZMA_UNUSED SInStreamIS* self = (SInStreamIS*)_self;
	self->f->clear();
	self->f->seekg(offset, (std::ios_base::seekdir)seekOrigin);
	if(newPosition)*newPosition = self->f->tellg();
	return S_OK;
}

bool WINAPI MakeSInStreamIS(SInStreamIS *self, std::istream *f){
	self->f = f;
	self->vt = (IInStream_vt*)calloc(1,sizeof(IInStream_vt));
	if(!self->vt)return false;
	self->vt->QueryInterface = SInStreamIS_QueryInterface;
	self->vt->AddRef = SInStreamIS_AddRef;
	self->vt->Release = SInStreamIS_Release;
	self->vt->Read = SInStreamIS_Read;
	self->vt->Seek = SInStreamIS_Seek;
	self->refs = 1;
	return true;
}

static HRESULT WINAPI SOutStreamOS_QueryInterface(void* _self, const GUID* iid, void** out_obj){
	LZMA_UNUSED SOutStreamOS* self = (SOutStreamOS*)_self;
	*out_obj = NULL;
	return E_NOINTERFACE;
}

static u32 WINAPI SOutStreamOS_AddRef(void* _self){
	LZMA_UNUSED SOutStreamOS* self = (SOutStreamOS*)_self;
	return ++self->refs;
}

static u32 WINAPI SOutStreamOS_Release(void* _self){
	LZMA_UNUSED SOutStreamOS* self = (SOutStreamOS*)_self;
	if(--self->refs==0){
		free(self->vt);
		self->vt=NULL;
		//self->f->close();
	}
	return self->refs;
}

static HRESULT WINAPI SOutStreamOS_Write(void* _self, const void *data, u32 size, u32 *processedSize){
	LZMA_UNUSED SOutStreamOS* self = (SOutStreamOS*)_self;
	u64 pos = self->f->tellp();
	self->f->write((const char*)data, size);
	u32 writelen = (std::streamoff)self->f->tellp() - pos; // pcount does not exist in ostream... lol?
	if(size&&!writelen)return E_FAIL;
	if(processedSize)*processedSize = writelen;
	return S_OK;
}

static HRESULT WINAPI SOutStreamOS_Seek(void* _self, s64 offset, u32 seekOrigin, u64 *newPosition){
	LZMA_UNUSED SOutStreamOS* self = (SOutStreamOS*)_self;
	self->f->seekp(offset, (std::ios_base::seekdir)seekOrigin);
	if(newPosition)*newPosition = self->f->tellp();
	return S_OK;
}

static HRESULT WINAPI SOutStreamOS_SetSize(void* _self, u64 newSize){
	LZMA_UNUSED SOutStreamOS* self = (SOutStreamOS*)_self;
	return E_FAIL;
}

bool MakeSOutStreamOS(SOutStreamOS *self, std::ostream *f){
	self->f = f;
	self->vt = (IOutStream_vt*)calloc(1,sizeof(IOutStream_vt));
	if(!self->vt)return false;
	self->vt->QueryInterface = SOutStreamOS_QueryInterface;
	self->vt->AddRef = SOutStreamOS_AddRef;
	self->vt->Release = SOutStreamOS_Release;
	self->vt->Write = SOutStreamOS_Write;
	self->vt->Seek = SOutStreamOS_Seek;
	self->vt->SetSize = SOutStreamOS_SetSize;
	self->refs = 1;
	return true;
}

static HRESULT WINAPI SArchiveExtractCallbackOS_GetStream(void* _self, u32 index, /*ISequentialOutStream_*/IOutStream_ **outStream, s32 askExtractMode){
	SArchiveExtractCallbackOS *self = (SArchiveExtractCallbackOS*)_self;
	IOutStream_ *stream = (IOutStream_*)calloc(1,sizeof(SOutStreamOS));
	MakeSOutStreamOS((SOutStreamOS*)stream,self->f);
	*outStream = stream;
	return S_OK;
}

bool MakeSArchiveExtractCallbackOS(SArchiveExtractCallbackOS *self, IInArchive_ *archiver, const char *password){
	bool b = MakeSArchiveExtractCallbackBare((SArchiveExtractCallbackBare*)self,(IInArchive_*)archiver,password);
	if(!b)return false;
	self->vt->GetStream = SArchiveExtractCallbackOS_GetStream;
	return true;
}
