/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

/**
 * @file
 *  Support for BUSJTAG CoreSight-AXI-AP.
 *  BUSJTAG is a Coresight AXI-AP rather than a normal CPU.
 *  It can ONLY used to access memory or registers.
 *  Which means it can't halt, resume, step, use breakpoints or watchpoints and so on.
 */

#include <assert.h>
#include <stdlib.h>
#include <time.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "target/target.h"
//#include "log.h"
#include <jtag/jtag.h>
#include "target/breakpoints.h"
#include "helper/time_support.h"
#include <stdint.h>
#include "target/arm_adi_v5.h" 
#include "target/target_type.h"
#include "target/register.h"
#include "helper/list.h"

typedef struct {
	uint32_t debug_ap_num;
	struct adiv5_dap *dap;
	struct adiv5_ap *debug_ap;
} busjtag_info_t;

static busjtag_info_t *get_info(const struct target *target)
{
	return (busjtag_info_t *) target->arch_info;
}

static int busjtag_target_create(struct target *target, Jim_Interp *interp)
{
	LOG_DEBUG(" ");
	target->arch_info = calloc(1, sizeof(busjtag_info_t));
	if (!target->arch_info)
		return ERROR_FAIL;
	if (target->private_config == NULL) {
		LOG_ERROR("adiv5_jim_configure failed!");
		return ERROR_FAIL;
	}
 	return ERROR_OK;
}

static int busjtag_init_target(struct command_context *cmd_ctx, struct target *target)
{
	LOG_DEBUG(" ");
	target->state = TARGET_UNKNOWN;
	return ERROR_OK;
}

static int busjtag_arch_state(struct target *target)
{
	return ERROR_OK;
}

static int busjtag_poll(struct target *target)
{
	if (target->state == TARGET_UNKNOWN)
		target->state = TARGET_RUNNING;

	return ERROR_OK;
}

static int busjtag_halt(struct target *target)
{
	LOG_DEBUG(" ");
	target->state = TARGET_HALTED;
	return ERROR_OK;
}

static int busjtag_resume(struct target *target, int current,
	target_addr_t address, int handle_breakpoints, int debug_execution)
{
	LOG_DEBUG(" ");
	target->state = TARGET_RUNNING;
	return ERROR_OK;
}

static int busjtag_step(struct target *target, int current, target_addr_t address,
	int handle_breakpoints)
{
	LOG_DEBUG(" ");
	target->state = TARGET_HALTED;
	return ERROR_OK;
}

static int busjtag_assert_reset(struct target *target)
{
	target->state = TARGET_RESET;
	LOG_DEBUG(" ");
	return ERROR_OK;
}

static int busjtag_deassert_reset(struct target *target)
{
	if (target->reset_halt)
		target->state = TARGET_HALTED;
	else
		target->state = TARGET_RUNNING;

	LOG_DEBUG(" ");
	return ERROR_OK;
}

static int busjtag_read_memory(struct target *target, target_addr_t address,
	uint32_t size, uint32_t count, uint8_t *buffer)
{
	busjtag_info_t *info = get_info(target);
	LOG_DEBUG("Reading BUSJTAG memory at physical address 0x" TARGET_ADDR_FMT
		  "; size %" PRId32 "; count %" PRId32, address, size, count);
	if (count == 0 || buffer == NULL) {
		return ERROR_COMMAND_SYNTAX_ERROR;
	}
	return mem_ap_read_buf(info->debug_ap, buffer, size, count, address);
}

static int busjtag_write_memory(struct target *target, target_addr_t address,
	uint32_t size, uint32_t count, const uint8_t *buffer)
{
	busjtag_info_t *info = get_info(target);
	LOG_DEBUG("Writing BUSJTAG memory at physical address 0x" TARGET_ADDR_FMT
		  "; size %" PRId32 "; count %" PRId32, address, size, count);
	if (count == 0 || buffer == NULL) {
		return ERROR_COMMAND_SYNTAX_ERROR;
	}
	return mem_ap_write_buf(info->debug_ap, buffer, size, count, address);
}

static int busjtag_examine(struct target *target)
{
	int retval = ERROR_OK;
	if (target_was_examined(target)) {
		return ERROR_OK;
	}
	/* don't re-probe hardware after each reset */

	LOG_DEBUG("BUSJTAG init CoreSight-AXI-AP...");	
	busjtag_info_t *info = get_info(target);
	struct adiv5_private_config *pc = (struct adiv5_private_config *)target->private_config;
	info->dap = (struct adiv5_dap *)(pc->dap);
	info->debug_ap_num = target->ap_sel; //script specific

	/* Search for the AXI-AP - it is needed for access to debug registers */
	retval = dap_find_ap(info->dap, AP_TYPE_AXI_AP, &info->debug_ap);
	if (retval != ERROR_OK) {
		LOG_ERROR("Could not find AXI-AP for debug access");
		return retval;
	}

	retval = mem_ap_init(info->debug_ap);
	if (retval != ERROR_OK) {
		LOG_ERROR("Could not initialize the AXI-AP");
		return retval;
	}
	info->debug_ap->memaccess_tck = 10;
	if (info->debug_ap->ap_num == info->debug_ap_num) {
		LOG_INFO("Select AP [%d].", info->debug_ap_num);
	} else {
		LOG_ERROR("Can't find AXI-AP at ap[%d], maybe is ap[%d]. "
			"Please modify the target script -apsel option.",
			info->debug_ap_num, info->debug_ap->ap_num);
		return ERROR_FAIL;
	}
	info->dap->apsel = info->debug_ap_num;

	target->state = TARGET_UNKNOWN;
	target_set_examined(target);
	LOG_DEBUG("BUSJTAG examination end.");
	LOG_INFO("Attention, BUSJTAG is a Coresight AXI-AP rather than a normal CPU."
	"It can only used to access memory or registers."
	"Which means it can't step, resume, halt, insert breakpoint or watchpoint and so on.");

	return ERROR_OK;
}

static void busjtag_deinit_target(struct target *target)
{	
	LOG_DEBUG("deinit BUSJTAG-AXI-AP...");
	if (target->private_config) {
		free(target->private_config);
		target->private_config = NULL;
	}
	if (target->arch_info) {
		free(target->arch_info);
		target->arch_info = NULL;
	}
}

COMMAND_HANDLER(handle_busjtag_help_command)
{
	if (CMD_ARGC != 0) {
		LOG_ERROR("Command takes exactly no parameter");
		return ERROR_COMMAND_SYNTAX_ERROR;
	}
	command_print(CMD, "BUSJTAG is a Coresight AXI-AP rather than CPU."
	"It can only used to access memory or registers."
	"Which means it can't step, resume, halt, insert breakpoint or watchpoint and so on.");
	return ERROR_OK;
}

static const struct command_registration busjtag_exec_command_handlers[] = {
	{
		.name = "help",
		.handler = handle_busjtag_help_command,
		.mode = COMMAND_ANY,
		.help = "display busjtag help information",
		.usage = "busjtag help",
	},
	COMMAND_REGISTRATION_DONE
};

static const struct command_registration busjtag_command_handlers[] = {
	{
		.name = "busjtag",
		.mode = COMMAND_ANY,
		.help = "BUSJTAG Command Group",
		.usage = "",
		.chain = busjtag_exec_command_handlers
	},
	COMMAND_REGISTRATION_DONE
};

static int bkpt_not_implemented(struct target *target,
		struct breakpoint *breakpoint)
{
	LOG_ERROR("BUSJTAG doesn't have breakpoint!");
	return ERROR_FAIL;
}

static int watchpoint_not_implemented(struct target *target, struct watchpoint *watchpoint)
{
	LOG_ERROR("BUSJTAG doesn't have watchpoint!");
	return ERROR_FAIL;
}

static int get_gdb_reg_list_not_implemented(struct target *target, struct reg **reg_list[],
	int *reg_list_size, enum target_register_class reg_class)
{
	LOG_DEBUG("BUSJTAG doesn't have gdb_reg_list!");
	return ERROR_OK;
}

struct target_type busjtag_target = {
	.name = "busjtag",

	.target_create = busjtag_target_create,
	.init_target = busjtag_init_target,
	.examine = busjtag_examine,
	.target_jim_configure = adiv5_jim_configure,
	.deinit_target = busjtag_deinit_target,

	.poll = busjtag_poll,
	.arch_state = busjtag_arch_state,

	.halt = busjtag_halt,
	.resume = busjtag_resume,
	.step = busjtag_step,

	.assert_reset = busjtag_assert_reset,
	.deassert_reset = busjtag_deassert_reset,

	.read_memory = busjtag_read_memory,
	.write_memory = busjtag_write_memory,
	
	.get_gdb_reg_list = get_gdb_reg_list_not_implemented,

	.add_breakpoint = bkpt_not_implemented,
	.add_context_breakpoint = bkpt_not_implemented,
	.add_hybrid_breakpoint = bkpt_not_implemented,
	.remove_breakpoint = bkpt_not_implemented,
	.add_watchpoint = watchpoint_not_implemented,
	.remove_watchpoint = watchpoint_not_implemented,

	.commands = busjtag_command_handlers,	
};
