#ifndef _LZMA_IOSTREAM_H_
#define _LZMA_IOSTREAM_H_

#include <iostream>
#include "lzma.h"

typedef struct{
	IInStream22_vt *vt;
	u32 refs;
	std::istream *f;
} SInStreamIS22;

bool WINAPI MakeSInStreamIS22(SInStreamIS22 *self, std::istream *f);

typedef struct{
	IOutStream22_vt *vt;
	u32 refs;
	std::ostream *f;
} SOutStreamOS22;

bool MakeSOutStreamOS22(SOutStreamOS22 *self, std::ostream *f);

typedef struct{
	IArchiveExtractCallback22_vt *vt;
	u32 refs;
	SCryptoGetTextPasswordFixed22 setpassword;
	IInArchive22_ *archiver;
	u32 lastIndex;
	std::ostream *f;
} SArchiveExtractCallbackOS22;

bool MakeSArchiveExtractCallbackOS22(SArchiveExtractCallbackOS22 *self, IInArchive22_ *archiver, const char *password);

typedef struct{
	IInStream23_vt *vt;
	u32 refs;
	std::istream *f;
} SInStreamIS23;

bool WINAPI MakeSInStreamIS23(SInStreamIS23 *self, std::istream *f);

typedef struct{
	IOutStream23_vt *vt;
	u32 refs;
	std::ostream *f;
} SOutStreamOS23;

bool MakeSOutStreamOS23(SOutStreamOS23 *self, std::ostream *f);

typedef struct{
	IArchiveExtractCallback23_vt *vt;
	u32 refs;
	SCryptoGetTextPasswordFixed23 setpassword;
	IInArchive23_ *archiver;
	u32 lastIndex;
	std::ostream *f;
} SArchiveExtractCallbackOS23;

bool MakeSArchiveExtractCallbackOS23(SArchiveExtractCallbackOS23 *self, IInArchive23_ *archiver, const char *password);

#endif
