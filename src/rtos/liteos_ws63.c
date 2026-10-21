// SPDX-License-Identifier: GPL-2.0-or-later
/* BearPi WS63 SDK v1.0.102: LosTaskCB=96, RV32F TaskContext=272.
 * Explicit opt-in: do not auto-detect a layout from symbol names alone.
 * Cooperative context saves only callee-saved registers reliably.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "rtos.h"
#include <server/gdb_server.h>
#include <target/riscv/riscv.h>
#include <target/riscv/gdb_regs.h>

int rtos_thread_packet(struct connection *connection, const char *packet, int packet_size);

static const char *const liteos_symbols[] = {
	"g_osTaskCBArray", "g_taskMaxNum", "g_newTask", "g_taskScheduled", "g_intCount", NULL
};

static bool liteos_ram(uint32_t address, uint32_t size)
{
	return (address >= 0xa00000 && address <= 0xa88000 && size <= 0xa88000 - address) ||
		   (address >= 0x180000 && address <= 0x1c8000 && size <= 0x1c8000 - address);
}

static int liteos_symbol_list(struct symbol_table_elem **symbols)
{
	*symbols = calloc(ARRAY_SIZE(liteos_symbols), sizeof(**symbols));
	if (!*symbols)
		return ERROR_FAIL;
	for (unsigned int i = 0; i < ARRAY_SIZE(liteos_symbols); ++i)
		(*symbols)[i].symbol_name = liteos_symbols[i];
	return ERROR_OK;
}

static bool liteos_detect(struct target *target)
{
	/* Deliberately disabled for '-rtos auto'. */
	return false;
}

static int liteos_update(struct rtos *rtos)
{
	if (!rtos->symbols || !rtos->symbols[0].address)
		return ERROR_OK;
	struct target *target = rtos->target;

	if (target->state != TARGET_HALTED)
		return ERROR_TARGET_NOT_HALTED;
	rtos_free_threadlist(rtos);
	rtos->current_thread = 1;
	uint32_t array, count, current, scheduled;
	riscv_reg_t pc, sp;

	if (riscv_get_register(target, &pc, GDB_REGNO_PC) != ERROR_OK ||
			riscv_get_register(target, &sp, GDB_REGNO_SP) != ERROR_OK)
		return ERROR_FAIL;
	if (pc == 0x100000)
		return ERROR_OK;
	if (target_read_u32(target, rtos->symbols[0].address, &array) != ERROR_OK ||
			target_read_u32(target, rtos->symbols[1].address, &count) != ERROR_OK ||
			target_read_u32(target, rtos->symbols[2].address, &current) != ERROR_OK ||
			target_read_u32(target, rtos->symbols[3].address, &scheduled) != ERROR_OK)
		return ERROR_FAIL;
	if (!scheduled || !array || !count)
		return ERROR_OK;
	if (count > 512 || !liteos_ram(array, count * 96)) {
		LOG_ERROR("Invalid WS63 LiteOS task array; requires matching SDK v1.0.102 ELF/layout");
		return ERROR_FAIL;
	}
	rtos->thread_details = calloc(count + 1, sizeof(*rtos->thread_details));
	if (!rtos->thread_details)
		return ERROR_FAIL;
	for (uint32_t i = 0; i < count; ++i) {
		uint32_t tcb = array + i * 96;
		uint8_t raw[96];

		if (target_read_buffer(target, tcb, sizeof(raw), raw) != ERROR_OK)
			return ERROR_FAIL;
		uint16_t status = le_to_h_u16(raw + 4);

		if (!status || (status & 1))
			continue;
		uint32_t low = le_to_h_u32(raw + 16), size = le_to_h_u32(raw + 12);

		if (!liteos_ram(low, size) || size < 272)
			return ERROR_FAIL;
		uint32_t name = le_to_h_u32(raw + 40);
		uint8_t text[65] = {0};

		if (!(liteos_ram(name, 64) || (name >= 0x100000 && name <= 0x600000 - 64)))
			return ERROR_FAIL;
		if (target_read_buffer(target, name, 64, text) != ERROR_OK)
			return ERROR_FAIL;
		struct thread_detail *td = &rtos->thread_details[rtos->thread_count++];

		td->threadid = tcb;
		td->exists = true;
		td->thread_name_str = strdup((char *)text);
		td->extra_info_str = alloc_printf("task=%" PRIu32 " priority=%u state=0x%x",
				le_to_h_u32(raw + 20), le_to_h_u16(raw + 6), status);
		if (!td->thread_name_str || !td->extra_info_str)
			return ERROR_FAIL;
		if (tcb == current && sp >= low && sp <= low + size)
			rtos->current_thread = tcb;
	}
	if (rtos->current_thread == 1) {
		struct thread_detail *td = &rtos->thread_details[rtos->thread_count++];

		td->threadid = 1;
		td->exists = true;
		td->thread_name_str = strdup("boot/interrupt context");
	}
	return ERROR_OK;
}

static int liteos_context(struct rtos *rtos, threadid_t id, uint8_t *context, uint32_t *sp)
{
	bool found = false;

	for (int i = 0; i < rtos->thread_count; ++i)
		if (rtos->thread_details[i].threadid == id && id != 1)
			found = true;
	if (!found || !liteos_ram(id, 96))
		return ERROR_FAIL;
	uint32_t low, size;

	if (target_read_u32(rtos->target, id, sp) != ERROR_OK ||
			target_read_u32(rtos->target, id + 12, &size) != ERROR_OK ||
			target_read_u32(rtos->target, id + 16, &low) != ERROR_OK)
		return ERROR_FAIL;
	if (!liteos_ram(low, size) || *sp < low || size < 272 || *sp > low + size - 272)
		return ERROR_FAIL;
	return target_read_buffer(rtos->target, *sp, 272, context);
}

static bool liteos_reg_value(uint32_t number, const uint8_t *ctx, uint32_t sp, uint32_t *value)
{
	if (number == 0) {
		*value = 0;
		return true;
	}
	if (number == 2) {
		*value = sp + 272;
		return true;
	}
	if (number == 32) {
		*value = le_to_h_u32(ctx + 4);
		return true;
	}
	if (number == GDB_REGNO_MSTATUS) {
		*value = le_to_h_u32(ctx);
		return true;
	}
	if (number == (GDB_REGNO_CSR0 + 3)) {
		*value = le_to_h_u32(ctx + 256);
		return true;
	}
	if (number == 8 || number == 9) {
		*value = le_to_h_u32(ctx + 60 - (number - 8) * 4);
		return true;
	}
	if (number >= 18 && number <= 27) {
		*value = le_to_h_u32(ctx + 52 - (number - 18) * 4);
		return true;
	}
	if (number == GDB_REGNO_FPR0 + 8 || number == GDB_REGNO_FPR0 + 9) {
		*value = le_to_h_u32(ctx + 172 - (number - GDB_REGNO_FPR0 - 8) * 4);
		return true;
	}
	if (number >= GDB_REGNO_FPR0 + 18 && number <= GDB_REGNO_FPR0 + 27) {
		*value = le_to_h_u32(ctx + 164 - (number - GDB_REGNO_FPR0 - 18) * 4);
		return true;
	}
	return false;
}

static int liteos_regs(struct rtos *rtos, int64_t id, struct rtos_reg **regs, int *count)
{
	uint8_t ctx[272];
	uint32_t sp;
	int ret = liteos_context(rtos, id, ctx, &sp);

	if (ret != ERROR_OK)
		return ret;
	*count = 33;
	*regs = calloc(*count, sizeof(**regs));
	if (!*regs)
		return ERROR_FAIL;
	for (int i = 0; i < *count; ++i) {
		uint32_t value;
		(*regs)[i].number = i;
		(*regs)[i].size = 32;
		(*regs)[i].unavailable = !liteos_reg_value(i, ctx, sp, &value);
		if (!(*regs)[i].unavailable)
			h_u32_to_le((*regs)[i].value, value);
	}
	return ERROR_OK;
}

static int liteos_reg(struct rtos *rtos, int64_t id, uint32_t number, struct rtos_reg *reg)
{
	uint8_t ctx[272];
	uint32_t sp, val;
	int ret = liteos_context(rtos, id, ctx, &sp);

	if (ret != ERROR_OK)
		return ret;
	reg->number = number;
	reg->size = 32;
	reg->unavailable = !liteos_reg_value(number, ctx, sp, &val);
	if (!reg->unavailable)
		h_u32_to_le(reg->value, val);
	return ERROR_OK;
}

static int liteos_create(struct target *target)
{
	if (strcmp(target_type_name(target), "riscvcs") || !riscv_info(target)->ws63)
		return ERROR_FAIL;
	target->rtos->gdb_thread_packet = rtos_thread_packet;
	return ERROR_OK;
}

struct rtos_type liteos_ws63_rtos = {
	.name = "liteos_ws63",
	.thread_registers_read_only = true,
	.supports_unavailable_registers = true,
	.detect_rtos = liteos_detect,
	.create = liteos_create,
	.update_threads = liteos_update,
	.get_thread_reg_list = liteos_regs,
	.get_thread_reg = liteos_reg,
	.get_symbol_list_to_lookup = liteos_symbol_list,
};

int liteos_ws63_symbols(struct command_invocation *cmd)
{
	struct target *target = get_current_target(CMD_CTX);

	if (CMD_ARGC != 5 || !target->rtos || target->rtos->type != &liteos_ws63_rtos)
		return ERROR_COMMAND_SYNTAX_ERROR;
	uint32_t values[5];

	for (unsigned int i = 0; i < 5; ++i) {
		COMMAND_PARSE_NUMBER(u32, CMD_ARGV[i], values[i]);
		if (!liteos_ram(values[i], 4) || values[i] % 4)
			return ERROR_COMMAND_ARGUMENT_INVALID;
	}
	if (!target->rtos->symbols && liteos_symbol_list(&target->rtos->symbols) != ERROR_OK)
		return ERROR_FAIL;
	for (unsigned int i = 0; i < 5; ++i)
		target->rtos->symbols[i].address = values[i];
	return ERROR_OK;
}
