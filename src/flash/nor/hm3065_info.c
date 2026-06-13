/*
* Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2024-2024. All rights reserved.
* Description: Flash drive of the 16KB info area
* Create: 2024-7-26
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "imp.h"
#include <target/image.h>


/*********************************************************************
*
*       Defines (fixed)
*
**********************************************************************/
#define HM3065_INFO_FLASH_PAGE_SIZE    0x2000
#define HM3065_INFO_FLASH_PAGES        2
#define PAGE_SIZE_SHIFT                2   /* Flash needs to be programmed in units of 2 ^ 2 =  bytes */
#define FLASH_MAX_PGM_WORD_SIZE        0x80
#define FLASH_MIN_PGM_BYTES_SIZE       0x10
#define PAGE_WORD_SIZE                 4
#define FLASH_MIN_PGM_WORDS_SIZE       4
#define FLASH_ONE_WORD_BYTES_SIZE      4

#define FLASH_OPERATION_READ          0x00000001U
#define FLASH_OPERATION_PROGRAM       0x00000003U
#define FLASH_OPERATION_ERASE         0x00000005U

#define FLASH_READ_BASE_ADDR          0x3000000
#define FLASH_OFFSET_BASE_ADDR        0x3800000

#define FLASH_KEY_REGISTER_UNLOCK_VALUE   0xFEDCBA98
#define FLASH_KEY_REGISTER_LOCK_VALUE     0x0

#define EFC ((EFC_RegStruct *)0x14710000)

/* Flash Register Description Definition----------------------------------- */
typedef union {
    uint32_t reg;
    struct {
        uint32_t cmd_start       : 1;
        uint32_t reserved0       : 5;
        uint32_t exec_state      : 2;
        uint32_t cmd_code        : 3;
        uint32_t reserved1       : 9;
        uint32_t cmd_pgm_size    : 6;
        uint32_t reserved3       : 2;
        uint32_t cmd_read_size   : 2;
        uint32_t reserved4       : 2;
    } BIT;
} EFLASH_CMD_REG;

typedef union {
    uint32_t reg;
    struct {
        uint32_t reserved0        : 2;
        uint32_t cmd_addr         : 22;
        uint32_t reserved1        : 8;
    } BIT;
} EFLASH_ADDR_REG;

typedef union {
    uint32_t reg;
    struct {
        uint32_t reserved0        : 1;
        uint32_t int_mode         : 1;
        uint32_t reserved1        : 30;
    } BIT;
} CMD_CFG_COMMON_REG;

typedef union {
    uint32_t reg;
    struct {
         uint32_t reserved0             : 16;
         uint32_t int_raw_err_illegal   : 1;
         uint32_t int_raw_err_smwr      : 1;
         uint32_t reserved1             : 14;
    } BIT;
} INT_RAW_STATUS_REG;

typedef union {
    uint32_t reg;
    struct {
         uint32_t reserved0             : 16;
         uint32_t int_clr_err_illegal   : 1;
         uint32_t int_clr_err_smwr      : 1;
         uint32_t reserved1             : 14;
    } BIT;
} INT_CLEAR_REG;

typedef union {
    uint32_t reg;
    struct {
        uint32_t cache_enable           : 1;
        uint32_t reserved0              : 3;
        uint32_t cache_replacement_sel  : 1;
        uint32_t reserved1              : 3;
        uint32_t cache_invalid_req      : 1;
        uint32_t reserved2              : 23;
    } BIT;
} CACHE_CTRL_REG;

typedef union {
    uint32_t reg;
    struct {
        uint32_t opcode_illegal        : 3;
        uint32_t reserved0             : 1;
        uint32_t mid_illegal           : 3;
        uint32_t reserved1             : 1;
        uint32_t nvr1_illegal          : 1;
        uint32_t nvr2_illegal          : 1;
        uint32_t reserved2             : 2;
        uint32_t region0_illegal       : 1;
        uint32_t region1_illegal       : 1;
        uint32_t reserved3             : 2;
        uint32_t parameter_illegal     : 1;
        uint32_t address_unmap         : 1;
        uint32_t reserved4             : 2;
        uint32_t ahb_error             : 1;
        uint32_t reserved5             : 11;
    } BIT;
} FLASH_STATUS_REG;

typedef union {
    uint32_t reg;
    struct {
        uint32_t magic_lock    : 32;
    } BIT;
} MAGIC_LOCK_REG;

typedef union {
    uint32_t reg;
    struct {
        uint32_t pgm_wdata : 32;
    } BIT;
} PGM_WDATA_REG;

typedef union {
    uint32_t reg;
    struct {
        uint32_t pgm_wdata_clr    : 1;
        uint32_t reserved0        : 7;
        uint32_t pgm_wbuf_cnt     : 8;
        uint32_t reserved1        : 16;
    } BIT;
} BUF_CLEAR_REG;

typedef struct {
    EFLASH_CMD_REG                  EFLASH_CMD;                   /**< Offset Address: 0x0000. */
    EFLASH_ADDR_REG                 EFLASH_ADDR;                  /**< Offset Address: 0x0004. */
    unsigned char                   space1[120];
    CMD_CFG_COMMON_REG              CMD_CFG_COMMON;               /**< Offset Address: 0x0080. */
    unsigned char                   space2[124];
    INT_RAW_STATUS_REG              INT_RAW_STATUS;               /**< Offset Address: 0x0100. */
    unsigned char                   space3[8];
    INT_CLEAR_REG                   INT_CLEAR;                    /**< Offset Address: 0x010C. */
    unsigned char                   space4[20];
    CACHE_CTRL_REG                  CACHE_CTRL;                   /**< Offset Address: 0x0124. */
    unsigned char                   space8[8];
    FLASH_STATUS_REG                FLASH_STATUS;                 /**< Offset Address: 0x0130. */
    unsigned char                   space5[204];
    MAGIC_LOCK_REG                  MAGIC_LOCK;                   /**< Offset Address: 0x0200. */
    unsigned char                   space6[508];
    PGM_WDATA_REG                   PGM_WDATA;                    /**< Offset Address: 0x0400. */
    unsigned char                   space7[508];
	unsigned int                    FLASH_RDATA;                  /**< Offset Address: 0x0600. */
    BUF_CLEAR_REG                   BUF_CLEAR;                    /**< Offset Address: 0x0604. */
} volatile EFC_RegStruct;


static int flash_read_worad(struct flash_bank *bank, uint32_t *src_addr, uint32_t dest_addr);

static uint32_t get_write_alignment_words(uint32_t dest_addr)
{
	uint32_t num_words = 0;

	num_words = dest_addr % FLASH_MAX_PGM_WORD_SIZE;
	if (num_words > 0) {
		return FLASH_MAX_PGM_WORD_SIZE - num_words;
	}
	return 0;
}

static int flash_write_words(struct flash_bank *bank, uint8_t *src_addr, uint32_t dest_addr, uint32_t word_num)
{
	struct target *target = bank->target;
	int retval = ERROR_OK;
	uint32_t read_reg;
	EFC_RegStruct *p = EFC;
	uint8_t *new_buffer = NULL;
	uint32_t front_pad_num;
	uint32_t total_word_num = word_num;
	uint32_t write_size;

	/* Calculate the number of pre-offset bytes. */
	front_pad_num = dest_addr % FLASH_MIN_PGM_WORDS_SIZE;
	/* The write address must be 16-byte aligned. */
	dest_addr -= front_pad_num;
	/* set EFLASH_ADDR.BIT.cmd_addr to dest_addr. */
	retval = target_write_u32(target, &(p->EFLASH_ADDR.reg), (dest_addr << 2));
	if (retval != ERROR_OK) {
		return retval;
	}

	total_word_num += front_pad_num;
	new_buffer = malloc(FLASH_ONE_WORD_BYTES_SIZE * total_word_num);
	if (new_buffer == NULL) {
		LOG_ERROR("malloc fail");
		return ERROR_FAIL;
	}
	/* Read back the value of the pre-offset memory for padding. */
	for (uint32_t i = 0; i < front_pad_num; i++) {
		uint32_t memory = 0xffffffff;
		retval = flash_read_worad(bank, &memory, FLASH_READ_BASE_ADDR + (dest_addr << 2) + i * FLASH_ONE_WORD_BYTES_SIZE);
		if (retval != ERROR_OK) {
			if (new_buffer != NULL) {
				free(new_buffer);
			}
			return retval;
		}
		new_buffer[i*FLASH_ONE_WORD_BYTES_SIZE + 3] = (uint8_t)(memory >> 24);
		new_buffer[i*FLASH_ONE_WORD_BYTES_SIZE + 2] = (uint8_t)(memory >> 16);
		new_buffer[i*FLASH_ONE_WORD_BYTES_SIZE + 1] = (uint8_t)(memory >> 8);
		new_buffer[i*FLASH_ONE_WORD_BYTES_SIZE + 0] = (uint8_t)(memory >> 0);
	}
	for (uint32_t i = front_pad_num; i < total_word_num; i++) {
		new_buffer[i*FLASH_ONE_WORD_BYTES_SIZE + 3] = src_addr[(i - front_pad_num) * FLASH_ONE_WORD_BYTES_SIZE + 3];
		new_buffer[i*FLASH_ONE_WORD_BYTES_SIZE + 2] = src_addr[(i - front_pad_num) * FLASH_ONE_WORD_BYTES_SIZE + 2];
		new_buffer[i*FLASH_ONE_WORD_BYTES_SIZE + 1] = src_addr[(i - front_pad_num) * FLASH_ONE_WORD_BYTES_SIZE + 1];
		new_buffer[i*FLASH_ONE_WORD_BYTES_SIZE + 0] = src_addr[(i - front_pad_num) * FLASH_ONE_WORD_BYTES_SIZE + 0];
	}
	retval = target_write_memory(target, &(p->PGM_WDATA.reg), FLASH_ONE_WORD_BYTES_SIZE, total_word_num, new_buffer);
	if (retval != ERROR_OK) {
		if (new_buffer != NULL) {
			free(new_buffer);
		}
		return retval;
	}
	free(new_buffer);

	/* Rear 16-byte alignment. */
	if ((total_word_num % FLASH_MIN_PGM_WORDS_SIZE) != 0) {
		for (uint32_t i = 0; i < (FLASH_MIN_PGM_WORDS_SIZE - total_word_num % FLASH_MIN_PGM_WORDS_SIZE); i++) {
			retval = target_write_u32(target, &(p->PGM_WDATA.reg), 0xffffffff);
			if (retval != ERROR_OK) {
				return retval;
			}
		}
		/* The minimum unit of writing to the flash memory is four words. */
		write_size = (total_word_num >> PAGE_SIZE_SHIFT) + 1;
	} else {
		write_size = total_word_num >> PAGE_SIZE_SHIFT;
	}

	retval = target_read_u32(target, &(p->EFLASH_CMD.reg), &read_reg);
	if (retval != ERROR_OK) {
		return retval;
	}
	/* set EFLASH_CMD.BIT.cmd_pgm_size to write_size. */
	read_reg = ((write_size & 0x3f) << 20) | (read_reg & 0xfc0fffff);
	/* set EFLASH_CMD.BIT.cmd_code to FLASH_OPERATION_PROGRAM. */
	read_reg = ((FLASH_OPERATION_PROGRAM & 0x7) << 8) | (read_reg & 0xfffff8ff);
	/* set EFLASH_CMD.BIT.cmd_start to 0x1. */
	read_reg = 0x1 | (read_reg & 0xfffffffe);
	retval = target_write_u32(target, &(p->EFLASH_CMD.reg), read_reg);
	if (retval != ERROR_OK) {
		return retval;
	}

	do {
		retval = target_read_u32(target, &(p->EFLASH_CMD.reg), &read_reg);
		if (retval != ERROR_OK) {
			return retval;
		}
	} while((read_reg & 0x1));

	return retval;
}

static int hm3065_info_init(struct flash_bank *bank)
{
	struct target *target = bank->target;
	EFC_RegStruct *p = EFC;
	int retval = ERROR_OK;

	if (target->state != TARGET_HALTED) {
		LOG_ERROR("Target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}

	retval = target_write_u32(target, &(p->MAGIC_LOCK.reg), FLASH_KEY_REGISTER_UNLOCK_VALUE);
	if (retval != ERROR_OK) {
		return retval;
	}
	/* set CMD_CFG_COMMON.BIT.int_mode to 0 */
	retval = target_write_u32(target, &(p->CMD_CFG_COMMON.reg), (0x0 << 1));
	if (retval != ERROR_OK) {
		return retval;
	}

	return retval;
}

static int hm3065_info_uninit(struct flash_bank *bank)
{
	struct target *target = bank->target;
	EFC_RegStruct *p = EFC;
	int retval = ERROR_OK;

	if (target->state != TARGET_HALTED) {
		LOG_ERROR("Target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}
	retval = target_write_u32(target, &(p->MAGIC_LOCK.reg), FLASH_KEY_REGISTER_LOCK_VALUE);
	if (retval != ERROR_OK) {
		return retval;
	}

	return retval;
}

static int hm3065_info_probe(struct flash_bank *bank)
{
	bank->base = FLASH_OFFSET_BASE_ADDR;
	return ERROR_OK;
}

FLASH_BANK_COMMAND_HANDLER(hm3065_info_flash_bank_command)
{
	uint32_t offset = 0;

	if (bank->sectors != NULL) {
		free(bank->sectors);
	}
	bank->write_start_alignment = 4; /* 4-byte alignment */
	bank->write_end_alignment = 4; /* 4-byte alignment */
	bank->size = HM3065_INFO_FLASH_PAGE_SIZE * HM3065_INFO_FLASH_PAGES;

	bank->num_sectors = HM3065_INFO_FLASH_PAGES;
	bank->sectors = malloc(sizeof(struct flash_sector) * bank->num_sectors);
	if (bank->sectors == NULL) {
		LOG_ERROR("malloc fail");
		return ERROR_FAIL;
	}
	for (uint32_t i = 0; i < bank->num_sectors; i++) {
		bank->sectors[i].offset = offset;
		bank->sectors[i].size = HM3065_INFO_FLASH_PAGE_SIZE;
		offset += HM3065_INFO_FLASH_PAGE_SIZE;
		bank->sectors[i].is_erased = -1;
		bank->sectors[i].is_protected = -1;
	}
	for (uint32_t i = 0; i < bank->num_sectors; i++) {
		bank->sectors[i].is_protected = 0;
	}

	return ERROR_OK;
}

static int hm3065_info_erase(struct flash_bank *bank, uint32_t first, uint32_t last)
{
	struct target *target = bank->target;
	int retval = ERROR_OK;
	uint32_t read_reg;
	EFC_RegStruct *p = EFC;

	if (target->state != TARGET_HALTED) {
		LOG_ERROR("Target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}
	hm3065_info_init(bank);

	for (uint32_t curr_page = first; curr_page <= last; curr_page++) {
		/* The unit of the flash address is word.The address is 4-byte aligned. Write register shifted left by 2 bits */
		retval = target_write_u32(target, &(p->EFLASH_ADDR.reg), ((FLASH_OFFSET_BASE_ADDR - FLASH_READ_BASE_ADDR + bank->sectors[curr_page].offset)/4 << 2));
		if (retval != ERROR_OK) {
			return retval;
		}

		retval = target_read_u32(target, &(p->EFLASH_CMD.reg), &read_reg);
		if (retval != ERROR_OK) {
			return retval;
		}
		/* set EFLASH_CMD.BIT.cmd_code to FLASH_OPERATION_ERASE. */
		read_reg = ((FLASH_OPERATION_ERASE & 0x7) << 8) | (read_reg & 0xfffff8ff);
		/* set EFLASH_CMD.BIT.cmd_start to 0x1. */
		read_reg = 0x1 | (read_reg & 0xfffffffe);
		retval = target_write_u32(target, &(p->EFLASH_CMD.reg), read_reg);
		if (retval != ERROR_OK) {
			return retval;
		}

		do {
			retval = target_read_u32(target, &(p->EFLASH_CMD.reg), &read_reg);
			if (retval != ERROR_OK) {
				return retval;
			}
		} while((read_reg & 0x1));
	}

	retval = target_read_u32(target, &(p->CACHE_CTRL.reg), &read_reg);
	if (retval != ERROR_OK) {
		return retval;
	}
	/* set CACHE_CTRL.BIT.cache_invalid_req to 0x1. */
	read_reg = (0x1 << 8) | (read_reg & 0xfffffeff);
	retval = target_write_u32(target, &(p->CACHE_CTRL.reg), read_reg);
	if (retval != ERROR_OK) {
		return retval;
	}
	/* O.K., erase successful */
	return retval;
}

static int hm3065_info_write_page(struct flash_bank *bank, const uint8_t *buffer, uint32_t offset, uint32_t count) 
{
	uint8_t *new_buffer = buffer;
	uint32_t num_words;
	uint32_t num_4words;
	struct target *target = bank->target;
	uint32_t read_reg;
	EFC_RegStruct *p = EFC;
	int retval = ERROR_OK;

	if (target->state != TARGET_HALTED) {
		LOG_ERROR("Target not halted");
		retval = ERROR_TARGET_NOT_HALTED;
		return retval;
	}

	offset += FLASH_OFFSET_BASE_ADDR - FLASH_READ_BASE_ADDR;
	if (offset & 0x3) { /* 4-byte alignment */
		LOG_ERROR("offset 0x%" PRIx32 " breaks required 4-word alignment", offset);
		return ERROR_FLASH_DST_BREAKS_ALIGNMENT;
	}

	num_words = count >> 2;
	/* The minimum unit of writing to the flash memory is four words. */
	num_4words = num_words >> 2;
	if (num_words % PAGE_WORD_SIZE) {
		num_4words += 1;
	}
	/* The unit of the flash address is word. */
	offset = offset >> 2;

	do {
		retval = flash_write_words(bank, new_buffer, offset, PAGE_WORD_SIZE);
		if (retval != ERROR_OK) {
			return retval;
		}
		new_buffer += PAGE_WORD_SIZE * FLASH_ONE_WORD_BYTES_SIZE;
		offset += PAGE_WORD_SIZE;
	} while (--num_4words);

	retval = target_read_u32(target, &(p->CACHE_CTRL.reg), &read_reg);
	if (retval != ERROR_OK) {
		return retval;
	}
	/* set CACHE_CTRL.BIT.cache_invalid_req to 0x1. */
	read_reg = (0x1 << 8) | (read_reg & 0xfffffeff);
	retval = target_write_u32(target, &(p->CACHE_CTRL.reg), read_reg);
	if (retval != ERROR_OK) {
		return retval;
	}

	hm3065_info_uninit(bank);

	return retval;
}

static int hm3065_info_write(struct flash_bank *bank, const uint8_t *buffer, uint32_t offset, uint32_t count) {
	uint32_t current_words;
	uint32_t current_len;
	uint32_t data_remain_bytes;
	uint32_t data_remain_words;
	uint8_t *new_buffer = buffer;
	struct target *target = bank->target;
	uint32_t read_reg;
	EFC_RegStruct *p = EFC;
	int retval = ERROR_OK;

	if (target->state != TARGET_HALTED) {
		LOG_ERROR("Target not halted");
		retval = ERROR_TARGET_NOT_HALTED;
		return retval;
	}

	offset += FLASH_OFFSET_BASE_ADDR - FLASH_READ_BASE_ADDR;
	if (offset & 0x3) { /* 4-byte alignment */
		LOG_ERROR("offset 0x%" PRIx32 " breaks required 4-word alignment", offset);
		return ERROR_FLASH_DST_BREAKS_ALIGNMENT;
	}

	/* The unit of the flash address is word. */
	offset = offset >> 2;
	current_words = get_write_alignment_words(offset);
	if ((count > (current_words * FLASH_ONE_WORD_BYTES_SIZE)) && (current_words > 0)) {
		retval = flash_write_words(bank, new_buffer, offset, current_words);
		if (retval != ERROR_OK) {
			return retval;
		}
		new_buffer += current_words * FLASH_ONE_WORD_BYTES_SIZE;
		offset += current_words;
		current_len = count - current_words * FLASH_ONE_WORD_BYTES_SIZE;
	} else {
		current_len = count;
	}

	uint32_t number_of_writes = current_len / (FLASH_MAX_PGM_WORD_SIZE * FLASH_ONE_WORD_BYTES_SIZE);
	for (uint32_t i = 0; i < number_of_writes; i++) {
		retval = flash_write_words(bank, new_buffer, offset, FLASH_MAX_PGM_WORD_SIZE);
		if (retval != ERROR_OK) {
			return retval;
		}
		new_buffer += FLASH_MAX_PGM_WORD_SIZE * FLASH_ONE_WORD_BYTES_SIZE;
		offset += FLASH_MAX_PGM_WORD_SIZE;
	}

	data_remain_bytes = (current_len % (FLASH_MAX_PGM_WORD_SIZE * FLASH_ONE_WORD_BYTES_SIZE));
	data_remain_words = data_remain_bytes % FLASH_ONE_WORD_BYTES_SIZE;
	if (data_remain_bytes > 0) {
		if (data_remain_words > 0) {
			retval = flash_write_words(bank, new_buffer, offset, data_remain_bytes / FLASH_ONE_WORD_BYTES_SIZE + 1);
		} else {
			retval = flash_write_words(bank, new_buffer, offset, data_remain_bytes / FLASH_ONE_WORD_BYTES_SIZE);
		}
		if (retval != ERROR_OK) {
			return retval;
		}
	}

	retval = target_read_u32(target, &(p->CACHE_CTRL.reg), &read_reg);
	if (retval != ERROR_OK) {
		return retval;
	}
	/* set CACHE_CTRL.BIT.cache_invalid_req to 0x1. */
	read_reg = (0x1 << 8) | (read_reg & 0xfffffeff);
	retval = target_write_u32(target, &(p->CACHE_CTRL.reg), read_reg);
	if (retval != ERROR_OK) {
		return retval;
	}

	hm3065_info_uninit(bank);

	return retval;
}

static int flash_read_worad(struct flash_bank *bank, uint32_t *src_addr, uint32_t dest_addr)
{
    struct target *target = bank->target;
    int retval = ERROR_OK;
	uint32_t read_reg;
	EFC_RegStruct *p = EFC;

    /* set EFLASH_ADDR.BIT.cmd_addr to dest_addr. */
	retval = target_write_u32(target, &(p->EFLASH_ADDR.reg), (dest_addr << 2));
	if (retval != ERROR_OK) {
		return retval;
	}

    retval = target_read_u32(target, &(p->EFLASH_CMD.reg), &read_reg);
	if (retval != ERROR_OK) {
		return retval;
	}
    /* set EFLASH_CMD.BIT.read_size. set [28:29] = 00, 1 word*/
	read_reg = read_reg & 0xcfffffff;
    /* set EFLASH_CMD.BIT.cmd_code to FLASH_OPERATION_READ. */
	read_reg = ((FLASH_OPERATION_READ & 0x7) << 8) | (read_reg & 0xfffff8ff);
	/* set EFLASH_CMD.BIT.cmd_start to 0x1. */
	read_reg = 0x1 | (read_reg & 0xfffffffe);
	retval = target_write_u32(target, &(p->EFLASH_CMD.reg), read_reg);
	if (retval != ERROR_OK) {
		return retval;
	}

	do {
		retval = target_read_u32(target, &(p->EFLASH_CMD.reg), &read_reg);
		if (retval != ERROR_OK) {
			return retval;
		}
	} while((read_reg & 0x1));

	retval = target_read_u32(target, &(p->INT_RAW_STATUS.reg), &read_reg);
	if (retval != ERROR_OK) {
		return retval;
	}
	if (read_reg != 0) {
		return ERROR_FAIL;
	}

    retval = target_read_u32(target, &(p->FLASH_RDATA), src_addr);
	if (retval != ERROR_OK) {
		return retval;
	}
    return retval;
}

static int hm3065_info_read(struct flash_bank *bank, uint8_t *buffer, uint32_t offset, uint32_t count)
{
	struct target *target = bank->target;
	uint32_t read_reg;
	uint32_t dest_addr;
	uint32_t num_words;
	uint32_t datalast;
	uint8_t temp_data[4]; /* 4-byte alignment */
	int retval = ERROR_OK;

	if (target->state != TARGET_HALTED) {
		LOG_ERROR("Target not halted");
		retval = ERROR_TARGET_NOT_HALTED;
		return retval;
	}

	hm3065_info_init(bank);

	offset += FLASH_OFFSET_BASE_ADDR - FLASH_READ_BASE_ADDR;
	if (offset & 0x3) { /* 4-byte alignment */
		LOG_ERROR("offset 0x%" PRIx32 " breaks required 4-word alignment", offset);
		return ERROR_FLASH_DST_BREAKS_ALIGNMENT;
	}

	/* The unit of the flash address is word. */
	dest_addr = offset >> 2;
	num_words = count >> 2;
    LOG_INFO("dest_addr:0x%x, num_words:%d, offset:%x", dest_addr, num_words,offset);
	for (int i = 0; i < num_words; i++) {
        retval = flash_read_worad(bank, (uint32_t)temp_data, dest_addr);
		if (retval != ERROR_OK) {
			return retval;
		}
		for (int j = 0; j < 4; j++) { /* 4-byte alignment */
			*buffer++ = temp_data[j];
		}
		dest_addr++;
	}

    datalast = count % 4; /* 4-byte alignment */
	if (datalast != 0) {
	     retval = flash_read_worad(bank, (uint32_t)temp_data, dest_addr);
		if (retval != ERROR_OK) {
			return retval;
		}
		for (int k = 0; k < datalast; k++) {
			*buffer++ = temp_data[k];
		}	
	}

	hm3065_info_uninit(bank);

	return retval;
}

static int hm3065_info_verify(struct flash_bank *bank, const uint8_t *buffer, uint32_t offset, uint32_t count)
{
    int retval = ERROR_OK;
    uint8_t *target_buffer = NULL;

	if (bank == NULL || buffer == NULL) {
	    return ERROR_FAIL;
	}
    target_buffer = malloc(sizeof(uint8_t) *count);
	if (target_buffer == NULL) {
	    return ERROR_FAIL;
	}
    retval = hm3065_info_read(bank, target_buffer, offset, count);
	if (retval != ERROR_OK) {
		return ERROR_FAIL;
	}
	for (int i = 0; i < count; i++) {
		if (buffer[i] != target_buffer[i]) {
			return ERROR_FAIL;
		}
	}
    return ERROR_OK;
}

struct flash_driver hm3065_info_flash = {
	.name = "hm3065_info",
	.flash_bank_command = hm3065_info_flash_bank_command,
	.erase = hm3065_info_erase,
	.write = hm3065_info_write,
	.read = hm3065_info_read,
	.probe = hm3065_info_probe,
	.auto_probe = hm3065_info_probe,
	.erase_check = default_flash_blank_check,
	.free_driver_priv = default_flash_free_driver_priv,
	.verify = hm3065_info_verify,
};