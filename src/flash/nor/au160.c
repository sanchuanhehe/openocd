/*
* Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2023-2023. All rights reserved.
* Description: Flash drive of the 160KB main area
* Create: 2023-5-26
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
#define AU160_FLASH_PAGE_SIZE    0x2000
#define AU160_FLASH_PAGES        20
#define PAGE_SIZE_SHIFT              (2)    /* Flash needs to be programmed in units of 2 ^ 2 =  bytes */
#define SECTOR_SIZE_SHIFT            (13)   /* Flash needs to be erased in units of 2 ^ 13 = 8192 bytes */
#define FLASH_MAX_PGM_WORD_SIZE      0x80
#define FLASH_MIN_PGM_BYTES_SIZE     0x10
#define PAGE_WORD_SIZE               4
#define FLASH_MIN_PGM_WORDS_SIZE     4
#define FLASH_ONE_WORD_BYTES_SIZE    4

#define FLASH_OPERATION_PROGRAM       0x00000002U
#define FLASH_OPERATION_ERASE         0x00000004U
#define FLASH_OPERATION_MASS_ERASE    0x00000006U

#define FLASH_READ_BASE_ADDR          0x3000000

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
    unsigned char                   space7[512];
    BUF_CLEAR_REG                   BUF_CLEAR;                    /**< Offset Address: 0x0604. */
} volatile EFC_RegStruct;


int g_au160WriteFlag = 0;

static uint32_t GetWriteAlignmentWords(uint32_t destAddr)
{
	uint32_t numWords = 0;

	numWords = destAddr % FLASH_MAX_PGM_WORD_SIZE;
	if (numWords > 0) {
		return FLASH_MAX_PGM_WORD_SIZE - numWords;
	}
	return 0;
}

static int FLASH_WriteWords(struct flash_bank *bank, uint8_t *srcAddr, uint32_t destAddr, uint32_t wordNum)
{
	struct target *target = bank->target;
	int retval = ERROR_OK;
	uint32_t readReg;
	EFC_RegStruct *p = EFC;
	uint8_t *newBuffer = NULL;
	uint32_t frontPadNum;
	uint32_t totalWordNum = wordNum;
	uint32_t writeSize;

	/* Check whether the bus is busy. */
	// retval = target_read_u32(target, &(p->EFLASH_CMD.reg), &readReg);
	// if (retval != ERROR_OK) {
	// 	return retval;
	// } else if (readReg & 0x1) {
	// 	return 1;
	// }
	/* unlock */
	// retval = target_write_u32(target, &(p->MAGIC_LOCK.reg), FLASH_KEY_REGISTER_UNLOCK_VALUE);
	// if (retval != ERROR_OK) {
	// 	return retval;
	// }
	// retval = target_read_u32(target, &(p->BUF_CLEAR.reg), &readReg);
	// if (retval != ERROR_OK) {
	// 	return retval;
	// } else if (((readReg >> 8) & 0xff) != 0) {
	// 	/* set BUF_CLEAR.BIT.pgm_wdata_clr to 0x1. */
	// 	readReg |= 0x1;
	// 	retval = target_write_u32(target, &(p->BUF_CLEAR.reg), readReg);
	// 	if (retval != ERROR_OK) {
	// 		return retval;
	// 	}
	// }

	/* Calculate the number of pre-offset bytes. */
	frontPadNum = destAddr % FLASH_MIN_PGM_WORDS_SIZE;
	/* The write address must be 16-byte aligned. */
	destAddr -= frontPadNum;
	/* set EFLASH_ADDR.BIT.cmd_addr to destAddr. */
	retval = target_write_u32(target, &(p->EFLASH_ADDR.reg), (destAddr << 2));
	if (retval != ERROR_OK) {
		return retval;
	}

	totalWordNum += frontPadNum;
	newBuffer = malloc(FLASH_ONE_WORD_BYTES_SIZE * totalWordNum);
	if (newBuffer == NULL) {
		return ERROR_FAIL;
	}
	/* Read back the value of the pre-offset memory for padding. */
	for (uint32_t i = 0; i < frontPadNum; i++) {
		uint32_t memory = 0xffffffff;
		retval = target_read_u32(target, FLASH_READ_BASE_ADDR + (destAddr << 2) + i * FLASH_ONE_WORD_BYTES_SIZE, &memory);
		if (retval != ERROR_OK) {
			return retval;
		}
		// LOG_INFO("memory %x address %x", memory, (FLASH_READ_BASE_ADDR + (destAddr << 2) + i * FLASH_ONE_WORD_BYTES_SIZE));
		newBuffer[i*FLASH_ONE_WORD_BYTES_SIZE + 3] = (uint8_t)(memory >> 24);
		newBuffer[i*FLASH_ONE_WORD_BYTES_SIZE + 2] = (uint8_t)(memory >> 16);
		newBuffer[i*FLASH_ONE_WORD_BYTES_SIZE + 1] = (uint8_t)(memory >> 8);
		newBuffer[i*FLASH_ONE_WORD_BYTES_SIZE + 0] = (uint8_t)(memory >> 0);
	}
	for (uint32_t i = frontPadNum; i < totalWordNum; i++) {
		newBuffer[i*FLASH_ONE_WORD_BYTES_SIZE + 3] = srcAddr[(i - frontPadNum) * FLASH_ONE_WORD_BYTES_SIZE + 3];
		newBuffer[i*FLASH_ONE_WORD_BYTES_SIZE + 2] = srcAddr[(i - frontPadNum) * FLASH_ONE_WORD_BYTES_SIZE + 2];
		newBuffer[i*FLASH_ONE_WORD_BYTES_SIZE + 1] = srcAddr[(i - frontPadNum) * FLASH_ONE_WORD_BYTES_SIZE + 1];
		newBuffer[i*FLASH_ONE_WORD_BYTES_SIZE + 0] = srcAddr[(i - frontPadNum) * FLASH_ONE_WORD_BYTES_SIZE + 0];
	}
	// LOG_INFO("destAddr %x, newBuffer:%x%x%x%x", destAddr, newBuffer[frontPadNum*FLASH_ONE_WORD_BYTES_SIZE + 0], newBuffer[frontPadNum*FLASH_ONE_WORD_BYTES_SIZE + 1],
	// 													  newBuffer[frontPadNum*FLASH_ONE_WORD_BYTES_SIZE + 2], newBuffer[frontPadNum*FLASH_ONE_WORD_BYTES_SIZE + 3]);

	retval = target_write_memory(target, &(p->PGM_WDATA.reg), FLASH_ONE_WORD_BYTES_SIZE, totalWordNum, newBuffer);
	if (retval != ERROR_OK) {
		return retval;
	}
	free(newBuffer);

	/* Rear 16-byte alignment. */
	if ((totalWordNum % FLASH_MIN_PGM_WORDS_SIZE) != 0) {
		for (uint32_t i = 0; i < (FLASH_MIN_PGM_WORDS_SIZE - totalWordNum % FLASH_MIN_PGM_WORDS_SIZE); i++) {
			retval = target_write_u32(target, &(p->PGM_WDATA.reg), 0xffffffff);
			if (retval != ERROR_OK) {
				return retval;
			}
		}
		/* The minimum unit of writing to the flash memory is four words. */
		writeSize = (totalWordNum >> PAGE_SIZE_SHIFT) + 1;
	} else {
		writeSize = totalWordNum >> PAGE_SIZE_SHIFT;
	}

	retval = target_read_u32(target, &(p->EFLASH_CMD.reg), &readReg);
	if (retval != ERROR_OK) {
		return retval;
	}
	/* set EFLASH_CMD.BIT.cmd_pgm_size to writeSize. */
	readReg = ((writeSize & 0x3f) << 20) | (readReg & 0xfc0fffff);
	/* set EFLASH_CMD.BIT.cmd_code to FLASH_OPERATION_PROGRAM. */
	readReg = ((FLASH_OPERATION_PROGRAM & 0x7) << 8) | (readReg & 0xfffff8ff);
	/* set EFLASH_CMD.BIT.cmd_start to 0x1. */
	readReg = 0x1 | (readReg & 0xfffffffe);
	retval = target_write_u32(target, &(p->EFLASH_CMD.reg), readReg);
	if (retval != ERROR_OK) {
		return retval;
	}

	do {
		retval = target_read_u32(target, &(p->EFLASH_CMD.reg), &readReg);
		if (retval != ERROR_OK) {
			return retval;
		}
	} while((readReg & 0x1));

	// retval = target_read_u32(target, &(p->INT_RAW_STATUS.reg), &readReg);
	// if (retval != ERROR_OK) {
	// 	return retval;
	// }
	/* check whether the value of INT_RAW_STATUS.BIT.int_raw_err_illegal or INT_RAW_STATUS.BIT.int_raw_err_smwr is 1. */
	// if ((readReg & (0x1 << 16)) || (readReg & (0x1 << 17))) {
	// 	/* set INT_CLEAR.BIT.int_clr_err_illegal and INT_CLEAR.BIT.int_clr_err_smwr to 1. */
	// 	readReg = 0xffffffff;
	// 	retval = target_write_u32(target, &(p->INT_CLEAR.reg), readReg);
	// 	if (retval != ERROR_OK) {
	// 		return retval;
	// 	}
	// 	retval = target_write_u32(target, &(p->MAGIC_LOCK.reg), FLASH_KEY_REGISTER_LOCK_VALUE);
	// 	if (retval != ERROR_OK) {
	// 		return retval;
	// 	}
	// 	return 1;
	// }
	/* lock */
	// retval = target_write_u32(target, &(p->MAGIC_LOCK.reg), FLASH_KEY_REGISTER_LOCK_VALUE);
	// if (retval != ERROR_OK) {
	// 	return retval;
	// }

	return retval;
}

static int au160_Init(struct flash_bank *bank)
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
	// retval = target_write_u32(target, &(p->MAGIC_LOCK.reg), FLASH_KEY_REGISTER_LOCK_VALUE);
	// if (retval != ERROR_OK) {
	// 	return retval;
	// }

	return retval;
}

static int au160_UnInit(struct flash_bank *bank)
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

static int au160_probe(struct flash_bank *bank)
{
	bank->base = FLASH_READ_BASE_ADDR;
	return ERROR_OK;
}

FLASH_BANK_COMMAND_HANDLER(au160_flash_bank_command)
{
	uint32_t offset = 0;

	if (bank->sectors != NULL) {
		free(bank->sectors);
	}
	bank->write_start_alignment = 4;
	bank->write_end_alignment = 4;
	bank->size = AU160_FLASH_PAGE_SIZE * AU160_FLASH_PAGES;

	bank->num_sectors = AU160_FLASH_PAGES;
	bank->sectors = malloc(sizeof(struct flash_sector) * bank->num_sectors);
	if (bank->sectors == NULL) {
		return ERROR_FAIL;
	}
	for (uint32_t i = 0; i < bank->num_sectors; i++) {
		bank->sectors[i].offset = offset;
		bank->sectors[i].size = AU160_FLASH_PAGE_SIZE;
		offset += AU160_FLASH_PAGE_SIZE;
		bank->sectors[i].is_erased = -1;
		bank->sectors[i].is_protected = -1;
	}
	for (uint32_t i = 0; i < bank->num_sectors; i++) {
		bank->sectors[i].is_protected = 0;
	}

	return ERROR_OK;
}

static int au160_erase(struct flash_bank *bank, uint32_t first, uint32_t last)
{
	// g_au160WriteFlag = 0;
	struct target *target = bank->target;
	int retval = ERROR_OK;
	uint32_t readReg;
	EFC_RegStruct *p = EFC;

	if (target->state != TARGET_HALTED) {
		LOG_ERROR("Target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}
	au160_Init(bank);

	// retval = target_read_u32(target, &(p->EFLASH_CMD.reg), &readReg);
	// if (retval != ERROR_OK) {
	// 	return retval;
	// } else if (readReg & 0x1) {
	// 	return 1;
	// }

	// retval = target_write_u32(target, &(p->MAGIC_LOCK.reg), FLASH_KEY_REGISTER_UNLOCK_VALUE);
	// if (retval != ERROR_OK) {
	// 	return retval;
	// }
	for (uint32_t curr_page = first; curr_page <= last; curr_page++) {
		/* The unit of the flash address is word. */
		// LOG_INFO("p->EFLASH_ADDR.BIT.cmd_addr = 0x%x, bank->sectors[curr_page].offset = 0x%x", (&p->EFLASH_ADDR.reg), (bank->sectors[curr_page].offset)/4);
		retval = target_write_u32(target, &(p->EFLASH_ADDR.reg), ((bank->sectors[curr_page].offset)/4 << 2));
		if (retval != ERROR_OK) {
			return retval;
		}

		retval = target_read_u32(target, &(p->EFLASH_CMD.reg), &readReg);
		if (retval != ERROR_OK) {
			return retval;
		}
		/* set EFLASH_CMD.BIT.cmd_code to FLASH_OPERATION_ERASE. */
		readReg = ((FLASH_OPERATION_ERASE & 0x7) << 8) | (readReg & 0xfffff8ff);
		/* set EFLASH_CMD.BIT.cmd_start to 0x1. */
		readReg = 0x1 | (readReg & 0xfffffffe);
		retval = target_write_u32(target, &(p->EFLASH_CMD.reg), readReg);
		if (retval != ERROR_OK) {
			return retval;
		}

		do {
			retval = target_read_u32(target, &(p->EFLASH_CMD.reg), &readReg);
			if (retval != ERROR_OK) {
				return retval;
			}
		} while((readReg & 0x1));

		// retval = target_read_u32(target, &(p->INT_RAW_STATUS.reg), &readReg);
		// if (retval != ERROR_OK) {
		// 	return retval;
		// }
		/* check whether the value of INT_RAW_STATUS.BIT.int_raw_err_illegal or INT_RAW_STATUS.BIT.int_raw_err_smwr is 1. */
		// if ((readReg & (0x1 << 16)) || (readReg & (0x1 << 17))) {
		// 	/* set INT_CLEAR.BIT.int_clr_err_illegal and INT_CLEAR.BIT.int_clr_err_smwr to 1. */
		// 	readReg = 0xffffffff;
		// 	retval = target_write_u32(target, &(p->INT_CLEAR.reg), readReg);
		// 	if (retval != ERROR_OK) {
		// 		return retval;
		// 	}
		// 	retval = target_write_u32(target, &(p->MAGIC_LOCK.reg), FLASH_KEY_REGISTER_LOCK_VALUE);
		// 	if (retval != ERROR_OK) {
		// 		return retval;
		// 	}
		// 	return 1;
		// }
	}
	// retval = target_write_u32(target, &(p->MAGIC_LOCK.reg), FLASH_KEY_REGISTER_LOCK_VALUE);
	// if (retval != ERROR_OK) {
	// 	return retval;
	// }
	retval = target_read_u32(target, &(p->CACHE_CTRL.reg), &readReg);
	if (retval != ERROR_OK) {
		return retval;
	}
	/* set CACHE_CTRL.BIT.cache_invalid_req to 0x1. */
	readReg = (0x1 << 8) | (readReg & 0xfffffeff);
	retval = target_write_u32(target, &(p->CACHE_CTRL.reg), readReg);
	if (retval != ERROR_OK) {
		return retval;
	}

	/* O.K., erase successful */
	return retval;
}

static int au160_writepage(struct flash_bank *bank, const uint8_t *buffer, uint32_t offset, uint32_t count) 
{
	uint8_t *newBuffer = buffer;
	uint32_t numWords;
	uint32_t num4Words;
	struct target *target = bank->target;
	uint32_t readReg;
	EFC_RegStruct *p = EFC;
	int retval = ERROR_OK;

	if (target->state != TARGET_HALTED) {
		LOG_ERROR("Target not halted");
		retval = ERROR_TARGET_NOT_HALTED;
		return retval;
	}
	if (offset & 0x3) {
		LOG_ERROR("offset 0x%" PRIx32 " breaks required 4-word alignment", offset);
		return ERROR_FLASH_DST_BREAKS_ALIGNMENT;
	}

	numWords = count >> 2;
	/* The minimum unit of writing to the flash memory is four words. */
	num4Words = numWords >> 2;
	//LOG_INFO("count = %d, numWords = %d, num4Words = %d", count, numWords, num4Words);
	if (numWords % PAGE_WORD_SIZE) {
		num4Words += 1;
	}
	/* The unit of the flash address is word. */
	offset = offset >> 2;

	do {
		retval = FLASH_WriteWords(bank, newBuffer, offset, PAGE_WORD_SIZE);
		if (retval != ERROR_OK) {
			return retval;
		}
		newBuffer += PAGE_WORD_SIZE * FLASH_ONE_WORD_BYTES_SIZE;
		offset += PAGE_WORD_SIZE;
	} while (--num4Words);

	retval = target_read_u32(target, &(p->CACHE_CTRL.reg), &readReg);
	if (retval != ERROR_OK) {
		return retval;
	}
	/* set CACHE_CTRL.BIT.cache_invalid_req to 0x1. */
	readReg = (0x1 << 8) | (readReg & 0xfffffeff);
	retval = target_write_u32(target, &(p->CACHE_CTRL.reg), readReg);
	if (retval != ERROR_OK) {
		return retval;
	}

	au160_UnInit(bank);

	return retval;
}

static int au160_write(struct flash_bank *bank, const uint8_t *buffer, uint32_t offset, uint32_t count) {
	uint32_t currentWords;
	uint32_t currentLen;
	uint32_t dataRemainBytes;
	uint32_t dataRemainWords;
	uint8_t *newBuffer = buffer;
	struct target *target = bank->target;
	uint32_t readReg;
	EFC_RegStruct *p = EFC;
	int retval = ERROR_OK;

	if (target->state != TARGET_HALTED) {
		LOG_ERROR("Target not halted");
		retval = ERROR_TARGET_NOT_HALTED;
		return retval;
	}

	if (offset & 0x3) {
		LOG_ERROR("offset 0x%" PRIx32 " breaks required 4-word alignment", offset);
		return ERROR_FLASH_DST_BREAKS_ALIGNMENT;
	}

	/* The unit of the flash address is word. */
	offset = offset >> 2;
	currentWords = GetWriteAlignmentWords(offset);
	// LOG_INFO("offset %x, currentWords %d, count %d", offset, currentWords, count);

	// retval = target_write_u32(target, &(p->MAGIC_LOCK.reg), FLASH_KEY_REGISTER_UNLOCK_VALUE);
	// if (retval != ERROR_OK) {
	// 	return retval;
	// }

	if ((count > (currentWords * FLASH_ONE_WORD_BYTES_SIZE)) && (currentWords > 0)) {
		retval = FLASH_WriteWords(bank, newBuffer, offset, currentWords);
		if (retval != ERROR_OK) {
			return retval;
		}
		newBuffer += currentWords * FLASH_ONE_WORD_BYTES_SIZE;
		offset += currentWords;
		currentLen = count - currentWords * FLASH_ONE_WORD_BYTES_SIZE;
	} else {
		currentLen = count;
	}

	uint32_t numberOfWrites = currentLen / (FLASH_MAX_PGM_WORD_SIZE * FLASH_ONE_WORD_BYTES_SIZE);
	for (uint32_t i = 0; i < numberOfWrites; i++) {
		retval = FLASH_WriteWords(bank, newBuffer, offset, FLASH_MAX_PGM_WORD_SIZE);
		if (retval != ERROR_OK) {
			return retval;
		}
		newBuffer += FLASH_MAX_PGM_WORD_SIZE * FLASH_ONE_WORD_BYTES_SIZE;
		offset += FLASH_MAX_PGM_WORD_SIZE;
	}

	dataRemainBytes = (currentLen % (FLASH_MAX_PGM_WORD_SIZE * FLASH_ONE_WORD_BYTES_SIZE));
	dataRemainWords = dataRemainBytes % FLASH_ONE_WORD_BYTES_SIZE;
	// LOG_INFO("dataRemainBytes %d, dataRemainWords %d", dataRemainBytes, dataRemainWords);
	if (dataRemainBytes > 0) {
		if (dataRemainWords > 0) {
			retval = FLASH_WriteWords(bank, newBuffer, offset, dataRemainBytes / FLASH_ONE_WORD_BYTES_SIZE + 1);
		} else {
			retval = FLASH_WriteWords(bank, newBuffer, offset, dataRemainBytes / FLASH_ONE_WORD_BYTES_SIZE);
		}
		if (retval != ERROR_OK) {
			return retval;
		}
	}

	// retval = target_write_u32(target, &(p->MAGIC_LOCK.reg), FLASH_KEY_REGISTER_LOCK_VALUE);
	// if (retval != ERROR_OK) {
	// 	return retval;
	// }
	retval = target_read_u32(target, &(p->CACHE_CTRL.reg), &readReg);
	if (retval != ERROR_OK) {
		return retval;
	}
	/* set CACHE_CTRL.BIT.cache_invalid_req to 0x1. */
	readReg = (0x1 << 8) | (readReg & 0xfffffeff);
	retval = target_write_u32(target, &(p->CACHE_CTRL.reg), readReg);
	if (retval != ERROR_OK) {
		return retval;
	}

	au160_UnInit(bank);

	// g_au160WriteFlag = 0;
	return retval;
}

struct flash_driver au160_flash = {
	.name = "au160",
	.flash_bank_command = au160_flash_bank_command,
	.erase = au160_erase,
	.write = au160_write,
	.read = default_flash_read,
	.probe = au160_probe,
	.auto_probe = au160_probe,
	.erase_check = default_flash_blank_check,
	.free_driver_priv = default_flash_free_driver_priv,
};