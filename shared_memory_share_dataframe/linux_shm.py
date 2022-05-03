import mmap
from io import StringIO
from struct import unpack
import pandas as pd

# 注意这里写的大小，一定要不大于C++中开启的共享内存大小，否则映射失败
SHARE_MEMORY_FILE_SIZE_BYTES = 100*1024
f = open("/dev/shm/shared_memory1",'r+b')
fd = f.fileno()
print(fd)
mmap_file = mmap.mmap(fd, 0)
buffer_size = unpack("i", mmap_file.read(4))
print(f"Buffer size: {buffer_size}")

ex_factor = pd.read_csv(StringIO(mmap_file.read(buffer_size[0]).decode()), index_col=0)
print(ex_factor)
