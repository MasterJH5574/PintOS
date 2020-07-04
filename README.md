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



#### proj2

##### Argument Passing

用给定的`strtok_r`函数对读入命令行进行分割，并按照pintos文档 Section 4.5.1 [Program Startup Details], page 42 中给出的栈的设置方式，
在setup_stack成功后，将对应的信息放入栈的对应位置中。

#### proj3

##### swap

使用系统提供的`block`接口，并用`block`的序号作为索引进行操作。

swap并通过链表维护swap在load时空出的块，使其在下一次store时可以继续使用。


#### proj4



## bonus