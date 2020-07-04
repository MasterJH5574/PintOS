# PintOS Report

## 分工

Project1 - Threads：金弘义，庄永昊

Project2 - User Proggram：陆嘉馨，赖睿航

Project3 - Virtual Memory：金弘义，庄永昊，陆嘉馨，赖睿航

Project4 - File System：金弘义，庄永昊，陆嘉馨，赖睿航

Bonus - ELF Sharing: 金弘义，庄永昊，赖睿航



## 时间

6/7-6/11 完成 proj1 和 proj2

6/24-6/28 完成 proj3 和 proj4

7/2-7/4 完成 bonus - ELF Sharing




## 架构设计

### Project1 - Threads


#### Function timer_sleep()

给要休眠的进程记录一个 wake time ，放入休眠队列，并调用 `sema_down()` 阻塞当前进程。每次时钟中断都对休眠队列里的每个进程检查当前时间是否超过 wake time，如果超过则移出休眠队列并且调用 `sema_up()` 使该进程可以继续被执行。

#### Priority

维护多个优先级队列（包括 `ready_list` , semaphore 的等待队列，和 conditional variable 的等待队列）。`ready_list` 中下一个要运行的线程和等待队列中下一个要唤醒的进程都是优先级最高的线程。 

Priority donation：当高优先级线程 A 由于一个锁被阻塞时，如果锁被低优先级线程 B 占用，它会把 B 的优先级提升到 A 的优先级。优先级会一直随着等待的链向前传递。实现方法：给线程记录一个正在等待的线程 `lock_waiting_for` ，这样就形成了一个链，然后当调用 `lock_acquire()` 时沿着这个链向前更新优先级。另外记录每个线程获得的锁 `acquired_locks` ,当调用 `lock_release()` 时会根据这些锁的等待队列计算新的优先级

由于 priority donation 的影响，可能随时会改变线程的优先级，如果每次改变都维护优先级队列可能开销会很大，所以在每次选取优先级最高的线程时都要进行 sort。

#### BSD Scheduler




### Project2

#### Argument Passing

具体实现：用给定的 `strtok_r` 函数对读入命令行进行分割，并按照 PintOS 文档 Section 4.5.1 [Program Startup Details], page 42 中给出的栈的设置方式，在 `setup_stack()` 成功后，将对应的信息放入栈的对应位置中。

#### System Calls

* 由于用户传入的 syscall 参数的地址是在 user space，所以要取出 syscall number 和用户传入的 syscall 参数，可能需要 dereference `esp`, `esp + 4`, `esp + 8` 和 `esp + 12`。为了避免 kernel thread 触发 page fault，需要检查 syscall 的参数所对应的 user address 检查合法性，如果不合法则直接调用 `exit(-1)` 结束线程。
* 完成 proj2 的过程中尚未考虑要固定 syscall 参数所在页，这一操作在 proj3 的实现中得到完成。
* 为保证线程安全，需要用 `filesys_lock` 来保证任意时刻在进行文件系统操作的线程最多只有一个。

具体实现：
* `sys_halt` 直接调用 `shutdown_power_off()`。
* `sys_exit` 将参数 `status` 存入当前线程的 `exit_code` 成员，再调用 `thread_exit()`。在 `sys_exit()` ， `thread_exit()` 和 `process_exit()` 里实现了对线程所占用的一些资源的释放。
* `sys_exec` 直接调用 `process_execute()`。
* `sys_wait`
* `sys_create` 和 `sys_remove` 直接调用 `filesys_create()`  和 `filesys_remove()`。
* `sys_open` 先调用 `filesys_open()`，再为当前进程新开一个 `file_descriptor` 表示该线程打开了这个文件。
* `sys_filesize` 在检查 `fd` 是否合法后直接调用 `file_length()`。
* `sys_read` 检查 `fd`，若 `fd` 为 0 则反复调用 `input_getc()` 读取输入，`fd` 为其它合法情况则调用 `file_read`。
* `sys_write` 与 `sys_read` 类似，只是若 `fd` 为 1 则调用 `putbuf()`。
* `sys_seek` 和 `sys_tell` 在检查 `fd` 合法后分别调用 `file_seek()` 和 `file_tell()`。
* `sys_close` 在检查 `fd` 后调用 `file_close()`，再将对应的 `file_descriptor`  从当前线程的相应列表中移除。

### Project3

#### Frame



#### Supplemental Page Table

核心为 `struct page_table_entry` ，表示 supplemental page table entry。

```c
struct page_table_entry {
  void *upage;                            /* As the key of page table. */
  enum page_status status;                /* Show where the page is. */
  bool writable;                          /* Whether this page is writable. */
  bool pinned;                            /* Whether the page is pinned. */

  void *frame;                            /* If status == FRAME. */

  block_sector_t swap_index;              /* If status == SWAP. */

  struct file *file;                      /* If the page comes from a file. */
  off_t file_offset;
  uint32_t read_bytes;
  uint32_t zero_bytes;

  struct hash_elem elem;
};
```

每个线程维护一个哈希表作为 supplemental page table，从一个 upage 映射到一个 spte。

正常情况下，用户态线程进行内存访问会直接走 pagedir，不会用到 spt。但是当一个进程触发 page fault 时，需要知道对应的页是在文件中还是在 swap 中，因此需要 *supplemental* pte 记录这些信息。

同样，在进行 mmap/munmap 调用和将一个 frame 替换到 swap/file 时，也需要用到 spt。

#### Swap

使用系统提供的`block`接口，并用`block`的序号作为索引进行swap操作。

swap通过链表维护swap在load时空出的块，使其在下一次store时可以继续使用。

#### Eviction



#### Page Fault Handler

1. 区分触发 page fault 的 `fault_addr` 是否需要栈增长。
2. 如果需要栈增长则让栈增长一个页；否则查看对应的 spte，去到 swap/file 中将数据读入。

#### VM System Calls

##### Pin

为了避免 syscall 参数所在的页在 syscall 运行过程中被替换出内存，进而导致运行 syscall 的 kernel 触发 page fault 造成麻烦，需要在执行 syscall 之前将参数所在的页读进内存（如果本来不在的话）再固定住。固定住一个页之后，这个页在取消固定之前不会被替换到 swap/file 中。

当 syscall 执行完毕时，取消固定。

##### MMAP/MUNMAP

* `sys_mmap` 不直接把文件读入内存，而是只建好想应的 spte，等到用户尝试访问而触发 page fault 时再将文件读入。
* `sys_munmap` 移除该线程所对应的 mmap 信息和 spte 即可。移除时如果对应的页表现为 dirty，则需要写回文件。同样，在一个线程退出时也需要将 dirty 的 mmap 页写回文件。

### Project4

#### Buffer Cache

使用 hash + list 实现的 LRU，每个 block 代表一个扇区。

#### Extensible Files

改变 inode 结构：由一个起始页和多个目录页构成，不要求连续，用扇区地址来链接。起始页记录 metadata ，目录页记录文件页的扇区地址。扩充文件就是不断申请页，在目录页里添加扇区地址（如果大小不够再申请目录页）

#### Subdirectory



#### File System System Calls

* 修改 `sys_open` ， `sys_close` 使其分别能打开、关闭一个目录。
* 修改其它文件相关的 system calls 使其能拒绝对目录进行操作。
* `sys_chdir` 找到并打开目标目录，将当前线程所记录的当前目录改为目标目录，最后关闭原目录。
* `sys_mkdir` 在 parse 路径后检查被创建的目录是否为根目录，不是根目录的话则调用 `subdir_create()`。
* `sys_readdir` 检查对应的 file descriptor 是否表示目录，再直接调用 `dir_readdir()`。
* `sys_isdir` 直接检查对应的 file descriptor 是否表示目录。
* `sys_inumber` 调用 `inode_get_inumber()` 获取一个 inode 对应的 inumber。

### Bonus

#### Sharing

在 VM 中实现了 sharing：

> When multiple processes are created that use the same executable file, share read-only pages among those processes instead of creating separate copies of read-only segments for each process.

------

`load_segment()` 时如果遇到只读的 segment，则将 user space 中对应的若干个页映射到这个 segment。

对 inode 记录打开线程的列表。page fault handler 中如果 `fault_addr` 对应的 spte 显示这一页在文件里，就遍历 inode 的打开线程。如果某一线程中 `fault_addr` 在内存某个frame，就在 pagedir 和 spt 把 `fault_addr` 指向这个 frame。如果没有找到内存中的 `fault_addr`，就从文件里读入，并更新 pagedir 和 supplemental page table。

//todo: frame table的改变

#### Run File System Test Cases with Virtual Memory

在开启 VM 功能的条件下成功运行 proj4 的文件系统。