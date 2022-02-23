make clean

echo "Win32"
WIN32=1 ARCH="-mmmx -msse -msse2 -pthread" LIBS="-lole32 -loleaut32" PREFIX=i686-w64-mingw32- SUFF=.exe make
make clean

echo "Win64"
WIN32=1 ARCH="-march=core2 -mfpmath=sse -mmmx -msse -msse2 -msse3 -mno-ssse3 -flto -pthread" LIBS="-lole32 -loleaut32" PREFIX=x86_64-w64-mingw32- SUFF=_amd64.exe make
make clean
