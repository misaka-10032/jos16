/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)                           \
	.globl name;		/* define global symbol for 'name' */	 \
	.type name, @function;	/* symbol type is function */  \
	.align 2;		/* align function definition */            \
	name:			/* function starts here */                   \
	pushl $(num);                                          \
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)   \
	.globl name;                        \
	.type name, @function;              \
	.align 2;                           \
	name:                               \
	pushl $0;                           \
	pushl $(num);                       \
	jmp _alltraps

#define TRAPHANDLER_IRQ(name, num)    \
	.globl name;                        \
	.type name, @function;              \
	.align 2;                           \
	name:                               \
	pushl $0;                           \
	pushl $(IRQ_OFFSET+num);            \
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 * https://pdos.csail.mit.edu/6.828/2016/readings/i386/s09_08.htm
 * th = trap handler
 */

TRAPHANDLER_NOEC(th_divide , T_DIVIDE )
TRAPHANDLER_NOEC(th_debug  , T_DEBUG  )
TRAPHANDLER_NOEC(th_nmi    , T_NMI    )
TRAPHANDLER_NOEC(th_brkpt  , T_BRKPT  )
TRAPHANDLER_NOEC(th_oflow  , T_OFLOW  )
TRAPHANDLER_NOEC(th_bound  , T_BOUND  )
TRAPHANDLER_NOEC(th_illop  , T_ILLOP  )
TRAPHANDLER_NOEC(th_device , T_DIVIDE )
TRAPHANDLER_NOEC(th_fperr  , T_FPERR  )
TRAPHANDLER_NOEC(th_mchk   , T_MCHK   )
TRAPHANDLER_NOEC(th_simderr, T_SIMDERR)
TRAPHANDLER_NOEC(th_syscall, T_SYSCALL)

TRAPHANDLER(th_dblflt, T_DBLFLT)
TRAPHANDLER(th_tss   , T_TSS   )
TRAPHANDLER(th_segnp , T_SEGNP )
TRAPHANDLER(th_stack , T_STACK )
TRAPHANDLER(th_gpflt , T_GPFLT )
TRAPHANDLER(th_pgflt , T_PGFLT )
TRAPHANDLER(th_align , T_ALIGN )

TRAPHANDLER_IRQ(th_timer   , IRQ_TIMER   )
TRAPHANDLER_IRQ(th_kbd     , IRQ_KBD     )
TRAPHANDLER_IRQ(th_serial  , IRQ_SERIAL  )
TRAPHANDLER_IRQ(th_spurious, IRQ_SPURIOUS)
TRAPHANDLER_IRQ(th_ide     , IRQ_IDE     )
TRAPHANDLER_IRQ(th_error   , IRQ_ERROR   )

/*
 * Lab 3: Your code here for _alltraps
 */

_alltraps:

  // push in the reverse order as defined in trap.h
  // all after tf_trapno are pushed in macro and by hardware.
  pushw  $0x0  // tf_padding2
  pushw  %ds   // tf_ds
  pushw  $0x0  // tf_padding1
  pushw  %es   // tf_es
  pushal       // tf_regs

  // load GD_KD (GDT selector) into %ds and %es
  movl   $GD_KD, %eax
  movw   %ax, %ds
  movw   %ax, %es

  // pretend to have a pointer to Trapframe as argument
  pushl  %esp
  call   trap

