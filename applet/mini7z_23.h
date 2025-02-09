static int extract(const char *password,const char *arc, const char *dir, int argc, const char **argv){
	SInStreamFile sin;
	MakeSInStreamFile(&sin,arc);
	void *archiver=NULL;
	unsigned char arctype=0;
	for(;arctype<0xff;arctype++){
		if(!lzmaCreateArchiver(&archiver,arctype,0,0)){
			sin.vt->Seek(&sin,0,SEEK_SET,NULL); // archiver should do this automatically lol lol lol
			if(!lzmaOpenArchive(archiver,&sin,password,arc))break;
			lzmaDestroyArchiver(&archiver,0);
		}
	}
	if(!archiver){
		fprintf(stderr,"7z.so could not open the file as any archive (possibly encrypted).\n");
		sin.vt->Release(&sin);
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

	return 0;
}

static int list(const char *password,const char *arc, int argc, const char **argv){
	SInStreamFile sin;
	MakeSInStreamFile(&sin,arc);
	void *archiver=NULL;
	unsigned char arctype=0;
	for(;arctype<0xff;arctype++){
		if(!lzmaCreateArchiver(&archiver,arctype,0,0)){
			sin.vt->Seek(&sin,0,SEEK_SET,NULL); // archiver should do this automatically lol lol lol
			if(!lzmaOpenArchive(archiver,&sin,password,arc))break;
			lzmaDestroyArchiver(&archiver,0);
		}
	}
	if(!archiver){
		fprintf(stderr,"7z.so could not open the file as any archive (possibly encrypted).\n");
		sin.vt->Release(&sin);
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
			printf("%-40ls %10"LLU" %10"LLU" %s %-20ls\n",propPath.bstrVal,propPackedSize.uhVal.QuadPart,propSize.uhVal.QuadPart,cbuf,propMethod.bstrVal);
		}
		PropVariantClear(&propPath);
		PropVariantClear(&propMethod);
	}
	printf("---------------------------------------- ---------- ---------- ------------------- --------------------\n");
	lzmaDestroyArchiver(&archiver,0);
	sin.vt->Release(&sin);

	return 0;
}

static int add(const char *password,unsigned char arctype,int level,const char *arc, int argc, const char **argv){
	void *archiver=NULL;
	if(lzmaCreateArchiver(&archiver,arctype,1,level)){
		fprintf(stderr,"7z.so does not have the encoder for this file type.\n");
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
	lzmaUpdateArchive(archiver,&sout,argc,&supdate);
	lzmaDestroyArchiver(&archiver,1);
	sout.vt->Release(&sout);

	return 0;
}
