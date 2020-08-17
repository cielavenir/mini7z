#include "../compat.h"
#include <string.h>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__bsdi__) || defined(__DragonFly__)
#include <link.h>
#endif

#if defined(_WIN32) || (!defined(__GNUC__) && !defined(__clang__))
#else
int GetModuleFileNameA(void *hModule,char *pFilename,int nSize){
	if(!pFilename)return 0;
	pFilename[0]=0;
#if defined(NODLOPEN)
	return 0;
#elif defined(FEOS)
	char *fname = FeOS_GetModuleName(hModule);
	if(fname){
		int s = strlen(fname);
		int nCopy = min(nSize,s+1);
		memcpy(pFilename,fname,nCopy);
		return nCopy;
	}
#elif defined(__APPLE__)
	// https://stackoverflow.com/a/54201385
	// Since we know the image we want will always be near the end of the list, start there and go backwards
	for(int i=_dyld_image_count()-1; i>=0; i--){
		const char* image_name = _dyld_get_image_name(i);

		// Why dlopen doesn't effect _dyld stuff: if an image is already loaded, it returns the existing handle.
		void* probe_handle = dlopen(image_name, RTLD_LAZY);
		dlclose(probe_handle);

		if(hModule == probe_handle){
			int s = strlen(image_name);
			int nCopy = min(nSize,s+1);
			memcpy(pFilename,image_name,nCopy);
			return nCopy;
		}
	}
#elif defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__bsdi__) || defined(__DragonFly__)
	struct link_map *lm=NULL;
	dlinfo(hModule, RTLD_DI_LINKMAP, &lm);
	if(lm && lm->l_name){
		int s = strlen(lm->l_name);
		int nCopy = min(nSize,s+1);
		memcpy(pFilename,lm->l_name,nCopy);
		return nCopy;
	}
#endif
	return 0;
}
#endif
