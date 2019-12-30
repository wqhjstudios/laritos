#include <log.h>

#include <stdbool.h>
#include <core.h>
#include <process/core.h>
#include <utils/assert.h>
#include <dstruct/list.h>
#include <sync/condition.h>
#include <sched/core.h>

int condition_init(condition_t *cond) {
    INIT_LIST_HEAD(&cond->blocked);
    return 0;
}

static void condition_block_current_process(condition_t *cond) {
    pcb_t *pcb = process_get_current();

    assert(list_empty(&pcb->sched.blockedlst), "A process can only be waiting for max 1 event");
    // TODO Order list by process priority
    list_add_tail(&pcb->sched.blockedlst, &cond->blocked);

    irqctx_t ctx2;
    spinlock_acquire(&_laritos.proclock, &ctx2);
    sched_move_to_blocked_locked(pcb);
    spinlock_release(&_laritos.proclock, &ctx2);

    verbose_async("pid=%u waiting for condition=0x%p", pcb->pid, cond);
}

void condition_wait_locked(condition_t *cond, spinlock_t *spin, irqctx_t *ctx) {
    condition_block_current_process(cond);

    spinlock_release(spin, ctx);
    schedule();
    spinlock_acquire(spin, ctx);
}

static inline void wakeup_pcb(pcb_t *pcb, condition_t *cond) {
    if (pcb == NULL) {
        return;
    }

    list_del_init(&pcb->sched.blockedlst);

    verbose_async("Waking up pid=%u waiting for condition=0x%p", pcb->pid, cond);
    irqctx_t ctx2;
    spinlock_acquire(&_laritos.proclock, &ctx2);
    sched_move_to_ready_locked(pcb);
    spinlock_release(&_laritos.proclock, &ctx2);
}

pcb_t *condition_notify_locked(condition_t *cond) {
    pcb_t *pcb = list_first_entry_or_null(&cond->blocked, pcb_t, sched.blockedlst);
    wakeup_pcb(pcb, cond);
    return pcb;
}

bool condition_notify_all_locked(condition_t *cond) {
    pcb_t *pcb;
    pcb_t *tmp;
    bool proc_awakened = false;
    list_for_each_entry_safe(pcb, tmp, &cond->blocked, sched.blockedlst) {
        wakeup_pcb(pcb, cond);
        proc_awakened = true;
    }
    return proc_awakened;
}
