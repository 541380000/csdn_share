#include <iostream>
#include <sys/mman.h>
#include <fcntl.h>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#include <chrono>
#include <thread>
using namespace std;

int main() {
    // 打开一个文件，在ubuntu中，这个文件会被创建在/dev/shm/下
    int shm_fd = shm_open("shared_memory1", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    const int64_t size = 1024LL * 10 * 1024;
    // 将文件扩展到相应大小，稍后写入数据
    ftruncate(shm_fd, size);
    perror("shm_open");
    // 调用mmap将共享内存映射到本进程的虚拟内存空间，返回指针
    char* ptr = static_cast<char *>(mmap(nullptr, 1024LL * 10 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));
    perror("mmap");
    // 写入数据
    char msg[] = "index,a,b,c\n0,1,1,1\n1,2,2,2\n2,3,3,3\n";
    int len = strlen(msg);
    memcpy(ptr, &len, 4);
    ptr = ptr + 4;
    memcpy(ptr , msg, len);
    // 等待python读取
    this_thread::sleep_for(chrono::seconds(10));
    // 关闭共享内存对象，关闭之后，已经mmap的进程仍然可以使用该块内存，但新的进程无法再进行mmap
    shm_unlink("shared_memory1");
    // 解除映射，解除之后，该共享内存会被删掉
    munmap(ptr-4, 1024LL * 10 * 1024);
}
