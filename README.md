# report

## 分工

thread：金弘义，庄永昊

userprog：陆嘉馨，赖睿航

vm：金弘义，庄永昊，陆嘉馨，赖睿航

filesys：金弘义，庄永昊，陆嘉馨，赖睿航



## 时间

6/7-6/11完成proj1和proj2 

6/24-7/4完成proj3和proj4



## 架构设计

#### proj1

###### timer_sleep: 

给要休眠的进程记录一个wake_time，放入休眠队列，并调用sema_down阻塞当前进程。每次时钟中断都对休眠队列里的每个进程检查当前时间是否超过wake_time，如果超过则移出休眠队列并且调用sema_up使该进程可以继续被执行。

###### priority：

维护多个优先级队列（包括ready_list,semaphore的等待队列，和conditional variable的等待队列）。ready_list中下一个要运行的线程和等待队列中下一个要唤醒的进程都是优先级最高的线程。 

priority donation：当高优先级线程A由于一个锁被阻塞时，如果锁被低优先级线程B占用，它会把B的优先级提升到A的优先级。优先级会一直随着等待的链向前传递。实现方法：给线程记录一个正在等待的线程lock_waiting_for，这样就形成了一个链，然后当调用lock_acquire时沿着这个链向前更新优先级。另外记录每个线程获得的锁 acquired_locks,当调用lock_release时会根据这些锁的等待队列计算新的优先级

由于priority donation的影响，可能随时会改变线程的优先级，如果每次改变都维护优先级队列可能开销会很大，所以在每次选取优先级最高的线程时都要进行sort。

//todo: bsd scheduler

#### proj2



#### proj3



#### proj4

###### buffer cache:

使用hash+list实现的lru，每个block代表一个扇区。

###### extensible files：

改变inode结构：由一个起始页和多个目录页构成，不要求连续，用扇区地址来链接。起始页记录metadata，目录页记录文件页的扇区地址。扩充文件就是不断申请页，在目录页里添加扇区地址（如果大小不够再申请目录页）

//todo: subdirectory

#### sharing

load_segment时如果遇到只读的页，将它mmap到可执行文件上。

对inode记录打开线程的列表。page fault handler中如果fault_addr对应的supplemental page table entry显示这一页在文件里，就遍历inode的打开线程。如果某一线程中fault_addr在内存某个frame，就在pagedir和supplemental page table把fault_addr指向这个frame。如果没有找到内存中的fault_addr，就从文件里load，并更新pagedir和supplemental page table。

//todo: frame table的改变

## bonus

在vm中实现了sharing： when multiple processes are created that use the same executable file, share read-only pages among those processes instead of creating separate copies of read-only segments for each process.

在开启vm功能的条件下成功运行proj4的文件系统