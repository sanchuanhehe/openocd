/*
* Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2023-2023. All rights reserved.
* Description: 3066H flash drive of OpenOCD
* Create: 2023-5-26
*/
#include <stdio.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "imp.h"
#include <target/image.h>
#include "chiptype.h"

#define hi3066h_INFO_FLASH_PAGES            0x20
#define hi3066h_INFO_SECTORS_SIZE            0x400
#define U32 unsigned long
#define PAGE_WORD_SIZE               4
#define FLASH_BASE_ADDR               0x3800000
#define FLASH_RAED_BASE_ADDR               0x3000000
#define FLASH_ONE_WORD_BYTES_SIZE    4
#define FLASH_MIN_PGM_WORDS_SIZE     2
#define FLASH_KEY_REGISTER_UNLOCK_VALUE   0xFEDCBA98
#define FLASH_OPERATION_PROGRAM       0x00000003U
#define FLASH_KEY_REGISTER_LOCK_VALUE     0x0
#define FLASH_OPERATION_ERASE         0x00000005U
#define FLASH_ROW_WORD_SIZE          64 //256bytes / 4 = 128 words
#define FLASH_MAX_PGM_WORD_SIZE      64
#define PAGE_SIZE_SHIFT                 2   /* Flash needs to be programmed in units of 2 ^ 2 =  bytes */

#define SECTOR_SIZE_SHIFT            (10)   // Flash needs to be erased in units of 2 ^ 10 = 1024 bytes
#define FLASH_ONE_PAGE_SIZE          0x400  //Size of a page,unit: bytes. 1K. 

target_addr_t g_info_base_adress = 0x3800000; // cpu0

int last_info_flag = 0;

typedef union {
    U32 reg;
    struct {
        U32    cmd_start             : 1   ; /* [0] */
        U32    reserved_0            : 5   ; /* [5..1] */
        U32    exec_state            : 2   ; /* [7..6] */
        U32    cmd_code              : 3   ; /* [10..8] */
        U32    reserved_1            : 1   ; /* [11] */
        U32    cmd_meb               : 2   ; /* [13..12] */
        U32    reserved_2            : 6   ; /* [19..14] */
        U32    cmd_pgm_size          : 6   ; /* [25..20] */
        U32    reserved_3            : 2   ; /* [27..26] */
        U32    cmd_read_size         : 2   ; /* [29..28] */
        U32    reserved_4            : 2   ; /* [31..30] */
    } BIT;
} EFLASH_CMD_REG;

typedef union {
    U32 reg;
    struct {
        U32 reserved0        : 2;
        U32 cmd_addr         : 22;
        U32 reserved1        : 8;
    } BIT;
} EFLASH_ADDR_REG;

typedef union {
    U32 reg;
    struct {
        U32 reserved0        : 1;
        U32 int_mode         : 1;
        U32 reserved1        : 30;
    } BIT;
} CMD_CFG_COMMON_REG;

typedef union {
    U32 reg;
    struct {
        U32    reserved_0            : 4   ; /* [3..0] */
        U32    int_raw_finish        : 1   ; /* [4] */
        U32    reserved_1            : 11  ; /* [15..5] */
        U32    int_raw_err_illegal   : 1   ; /* [16] */
        U32    int_raw_err_tcm       : 1   ; /* [17] */
        U32    reserved_2            : 2   ; /* [19..18] */
        U32    int_raw_err_erase     : 2   ; /* [21..20] */
        U32    int_raw_err_ecc_corr  : 2   ; /* [23..22] */
        U32    int_raw_err_ecc_chk   : 2   ; /* [25..24] */
        U32    reserved_3            : 6   ; /* [31..26] */
    } BIT;
} INT_RAW_STATUS_REG;

typedef union {
    U32 reg;
    struct {
        U32    reserved_0            : 4   ; /* [3..0] */
        U32    int_clr_finish        : 1   ; /* [4] */
        U32    reserved_1            : 11  ; /* [15..5] */
        U32    int_clr_err_illegal   : 1   ; /* [16] */
        U32    int_clr_err_tcm       : 1   ; /* [17] */
        U32    reserved_2            : 2   ; /* [19..18] */
        U32    int_clr_err_erase     : 2   ; /* [21..20] */
        U32    int_clr_err_ecc_corr  : 2   ; /* [23..22] */
        U32    int_clr_err_ecc_chk   : 2   ; /* [25..24] */
        U32    reserved_3            : 6   ; /* [31..26] */
    } BIT;
} INT_CLEAR_REG;

typedef union {
    U32 reg;
    struct {
        U32    cache_enable0         : 1   ; /* [0] */
        U32    cache_enable1         : 1   ; /* [1] */
        U32    reserved_0            : 2   ; /* [3..2] */
        U32    cache_replacement_sel : 1   ; /* [4] */
        U32    reserved_1            : 3   ; /* [7..5] */
        U32    cache_invalid_req0    : 1   ; /* [8] */
        U32    cache_invalid_req1    : 1   ; /* [9] */
        U32    reserved_2            : 2   ; /* [11..10] */
        U32    cache_policy_sel      : 1   ; /* [12] */
        U32    reserved_3            : 19  ; /* [31..13] */
    } BIT;
} HIRDACCLR_CACHE_CTRL_REG;

typedef union {
    U32 reg;
    struct {
        U32    opcode_illegal        : 3   ; /* [2..0] */
        U32    reserved_0            : 1   ; /* [3] */
        U32    mid_illegal           : 4   ; /* [7..4] */
        U32    info_rgn0_illegal     : 1   ; /* [8] */
        U32    info_rgn1_illegal     : 1   ; /* [9] */
        U32    info_rgn2_illegal     : 1   ; /* [10] */
        U32    info_rgn3_illegal     : 1   ; /* [11] */
        U32    info_rgn4_illegal     : 1   ; /* [12] */
        U32    reserved_1            : 2   ; /* [14..13] */
        U32    main_rgn0_illegal     : 1   ; /* [15] */
        U32    main_rgn1_illegal     : 1   ; /* [16] */
        U32    main_rgn2_illegal     : 1   ; /* [17] */
        U32    reserved_2            : 2   ; /* [19..18] */
        U32    parameter_illegal     : 1   ; /* [20] */
        U32    address_unmap         : 1   ; /* [21] */
        U32    reserved_3            : 10  ; /* [31..22] */
    } BIT;
} FLASH_STATUS_REG;

typedef union {
    U32 reg;
    struct {
        U32 magic_lock    : 32;
    } BIT;
} MAGIC_LOCK_REG;

typedef union {
    U32 reg;
    struct {
        U32 pgm_wdata : 32;
    } BIT;
} PGM_WDATA_REG;

typedef union {
    U32 reg;
    struct {
        U32 pgm_wdata_clr    : 1;
        U32 reserved0        : 7;
        U32 pgm_wbuf_cnt     : 8;
        U32 reserved1        : 16;
    } BIT;
} BUF_CLEAR_REG;

typedef struct {
    EFLASH_CMD_REG                  EFLASH_CMD;                  /**< Offset Address: 0x0000. */
    EFLASH_ADDR_REG                 EFLASH_ADDR;                 /**< Offset Address: 0x0004. */
    unsigned char                   space1[120];
    CMD_CFG_COMMON_REG              CMD_CFG_COMMON;              /**< Offset Address: 0x0080. */
    unsigned char                   space2[124];
    INT_RAW_STATUS_REG              INT_RAW_STATUS;              /**< Offset Address: 0x0100. */
    unsigned char                   space3[8];
    INT_CLEAR_REG                   INT_CLEAR;                   /**< Offset Address: 0x010C. */
    unsigned char                   space4[24];

    unsigned char                   space5[8];
    FLASH_STATUS_REG                FLASH_STATUS;                /**< Offset Address: 0x0130. */
    unsigned char                   space6[204];
    MAGIC_LOCK_REG                  MAGIC_LOCK;                  /**< Offset Address: 0x0200. */
    unsigned char                   space7[508];
    PGM_WDATA_REG                   PGM_WDATA;                   /**< Offset Address: 0x0400. */
    unsigned char                   space8[512];
    BUF_CLEAR_REG                   BUF_CLEAR;                   /**< Offset Address: 0x0604. */
    unsigned char                   space9[2844];
    HIRDACCLR_CACHE_CTRL_REG        CACHE_CTRL;                  /**< Offset Address: 0x01124. */
} volatile hi3066h_efc_reg;

#define EFC ((hi3066h_efc_reg *)0x14710000)

struct target *g_hi3066h_info_target = NULL;

static int set_reg_value(target_addr_t addr, uint32_t value, uint32_t mask, uint32_t offset)
{
    int retval = ERROR_OK;
    uint32_t read_reg = 0;
    if (mask != 0x0) {
        if (target_read_u32(g_hi3066h_info_target, addr, &read_reg) != ERROR_OK) {
            return ERROR_TARGET_INVALID;
        }
    }

    read_reg = (read_reg & (mask)) | (value << offset);
    retval = target_write_u32(g_hi3066h_info_target, addr, read_reg);
    if (retval != ERROR_OK) {
		return retval;
	}
    return retval;
}

static int get_reg_value(target_addr_t addr, uint32_t* read_reg_value, uint32_t mask, uint32_t offset)
{
    int retval = ERROR_OK;
    uint32_t read_reg = 0;
    if (target_read_u32(g_hi3066h_info_target, addr, &read_reg) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }
    *read_reg_value = (read_reg >> offset) & mask;
    return retval;
}

int g_flg_info = 0;

static int hi3066h_info_probe(struct flash_bank *bank)
{
    if (g_flg_info != 0) {
        return ERROR_OK;
    }
    g_flg_info++;
    
    bank->base = g_info_base_adress;
    g_hi3066h_info_target = bank->target;
    int retval = ERROR_OK;
    hi3066h_efc_reg *p = EFC;

    if (set_reg_value(&(p->MAGIC_LOCK.reg), FLASH_KEY_REGISTER_UNLOCK_VALUE, ~(0xffffffff), 0) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (set_reg_value(&(p->CMD_CFG_COMMON.reg), 0, 0xFFFFFFFD, 1) != ERROR_OK) { // 0xffffffff represent MASK for 0:32
        return ERROR_TARGET_INVALID;
    }

    if (set_reg_value(&(p->MAGIC_LOCK.reg), FLASH_KEY_REGISTER_LOCK_VALUE, ~(0xffffffff), 0) != ERROR_OK) { // 0xffffffff represent MASK for 0:32
        return ERROR_TARGET_INVALID;
    }
    return ERROR_OK;
}

FLASH_BANK_COMMAND_HANDLER(hi3066h_info_flash_bank_command)
{
    uint32_t offset = 0;

    if (bank->sectors != NULL) {
        free(bank->sectors);
    }
    bank->write_start_alignment = 16; /* 16-byte alignment */
    bank->write_end_alignment = 16; /* 16-byte alignment */
    bank->size = hi3066h_INFO_SECTORS_SIZE * hi3066h_INFO_FLASH_PAGES;

    bank->num_sectors = hi3066h_INFO_FLASH_PAGES;
    bank->sectors = malloc(sizeof(struct flash_sector) * bank->num_sectors);
    if (bank->sectors == NULL) {
        LOG_ERROR("malloc fail");
        return ERROR_FAIL;
    }
    for (uint32_t i = 0; i < bank->num_sectors; i++) {
        bank->sectors[i].offset = offset;
        bank->sectors[i].size = hi3066h_INFO_SECTORS_SIZE;
        offset += hi3066h_INFO_SECTORS_SIZE;
        bank->sectors[i].is_erased = -1;
        bank->sectors[i].is_protected = -1;
    }
    for (uint32_t i = 0; i < bank->num_sectors; i++) {
        bank->sectors[i].is_protected = 0;
    }

    return ERROR_OK;
}

static int hi3066h_info_sector_erase(struct flash_bank *bank, uint32_t first, uint32_t last)
{
    last_info_flag = 0;
    // g_flg = 0;
    U32 SectorAddrValue = g_info_base_adress - ((1 << SECTOR_SIZE_SHIFT));
    hi3066h_efc_reg *p;
    U32 destAddr = 0;
    p = EFC;

    if (set_reg_value(&(p->MAGIC_LOCK.reg), FLASH_KEY_REGISTER_UNLOCK_VALUE, ~(0xfffffff), 0) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    for (int i = first; i <= last; i++) {
        uint32_t read_reg = 0;

        if (get_reg_value(&(p->EFLASH_CMD.reg), &read_reg, 0x1, 0) != ERROR_OK) {
            return ERROR_TARGET_INVALID;
        }

        if (read_reg) {
            return ERROR_TARGET_INVALID;
        }

        if (target_write_u32(g_hi3066h_info_target, &(p->EFLASH_ADDR.reg),
            ((FLASH_BASE_ADDR - FLASH_RAED_BASE_ADDR + bank->sectors[i].offset)/4 << 2))) {
                return ERROR_TARGET_INVALID;
        }
        if (target_read_u32(g_hi3066h_info_target, &(p->EFLASH_CMD.reg), &read_reg) != ERROR_OK) {
            return ERROR_TARGET_INVALID;
        }
        /* set EFLASH_CMD.BIT.cmd_code to FLASH_OPERATION_ERASE. */
        read_reg = ((FLASH_OPERATION_ERASE & 0x7) << 8) | (read_reg & 0xfffff8ff);
        /* set EFLASH_CMD.BIT.cmd_start to 0x1. */
        read_reg = 0x1 | (read_reg & 0xfffffffe);
        if (target_write_u32(g_hi3066h_info_target, &(p->EFLASH_CMD.reg), read_reg) != ERROR_OK) {
            return ERROR_TARGET_INVALID;
        }

        do {
            if (target_read_u32(g_hi3066h_info_target, &(p->EFLASH_CMD.reg), &read_reg) != ERROR_OK) {
                return ERROR_TARGET_INVALID;
            }
        } while((read_reg & 0x1));

        uint32_t read_reg_index = 0;
        if (get_reg_value(&(p->INT_RAW_STATUS.reg), &read_reg, 0x1, 16) != ERROR_OK) {
            return ERROR_TARGET_INVALID;
        }

        if (get_reg_value(&(p->INT_RAW_STATUS.reg), &read_reg_index, 0x1, 17) != ERROR_OK) {
            return ERROR_TARGET_INVALID;
        }

        if (read_reg || read_reg_index) {
            if (set_reg_value(&(p->INT_CLEAR.reg), 1, 0xFFFEFFFF, 16) != ERROR_OK) {
                return ERROR_TARGET_INVALID;
            }

            if (set_reg_value(&(p->INT_CLEAR.reg), 1, 0xFFFDFFFF, 17) != ERROR_OK) {
                return ERROR_TARGET_INVALID;
            }

            if (set_reg_value(&(p->MAGIC_LOCK.reg), FLASH_KEY_REGISTER_LOCK_VALUE, ~(0xffffffff), 0) != ERROR_OK) {
                return ERROR_TARGET_INVALID;
            }
            return ERROR_TARGET_INVALID;
        }
    }

    if (set_reg_value(&(p->CACHE_CTRL.reg), 1, 0xFFFFFEFF, 8) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (set_reg_value(&(p->CACHE_CTRL.reg), 1, 0xFFFFFDFF, 9) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (set_reg_value(&(p->MAGIC_LOCK.reg), FLASH_KEY_REGISTER_LOCK_VALUE, ~(0xffffffff), 0) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }
    LOG_INFO("3066H info erase suc");

    return ERROR_OK;
}

static U32 GetWriteAlignmentWords(U32 destAddr)
{
    U32 posInRow = 0;

    posInRow = destAddr % FLASH_ROW_WORD_SIZE;
    if (posInRow > 0) {
        return FLASH_ROW_WORD_SIZE - posInRow;
    }
    return 0;
}

static int hi3066h_flash_fast_write_words(uint8_t* srcAddr, U32 destAddr, U32 wordNum)
{
    hi3066h_efc_reg *p;
    uint32_t read_reg = 0;
    uint32_t read_reg_data = 0;
    uint32_t read_reg_index = 0;
    U32* data = NULL;
    data = (U32*)srcAddr;
    U32 i;
    U32 pgmSize;
    unsigned int defaultFillValue = 0xFFFFFFFF;
    unsigned int *defaultFillValuePtr = 0xFFFFFFFF;
    unsigned int defaultFillCount = 0;

    if (wordNum <= FLASH_MIN_PGM_WORDS_SIZE) {
        pgmSize = 0;
    } else if (((wordNum % FLASH_MIN_PGM_WORDS_SIZE) != 0 && (((wordNum + 1) % FLASH_ONE_WORD_BYTES_SIZE) == 0)) ||
               ((wordNum % FLASH_ONE_WORD_BYTES_SIZE) == 0)) {
        pgmSize = (wordNum % FLASH_MIN_PGM_WORDS_SIZE == 0) ? (wordNum / FLASH_ONE_WORD_BYTES_SIZE) :
                    (wordNum / FLASH_ONE_WORD_BYTES_SIZE + 1);
    } else if (((wordNum % FLASH_MIN_PGM_WORDS_SIZE) != 0 && (((wordNum + 1) % FLASH_ONE_WORD_BYTES_SIZE) != 0)) ||
               ((wordNum % FLASH_ONE_WORD_BYTES_SIZE) != 0)) {
        pgmSize = wordNum / FLASH_ONE_WORD_BYTES_SIZE;
    }
    p = EFC;
    if (get_reg_value(&(p->EFLASH_CMD.reg), &read_reg, 0x1, 0) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }
    if (read_reg) {
        return ERROR_TARGET_INVALID;
    }

    if (set_reg_value(&(p->EFLASH_ADDR.reg), destAddr, 0xFF000003, 2) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (set_reg_value(&(p->EFLASH_CMD.reg), pgmSize, 0xFC0FFFFF, 20) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (set_reg_value(&(p->EFLASH_CMD.reg), FLASH_OPERATION_PROGRAM, 0xFFFFF8FF, 8) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (get_reg_value(&(p->BUF_CLEAR.reg), &read_reg, 0xff, 8) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }
    if (read_reg != 0) {
        if (set_reg_value(&(p->BUF_CLEAR.reg), 1, 0xFFFFFFFE, 0) != ERROR_OK) {
            return ERROR_TARGET_INVALID;
        }
    }

    for (i = 0; i < wordNum; i++) {
        if ((data[i] == 0xffffffff) && !last_info_flag) {
            data[i] = 0x0;
        }
    }

    for (i = 0; i < wordNum; i += FLASH_MAX_PGM_WORD_SIZE) {
        if (target_write_memory(g_hi3066h_info_target, &(p->PGM_WDATA.reg), 4, FLASH_MAX_PGM_WORD_SIZE, data) != ERROR_OK) {
            return ERROR_TARGET_INVALID;
        }
    }

    if (wordNum % FLASH_MAX_PGM_WORD_SIZE != 0) {
        if (target_write_memory(g_hi3066h_info_target, &(p->PGM_WDATA.reg), 4, wordNum % FLASH_MAX_PGM_WORD_SIZE, data) != ERROR_OK) {
            return ERROR_TARGET_INVALID;
        }
    }

    defaultFillCount = (pgmSize == 0) ? (FLASH_MIN_PGM_WORDS_SIZE - wordNum) :
                       (pgmSize * FLASH_ONE_WORD_BYTES_SIZE - wordNum);

    for (i = 0; i < defaultFillCount; i++) {
        if (target_write_memory(g_hi3066h_info_target, &(p->PGM_WDATA.reg), 4, 1, defaultFillValuePtr) != ERROR_OK) {
            return ERROR_TARGET_INVALID;
        }
    }

    if (set_reg_value(&(p->EFLASH_CMD.reg), 1, 0xFFFFFFFE, 0) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (get_reg_value(&(p->EFLASH_CMD.reg), &read_reg, 0x1, 0) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }
    while (read_reg) {
        if (get_reg_value(&(p->EFLASH_CMD.reg), &read_reg, 0x1, 0) != ERROR_OK) {
            return ERROR_TARGET_INVALID;
        }
    }
    return 0;
}

static int FLASH_fast_WriteData(uint8_t *srcAddress, U32 *destAddress, U32 wordLen, uint32_t* buffer_offset)
{
    volatile unsigned int currentLen = wordLen;
    unsigned int wordNum;
    unsigned int writeWordSize = 0;
    U32 destAddr = *destAddress;
    int ret = 0;

    /* Calculate the number of words.
       If the number of words is less than one word, the value is calculated as one word. */
    wordNum = ((currentLen % FLASH_ONE_WORD_BYTES_SIZE) != 0) ? ((currentLen / FLASH_ONE_WORD_BYTES_SIZE) + 1)
                                                              : (currentLen / FLASH_ONE_WORD_BYTES_SIZE);
    /* write the data. */
    while (wordNum > 0) {
        if (wordNum <= FLASH_MIN_PGM_WORDS_SIZE) { /* The value of wordNum is less than or equal to 2. */
            ret = hi3066h_flash_fast_write_words(srcAddress, destAddr, wordNum);
            writeWordSize = wordNum;
            destAddr += FLASH_MIN_PGM_WORDS_SIZE;
        } else if (((wordNum % FLASH_MIN_PGM_WORDS_SIZE) != 0 && (((wordNum + 1) % FLASH_ONE_WORD_BYTES_SIZE) == 0)) ||
                   ((wordNum % FLASH_ONE_WORD_BYTES_SIZE) == 0)) {
            ret = hi3066h_flash_fast_write_words(srcAddress, destAddr, wordNum);
            writeWordSize = (wordNum % FLASH_MIN_PGM_WORDS_SIZE == 0) ? (wordNum / FLASH_ONE_WORD_BYTES_SIZE)
                                                                      : ((wordNum + 1) / FLASH_ONE_WORD_BYTES_SIZE);
            destAddr += (writeWordSize * FLASH_ONE_WORD_BYTES_SIZE);
            writeWordSize = wordNum;
        } else {
            writeWordSize = ((unsigned int)(wordNum / FLASH_ONE_WORD_BYTES_SIZE) * FLASH_ONE_WORD_BYTES_SIZE);
            ret = hi3066h_flash_fast_write_words(srcAddress, destAddr, wordNum);
            destAddr += writeWordSize;
        }
        srcAddress += (writeWordSize * FLASH_ONE_WORD_BYTES_SIZE);
        wordNum -= (wordNum <= writeWordSize) ? wordNum : writeWordSize;

        if (ret != 0) {
            return 1;
        }
    }
    *buffer_offset += (writeWordSize * FLASH_ONE_WORD_BYTES_SIZE);
    *destAddress = destAddr;
    return ret;
}

static int hi3066h_info_fast_flash_write(struct flash_bank *bank, const uint8_t *buffer, uint32_t offset, uint32_t count) {
    U32 destAddr = 0;
    U32 currentWords;
    U32 currentLen;
    U32 dataLeft;
    U32 dataLast;
    U32 i;
    int ret = ERROR_OK;
    unsigned int wordNum;
    unsigned int writeWordSize = 0;
    if (buffer == NULL) {
        return ERROR_TARGET_NOT_HALTED;
    }
    uint32_t buffer_offset = 0;
    hi3066h_efc_reg *p;
    p = EFC;
    U32 DestAddr = g_info_base_adress + offset;

    if (set_reg_value(&(p->CACHE_CTRL.reg), 1, 0xFFFFFEFF, 8) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (set_reg_value(&(p->CACHE_CTRL.reg), 1, 0xFFFFFDFF, 9) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (set_reg_value(&(p->BUF_CLEAR.reg), 1, 0xFFFFFFFE, 0) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (set_reg_value(&(p->MAGIC_LOCK.reg), FLASH_KEY_REGISTER_UNLOCK_VALUE, ~(0xffffffff), 0) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (DestAddr >= FLASH_BASE_ADDR) {
        destAddr =  DestAddr - FLASH_BASE_ADDR;
        destAddr = destAddr >> 2;
    }

    currentWords = GetWriteAlignmentWords(destAddr);
    if (count > (currentWords * FLASH_ONE_WORD_BYTES_SIZE) && currentWords > 0) {
        ret = FLASH_fast_WriteData(buffer + buffer_offset, &destAddr, (currentWords * FLASH_ONE_WORD_BYTES_SIZE), &buffer_offset);
        if (ret != 0) {
            return ERROR_TARGET_NOT_HALTED;
        }
        currentLen = count - currentWords * FLASH_ONE_WORD_BYTES_SIZE;
    } else {
        currentLen = count;
    }
    int flg = (currentLen % (FLASH_MAX_PGM_WORD_SIZE * FLASH_ONE_WORD_BYTES_SIZE));

    for (i = 0; i < currentLen / (FLASH_MAX_PGM_WORD_SIZE * FLASH_ONE_WORD_BYTES_SIZE); i++) {
        if (flg == 0 && (i == (currentLen / (FLASH_MAX_PGM_WORD_SIZE * FLASH_ONE_WORD_BYTES_SIZE)) - 1)) {
            last_info_flag++;
        }
        ret = FLASH_fast_WriteData(buffer + buffer_offset, &destAddr, (FLASH_MAX_PGM_WORD_SIZE * FLASH_ONE_WORD_BYTES_SIZE), &buffer_offset);
        if (ret != 0) {
            return ERROR_TARGET_NOT_HALTED;
        }
    }

    currentLen = (currentLen % (FLASH_MAX_PGM_WORD_SIZE * FLASH_ONE_WORD_BYTES_SIZE));
    /* Calculate the number of words. */
    ret = FLASH_fast_WriteData(buffer + buffer_offset, &destAddr, currentLen, &buffer_offset);
    if (ret != 0) {
        return 0;
    }

    if (set_reg_value(&(p->CACHE_CTRL.reg), 1, 0xFFFFFEFF, 8) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (set_reg_value(&(p->CACHE_CTRL.reg), 1, 0xFFFFFDFF, 9) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (set_reg_value(&(p->BUF_CLEAR.reg), 1, 0xFFFFFFFE, 0) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    uint32_t read_reg = 0;
    uint32_t read_reg_index = 0;
    if (get_reg_value(&(p->INT_RAW_STATUS.reg), &read_reg, 0x1, 16) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (get_reg_value(&(p->INT_RAW_STATUS.reg), &read_reg_index, 0x1, 17) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }
    if (read_reg || read_reg_index) {

        if (set_reg_value(&(p->INT_CLEAR.reg), 1, 0xFFFDFFFF, 17) != ERROR_OK) {
            return ERROR_TARGET_INVALID;
        }

        if (set_reg_value(&(p->INT_CLEAR.reg), 1, 0xFFFEFFFF, 16) != ERROR_OK) {
            return ERROR_TARGET_INVALID;
        }

        if (set_reg_value(&(p->MAGIC_LOCK.reg), FLASH_KEY_REGISTER_LOCK_VALUE, ~(0xffffffff), 0) != ERROR_OK) {
            return ERROR_TARGET_INVALID;
        }
        return ERROR_TARGET_INVALID;
    }

    if (set_reg_value(&(p->MAGIC_LOCK.reg), FLASH_KEY_REGISTER_LOCK_VALUE, ~(0xffffffff), 0) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }
	
    LOG_INFO("3066H info flash write suc");
    return 0;
}

struct flash_driver hi3066h_info_flash = {
    .name = "hi3066h_info",
    .flash_bank_command = hi3066h_info_flash_bank_command,
    .erase = hi3066h_info_sector_erase,
    .write = hi3066h_info_fast_flash_write,
    .read = default_flash_read,
    .probe = hi3066h_info_probe,
    .auto_probe = hi3066h_info_probe,
    .erase_check = default_flash_blank_check,
    .free_driver_priv = default_flash_free_driver_priv,
};