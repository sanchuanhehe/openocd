/*
* Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2023-2023. All rights reserved.
* Description: nb flash drive of OpenOCD
* Create: 2023-5-26
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "imp.h"
#include <target/image.h>

#define NB_INFO_FLASH_PAGE_SIZE    0x100 /* Size of a page,unit: bytes. 1K. */
#define NB_INFO_FLASH_PAGES        0x4000
#define FLASH_OFFSET_BASE_ADDR        0x1000000
#define SFC_PRINT_BUFF_LEN              1024 * 10

struct target *g_nb_target = NULL;

static int nb_info_probe(struct flash_bank *bank)
{
    bank->base = FLASH_OFFSET_BASE_ADDR;
    g_nb_target = bank->target;
    return ERROR_OK;
}

FLASH_BANK_COMMAND_HANDLER(nb_info_flash_bank_command)
{
    uint32_t offset = 0;

    if (bank->sectors != NULL) {
        free(bank->sectors);
    }
    bank->write_start_alignment = 4; /* 4-byte alignment */
    bank->write_end_alignment = 4; /* 4-byte alignment */
    bank->size = NB_INFO_FLASH_PAGE_SIZE * NB_INFO_FLASH_PAGES;

    bank->num_sectors = NB_INFO_FLASH_PAGES;
    bank->sectors = malloc(sizeof(struct flash_sector) * bank->num_sectors);
    if (bank->sectors == NULL) {
        LOG_ERROR("malloc fail");
        return ERROR_FAIL;
    }
    for (uint32_t i = 0; i < bank->num_sectors; i++) {
        bank->sectors[i].offset = offset;
        bank->sectors[i].size = NB_INFO_FLASH_PAGE_SIZE;
        offset += NB_INFO_FLASH_PAGE_SIZE;
        bank->sectors[i].is_erased = -1;
        bank->sectors[i].is_protected = -1;
    }
    for (uint32_t i = 0; i < bank->num_sectors; i++) {
        bank->sectors[i].is_protected = 0;
    }

    return ERROR_OK;
}

static int nb_info_erase(struct flash_bank *bank, uint32_t first, uint32_t last)
{
    g_nb_target = bank->target;
    int retval = ERROR_OK;
    uint32_t read_reg;

    if (g_nb_target->state != TARGET_HALTED) {
        LOG_ERROR("Target not halted");
        return ERROR_TARGET_NOT_HALTED;
    }
    return retval;
}

static int nb_info_write(struct flash_bank *bank, const uint8_t *buffer, uint32_t offset, uint32_t count) {
    g_nb_target = bank->target;
    int retval = ERROR_OK;
    if (g_nb_target->state != TARGET_HALTED) {
        LOG_ERROR("Target not halted");
        retval = ERROR_TARGET_NOT_HALTED;
        return retval;
    }
    uint32_t remained_len = count;
    uint32_t start_addr = offset + FLASH_OFFSET_BASE_ADDR;
    LOG_ERROR("nb start addr is %x\n", start_addr);
    while (remained_len > 0) {
        uint32_t cur_len = remained_len > SFC_PRINT_BUFF_LEN ? SFC_PRINT_BUFF_LEN : remained_len;
        remained_len -= cur_len;
        start_addr += cur_len;
        buffer += cur_len;
        LOG_ERROR("nb write remained len is %d\n", remained_len);
    }
    return retval;
}

struct flash_driver nb_info_flash = {
    .name = "nb",
    .flash_bank_command = nb_info_flash_bank_command,
    .erase = nb_info_erase,
    .write = nb_info_write,
    .read = default_flash_read,
    .probe = nb_info_probe,
    .auto_probe = nb_info_probe,
    .erase_check = default_flash_blank_check,
    .free_driver_priv = default_flash_free_driver_priv,
};