#include <iostream>
#include <windows.h>
#include <thread>
using namespace std;

// 共享内存的大小
#define SHARE_MEMORY_FILE_SIZE_BYTES (100LL*1024*1024)


int main() {
	// 创建文件
	// https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea
	HANDLE dumpFd = CreateFile(
		L"shared_memory1",			// 名称
		GENERIC_READ | GENERIC_WRITE,	// 权限，读写
		FILE_SHARE_READ | FILE_SHARE_DELETE,	// 其他进程对该内存的权限，可读、可删除
		nullptr,		
		OPEN_ALWAYS,							// 共享内存存在时，将其打开，如果不存在，创建
		FILE_ATTRIBUTE_NORMAL,					// 文件系统中，该共享内存的权限
		nullptr
		);
	if (dumpFd == INVALID_HANDLE_VALUE) {
		cout << "create file error" << endl;
		perror("CreateFile");
		return -1;
	}
	// https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createfilemappinga
	HANDLE sharedFd = CreateFileMapping(
		dumpFd,
		nullptr,
		PAGE_READWRITE,					// 要执行的操作，读写
		0,	
		SHARE_MEMORY_FILE_SIZE_BYTES,	// 要映射的大小，如果不够大，操作系统会将该内存扩充至该大小
		L"shared_memory1"
		);

	if (!sharedFd && false) {
		cout << "CreateFileMapping error" << endl;
		perror("CreateFileMapping");
		return -1;
	}
	else {
		// https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-mapviewoffile
		LPVOID lp_base = MapViewOfFile(
			sharedFd,  // Handle of the map object
			FILE_MAP_READ | FILE_MAP_WRITE,  // Read and write access
			0,                    // High-order DWORD of the file offset
			0,                    // Low-order DWORD of the file offset
			SHARE_MEMORY_FILE_SIZE_BYTES);           // The number of bytes to map to view
		// 要共享的DataFrame
		char msg[] = "index,a,b,c\n0,1,1,1\n1,2,2,2\n2,3,3,3\n";
		int len = strlen(msg);
		char* ptr = (char*)lp_base;
		// 写入4字节，代表后续数据长度
		memcpy(ptr, &len, 4);
		ptr = ptr + 4;
		memcpy(ptr, msg, strlen(msg));
	}
	// 等待Python读取共享内存中的数据
	this_thread::sleep_for(chrono::seconds(100));
}
