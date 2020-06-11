## Project 2: User Program

### Tasks

- [ ] Argument passing [ljx]
- [ ] User memory access [lrh]
- [ ] System call infrastructure
- [ ] The `exit` system call
- [ ] The `write` system call for writing to fd 1, the system console
- [ ] For now, change `process_wait()` to an infinite loop

### System Calls

- [ ] `void halt(void)`
- [ ] `void exit(int status)`
- [ ] `pid_t exec(const char *cmd_line)`
- [ ] `int wait(pid_t pid)`
- [ ] `bool create(const char *file, unsigned initial_size)`
- [ ] `bool remove(const char *file)`
- [ ] `int open(const char *file)`
- [ ] `int filesize(int fd)`
- [ ] `int read(int fd, void *buffer, unsigned size)`
- [ ] `int write(int fd, const void *buffer, unsigned size)`
- [ ] `void seek(int fd, unsigned position)`
- [ ] `unsigned tell(int fd)`
- [ ] `void close(int fd)`