#include <stdint.h>

// Register structure in PUSHAD+PUSHFD order at the beginning of a function
struct x86Reg {
	size_t flags;
	size_t edi;
	size_t esi;
	size_t ebp;
	size_t esp;
	size_t ebx;
	size_t edx;
	size_t ecx;
	size_t eax;
	size_t retaddr;
};

/**
  * Breakpoint function type.
  * As these are looked up without any manual registration, the name of a
  * breakpoint function *must* be prefixed with "BP_".
  *
  * Parameters
  * ----------
  *	x86_reg_t *regs
  *		x86 general purpose registers at the time of the breakpoint.
  *		Can be read and written.
  *
  *	void *param
  *		User defined parameter for the breakpoint
  *
  * Return value
  * ------------
  *	true  - to execute the breakpoint codecave.
  *	false - to not execute the breakpoint codecave.
  *	      In this case, the retaddr element of [regs] can be manipulated to
  *	      specify a different address to resume code execution after the breakpoint.
  */
typedef bool BreakpointFunc(x86Reg* regs, void* param);

// Represents a breakpoint.
struct Breakpoint {
	BreakpointFunc* func = nullptr;
	void* param = nullptr;
	uintptr_t addr = 0;
	size_t cavesize = 0;
	uint8_t* cave = nullptr;
};

void BreakpointsApply(Breakpoint* breakpoints, size_t bp_count);