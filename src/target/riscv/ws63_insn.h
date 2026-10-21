/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef OPENOCD_TARGET_RISCV_WS63_INSN_H
#define OPENOCD_TARGET_RISCV_WS63_INSN_H
#include <stdint.h>
#include <stdbool.h>
/* Decode scalar accesses only. WS63 short byte/halfword opcodes are not Zcb.
 * This describes the effective address; it never reads the accessed memory.
 */
struct ws63_access {
	unsigned int base, size;
	int32_t offset;
	bool load;
};
static inline bool ws63_decode_access(uint32_t code, struct ws63_access *a)
{
	uint16_t h = code;
	unsigned int q = h & 3, f = h >> 13;

	if (q == 3) {
		if ((h & 31) == 31)
			return false;
		unsigned int op = code & 127;

		f = (code >> 12) & 7;
		if (op == 3 && (f <= 2 || f == 4 || f == 5)) {
			a->size = 1U << (f & 3);
			a->load = true;
		} else if (op == 0x23 && f <= 2) {
			a->size = 1U << f;
			a->load = false;
		} else if ((op == 7 || op == 0x27) && f == 2) {
			a->size = 4;
			a->load = op == 7;
		} else {
			return false;
		}
		a->base = (code >> 15) & 31;
		a->offset = a->load ? code >> 20 : ((code >> 7) & 31) | ((code >> 25) << 5);
		if (a->offset & 0x800)
			a->offset -= 0x1000;
	} else if ((q == 0 || q == 2) && (f == 1 || f == 5)) {
		a->base = 8 + ((h >> 7) & 7);
		a->size = q == 0 ? 1 : 2;
		a->load = f == 1;
		a->offset = ((h >> 5) & 3) * 2 + ((h >> 10) & 3) * 8;
		a->offset += ((h >> 12) & 1) * (a->size == 1 ? 1 : 32);
	} else if (q == 0 && (f == 2 || f == 3 || f == 6 || f == 7)) {
		a->base = 8 + ((h >> 7) & 7);
		a->size = 4;
		a->load = f == 2 || f == 3;
		a->offset = ((h >> 6) & 1) * 4 + ((h >> 10) & 7) * 8 + ((h >> 5) & 1) * 64;
	} else if (q == 2 && (f == 2 || f == 3 || f == 6 || f == 7)) {
		a->base = 2;
		a->size = 4;
		a->load = f == 2 || f == 3;
		if (a->load) {
			if (f == 2 && ((h >> 7) & 31) == 0)
				return false;
			a->offset = ((h >> 4) & 7) * 4 + ((h >> 12) & 1) * 32 + ((h >> 2) & 3) * 64;
		} else {
			a->offset = ((h >> 9) & 15) * 4 + ((h >> 7) & 3) * 64;
		}
	} else {
		return false;
	}
	return true;
}
#endif
