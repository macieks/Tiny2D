#include <cstdio>
#include "Tiny2D.h"

File* File_Open(const std::string& name, int openMode)
{
   FILE* f = fopen(name.c_str(), openMode == 0 ? "rb" : "wb");
	return (File*) f;
}

void File_Close(File* file)
{
	fclose((FILE*) file);
}

int File_GetSize(File* file)
{
	const long offset = ftell((FILE*) file);
	fseek((FILE*) file, 0, SEEK_END);
	const long size = ftell((FILE*) file);
	fseek((FILE*) file, offset, SEEK_SET);
	return size;
}

void File_Seek(File* file, int offset)
{
	fseek((FILE*) file, offset, SEEK_SET);
}

int File_GetOffset(File* file)
{
	return ftell((FILE*) file);
}

bool File_Read(File* file, void* dst, int size)
{
	return fread(dst, size, 1, (FILE*) file) == 1;
}

bool File_Write(File* file, const void* src, int size)
{
	return fwrite(src, size, 1, (FILE*) file) == 1;
}
