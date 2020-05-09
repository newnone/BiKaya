#include "exc.h"
#include "pcb.h"
#include "sched.h"
#include "sem.h"
#include "syscall.h"


/**
 * Terminates the current syscall handling by placing the return value given
 * by @c retval at an appropriate old area register. Code placed after this
 * call is ignored, for a jump to the previously running user-space code is
 * performed.
 * @param retval Return value the currently executing syscall should return.
 */
static void sys_return(unsigned retval);
/**
 * Like @c sys_return(), but performs no jumps and can therefore be followed by
 * any other instruction.
 * @param retval Return value the currently executing syscall should return.
 */
static void sys_return_stay(unsigned retval);

static void sys_cpu_time(unsigned arg1, unsigned arg2, unsigned arg3);
static void sys_fork(unsigned arg1, unsigned arg2, unsigned arg3);
/**
 * Terminates the process identified by @c arg1 and all of its descendant
 * processes. Two cases might occur while invoking this syscall
 *
 * <ul>
 * 		<li>Either the previously running process is amongst the ones to be
 * 		killed</li>
 * 		<li>Or it is not</li>
 * </ul>
 *
 * In the first case, a context-switch is automatically operated at the end
 * of the syscall, with any other instruction following this syscall being
 * ignored. In the second, the execution resumes ordinarily.
 *
 * @param arg1 PCB pointer to the process to terminate. If @c NULL, the
 * previously running process is selected.
 * @param arg2 Unused.
 * @param arg3 Unused.
 * @return @c 0 in case of success, @c -1 otherwise (for example, in cases
 * where @c arg1 identifies an already freed or invalid PCB pointer).
 */
static void sys_kill(unsigned arg1, unsigned arg2, unsigned arg3);
/**
 * Implements the V() operation of classical semaphores. If no semaphore
 * could be identified starting from @c arg1, the kernel will enter @c PANIC
 * mode.
 * @param arg1 To be cast to <tt>(int *)</tt>, identifies the semaphore key.
 * @param arg2 Unused.
 * @param arg3 Unused.
 * @return The updated value of the identified semaphore.
 */
static void sys_verhogen(unsigned arg1, unsigned arg2, unsigned arg3);
/**
 * Implements the P() operation of classical semaphores. If no semaphore
 * could be identified starting from @c arg1, the kernel will enter @c PANIC
 * mode.
 * @param arg1 To be cast to <tt>(int *)</tt>, identifies the semaphore key.
 * @param arg2 Unused.
 * @param arg3 Unused.
 * @return The updated value of the identified semaphore.
 */
static void sys_passeren(unsigned arg1, unsigned arg2, unsigned arg3);
static void sys_iocmd(unsigned arg1, unsigned arg2, unsigned arg3);
static void sys_spec_passup(unsigned arg1, unsigned arg2, unsigned arg3);
static void sys_get_pid(unsigned arg1, unsigned arg2, unsigned arg3);


/**
 * A syscall function type.
 */
typedef void (*syscall_t)(unsigned, unsigned, unsigned);

/**
 * A mapping from syscall ID to the corresponding syscall function pointer.
 */
static syscall_t sys_id_to_syscall[] = {
		sys_cpu_time,
		sys_fork,
		sys_kill,
		sys_verhogen,
		sys_passeren,
		sys_iocmd,
		sys_spec_passup,
		sys_get_pid,
};


void bka_sys_call(unsigned id, unsigned arg1, unsigned arg2, unsigned arg3) {
	/* TODO Shall we perform checks on whether sys_return() is called by
	 * subordinate syscalls? */
	if (BKA_SYS_CPU_TIME <= id && id <= BKA_SYS_GETPID) {
		sys_id_to_syscall[id](arg1, arg2, arg3);
	} else {
		PANIC();
	}
}

void sys_return(unsigned retval) {
	sys_return_stay(retval);
	bka_na_exit(SYSBK_NEWAREA);
}

void sys_return_stay(unsigned retval) {
	state_t *old_area = (state_t *) SYSBK_OLDAREA;

#ifdef BKA_ARCH_UARM
	old_area->a1 = retval;
#elif defined(BKA_ARCH_UMPS)
	old_area->reg_v0 = retval;
#endif
}


void sys_cpu_time(unsigned arg1, unsigned arg2, unsigned arg3) {
	/* TODO Implement. */
	sys_return(-1);
}

void sys_fork(unsigned arg1, unsigned arg2, unsigned arg3) {
	/* TODO Implement. */
	sys_return(-1);
}

void sys_kill(unsigned arg1, unsigned arg2, unsigned arg3) {
	pcb_t *to_kill = ((pcb_t *) arg1) == NULL ? bka_sched_curr: (pcb_t *) arg1;

	switch (bka_sched_kill(to_kill)) {
		case BKA_E_INVARG:
			sys_return(-1);
		case BKA_SCHED_KR:
			sys_return_stay(0);
			bka_sched_resume();
		case 0:
		default:
			sys_return(0);
	}
}

void sys_verhogen(unsigned arg1, unsigned arg2, unsigned arg3) {
	int *semkey = (int *) arg1;
	semd_t *sem = bka_sem_get(semkey);
	pcb_t	*to_restore;

	if (!sem) {
		/* TODO Print out an error message */
		PANIC();
	}

	*(sem->key) += 1;

	/* TODO Le slide mostrano l'inverso di questa condizione, cioè
	 * *(sem->key) <= 0; qual'è la versione corretta? (Direi quella di questo
	 * codice.)
	 */
	if (*(sem->key) >= 0) {
		to_restore = bka_sem_dequeue(semkey);

		if (to_restore)
			bka_sched_enqueue(to_restore);

		sys_return((unsigned) *(sem->key));
	}
}

void sys_passeren(unsigned arg1, unsigned arg2, unsigned arg3) {
	int *semkey = (int *) arg1;
	semd_t *sem = bka_sem_get(semkey);

	if (!sem) {
		/* TODO Print out an error message */
		PANIC();
	}

	*(sem->key) -= 1;

	if (*(sem->key) < 0) {
		/* Suspend process */
		bka_sem_enqueue(sem->key, bka_sched_curr);
		sys_return_stay((unsigned) *(sem->key));

		bka_sched_drop();
		bka_sched_resume();
	} else {
		sys_return((unsigned) *(sem->key));
	}
}

void sys_iocmd(unsigned arg1, unsigned arg2, unsigned arg3) {
	/* TODO Implement. */
	sys_return(-1);
}

void sys_spec_passup(unsigned arg1, unsigned arg2, unsigned arg3) {
	/* TODO Implement. */
	sys_return(-1);
}

void sys_get_pid(unsigned arg1, unsigned arg2, unsigned arg3) {
	/* TODO Implement. */
	sys_return(-1);
}
