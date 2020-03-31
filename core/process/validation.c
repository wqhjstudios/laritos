/**
 * MIT License
 * Copyright (c) 2020-present Leandro Zungri
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <log.h>

#include <process/core.h>
#include <process/validation.h>


bool process_is_valid_kernel_exec_addr_locked(void *addr) {
    extern void *__text_start[];
    extern void *__text_end[];
    return addr >= (void *) __text_start && addr < (void *) __text_end;
}

bool process_is_valid_exec_addr_locked(pcb_t *pcb, void *addr) {
    return addr >= pcb->mm.text_start && (char *) addr < (char *) pcb->mm.text_start + pcb->mm.text_size;
}

bool process_is_valid_context_locked(pcb_t *pcb, spctx_t *ctx) {
    // TODO Add more validations (e.g. mode, lr)
    void *retaddr = arch_context_get_retaddr(ctx);

    // If it is a kernel process or it is currently running in kernel mode, then
    // validate if the address is within the kernel text address space
    if (pcb->kernel || arch_context_is_kernel(ctx)) {
        return process_is_valid_kernel_exec_addr_locked(retaddr);
    }
    return process_is_valid_exec_addr_locked(pcb, retaddr);
}
