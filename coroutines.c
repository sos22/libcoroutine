#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "coroutines.h"

#define DO_REG(name, offset)			        \
	"movq %" name ", " STRINGIFY(offset)"(%rdi)\n"	\
        RESTORE_REG(name, offset)

#define RESTORE_REG(name, offset)			\
	"movq " STRINGIFY(offset) "(%rsi), %" name "\n"

#define STRINGIFY(x) #x
#define STRINGIFY2(x) STRINGIFY(x)
asm ( ".text\n"
      ".globl run_coroutine\n"
      "run_coroutine:\n"
		/* %rdi points at the current routine save area, %rsi
		   points at the target routine, and %rdx contains the
		   value which we're supposed to be returning into the
		   new context. */

                /* Sanity check the supplied coroutines. */

                /* Old must currently be in use */
                "cmpq $0, 104(%rdi)\n"
                "je deactivate_bad_coroutine\n"

                /* New must not be in use */
                "cmpq $0, 104(%rsi)\n"
                "jne activate_bad_coroutine\n"

                /* Update the in_use flags */
                "movq $0, 104(%rdi)\n"
                "movq $1, 104(%rsi)\n"

                /* Set up return value */
                "movq %rdx, %rax\n"

                /* Do the switch */
		DO_REG("rbx", 0)
		DO_REG("rsp", 8)
		DO_REG("rbp", 16)
		DO_REG("r12", 24)
		DO_REG("r13", 32)
		DO_REG("r14", 40)
		DO_REG("r15", 48)
		RESTORE_REG("r9", 96)
		RESTORE_REG("r8", 88)
		RESTORE_REG("rcx", 80)
		RESTORE_REG("rdx", 72)
		RESTORE_REG("rdi", 56) /* Must be after all DO_REG */
		RESTORE_REG("rsi", 64) /* Must be last */
		"ret\n"
      ".previous\n"
);

/* Unusual calling convention: we pop arguments from the stack until
   we see the magic, and then use the next argument as a struct
   coroutine *.  This is necessary because there are varargs on the
   stack in the way, and we don't know how many.  Of course, it won't
   work if one of the arguments happens to be COROUTINE_NAME_MAGIC
   just by coincidence, but there's nothing we can do about that, and,
   since these are only used for debug messages when we're about to
   crash anyway, we just have to put up with it. */
#define COROUTINE_NAME_MAGIC 0xdeadbeef
extern unsigned coroutine_bad_return;

asm ( ".text\n"
      "coroutine_bad_return:"
		"popq %rdi\n"
                "cmpl $" STRINGIFY2(COROUTINE_NAME_MAGIC)", %edi\n"
                "jne coroutine_bad_return\n"
                "popq %rdi\n"
		"jmp coroutine_bad_return_c\n"
      ".previous\n" );

static void
push(struct coroutine *cr, const void *val)
{
	cr->rsp -= 8;
	*(const void **)cr->rsp = val;
}

void
make_coroutine(struct coroutine *out,
	       const char *name,
	       void *stack,
	       unsigned stack_size,
	       void *f,
	       unsigned nr_args,
	       ...)
{
	unsigned x;
	va_list args;

	memset(out, 0, sizeof(*out));
	out->rsp = (unsigned long)(stack + stack_size);
	out->name = name;
	push(out, (void *)COROUTINE_NAME_MAGIC);

	/* Set up arguments */
	va_start(args, nr_args);

	/* Register args */
	if (nr_args >= 1)
		out->rdi = va_arg(args, unsigned long);
	if (nr_args >= 2)
		out->rsi = va_arg(args, unsigned long);
	if (nr_args >= 3)
		out->rdx = va_arg(args, unsigned long);
	if (nr_args >= 4)
		out->rcx = va_arg(args, unsigned long);
	if (nr_args >= 5)
		out->r8 = va_arg(args, unsigned long);
	if (nr_args >= 6)
		out->r9 = va_arg(args, unsigned long);

	/* Stack args */
	if (nr_args > 6) {
		nr_args -= 6;
		out->rsp -= nr_args * 8;
		for (x = 0; x < nr_args; x++)
			((unsigned long *)out->rsp)[x] =
				va_arg(args, unsigned long);
	}

	push(out, &coroutine_bad_return);
	push(out, f);
}

/* Do the minimal initialisation so that the coroutine can be used as
   a source for run_coroutine. */
void
initialise_coroutine(struct coroutine *cr, const char *name)
{
	memset(cr, 0, sizeof(*cr));
	cr->in_use = 1;
	cr->name = name;
}

/* unused == don't generate warning if this is unused, used == emit
   code even if it appears to be unused. */
static void coroutine_bad_return_c(struct coroutine *cr)
	__attribute__((unused, noreturn, used));
static void activate_bad_coroutine(struct coroutine *src, struct coroutine *dest)
	__attribute__((unused, noreturn, used));
static void deactivate_bad_coroutine(struct coroutine *src, struct coroutine *dest)
	__attribute__((unused, noreturn, used));

static void coroutine_bad_return_c(struct coroutine *cr)
{
	/* Do it as two statements so that we get the first message
	 * even if cr turns out to be a bad pointer. */
	fputs("Coroutine returned unexpectedly: ", stderr);
	fputs(cr->name, stderr);
	fputc('\n', stderr);
	abort();
}

static void activate_bad_coroutine(struct coroutine *src, struct coroutine *dest)
{
	fputs("Activated bad coroutine: ", stderr);
	fputs(dest->name, stderr);
	fputs(" from ", stderr);
	fputs(src->name, stderr);
	fputc('\n', stderr);
	abort();
}

static void deactivate_bad_coroutine(struct coroutine *src, struct coroutine *dest)
{
	fputs("Deactivated bad coroutine: ", stderr);
	fputs(src->name, stderr);
	fputs(" for ", stderr);
	fputs(dest->name, stderr);
	fputc('\n', stderr);
	abort();
}
