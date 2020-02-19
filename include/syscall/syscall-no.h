#pragma once

typedef enum {
    SYSCALL_EXIT = 0,
    SYSCALL_YIELD,
    SYSCALL_PUTS,
    SYSCALL_GETPID,
    SYSCALL_TIME,
    SYSCALL_SLEEP,
    SYSCALL_MSLEEP,
    SYSCALL_USLEEP,
    SYSCALL_SET_PRIORITY,
    SYSCALL_SET_PROCESS_NAME,
    SYSCALL_READLINE,

    SYSCALL_LEN,
} syscall_t;
