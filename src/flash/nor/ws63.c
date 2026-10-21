// SPDX-License-Identifier: GPL-2.0-or-later
/* WS63 SFC / GD25Q32. The firmware must have initialized the SFC first.
 * Commands run through AP1; the CPU remains halted throughout programming.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "imp.h"
#include <helper/time_support.h>
#include <target/arm_adi_v5.h>
#include <target/riscv/riscv.h>
#include <target/riscv/gdb_regs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define WS63_SFC 0x48000000
#define WS63_BASE 0x200000
#define WS63_SIZE 0x400000
#define WS63_SECTOR 4096

struct ws63_flash {
	struct adiv5_ap *ap;
	bool probed;
	char *backup_dir;
};
struct ws63_session {
	uint32_t control, opcode, address;
	uint8_t data[64], sr1, sr2;
	bool saved, status_saved;
};
static int ws63_rd(struct ws63_flash *f, uint32_t off, uint32_t *v)
{
	return mem_ap_read_atomic_u32(f->ap, WS63_SFC + off, v);
}

static int ws63_wr(struct ws63_flash *f, uint32_t off, uint32_t v)
{
	return mem_ap_write_atomic_u32(f->ap, WS63_SFC + off, v);
}

static int ws63_idle(struct ws63_flash *f)
{
	int64_t end = timeval_ms() + 1000;
	uint32_t value;

	do {
		int ret = ws63_rd(f, 0x300, &value);

		if (ret != ERROR_OK || !(value & 1))
			return ret;
		keep_alive();
	} while (timeval_ms() < end);
	return ERROR_FLASH_OPERATION_FAILED;
}

static int ws63_cmd(struct ws63_flash *f, uint8_t op, int32_t address, const uint8_t *out,
		uint8_t *in, uint32_t length)
{
	if (length > 64 || (out && in))
		return ERROR_FAIL;
	int ret = ws63_idle(f);

	if (ret == ERROR_OK && address >= 0)
		ret = ws63_wr(f, 0x30c, address);
	if (ret == ERROR_OK && out) {
		uint8_t buf[64] = {0};

		memcpy(buf, out, length);
		ret = mem_ap_write_buf(f->ap, buf, 4, DIV_ROUND_UP(length, 4), WS63_SFC + 0x400);
	}
	if (ret == ERROR_OK)
		ret = ws63_wr(f, 0x308, op);
	uint32_t control = 3 | (address >= 0 ? 8 : 0);

	if (length)
		control |= 128 | ((length - 1) << 9) | (in ? 256 : 0);
	if (ret == ERROR_OK)
		ret = ws63_wr(f, 0x300, control);
	if (ret == ERROR_OK)
		ret = ws63_idle(f);
	if (ret == ERROR_OK && in) {
		uint8_t buf[64];

		ret = mem_ap_read_buf(f->ap, buf, 4, DIV_ROUND_UP(length, 4), WS63_SFC + 0x400);
		if (ret == ERROR_OK)
			memcpy(in, buf, length);
	}
	return ret;
}

static int ws63_ready(struct ws63_flash *f)
{
	int64_t end = timeval_ms() + 5000;
	uint8_t sr;

	do {
		int ret = ws63_cmd(f, 5, -1, NULL, &sr, 1);

		if (ret != ERROR_OK || !(sr & 1))
			return ret;
		alive_sleep(1);
	} while (timeval_ms() < end);
	return ERROR_FLASH_OPERATION_FAILED;
}

static int ws63_status(struct ws63_flash *f, uint8_t a, uint8_t b)
{
	uint8_t val[2] = {a & ~3, b};
	const uint8_t ops[2] = {1, 0x31};

	for (unsigned int i = 0; i < 2; ++i) {
		int ret = ws63_ready(f);

		if (ret == ERROR_OK)
			ret = ws63_cmd(f, 0x50, -1, NULL, NULL, 0);
		if (ret == ERROR_OK)
			ret = ws63_cmd(f, ops[i], -1, &val[i], NULL, 1);
		if (ret == ERROR_OK)
			ret = ws63_ready(f);
		if (ret != ERROR_OK)
			return ret;
	}
	uint8_t ra, rb;
	int ret = ws63_cmd(f, 5, -1, NULL, &ra, 1);

	if (ret == ERROR_OK)
		ret = ws63_cmd(f, 0x35, -1, NULL, &rb, 1);
	if (ret != ERROR_OK)
		return ret;
	return (ra & ~3) == (a & ~3) && rb == b ? ERROR_OK : ERROR_FLASH_OPERATION_FAILED;
}

static int ws63_begin(struct flash_bank *bank, struct ws63_session *s, bool write)
{
	memset(s, 0, sizeof(*s));
	if (bank->target->state != TARGET_HALTED)
		return ERROR_TARGET_NOT_HALTED;
	struct ws63_flash *f = bank->driver_priv;
	uint32_t dma;
	int ret = ws63_rd(f, 0x240, &dma);

	if (ret != ERROR_OK || (dma & 1))
		return ERROR_FLASH_OPERATION_FAILED;
	ret = ws63_idle(f);
	if (ret == ERROR_OK)
		ret = ws63_rd(f, 0x300, &s->control);
	if (ret == ERROR_OK)
		ret = ws63_rd(f, 0x308, &s->opcode);
	if (ret == ERROR_OK)
		ret = ws63_rd(f, 0x30c, &s->address);
	if (ret == ERROR_OK)
		ret = mem_ap_read_buf(f->ap, s->data, 4, 16, WS63_SFC + 0x400);
	if (ret != ERROR_OK)
		return ret;
	s->saved = true;
	ret = ws63_ready(f);
	if (ret == ERROR_OK)
		ret = ws63_cmd(f, 5, -1, NULL, &s->sr1, 1);
	if (ret == ERROR_OK)
		ret = ws63_cmd(f, 0x35, -1, NULL, &s->sr2, 1);
	if (ret != ERROR_OK)
		return ret;
	s->status_saved = true;
	if (write) {
		ret = riscv_set_register(bank->target, GDB_REGNO_CSR0 + 0x7c3, 12);
		if (ret == ERROR_OK)
			ret = ws63_status(f, s->sr1 & ~0x7c, s->sr2 & ~0x40);
	}
	return ret;
}

static int ws63_end(struct flash_bank *bank, struct ws63_session *s, int operation, bool write)
{
	struct ws63_flash *f = bank->driver_priv;
	int ret = ERROR_OK;

	if (s->status_saved) {
		ret = ws63_ready(f);
		if (ret == ERROR_OK && write)
			ret = ws63_status(f, s->sr1, s->sr2);
		if (ret == ERROR_OK)
			ret = ws63_cmd(f, s->sr1 & 2 ? 6 : 4, -1, NULL, NULL, 0);
	}
	if (s->saved && ret == ERROR_OK) {
		ret = mem_ap_write_buf(f->ap, s->data, 4, 16, WS63_SFC + 0x400);
		if (ret == ERROR_OK)
			ret = ws63_wr(f, 0x308, s->opcode);
		if (ret == ERROR_OK)
			ret = ws63_wr(f, 0x30c, s->address);
		if (ret == ERROR_OK)
			ret = ws63_wr(f, 0x300, s->control & ~1);
	}
	if (write && ret == ERROR_OK)
		ret = riscv_set_register(bank->target, GDB_REGNO_CSR0 + 0x7c3, 12);
	if (write && ret == ERROR_OK)
		ret = riscv_set_register(bank->target, GDB_REGNO_CSR0 + 0x7c2, 4);
	if (write) {
		riscv_info(bank->target)->ws63_flash_failed |= operation != ERROR_OK ||
				ret != ERROR_OK;
		riscv_info(bank->target)->ws63_flash_changed = true;
	}
	if (operation != ERROR_OK || ret != ERROR_OK) {
		LOG_ERROR("WS63 Flash operation failed; target remains halted. Restore from backups before "
				  "continuing.");
		return operation != ERROR_OK ? operation : ret;
	}
	return ERROR_OK;
}

static int ws63_wren(struct ws63_flash *f)
{
	int ret = ws63_ready(f);

	if (ret == ERROR_OK)
		ret = ws63_cmd(f, 6, -1, NULL, NULL, 0);
	uint8_t sr;

	if (ret == ERROR_OK)
		ret = ws63_cmd(f, 5, -1, NULL, &sr, 1);
	if (ret == ERROR_OK && !(sr & 2))
		ret = ERROR_FLASH_OPERATION_FAILED;
	return ret;
}

static int ws63_read(struct flash_bank *bank, uint8_t *buffer, uint32_t offset, uint32_t count)
{
	if (offset > bank->size || count > bank->size - offset)
		return ERROR_FLASH_DST_OUT_OF_BANK;
	struct ws63_flash *f = bank->driver_priv;
	/* Large backups must not starve the GDB connection's keep-alive. */
	for (uint32_t pos = 0; pos < count;) {
		uint32_t n = MIN(count - pos, 4096U);
		int ret = mem_ap_read_buf(f->ap, buffer + pos, 1, n, bank->base + offset + pos);

		if (ret != ERROR_OK)
			return ret;
		pos += n;
		keep_alive();
	}
	return ERROR_OK;
}
/* A durable raw backup precedes each erasure or program operation. The XIP address is in its name.
 * Never silently overwrite an older backup, even within one GDB load.
 */
static int ws63_backup(struct flash_bank *bank, uint32_t offset, uint32_t count)
{
#ifdef _WIN32
	/* POSIX directory fsync is part of the backup contract. Until a native
	 * durable backup is implemented, keep Windows debug/read access usable
	 * and reject mutation before touching the device.
	 */
	LOG_ERROR("WS63 Flash writes require durable backups on a POSIX host");
	return ERROR_FLASH_OPER_UNSUPPORTED;
#else
	if (bank->target->state != TARGET_HALTED)
		return ERROR_TARGET_NOT_HALTED;
	struct ws63_flash *f = bank->driver_priv;
	int created = mkdir(f->backup_dir, 0700);

	if (created && errno != EEXIST)
		return ERROR_FAIL;
	/* Sync the directory's own link before relying on its durable contents. */
	if (!created) {
		char *parent = alloc_printf("%s/..", f->backup_dir);

		if (!parent)
			return ERROR_FAIL;
		int fd = open(parent, O_RDONLY | O_DIRECTORY);

		free(parent);
		if (fd < 0)
			return ERROR_FAIL;
		int ret = fsync(fd);

		if (close(fd) || ret)
			return ERROR_FAIL;
	}
	char *name =
			alloc_printf("%s/%08" PRIx32 "-XXXXXX", f->backup_dir,
					(uint32_t)bank->base + offset);
	uint8_t *buffer = malloc(count);

	if (!name || !buffer) {
		free(name);
		free(buffer);
		return ERROR_FAIL;
	}
	int ret = ws63_read(bank, buffer, offset, count);
	int fd = ret == ERROR_OK ? mkstemp(name) : -1;

	if (fd < 0)
		ret = ERROR_FAIL;
	if (fd >= 0) {
		uint32_t done = 0;

		while (done < count) {
			ssize_t n = write(fd, buffer + done, count - done);

			if (n < 0 && errno == EINTR)
				continue;
			if (n <= 0) {
				ret = ERROR_FAIL;
				break;
			}
			done += n;
		}
		if (fsync(fd))
			ret = ERROR_FAIL;
		if (close(fd))
			ret = ERROR_FAIL;
		int dirfd = open(f->backup_dir, O_RDONLY | O_DIRECTORY);

		if (dirfd < 0) {
			ret = ERROR_FAIL;
		} else {
			if (fsync(dirfd))
				ret = ERROR_FAIL;
			close(dirfd);
		}
	}
	if (ret == ERROR_OK)
		LOG_INFO("WS63 Flash backup: %s (%" PRIu32 " bytes)", name, count);
	free(buffer);
	free(name);
	return ret;
#endif
}

static int ws63_erase(struct flash_bank *bank, unsigned int first, unsigned int last)
{
	if (last >= bank->num_sectors || first > last)
		return ERROR_FLASH_SECTOR_INVALID;
	if (riscv_ws63_check_write(bank->target, bank->base + first * WS63_SECTOR,
			(last - first + 1) * WS63_SECTOR) != ERROR_OK)
		return ERROR_FAIL;
	int ret = ws63_backup(bank, first * WS63_SECTOR, (last - first + 1) * WS63_SECTOR);

	if (ret != ERROR_OK)
		return ret;
	struct ws63_flash *f = bank->driver_priv;
	struct ws63_session s;

	ret = ws63_begin(bank, &s, true);
	for (unsigned int i = first; ret == ERROR_OK && i <= last; ++i) {
		ret = ws63_wren(f);
		if (ret == ERROR_OK)
			ret = ws63_cmd(f, 0x20, i * WS63_SECTOR, NULL, NULL, 0);
		if (ret == ERROR_OK)
			ret = ws63_ready(f);
		/* Verify via SPI, independent of the XIP/cache view. */
		uint8_t actual[64];

		for (unsigned int off = 0; ret == ERROR_OK &&
				off < WS63_SECTOR; off += sizeof(actual)) {
			ret = ws63_cmd(f, 3, i * WS63_SECTOR + off, NULL, actual, sizeof(actual));
			if (ret == ERROR_OK)
				for (unsigned int n = 0; n < sizeof(actual); ++n)
					if (actual[n] != 0xff)
						ret = ERROR_FLASH_OPERATION_FAILED;
		}
		if (ret == ERROR_OK)
			bank->sectors[i].is_erased = 1;
		keep_alive();
	}
	return ws63_end(bank, &s, ret, true);
}

static int ws63_write(struct flash_bank *bank, const uint8_t *buffer,
		uint32_t offset, uint32_t count)
{
	if (offset > bank->size || count > bank->size - offset)
		return ERROR_FLASH_DST_OUT_OF_BANK;
	if (!count)
		return ERROR_OK;
	if (riscv_ws63_check_write(bank->target, bank->base + offset, count) != ERROR_OK)
		return ERROR_FAIL;
	int ret = ws63_backup(bank, offset, count);

	if (ret != ERROR_OK)
		return ret;
	struct ws63_flash *f = bank->driver_priv;
	struct ws63_session s;

	ret = ws63_begin(bank, &s, true);
	for (uint32_t pos = 0; ret == ERROR_OK && pos < count;) {
		uint32_t n = MIN(64U, MIN(count - pos, 256 - ((offset + pos) & 255)));

		ret = ws63_wren(f);
		if (ret == ERROR_OK)
			ret = ws63_cmd(f, 2, offset + pos, buffer + pos, NULL, n);
		if (ret == ERROR_OK)
			ret = ws63_ready(f);
		uint8_t actual[64];

		if (ret == ERROR_OK)
			ret = ws63_cmd(f, 3, offset + pos, NULL, actual, n);
		if (ret == ERROR_OK && memcmp(actual, buffer + pos, n))
			ret = ERROR_FLASH_OPERATION_FAILED;
		pos += n;
		keep_alive();
	}
	return ws63_end(bank, &s, ret, true);
}

static int ws63_probe(struct flash_bank *bank)
{
	if (strcmp(target_type_name(bank->target), "riscvcs") ||
			!riscv_info(bank->target)->ws63)
		return ERROR_FAIL;
	struct ws63_flash *f = bank->driver_priv;

	if (f->probed)
		return ERROR_OK;
	struct adiv5_private_config *pc =
		(struct adiv5_private_config *)bank->target->private_config;

	if (!pc->dap)
		return ERROR_FAIL;
	if (!f->ap) {
		f->ap = dap_get_ap(pc->dap, 1);
		if (!f->ap)
			return ERROR_FAIL;
		int ret = mem_ap_init(f->ap);

		if (ret != ERROR_OK) {
			dap_put_ap(f->ap);
			f->ap = NULL;
			return ret;
		}
	}
	uint32_t base, size;
	int ret = ws63_rd(f, 0x218, &base);

	if (ret == ERROR_OK)
		ret = ws63_rd(f, 0x210, &size);
	if (ret != ERROR_OK || base != WS63_BASE || ((size >> 8) & 15) != 7) {
		LOG_ERROR("WS63 SFC must already map 4 MiB CS1 Flash at 0x200000");
		return ERROR_FAIL;
	}
	struct ws63_session s;

	ret = ws63_begin(bank, &s, false);
	uint8_t id[3];

	if (ret == ERROR_OK)
		ret = ws63_cmd(f, 0x9f, -1, NULL, id, 3);
	if (ret == ERROR_OK && (id[0] != 0xc8 || id[1] != 0x40 || id[2] != 0x16))
		ret = ERROR_FLASH_OPERATION_FAILED;
	ret = ws63_end(bank, &s, ret, false);
	if (ret == ERROR_OK) {
		f->probed = true;
		LOG_INFO("WS63 GD25Q32: 4 MiB, 4096-byte sectors");
	}
	return ret;
}

FLASH_BANK_COMMAND_HANDLER(ws63_flash_bank_command)
{
	if (CMD_ARGC < 6 || CMD_ARGC > 7 || bank->base != WS63_BASE || bank->size != WS63_SIZE ||
			strcmp(target_type_name(bank->target), "riscvcs") ||
			!riscv_info(bank->target)->ws63)
		return ERROR_COMMAND_SYNTAX_ERROR;
	struct ws63_flash *f = calloc(1, sizeof(*f));

	if (!f)
		return ERROR_FAIL;
	f->backup_dir = strdup(CMD_ARGC == 7 ? CMD_ARGV[6] : "ws63-flash-backups");
	bank->driver_priv = f;
	bank->num_sectors = WS63_SIZE / WS63_SECTOR;
	bank->sectors = alloc_block_array(0, WS63_SECTOR, bank->num_sectors);
	if (!f->backup_dir || !bank->sectors)
		return ERROR_FAIL;
	riscv_info(bank->target)->ws63_flash_enabled = true;
	return ERROR_OK;
}

static void ws63_free(struct flash_bank *bank)
{
	struct ws63_flash *f = bank->driver_priv;

	if (f) {
		if (f->ap)
			dap_put_ap(f->ap);
		free(f->backup_dir);
		free(f);
	}
}

static int ws63_info(struct flash_bank *bank, struct command_invocation *cmd)
{
	int ret = ws63_probe(bank);

	if (ret != ERROR_OK)
		return ret;
	command_print_sameline(cmd,
			"WS63 SFC / GD25Q32, 4 MiB, 4 KiB sectors; status protection is preserved across writes");
	return ERROR_OK;
}

const struct flash_driver ws63_flash = {
	.name = "ws63",
	.info = ws63_info,
	.usage = "[backup_directory]",
	.flash_bank_command = ws63_flash_bank_command,
	.erase = ws63_erase,
	.write = ws63_write,
	.read = ws63_read,
	.probe = ws63_probe,
	.auto_probe = ws63_probe,
	.erase_check = default_flash_blank_check,
	.free_driver_priv = ws63_free,
};

/* Software breakpoints preserve the rest of every affected erase block. */
int ws63_flash_patch(struct target *target, target_addr_t address,
		const uint8_t *data, uint32_t count)
{
	if (!count || address < WS63_BASE || address >= WS63_BASE + WS63_SIZE ||
			count > WS63_BASE + WS63_SIZE - address)
		return ERROR_FAIL;
	struct flash_bank *bank;
	int ret = get_flash_bank_by_addr(target, address, true, &bank);

	if (ret != ERROR_OK || !bank || bank->driver != &ws63_flash)
		return ERROR_FAIL;
	bool changed = riscv_info(target)->ws63_flash_changed;
	uint32_t first = (address - WS63_BASE) / WS63_SECTOR;
	uint32_t last = (address + count - 1 - WS63_BASE) / WS63_SECTOR;
	uint32_t total = (last - first + 1) * WS63_SECTOR;
	uint8_t *buffer = malloc(total);

	if (!buffer)
		return ERROR_FAIL;
	ret = ws63_read(bank, buffer, first * WS63_SECTOR, total);
	if (ret == ERROR_OK) {
		memcpy(buffer + address - WS63_BASE - first * WS63_SECTOR, data, count);
		ret = ws63_erase(bank, first, last);
	}
	if (ret == ERROR_OK)
		ret = ws63_write(bank, buffer, first * WS63_SECTOR, total);
	free(buffer);
	if (ret == ERROR_OK)
		riscv_info(target)->ws63_flash_changed = changed;
	return ret;
}
