/*
* Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2023-2023. All rights reserved.
* Description: hi3071 flash drive of OpenOCD
* Create: 2023-5-26
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <target/image.h>
#include "imp.h"
#include "chiptype.h"
#include "hi3071_main.h"

#define hi3071_flash_PAGE_SIZE      0x1000 /* Size of a page,unit: bytes. 1K. */
#define hi3071_flash_PAGES          0x20
#define FLASH_OFFSET_BASE_ADDR      0x00100000
#define U32                         unsigned long
#define I32                         signed long
#define SECTOR_SIZE_SHIFT           (12)
#define CPU_BUS_WIDTH 4
#define PAGE_SIZE_SHIFT              (3)
#define FOUR_BIT                     (4)
#define EIGHT_BIT                    (8)
#define U8  unsigned char
#define TOKEN_COUNT 4
#define FLASH_MAX_LEN (1024 * 1024)
#define BIN_OFFSET 512
#define BIN_HEAD_SIZE 16  // 头部占16字节
#define CHECK_SUM_LEN 16  // checksum需要有16byte的长度
#define PAD_VALUE 0XFF
#define START_OFFSET 4
#define SIX_OFFSET 8
#define END_OFFSET 12
#define LITTLE_ENDIAN_OFFESET 8
#define DEC_VALUE 10
#define DEFAULT_VALUE 255
#define MAX_LEN_VALUE 256
#define BIT_8  8
#define BIT_16 16
#define BIT_24 24
#define WRITE_TRY_CUNT 0x1000
#define READ_FAIL 0
#define READ_SUC 1
#define ERASE_TIME_CNT 0x10000000
#define FLASH_STATUS_MASK 0x11111111
#define REG_MASK_DEFAULT 0xFFFFFFFF
#define WATCH_DOG_REG_MASK 0xFFFFFFFE
#define SYS_CLOCK_REG_VALUE 0x05FAAF50
#define APP_START_OFFSET 0x10

struct target *g_hi3017_target = NULL;

// 将十六进制字符转换为4位数值，无效时返回255
static uint8_t hex_ascii_to_uint(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'A' && c <= 'F') {
        return DEC_VALUE + (c - 'A');
    } else if (c >= 'a' && c <= 'f') {
        return DEC_VALUE + (c - 'a');
    } else {
        return DEFAULT_VALUE; // 无效字符
    }
}

// 从十六进制字符串读取数值
static uint32_t read_hex(const char *arg)
{
    uint32_t ret = 0;
    size_t i = 0;
    size_t len = strlen(arg);
    // 检查字符串长度是否至少为4（如"0x00"）
    if (len < START_OFFSET) {
        return ret;
    }

    // 检查前缀是否为"0x"
    if (arg[0] != '0' || arg[1] != 'x') {
        return ret;
    }

    // 处理每个十六进制字符
    for (i = 2; i < len; ++i) { // 字符中包含0x, 从第2位开始读取值
        uint8_t temp = hex_ascii_to_uint(arg[i]);
        if (temp == DEFAULT_VALUE) {
            return READ_FAIL; // 遇到无效字符，返回0
        }
        ret = (ret << START_OFFSET) + temp; // 左移4位并累加当前值
    }

    return ret;
}

// 自动获取bin文件大小
uint32_t get_file_size(const char *filename) {
    FILE *fp = fopen(filename, "rb");  // 以二进制模式打开文件
    if (fp == NULL) {
        LOG_ERROR("bin file open fail");
        return -1;
    }

    // 将文件指针移动到文件末尾
    if (fseek(fp, 0, SEEK_END) != 0) {
        LOG_ERROR("fseek fail");
        fclose(fp);
        return -1;
    }

    // 获取文件大小
    uint32_t size = ftell(fp);
    if (size == -1L) {
        LOG_ERROR("ftell fail");
        fclose(fp);
        return -1;
    }
    
    fclose(fp);
    return size;
}

static int read_bin_file(uint8_t bin_file[], uint32_t len, const char *bin_file_name, uint8_t pad_byte)
{
    FILE *bin = fopen(bin_file_name, "rb");
    if (!bin) {
        LOG_ERROR("read %s failed! \n", bin_file_name);
        return READ_FAIL;
    }

    // 获取文件长度
    if (fseek(bin, 0, SEEK_END) == -1) {
        fclose(bin);
        return READ_FAIL;
    }
    // 获取文件指针bin的当前位置
    long end = ftell(bin);
    if (end == -1) {
        fclose(bin);
        return READ_FAIL;
    }
    // 尝试将文件指针bin移动到文件的开始位置
    if (fseek(bin, 0, SEEK_SET) != 0) {
        // 如果移动失败，返回读取失败的错误码
        fclose(bin);
        return READ_FAIL;
    }
    uint32_t bin_len = (uint32_t)end;

    // 读取文件内容到binFile的BIN_OFFSET之后的位置
    if (fread(bin_file, 1, bin_len, bin) != bin_len) {
        LOG_ERROR("Failed to read %s completely.\n", bin_file_name);
        fclose(bin);
        return READ_FAIL;
    }

    if (fclose(bin) != 0) {
        return READ_FAIL;
    }

    // 填充剩余部分到len
    for (uint32_t i = bin_len; i < len; ++i) {
        bin_file[i] = pad_byte;
    }

    return READ_SUC;
}

// 将数字 0~15 转换为对应的十六进制字符
char to_hex_char(unsigned int value) {
    if (value < 10) {
        return '0' + value;
    }
    return 'a' + (value - 10);
}

// 将数组拼接为十六进制字符串，并转换为十进制整数
unsigned int hex_array_to_decimal(const unsigned char *arr, int length) {
    // 最大可能长度：每个元素最多两位 + 结束符 '\0'
    char hex_str[MAX_LEN_VALUE] = {0};
    int idx = 0;

    // 从右向左遍历数组，拼接成十六进制字符串（高位在前）
    for (int i = length - 1; i >= 0; i--) {
        unsigned int val = arr[i];

        // 分解为高4位和低4位（即两个十六进制位）
        char high = val >> FOUR_BIT;
        char low = val & 0x0F;

        // 如果高位不为0或当前不是第一个字节，才添加高位
        if (high > 0 || idx > 0) {
            hex_str[idx++] = to_hex_char(high);
        }
        hex_str[idx++] = to_hex_char(low);
    }

    hex_str[idx] = '\0'; // 字符串结束

    // 手动将十六进制字符串转换为十进制整数
    unsigned int result = 0;
    for (int i = 0; hex_str[i] != '\0'; i++) {
        char c = hex_str[i];
        unsigned int digit;

        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            digit = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            digit = c - 'A' + 10;
        } else {
            // 非法字符跳过
            continue;
        }

        result = result * BIT_16 + digit;
    }

    return result;
}

static uint32_t get_checksum_value(uint8_t bin_file[], int offset)
{
    uint32_t value = 0;
    value = hex_array_to_decimal(bin_file + offset, 4);
    return value;
}

static int add_bin_head(uint8_t bin_file[], uint32_t Len)
{
    uint64_t app_len = get_checksum_value(bin_file, 0);
    uint64_t check_sum = get_checksum_value(bin_file, 4);
    uint32_t head_checksum = 0;
    uint32_t boot_check_sum = 0;

    for (uint32_t i = 0; i < app_len; ++i) {
        boot_check_sum += bin_file[i + APP_START_OFFSET];
    }

    if ((app_len + APP_START_OFFSET > Len) && (boot_check_sum != check_sum)) {
        LOG_ERROR("checksum verify fail");
        return ERROR_TARGET_FAILURE;
    }

    // 写入AppLen（小端序）
    bin_file[0] = (uint8_t)(app_len);
    bin_file[1] = (uint8_t)(app_len >> BIT_8);
    bin_file[2] = (uint8_t)(app_len >> BIT_16);
    bin_file[3] = (uint8_t)(app_len >> BIT_24);

    // 写入Checksum（小端序）
    bin_file[4] = (uint8_t)boot_check_sum;
    bin_file[5] = (uint8_t)(boot_check_sum >> BIT_8);
    bin_file[6] = (uint8_t)(boot_check_sum >> BIT_16);
    bin_file[7] = (uint8_t)(boot_check_sum >> BIT_24);

    // 设置resv字段为0xFF（注释要求）
    bin_file[8] = 0xFF;
    bin_file[9] = 0xFF;
    bin_file[10] = 0xFF;
    bin_file[11] = 0xFF;

    // 计算头部前8字节的校验和（AppLen和Checksum）
    for (uint32_t i = 0; i < BIT_8; ++i) {
        head_checksum += bin_file[i];
    }

    // 写入HeadCheck（小端序）
    bin_file[12] = (uint8_t)head_checksum;
    bin_file[13] = (uint8_t)(head_checksum >> BIT_8);
    bin_file[14] = (uint8_t)(head_checksum >> BIT_16);
    bin_file[15] = (uint8_t)(head_checksum >> BIT_24);

    return ERROR_OK;
}

static int check_sum_opt(void)
{
    uint8_t g_bin_file[FLASH_MAX_LEN] = {0};
    char str_pad[] = "0xff"; // 填充值取0xff
    uint32_t size = get_file_size(bin_name);
    U32 write_status = 1;
    uint8_t pad = read_hex(&str_pad);
    if (!read_bin_file(g_bin_file, size, bin_name, pad)) {
        return ERROR_TARGET_INVALID;
    }

    if (add_bin_head(g_bin_file, size) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    for (int i = 0; i < CPU_BUS_WIDTH; i++) {
        if (unloack_pflash_write(PFLASH_MEM_CTRL_ADDR) != ERROR_OK) {
            return ERROR_TARGET_INVALID;
        }
        if (target_write_buffer(g_hi3017_target, FLASH_OFFSET_BASE_ADDR + i * CPU_BUS_WIDTH,
            sizeof(U8) * CPU_BUS_WIDTH, g_bin_file + i * CPU_BUS_WIDTH) != ERROR_OK) {
            LOG_ERROR("hi3071 write checksum f");
            return ERROR_TARGET_INVALID;
        }
        if (get_write_status(&write_status) != ERROR_OK) {
            return ERROR_TARGET_INVALID;
        }
        U32 time_cnt = 0;
        while (write_status != IDLE) {
            if (get_write_status(&write_status) != ERROR_OK) {
                return ERROR_TARGET_INVALID;
            }
            time_cnt++;
            if (time_cnt >= WRITE_TRY_CUNT) {
                return ERROR_TARGET_INVALID;
            }
        }
    }

    return ERROR_OK;
}

static int set_mcu_reg_value(target_addr_t addr, uint32_t value, uint32_t mask, uint32_t offset)
{
    int retval = ERROR_OK;
    uint32_t read_reg = 0;
    if (mask != 0x0) {
        if (target_read_u32(g_hi3017_target, addr, &read_reg) != ERROR_OK) {
            return ERROR_TARGET_INVALID;
        }
    }

    read_reg = (read_reg & (mask)) | (value << offset);
    retval = target_write_u32(g_hi3017_target, addr, read_reg);
    if (retval != ERROR_OK) {
        return retval;
    }
    return retval;
}

static int get_mcu_reg_value(target_addr_t addr, uint32_t* read_reg_value, uint32_t mask, uint32_t offset)
{
    int retval = ERROR_OK;
    uint32_t read_reg = 0;
    if (target_read_u32(g_hi3017_target, addr, &read_reg) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }
    *read_reg_value = (read_reg >> offset) & mask;
    return retval;
}

static int watch_dog_reg_init(void)
{
    if (set_mcu_reg_value(&(reg_watchDog.WDT_MOD.bits), 1, WATCH_DOG_REG_MASK, 0) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (set_mcu_reg_value(&(reg_watchDog.WDT_EN), 0, ~(REG_MASK_DEFAULT), 0) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (set_mcu_reg_value(&(reg_watchDog.WDT_MOD.bits), 0, WATCH_DOG_REG_MASK, 0) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (target_write_u32(g_hi3017_target, &(reg_sysCtrlLock.SYS_LOCK), SYS_CLOCK_REG_VALUE) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }
    return ERROR_OK;
}

static int sys_ctrl_nor_reg_init(void)
{
    if (set_mcu_reg_value(&(reg_sysCtrlNor.SYSCTRL_P_DFLASH_TIMING.u32),
        0x44, ~(REG_MASK_DEFAULT), 0) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (set_mcu_reg_value(&(reg_sysCtrlNor.SYSCTRL_DFLASH_TIMING0.u32),
        FLASH_TIMERG0_TNVS | FLASH_TIMERG0_TPROG, ~(REG_MASK_DEFAULT), 0) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (set_mcu_reg_value(&(reg_sysCtrlNor.SYSCTRL_DFLASH_TIMING1.u32),
        FLASH_TIMERG1_TPGS | FLASH_TIMERG1_TRCV, ~(REG_MASK_DEFAULT), 0) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (set_mcu_reg_value(&(reg_sysCtrlNor.SYSCTRL_DFLASH_TIMING2.u32),
        FLASH_TIMERG2_TRCV | FLASH_TIMERG2_RCV, ~(REG_MASK_DEFAULT), 0) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (set_mcu_reg_value(&(reg_sysCtrlNor.SYSCTRL_DFLASH_TIMING3.u32),
        FLASH_TIMERG3_TERASE, ~(REG_MASK_DEFAULT), 0) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (set_mcu_reg_value(&(reg_sysCtrlNor.SYSCTRL_DFLASH_TIMING4.u32),
        FLASH_TIMERG4_TERASE, ~(REG_MASK_DEFAULT), 0) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (set_mcu_reg_value(&(reg_sysCtrlNor.SYSCTRL_PFLASH_TIMING0.u32),
        FLASH_TIMERG0_TNVS | FLASH_TIMERG0_TPROG, ~(REG_MASK_DEFAULT), 0) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (set_mcu_reg_value(&(reg_sysCtrlNor.SYSCTRL_PFLASH_TIMING1.u32),
        FLASH_TIMERG1_TPGS | FLASH_TIMERG1_TRCV, ~(REG_MASK_DEFAULT), 0) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (set_mcu_reg_value(&(reg_sysCtrlNor.SYSCTRL_PFLASH_TIMING2.u32),
        FLASH_TIMERG2_TRCV | FLASH_TIMERG2_RCV, ~(REG_MASK_DEFAULT), 0) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (set_mcu_reg_value(&(reg_sysCtrlNor.SYSCTRL_PFLASH_TIMING3.u32),
        FLASH_TIMERG3_TERASE, ~(REG_MASK_DEFAULT), 0) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (set_mcu_reg_value(&(reg_sysCtrlNor.SYSCTRL_PFLASH_TIMING4.u32),
        FLASH_TIMERG4_TERASE, ~(REG_MASK_DEFAULT), 0) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (set_mcu_reg_value(&(reg_sysCtrlNor.SYSCTRL_TIMING_LOAD.u32),
        0x0, ~(REG_MASK_DEFAULT), 0) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (set_mcu_reg_value(&(reg_sysCtrlNor.SYSCTRL_TIMING_LOAD.u32),
        0x3, ~(REG_MASK_DEFAULT), 0) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    return ERROR_OK;
}

static int set_bit(target_addr_t addr, uint32_t value)
{
    uint32_t read_reg = 0;
    if (target_read_u32(g_hi3017_target, addr, &read_reg) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }
    read_reg |= value;
    if (target_write_u32(g_hi3017_target, addr, read_reg) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }
    return ERROR_OK;
}

static int hi3071_info_probe(struct flash_bank *bank)
{
    bank->base = FLASH_OFFSET_BASE_ADDR;
    g_hi3017_target = bank->target;
    int retval = ERROR_OK;

    if (watch_dog_reg_init() != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (sys_ctrl_nor_reg_init() != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    uint32_t read_reg = 0;
    uint32_t read_reg_index = 0;
    if (get_mcu_reg_value(&(reg_sysCtrlNor.SYSCTRL_TIMING_LOAD_OVER.bits), &read_reg, 0x1, 0) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (get_mcu_reg_value(&(reg_sysCtrlNor.SYSCTRL_TIMING_LOAD_OVER.bits), &read_reg_index, 0x1, 1) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    while (read_reg == 0 || read_reg_index == 0) {
        if (get_mcu_reg_value(&(reg_sysCtrlNor.SYSCTRL_TIMING_LOAD_OVER.bits), &read_reg, 0x1, 0) != ERROR_OK) {
            return ERROR_TARGET_INVALID;
        }

        if (get_mcu_reg_value(&(reg_sysCtrlNor.SYSCTRL_TIMING_LOAD_OVER.bits), &read_reg_index, 0x1, 1) != ERROR_OK) {
            return ERROR_TARGET_INVALID;
        }
    }
    if (target_write_u32(g_hi3017_target, &(reg_sysCtrlNor.SYSCTRL_TIMING_LOAD_OVER.u32), 0x3) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (set_bit(((U32 *)(PFLASH_ENABLE)), BIT(0x3)) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (set_bit(((U32 *)(PFLASH_ENABLE)), BIT(0x2)) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    return ERROR_OK;
}

FLASH_BANK_COMMAND_HANDLER(hi3071_flash_bank_command)
{
    uint32_t offset = 0;

    if (bank->sectors != NULL) {
        free(bank->sectors);
    }
    bank->write_start_alignment = 4; /* 4-byte alignment */
    bank->write_end_alignment = 4; /* 4-byte alignment */
    bank->size = hi3071_flash_PAGE_SIZE * hi3071_flash_PAGES;

    bank->num_sectors = hi3071_flash_PAGES;
    bank->sectors = (struct flash_sector *)malloc(sizeof(struct flash_sector) * bank->num_sectors);
    if (bank->sectors == NULL) {
        LOG_ERROR("malloc fail");
        return ERROR_FAIL;
    }
    for (uint32_t i = 0; i < bank->num_sectors; i++) {
        bank->sectors[i].offset = offset;
        bank->sectors[i].size = hi3071_flash_PAGE_SIZE;
        offset += hi3071_flash_PAGE_SIZE;
        bank->sectors[i].is_erased = -1;
        bank->sectors[i].is_protected = -1;
    }
    for (uint32_t i = 0; i < bank->num_sectors; i++) {
        bank->sectors[i].is_protected = 0;
    }

    return ERROR_OK;
}

static int unlock_pflash_erase_all(target_addr_t addr)
{
    if (target_write_u32(g_hi3017_target, addr + 0xaa8, 0xAA) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (target_write_u32(g_hi3017_target, addr + 0xaa8, 0x80) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (target_write_u32(g_hi3017_target, addr + 0x554, 0x55) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (target_write_u32(g_hi3017_target, addr + 0xaa8, 0x10) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    return ERROR_OK;
}

static int get_erase_status(uint32_t* erase_status)
{
    uint32_t read_reg = 0;
    if (target_read_u32(g_hi3017_target, (volatile U32 *)(PFLASH_STATE), &read_reg) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }
    *erase_status = (read_reg & FLASH_STATUS_MASK);
    return ERROR_OK;
}

int get_write_status(uint32_t* write_status)
{
    uint32_t read_reg = 0;
    if (target_read_u32(g_hi3017_target, (volatile U32 *)(PFLASH_STATE), &read_reg) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }
    *write_status = (read_reg & FLASH_STATUS_MASK);
    return ERROR_OK;
}

static int erase_sector(U32 addr)
{
    if (target_write_u32(g_hi3017_target, FLASH_OFFSET_BASE_ADDR + 0xaa8, 0xAA) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (target_write_u32(g_hi3017_target, FLASH_OFFSET_BASE_ADDR + 0xaa8, 0x80) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (target_write_u32(g_hi3017_target, FLASH_OFFSET_BASE_ADDR + 0x554, 0x55) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (target_write_u32(g_hi3017_target, FLASH_OFFSET_BASE_ADDR, 0x50) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    uint32_t erase_status;
    U32 erase_time_cnt_nvr = 0;

    if (get_erase_status(&erase_status) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }
    while (erase_status != IDLE) {
        if (get_erase_status(&erase_status) != ERROR_OK) {
            return ERROR_TARGET_INVALID;
        }
        erase_time_cnt_nvr++;
        if (erase_time_cnt_nvr >= ERASE_TIME_CNT) {
            return ERROR_TARGET_INVALID;
        }
    }

    if (unlock_pflash_erase_all(PFLASH_MEM_CTRL_ADDR) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (get_erase_status(&erase_status) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }
    while (erase_status != IDLE) {
        if (get_erase_status(&erase_status) != ERROR_OK) {
            return ERROR_TARGET_INVALID;
        }
        erase_time_cnt_nvr++;
        if (erase_time_cnt_nvr >= ERASE_TIME_CNT) {
            return ERROR_TARGET_INVALID;
        }
    }
    return ERROR_OK;
}

U32 SectorAddr = FLASH_OFFSET_BASE_ADDR - ((1 << SECTOR_SIZE_SHIFT));

static int hi3071_info_erase(struct flash_bank *bank, uint32_t first, uint32_t last)
{
    g_hi3017_target = bank->target;
    if (erase_sector(SectorAddr) != ERROR_OK) {
        LOG_ERROR("hi3071 erase fail");
        return ERROR_TARGET_INVALID;
    }
    LOG_INFO("hi3071 erase suc");
    return ERROR_OK;
}

int unloack_pflash_write(U32 addr)
{
    if (target_write_u32(g_hi3017_target, addr + 0xaa8, 0xAA) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    if (target_write_u32(g_hi3017_target, addr + 0xaa8, 0xA0) != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }

    return ERROR_OK;
}

static int flash_word_write(U32 dest_addr, U32 num_bytes, U8 *p_src_buff)
{
    U32 write_status = 1;
    I32 write_size = num_bytes;
    volatile U8 *fromaddr, *toaddr;
    // uword writedata;
    toaddr = dest_addr;
    fromaddr = (volatile U8 *)p_src_buff;
    while (write_size > 0) {
        I32 write_size_buf = write_size;
        I32 i = 0;
        while (i < write_size_buf) {
            if (unloack_pflash_write(PFLASH_MEM_CTRL_ADDR) != ERROR_OK) {
                return ERROR_TARGET_INVALID;
            }
            
            if (target_write_buffer(g_hi3017_target, toaddr, sizeof(U8) * CPU_BUS_WIDTH, fromaddr) != ERROR_OK) {
                LOG_ERROR("hi3071 info write ff");
                return ERROR_TARGET_INVALID;
            }
            fromaddr += CPU_BUS_WIDTH;
            toaddr += CPU_BUS_WIDTH;
            i += CPU_BUS_WIDTH;
            if (get_write_status(&write_status) != ERROR_OK) {
                return ERROR_TARGET_INVALID;
            }
            U32 time_cnt = 0;
            while (write_status != IDLE) {
                if (get_write_status(&write_status) != ERROR_OK) {
                    return ERROR_TARGET_INVALID;
                }
                time_cnt++;
                if (time_cnt >= WRITE_TRY_CUNT) {
                    return ERROR_TARGET_INVALID;
                }
            }
        }
        write_size -= write_size_buf;
    }
    return ERROR_OK;
}

static int hi3071_info_write(struct flash_bank *bank, const uint8_t *buffer, uint32_t offset, uint32_t count)
{
    g_hi3017_target = bank->target;
    int retval = ERROR_OK;
    U32 num_pages;
    U32 dest_addr = FLASH_OFFSET_BASE_ADDR + offset;
    int more_write = 0;
    num_pages = (count >> PAGE_SIZE_SHIFT);
    if (count % EIGHT_BIT != 0 && count % FOUR_BIT == 0) {
        more_write = 1; // 需要将多余的数据写入
    }

    if (check_sum_opt() != ERROR_OK) {
        return ERROR_TARGET_INVALID;
    }
    do {
        retval = flash_word_write(dest_addr, (1uL << PAGE_SIZE_SHIFT), buffer);
        if (retval < 0) {
            return ERROR_TARGET_INVALID;
        }
        dest_addr += (1uL << PAGE_SIZE_SHIFT);
        buffer += (1uL << PAGE_SIZE_SHIFT);
    } while (--num_pages);

    if (more_write) {
        flash_word_write(dest_addr, CPU_BUS_WIDTH, buffer); // 需要将多余的数据写入
    }
    LOG_INFO("hi3071 info write suc");
    return retval;
}

struct flash_driver hi3071_flash = {
    .name = "hi3071",
    .flash_bank_command = hi3071_flash_bank_command,
    .erase = hi3071_info_erase,
    .write = hi3071_info_write,
    .read = default_flash_read,
    .probe = hi3071_info_probe,
    .auto_probe = hi3071_info_probe,
    .erase_check = default_flash_blank_check,
    .free_driver_priv = default_flash_free_driver_priv,
};