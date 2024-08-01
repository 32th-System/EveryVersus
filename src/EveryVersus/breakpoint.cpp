#include <Windows.h>

#include "breakpoint.h"

#include <assert.h>
#include <string.h>

#include "util.h"

#define log_printf(...)

constexpr size_t BPCaveBufferSize = 32;
constexpr size_t CALL_LEN = (sizeof(void*) + 1);
constexpr size_t BPMinCavesize = CALL_LEN;
constexpr size_t BPMaxCavesize = BPCaveBufferSize - CALL_LEN;

size_t breakpoint_process(Breakpoint* bp, x86Reg* regs)
{
	assert(bp);

	size_t esp_diff = 0;

	// POPAD ignores the ESP register, so we have to implement our own mechanism
	// to be able to manipulate it.
	size_t esp_prev = regs->esp;

	int cave_exec = bp->func(regs, bp->param);
	if (cave_exec) {
		// Point return address to codecave.
		regs->retaddr = (size_t)bp->cave;
	}
	if (esp_prev != regs->esp) {
		// ESP change requested.
		// Shift down the regs structure by the requested amount
		esp_diff = regs->esp - esp_prev;
		memmove((uint8_t*)(regs)+esp_diff, regs, sizeof(x86Reg));
	}
	return esp_diff;
}

void cave_fix(uint8_t* cave, uint8_t* bp_addr)
{
	/// Fix relative stuff
	/// ------------------

	// #1: Relative far call / jump at the very beginning
	if (cave[0] == 0xe8 || cave[0] == 0xe9) {
		size_t dist_old = *((size_t*)(cave + 1));
		size_t dist_new = dist_old + bp_addr - cave;

		memcpy(cave + 1, &dist_new, sizeof(dist_new));

		log_printf("fixing rel.addr. 0x%p to 0x%p... ", dist_old, dist_new);
	}
	/// ------------------
}

uint8_t bp_entry[] = {
	0x60,                         // pushad
	0x9C,                         // pushfd
	0x54,                         // push esp
	0x68, 0x00, 0x00, 0x00, 0x00, // push param
	0xE8, 0x00, 0x00, 0x00, 0x00, // call breakpoint_process
	0x8D, 0x64, 0x04, 0x08,       // lea esp,dword ptr ss:[esp+eax+8]
	0x9D,                         // popfd
	0x61,                         // popad
	0xC3,                         // ret
	0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
};

static_assert(sizeof(bp_entry) % 16 == 0);

void BreakpointsApply(Breakpoint* breakpoints, size_t bp_count)
{
	if (!bp_count) {
		return;
	}

	uint8_t* cave_source = (uint8_t*)VirtualAlloc(0, bp_count * BPCaveBufferSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	memset(cave_source, 0xcc, bp_count * BPCaveBufferSize);

	// Call cave construction
	size_t call_size = sizeof(bp_entry);
	uint8_t* cave_call = (uint8_t*)VirtualAlloc(0, bp_count * call_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	log_printf(
		"Setting up breakpoints... (source cave at 0x%p, call cave at 0x%p)\n"
		"-------------------------\n",
		cave_source, cave_call
	);

	uint8_t* callcave_p = cave_call;
	for (size_t i = 0; i < bp_count; i++) {
		Breakpoint* bp = &breakpoints[i];

		bp->cave = cave_source;

		memcpy(callcave_p, (uint8_t*)bp_entry, call_size);
		auto callcave_localptr = (Breakpoint**)(callcave_p + 4);
		*callcave_localptr = bp;

		uintptr_t callcave_funcptr = (uintptr_t)callcave_p + 9;
		
		uintptr_t breakpoint_process_rel = (uintptr_t)breakpoint_process - (callcave_funcptr + 4);
		*(uintptr_t*)callcave_funcptr = breakpoint_process_rel;

		size_t cave_dist = bp->addr - ((uintptr_t)bp->cave + CALL_LEN);
		size_t bp_dist = (size_t)callcave_p - (bp->addr + CALL_LEN);
		uint8_t bp_asm[BPCaveBufferSize];

		/// Cave assembly
		// Copy old code to cave
		memcpy(bp->cave, (void*)bp->addr, bp->cavesize);
		cave_fix(bp->cave, (uint8_t*)bp->addr);

		// JMP addr
		bp->cave[bp->cavesize] = 0xe9;
		memcpy(bp->cave + bp->cavesize + 1, &cave_dist, sizeof(cave_dist));

		/// Breakpoint assembly
		memset(bp_asm, 0x90, bp->cavesize);
		// CALL bp_entry
		bp_asm[0] = 0xe8;
		memcpy(bp_asm + 1, &bp_dist, sizeof(void*));

		memcpy((void*)bp->addr, bp_asm, bp->cavesize);

		callcave_p += call_size;
		cave_source += BPCaveBufferSize;
	}
	log_printf("-------------------------\n");
}