
#include <winsock.h>
//#include <sys/mman.h>

#include <sys/types.h>
#include <errno.h>
#pragma comment(lib, "wsock32.lib")
#pragma warning(disable: 4996)

#include <io.h>
#include <windows.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
//#include <unistd.h>
#include <time.h>

#define PROT_READ     0x1
#define PROT_WRITE    0x2
/* This flag is only available in WinXP+ */
#ifdef FILE_MAP_EXECUTE
#define PROT_EXEC     0x4
#else
#define PROT_EXEC        0x0
#define FILE_MAP_EXECUTE 0
#endif

#define MAP_SHARED    0x01
#define MAP_PRIVATE   0x02
#define MAP_ANONYMOUS 0x20
#define MAP_ANON      MAP_ANONYMOUS
#define MAP_FAILED    ((void *) -1)

#ifdef __USE_FILE_OFFSET64
# define DWORD_HI(x) (x >> 32)
# define DWORD_LO(x) ((x) & 0xffffffff)
#else
# define DWORD_HI(x) (0)
# define DWORD_LO(x) (x)
#endif

static void* mmap(void* start, size_t length, int prot, int flags, int fd, off_t offset)
{
	if (prot & ~(PROT_READ | PROT_WRITE | PROT_EXEC))
		return MAP_FAILED;
	if (fd == -1) {
		if (!(flags & MAP_ANON) || offset)
			return MAP_FAILED;
	}
	else if (flags & MAP_ANON)
		return MAP_FAILED;

	DWORD flProtect;
	if (prot & PROT_WRITE) {
		if (prot & PROT_EXEC)
			flProtect = PAGE_EXECUTE_READWRITE;
		else
			flProtect = PAGE_READWRITE;
	}
	else if (prot & PROT_EXEC) {
		if (prot & PROT_READ)
			flProtect = PAGE_EXECUTE_READ;
		else if (prot & PROT_EXEC)
			flProtect = PAGE_EXECUTE;
	}
	else
		flProtect = PAGE_READONLY;

	off_t end = length + offset;
	HANDLE mmap_fd, h;
	if (fd == -1)
		mmap_fd = INVALID_HANDLE_VALUE;
	else
		mmap_fd = (HANDLE)_get_osfhandle(fd);
	h = CreateFileMapping(mmap_fd, NULL, flProtect, DWORD_HI(end), DWORD_LO(end), NULL);
	if (h == NULL)
		return MAP_FAILED;

	DWORD dwDesiredAccess;
	if (prot & PROT_WRITE)
		dwDesiredAccess = FILE_MAP_WRITE;
	else
		dwDesiredAccess = FILE_MAP_READ;
	if (prot & PROT_EXEC)
		dwDesiredAccess |= FILE_MAP_EXECUTE;
	if (flags & MAP_PRIVATE)
		dwDesiredAccess |= FILE_MAP_COPY;
	void* ret = MapViewOfFile(h, dwDesiredAccess, DWORD_HI(offset), DWORD_LO(offset), length);
	if (ret == NULL) {
		CloseHandle(h);
		ret = MAP_FAILED;
	}
	return ret;
}

static void munmap(void* addr, size_t length)
{
	UnmapViewOfFile(addr);
	/* ruh-ro, we leaked handle from CreateFileMapping() ... */
}

#undef DWORD_HI
#undef DWORD_LO

void MMAP_TEST(char* READ, char* WRITE);
void FGET_TEST(char* READ, char* WRITE);
void FREAD_TEST(char* READ, char* WRITE);

int main(int argc, char* argv[])
{
	//MMAP_TEST("read2.txt", "write.txt");
	//FGET_TEST("read2.txt", "write1.txt");
	FREAD_TEST("read2.txt", "write2.txt");
	system("pause");
}

void MMAP_TEST(char* READ, char* WRITE)
{
	int fdin, fdout;
	char* src, * dst;
	struct _stat statbuf;
	clock_t Cur_Time_Start, Cur_Time_Final;
	Cur_Time_Start = clock();
	if ((fdin = open(READ, O_RDONLY)) < 0)
		printf("Error1", READ);
	if ((fdout = open(WRITE, O_RDWR | O_CREAT | O_TRUNC)) < 0)
		printf("Error2", WRITE);
	/*if ((fdout = open("write.txt", O_WRONLY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE)) < 0)
		printf("невозможно создать %s для записи", "write.txt");*/
	if (fstat(fdin, &statbuf) < 0)
		printf("fstat error");
	if (lseek(fdout, statbuf.st_size - 1, SEEK_SET) == -1)
		printf("Error lseek");
	if (write(fdout, "", 1) != 1)
		printf("Error write");
	if ((src = mmap(0, statbuf.st_size, PROT_READ, MAP_SHARED, fdin, 0)) == MAP_FAILED)
		printf("Error mmap");
	if ((dst = mmap(0,
	//Cur_Time_Start = clock();
	/*for (int i = 0; i < statbuf.st_size; i++)
	{
		dst[i] = src[i];
	}*/ statbuf.st_size, PROT_WRITE, MAP_SHARED, fdout, 0)) == MAP_FAILED)
		printf("Error mmap");
	//Cur_Time_Start = clock();
	/*for (int i = 0; i < statbuf.st_size; i++)
	{
		dst[i] = src[i];
	}*/
	memcpy(dst, src, statbuf.st_size);
	close(fdin);
	close(fdout);
	Cur_Time_Final = clock();
	double res = (double)(Cur_Time_Final - Cur_Time_Start) / CLOCKS_PER_SEC;
	printf("%f \n", res);
}

void FGET_TEST(char* READ, char* WRITE)
{
	clock_t Cur_Time_Start, Cur_Time_Final;
	Cur_Time_Start = clock();
	FILE* read = fopen(READ, "rb");
	FILE* write = fopen(WRITE, "wb");
	while (!feof(read))
	{
		char ch = fgetc(read);
		fputc(ch, write);
	}
	fclose(read);
	fclose(write);
	Cur_Time_Final = clock();
	double res = (double)(Cur_Time_Final - Cur_Time_Start) / CLOCKS_PER_SEC;
	printf("%f \n", res);
}

void FREAD_TEST(char* READ, char* WRITE)
{
	clock_t Cur_Time_Start, Cur_Time_Final;
	Cur_Time_Start = clock();
	FILE* read = fopen(READ, "rb");
	FILE* write = fopen(WRITE, "wb");
	while (!feof(read))
	{
		char buffer[256];
		fread(buffer, 1, 256, read);
		fwrite(buffer, 1, 256, write);
		/*char buffer[65536];
		fread(buffer, 1, 65536, read);
		fwrite(buffer, 1, 65536, write);*/
	}
	fclose(read);
	fclose(write);
	Cur_Time_Final = clock();
	double res = (double)(Cur_Time_Final - Cur_Time_Start) / CLOCKS_PER_SEC;
	printf("%f \n", res);
}