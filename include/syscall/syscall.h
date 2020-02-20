#pragma once

#include <syscall/syscall-no.h>
#include <time/core.h>
#include <stdint.h>
#include <generated/autoconf.h>

#ifdef CONFIG_CPU_32_BITS
#include <syscall/syscall32.h>
#else
#include <syscall/syscall64.h>
#endif

void syscall_exit(int status);
int syscall_yield(void);
int syscall_puts(const char *str);
int syscall_getpid(void);
int syscall_time(time_t *t);
int syscall_sleep(uint32_t secs);
int syscall_msleep(uint32_t msecs);
int syscall_usleep(uint32_t usecs);
int syscall_set_priority(uint8_t priority);
int syscall_set_process_name(char *name);
int syscall_readline(char *buf, int buflen);
int syscall_getc(void);
int syscall_backdoor(char *command, void *arg);
