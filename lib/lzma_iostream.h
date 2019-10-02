#include <iostream>
#include "lzma.h"

typedef struct{
    IInStream_vt *vt;
    u32 refs;
    std::istream *f;
} SInStreamIS;

bool WINAPI MakeSInStreamIS(SInStreamIS *self, std::istream *f);

typedef struct{
    IOutStream_vt *vt;
    u32 refs;
    std::ostream *f;
} SOutStreamOS;

bool MakeSOutStreamOS(SOutStreamOS *self, std::ostream *f);

typedef struct{
    IArchiveExtractCallback_vt *vt;
    u32 refs;
    SCryptoGetTextPasswordFixed setpassword;
    IInArchive_ *archiver;
    u32 lastIndex;
    std::ostream *f;
} SArchiveExtractCallbackOS;

bool MakeSArchiveExtractCallbackOS(SArchiveExtractCallbackOS *self, IInArchive_ *archiver, const char *password);
