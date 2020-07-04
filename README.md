# PintOS Report

## 分工

Project1 - Thread：金弘义，庄永昊

Project2 - User Proggram：陆嘉馨，赖睿航

Project3 - Virtual Memory：金弘义，庄永昊，陆嘉馨，赖睿航

Project4 - File System：金弘义，庄永昊，陆嘉馨，赖睿航

Bonus - ELF Sharing: 金弘义，庄永昊，赖睿航



## 时间

6/7-6/11 完成 proj1 和 proj2

6/24-6/28 完成 proj3 和 proj4

7/2-7/4 完成 bonus




## 架构设计

### Project1


#### timer_sleep: 

给要休眠的进程记录一个wake_time，放入休眠队列，并调用sema_down阻塞当前进程。每次时钟中断都对休眠队列里的每个进程检查当前时间是否超过wake_time，如果超过则移出休眠队列并且调用sema_up使该进程可以继续被执行。

#### Priority：

维护多个优先级队列（包括ready_list,semaphore的等待队列，和conditional variable的等待队列）。ready_list中下一个要运行的线程和等待队列中下一个要唤醒的进程都是优先级最高的线程。 

priority donation：当高优先级线程A由于一个锁被阻塞时，如果锁被低优先级线程B占用，它会把B的优先级提升到A的优先级。优先级会一直随着等待的链向前传递。实现方法：给线程记录一个正在等待的线程lock_waiting_for，这样就形成了一个链，然后当调用lock_acquire时沿着这个链向前更新优先级。另外记录每个线程获得的锁 acquired_locks,当调用lock_release时会根据这些锁的等待队列计算新的优先级

由于priority donation的影响，可能随时会改变线程的优先级，如果每次改变都维护优先级队列可能开销会很大，所以在每次选取优先级最高的线程时都要进行sort。

//todo: bsd scheduler



### Project2

#### 1. Argument Passing

具体实现：

#### 2. System Calls

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



### Project4

#### buffer cache:

使用hash+list实现的lru，每个block代表一个扇区。

#### extensible files：

改变inode结构：由一个起始页和多个目录页构成，不要求连续，用扇区地址来链接。起始页记录metadata，目录页记录文件页的扇区地址。扩充文件就是不断申请页，在目录页里添加扇区地址（如果大小不够再申请目录页）

//todo: subdirectory



### Bonus: ELF Sharing

在 VM 中实现了 sharing：

> When multiple processes are created that use the same executable file, share read-only pages among those processes instead of creating separate copies of read-only segments for each process.

------

load_segment 时如果遇到只读的页，将这个页 mmap 到指定的 upage 上。

对 inode 记录打开线程的列表。page fault handler中如果fault_addr对应的supplemental page table entry显示这一页在文件里，就遍历inode的打开线程。如果某一线程中fault_addr在内存某个frame，就在pagedir和supplemental page table把fault_addr指向这个frame。如果没有找到内存中的fault_addr，就从文件里load，并更新pagedir和supplemental page table。

//todo: frame table的改变

### 



在开启vm功能的条件下成功运行proj4的文件系统。