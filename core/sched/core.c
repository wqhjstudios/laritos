//#define DEBUG
#include <log.h>

#include <stdbool.h>

#include <cpu/core.h>
#include <process/core.h>
#include <process/validation.h>
#include <sched/core.h>
#include <sched/context.h>
#include <utils/assert.h>
#include <utils/debug.h>
#include <process/status.h>
#include <component/sched.h>
#include <mm/spprot.h>
#include <mm/exc-handlers.h>
#include <sync/spinlock.h>
#include <sync/atomic.h>

/**
 * NOTE: Must be called with irqs disabled
 */
static void sched_switch_to_locked(pcb_t *from, pcb_t *to) {
    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);

    sched_move_to_running_locked(to);

    spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);


    context_save_and_restore(from, to);

    // Once the *from* context is restored, it will continue execution from
    // here (actually from within the context_save_and_restore() function)

    insane_async("Resuming execution of pid=%u", process_get_current()->pid);
    // Check if the stack has been corrupted
    spprot_check(process_get_current());

#ifdef DEBUG
    debug_dump_processes();
#endif
}

/**
 * NOTE: Must be called with irqs disabled
 */
static void context_switch_locked(pcb_t *cur, pcb_t *to) {
    insane_async("Context switch pid=%u -> pid=%u", cur->pid, to->pid);

    // Update context switch stats
    atomic32_inc(&_laritos.stats.ctx_switches);

    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);

    // Check whether the process is actually running (i.e. not a zombie)
    if (cur->sched.status == PROC_STATUS_RUNNING) {
        sched_move_to_ready_locked(cur);
    }

    spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);

    sched_switch_to_locked(cur, to);
}

void schedule(void) {
    irqctx_t ctx;
    irq_disable_local_and_save_ctx(&ctx);

    cpu_t *c = cpu();
    pcb_t *curpcb = process_get_current();
    pcb_t *pcb = c->sched->ops.pick_ready_locked(c->sched, c, curpcb);;

    while (pcb != NULL && pcb != curpcb && !process_is_valid_context(pcb, pcb->mm.sp_ctx)) {
        exc_dump_process_info(pcb);
        error_async("Cannot switch to pid=%u, invalid context. Killing process...", pcb->pid);
        process_kill(pcb);
        pcb = c->sched->ops.pick_ready_locked(c->sched, c, curpcb);;
    }

    // If the current process is running and:
    //      - there is no other pcb ready,
    //      - or there is another pcb ready but with lower priority (i.e. higher number),
    // then continue execution of the current process
    irqctx_t pcbdata_ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &pcbdata_ctx);

    if (curpcb->sched.status == PROC_STATUS_RUNNING) {
        if (pcb == NULL || (curpcb->sched.priority < pcb->sched.priority)) {
            spinlock_release(&_laritos.proc.pcbs_data_lock, &pcbdata_ctx);
            irq_local_restore_ctx(&ctx);
            return;
        }
    }

    spinlock_release(&_laritos.proc.pcbs_data_lock, &pcbdata_ctx);

    assert(pcb != NULL, "No process ready for execution, where is the idle process?");
    if (curpcb != pcb) {
        context_switch_locked(curpcb, pcb);
    }

    irq_local_restore_ctx(&ctx);
}

void sched_execute_first_system_proc(pcb_t *pcb) {
    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);
    // Execute the first process
    sched_move_to_running_locked(pcb);
    spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);

    context_restore(pcb);
    // Execution will never reach this point
}
