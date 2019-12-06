#define DEBUG
#include <log.h>

#include <dstruct/list.h>
#include <cpu.h>
#include <core.h>
#include <mm/slab.h>
#include <mm/heap.h>
#include <process/status.h>
#include <process/pcb.h>
#include <sched/core.h>
#include <sched/context.h>
#include <generated/autoconf.h>

int pcb_init_global_context(void) {
    INIT_LIST_HEAD(&_laritos.proc.pcbs);
    INIT_LIST_HEAD(&_laritos.sched.ready_pcbs);
    INIT_LIST_HEAD(&_laritos.sched.blocked_pcbs);
    INIT_LIST_HEAD(&_laritos.sched.zombie_pcbs);

    _laritos.proc.pcb_slab = slab_create(CONFIG_PROCESS_MAX_CONCURRENT_PROCS, sizeof(pcb_t));
    return _laritos.proc.pcb_slab != NULL ? 0 : -1;
}

int pcb_deinit_global_context(void) {
    slab_destroy(_laritos.proc.pcb_slab);
    return 0;
}

void pcb_assign_pid(pcb_t *pcb) {
    pcb->pid = (uint16_t) slab_get_slab_position(_laritos.proc.pcb_slab, pcb);
}

pcb_t *pcb_alloc(void) {
    pcb_t *pcb = slab_alloc(_laritos.proc.pcb_slab);
    if (pcb != NULL) {
        memset(pcb, 0, sizeof(pcb_t));
        INIT_LIST_HEAD(&pcb->sched.pcb_node);
        INIT_LIST_HEAD(&pcb->sched.sched_node);
        pcb->sched.status = PCB_STATUS_NOT_INIT;
        pcb_set_name(pcb, "?");
    }
    return pcb;
}

int pcb_free(pcb_t *pcb) {
    if (pcb->sched.status != PCB_STATUS_NOT_INIT) {
        error_async("You can only free a process in PCB_STATUS_NOT_INIT state");
        return -1;
    }
    verbose_async("Freeing %s process with pid=%u", pcb->kernel ? "kernel" : "user", pcb->pid);
    // Free process image allocated in the OS heap
    free(pcb->mm.imgaddr);
    // Free pcb structure allocated in the pcb slab
    slab_free(_laritos.proc.pcb_slab, pcb);
    return 0;
}

int pcb_register(pcb_t *pcb) {
    pcb_assign_pid(pcb);
    debug_async("Registering process with pid=%u, priority=%u", pcb->pid, pcb->sched.priority);
    // TODO Mutex
    list_add_tail(&pcb->sched.pcb_node, &_laritos.proc.pcbs);
    sched_move_to_ready(pcb);
    return 0;
}

int pcb_unregister(pcb_t *pcb) {
    if (pcb->sched.status != PCB_STATUS_NOT_INIT && pcb->sched.status != PCB_STATUS_ZOMBIE) {
        error_async("You can only unregister a process in either PCB_STATUS_NOT_INIT or PCB_STATUS_ZOMBIE state");
        return -1;
    }
    debug_async("Un-registering process with pid=%u", pcb->pid);
    // TODO Mutex
    list_del(&pcb->sched.pcb_node);
    if (pcb->sched.status == PCB_STATUS_ZOMBIE) {
        sched_remove_from_zombie(pcb);
    }
    return pcb_free(pcb);
}

spctx_t *pcb_get_current_pcb_stack_context(void) {
    pcb_t *pcb = pcb_get_current();
    return pcb != NULL ? pcb->mm.sp_ctx : 0;
}

void pcb_kill(pcb_t *pcb) {
    verbose_async("Killing process pid=%u", pcb->pid);
    sched_move_to_zombie(pcb);
}

void pcb_kill_and_schedule(pcb_t *pcb) {
    pcb_kill(pcb);
    schedule();
}

int pcb_set_priority(pcb_t *pcb, uint8_t priority) {
    if (!pcb->kernel && priority < CONFIG_SCHED_PRIORITY_MAX_USER) {
        error_async("Invalid priority for a user process, max priority = %u", CONFIG_SCHED_PRIORITY_MAX_USER);
        return -1;
    }

    debug_async("Setting priority for process at 0x%p to %u", pcb, priority);
    uint8_t prev_prio = pcb->sched.priority;
    pcb->sched.priority = priority;

    // If the process is in the ready queue, then reorder it based on the new priority
    if (pcb->sched.status == PCB_STATUS_READY) {
        sched_move_to_ready(pcb);
    } else if (pcb->sched.status == PCB_STATUS_RUNNING && priority > prev_prio) {
        // If the process is running and it now has a lower priority (i.e. higher number), then re-schedule.
        // There may be now a higher priority ready process
        _laritos.sched.need_sched = true;
    }

    return 0;
}

static void kernel_main_wrapper(kproc_main_t main, void *data) {
    verbose_async("Starting kernel_main_wrapper(main=0x%p, data=0x%p)", main, data);
    pcb_t *pcb = pcb_get_current();
    pcb->exit_status = main(data);
    info_async("Exiting kernel process pid=%u, exitcode=%d", pcb->pid, pcb->exit_status);
    pcb_kill_and_schedule(pcb);

    // Execution will never reach this point
}

pcb_t *pcb_spawn_kernel_process(char *name, kproc_main_t main, void *data, uint32_t stacksize, uint8_t priority) {
    debug_async("Spawning kernel process (main=0x%p, data=0x%p, stacksize=%lu, prio=%u)", main, data, stacksize, priority);

    // Allocate PCB structure for this new process
    pcb_t *pcb = pcb_alloc();
    if (pcb == NULL) {
        error_async("Could not allocate space for PCB");
        goto error_pcb;
    }

    pcb_set_name(pcb, name);
    pcb->kernel = true;

    verbose_async("Allocating %lu bytes for kernel process stack", stacksize);
    // Since we only allocate space for the stack (for kernel processes), imgaddr is
    // just a pointer to the stack
    if ((pcb->mm.imgaddr = malloc(stacksize)) == NULL) {
        error_async("Couldn't allocate memory for kernel process at 0x%p", pcb);
        goto error_imgalloc;
    }
    pcb->mm.stack_bottom = pcb->mm.imgaddr;
    pcb->mm.stack_size = stacksize;
    pcb->mm.sp_ctx = (spctx_t *) ((char *) pcb->mm.stack_bottom + pcb->mm.stack_size - 8);
    pcb->mm.sp_ctx_prev = pcb->mm.sp_ctx;
    debug_async("Kernel process stack located at 0x%p, size=%lu", pcb->mm.imgaddr, pcb->mm.stack_size);

    context_init(pcb, kernel_main_wrapper, CPU_MODE_SUPERVISOR);

    // Set arguments for the kernel_main_wrapper
    arch_context_set_first_arg(pcb->mm.sp_ctx, main);
    arch_context_set_second_arg(pcb->mm.sp_ctx, data);

    pcb_set_priority(pcb, priority);

    if (pcb_register(pcb) < 0) {
        error_async("Could not register process loaded at 0x%p", pcb);
        goto error_pcbreg;
    }

    return pcb;

error_pcbreg:
error_imgalloc:
    pcb_free(pcb);
error_pcb:
    return NULL;
}
