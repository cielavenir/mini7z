make clean

echo "Win32"
#WIN32=1 ARCH="-mmmx -flto -pthread" LIBS="-Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive,-Bdynamic -lole32 -loleaut32" PREFIX=i686-w64-mingw32- SUFF=.exe make
#-flto -msse2 is broken on MinGW...
ZLIBNG_X86=1 WIN32=1 ARCH="-mmmx -msse -msse2 -pthread" LIBS="-Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive,-Bdynamic -lole32 -loleaut32" PREFIX=i686-w64-mingw32- SUFF=.exe make
make clean

echo "Win64"
ZLIBNG_X86=1 WIN32=1 ARCH="-march=core2 -mfpmath=sse -mmmx -msse -msse2 -msse3 -mno-ssse3 -flto -pthread" LIBS="-Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive,-Bdynamic -lole32 -loleaut32" PREFIX=x86_64-w64-mingw32- SUFF=_amd64.exe make
make clean
