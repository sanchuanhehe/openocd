// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HISPARK_TRACE_MODIFIES
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "imp.h"
#include <helper/time_support.h>
#include <target/image.h>
#include "jtag/drivers/cmsis_dap.h"
#include "flash_offline_load.h"
#include <target/register.h>
#include <time.h>
#include "src/target/arm_adi_v5.h"
#include "chiptype.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
// 需要做成传参start
#define BURN_PROGRAM_ADDR 0x2000004
#define START_FUNC_ADDR 0x2000028
#define START_TIME_OUT 500
#define SRAM_ADDR 0x2001800
#define FLASH_BASE_ADDR 0x3000000
#define PC_ADDR "0x03000004"
#define FLASH_WRITE_SIZE (512 * 20)
#define EBRAEK_ADDR_STR "0x2000000"
// 需要做成传参end

#define EBRAEK_ADDR 0x2000000
#define TIME_OUT_VALUE 5000

#define READ_FAIL 0
#define READ_SUC 1
#define OFFLINE_FLASH_MAX_LEN (1024 * 1024)
#define HEX_STRING_LEN 20

char* init_pc_func_addr = "";
char* erase_pc_func_addr = "";
char* g_write_pc_func_addr = "";
char* start_func_addr = "";
char* ebreak_addr_str = "";
long long g_start_timeout = 0;
long long erase_size = 0;
long long flash_base_addr = 0;
char *hex_to_string(long value) {
    char *hex_string = (char *)malloc(sizeof(char) * HEX_STRING_LEN);
    if (hex_string == NULL) {
        return NULL;  // 分配内存失败
    }
    sprintf(hex_string, "0x%llx", value);
    return hex_string;
}

void free_hex_string(char *hex_string) {
    if (hex_string != NULL) {
        free(hex_string);
    }
}

COMMAND_HANDLER(handle_flash_offline_load_image)
{
    uint8_t *buffer;
    size_t length;
    int retval;
    struct fileio *fileio;
    size_t write_cnt = 0;
    size_t buf_cnt = 0;

    if (CMD_ARGC != 1) { /* Check whether there is only one parameter. */
        return ERROR_COMMAND_SYNTAX_ERROR;
    }

    struct duration bench;
    duration_start(&bench);

    /* Open Binary File */
    if (fileio_open(&fileio, CMD_ARGV[0], FILEIO_READ, FILEIO_BINARY) != ERROR_OK) {
        return ERROR_FAIL;
    }

    /* Obtains the file length. */
    size_t filesize;
    retval = fileio_size(fileio, &filesize);
    if (retval != ERROR_OK) {
        fileio_close(fileio);
        return retval;
    }

    if (filesize < FLASH_OFFLINE_LOAD_HEAD_LEN) {
        LOG_ERROR("The file size is less than the file header length.");
        return ERROR_FAIL;
    }

    buffer = malloc(FLASH_OFFLINE_LOAD_ONCE_LEN);
    if (!buffer) {
        fileio_close(fileio);
        LOG_ERROR("Out of memory");
        return ERROR_FAIL;
    }

    /* Processing file headers */
    length = FLASH_OFFLINE_LOAD_HEAD_LEN;
    memset(buffer, 0, length);
    if (fileio_read(fileio, length, buffer, &buf_cnt) != ERROR_OK) {
        LOG_ERROR("Read head data from file fail !");
        free(buffer);
        fileio_close(fileio);
        return ERROR_FAIL;
    }

    if (buf_cnt != length) {
        LOG_ERROR("Short read");
        free(buffer);
        fileio_close(fileio);
        return ERROR_FAIL;
    }

    if (cmsis_dap_cmd_write_offline_head(buffer, length) != ERROR_OK) {
        LOG_ERROR("Write head data fail !");
        free(buffer);
        fileio_close(fileio);
        return ERROR_FAIL;
    }
    write_cnt += length;
    filesize -= length;

    /* Write data */
    while (filesize > 0) {
        length = MIN(filesize, FLASH_OFFLINE_LOAD_ONCE_LEN);
        memset(buffer, 0, length);

        if (fileio_read(fileio, length, buffer, &buf_cnt) != ERROR_OK) {
            LOG_ERROR("Read data from file fail !");
            free(buffer);
            fileio_close(fileio);
            if (cmsis_dap_cmd_write_offline_stop(OFFLINE_STOP_STATUS_FAIL) != ERROR_OK) {
                LOG_ERROR("stop fail error");
            }
            return ERROR_FAIL;
        }

        if (buf_cnt != length) {
            LOG_ERROR("Short read");
            free(buffer);
            fileio_close(fileio);
            if (cmsis_dap_cmd_write_offline_stop(OFFLINE_STOP_STATUS_FAIL) != ERROR_OK) {
                LOG_ERROR("stop fail error");
            }
            return ERROR_FAIL;
        }
        /* Data is transmitted through the DAP protocol. */
        if (cmsis_dap_cmd_write_offline_data(buffer, length) != ERROR_OK) {
            LOG_ERROR("Write data fail\r\n Completed write_cnt:%d bytes, %d bytes not completed", write_cnt, filesize);
            free(buffer);
            fileio_close(fileio);
            if (cmsis_dap_cmd_write_offline_stop(OFFLINE_STOP_STATUS_FAIL) != ERROR_OK) {
                LOG_ERROR("stop fail error");
            }
            return ERROR_FAIL;
        }
        write_cnt += length;
        filesize -= length;
    }
    /* Release resources. */
    free(buffer);
    fileio_close(fileio);

    if (cmsis_dap_cmd_write_offline_stop(OFFLINE_STOP_STATUS_DONE) != ERROR_OK) {
        LOG_ERROR("stop done fail");
        return ERROR_FAIL;
    }
    duration_measure(&bench);
    command_print(CMD, "write %zu bytes from file %s" " in %fs (%0.3f KiB/s)",
        write_cnt, CMD_ARGV[0],duration_elapsed(&bench), duration_kbps(&bench, write_cnt));
    return ERROR_OK;
}

// 自动获取bin文件大小
uint32_t get_offline_file_size(const char *filename) {
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

static int read_offline_bin_file(uint8_t bin_file[], uint32_t len, const char *bin_file_name)
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

    return READ_SUC;
}

void set_riscv_register_value(struct target *target, char* cmd_argv, char* addr_value)
{
	struct reg *reg = NULL;
    reg = register_get_by_name(target->reg_cache, cmd_argv, true);
    assert(reg); /* give clang a hint that we *know* reg is != NULL here */

	if (!reg->exist) {
        return;
    }
    char *value = buf_to_hex_str(reg->value, reg->size);
    uint8_t *buf = malloc(DIV_ROUND_UP(reg->size, 8));
    if (!buf) {
        return;
    }
    str_to_buf(addr_value, strlen(addr_value), buf, reg->size, 0);
    int retval = reg->type->set(reg, buf);
    if (retval != ERROR_OK) {
        LOG_ERROR("Could not write to register '%s'", reg->name);
    }
}

void write_register_opt(struct target *target, char* a0, char* a1, char* a2)
{
    int current = 1;
	target_addr_t addr = 0;
    char* pc_str = "pc";
    char* write_pc_func_addr = g_write_pc_func_addr;
    char* a0_str = "a0";
    char* a1_str = "a1";
    char* a2_str = "a2";
    char* ra_str = "ra";
    char* ra_value = ebreak_addr_str;

    set_riscv_register_value(target, a0_str, a0);
    set_riscv_register_value(target, a1_str, a1);
    set_riscv_register_value(target, a2_str, a2);
    set_riscv_register_value(target, ra_str, ra_value);
    set_riscv_register_value(target, pc_str, write_pc_func_addr);
    target_resume(target, current, addr, 1, 0);
    
    target_wait_state(target, TARGET_HALTED, TIME_OUT_VALUE);
}

void erase_register_opt(struct target *target, int page)
{
    int current = 1;
	target_addr_t addr = 0;
    char* pc_str = "pc";
    char* a0_str = "a0";
    char* a1_str = "a1";
    char* a2_str = "a2";

    char* ra_str = "ra";
    char* ra_value = ebreak_addr_str;

    char* a0_val = hex_to_string(flash_base_addr); // 需要修改
    char* a1_val = "0";
    char* a2_val = hex_to_string(page);

    set_riscv_register_value(target, a0_str, a0_val);
    set_riscv_register_value(target, a1_str, a1_val);
    set_riscv_register_value(target, a2_str, a2_val);
    set_riscv_register_value(target, ra_str, ra_value);
    set_riscv_register_value(target, pc_str, erase_pc_func_addr);
    target_resume(target, current, addr, 1, 0);
    target_wait_state(target, TARGET_HALTED, TIME_OUT_VALUE);
}

void start_opt(struct target *target)
{
    char* ra_str = "ra";
    set_riscv_register_value(target, ra_str, ebreak_addr_str);
    int current = 1;
	target_addr_t addr = 0;
    char* pc_str = "pc";
    set_riscv_register_value(target, pc_str, start_func_addr);
    target_resume(target, current, addr, 1, 0);
    target_wait_state(target, TARGET_HALTED, TIME_OUT_VALUE);
}

void init_register_opt(struct target *target)
{
    int current = 1;
	target_addr_t addr = 0;
    char* pc_str = "pc";
    char* a0_str = "a0";
    char* a1_str = "a1";
    char* a2_str = "a2";
    char* ra_str = "ra";
    char* ra_value = ebreak_addr_str;

    char* a0_val = "0";
    char* a1_val = "0";
    char* a2_val = "0";
    set_riscv_register_value(target, a0_str, a0_val);
    set_riscv_register_value(target, a1_str, a1_val);
    set_riscv_register_value(target, a2_str, a2_val);
    set_riscv_register_value(target, ra_str, ra_value);
    set_riscv_register_value(target, pc_str, init_pc_func_addr);
    target_resume(target, current, addr, 1, 0);
    target_wait_state(target, TARGET_HALTED, TIME_OUT_VALUE);
}

char** get_chip_param_value(const char* filename, int* line_count) {
    FILE *file = NULL;
    char **lines = NULL;
    char buffer[1024];       // 临时缓冲区
    int capacity = 10;       // 初始容量
    int count = 0;           // 实际行数

    // 参数检查
    if (filename == NULL || line_count == NULL) {
        return NULL;
    }

    // 打开文件
    file = fopen(filename, "r");
    if (file == NULL) {
        return NULL;
    }

    // 初始分配内存
    lines = (char**)malloc(capacity * sizeof(char*));
    if (lines == NULL) {
        fclose(file);
        return NULL;
    }

    // 逐行读取文件
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        // 移除行末的换行符
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
            len--;
        }
        if (len > 0 && buffer[len-1] == '\r') {
            buffer[len-1] = '\0';
        }
        // 如果容量不足，重新分配内存
        if (count >= capacity) {
            capacity *= 2;
            char **temp = (char**)realloc(lines, capacity * sizeof(char*));
            if (temp == NULL) {
                // 重新分配失败，释放已分配的内存
                for (int i = 0; i < count; i++) {
                    free(lines[i]);
                }
                free(lines);
                fclose(file);
                return NULL;
            }
            lines = temp;
        }
        // 为当前行分配内存
        lines[count] = (char*)malloc(strlen(buffer) + 1);
        if (lines[count] == NULL) {
            // 分配当前行失败，释放之前所有行的内存
            for (int i = 0; i < count; i++) {
                free(lines[i]);
            }
            free(lines);
            fclose(file);
            return NULL;
        }
        // 复制字符串到数组
        strcpy(lines[count], buffer);
        count++;
    }
    // 关闭文件
    fclose(file);
    // 返回结果
    *line_count = count;
    return lines;
}

void free_string_array(char** lines, int line_count) {
    if (lines == NULL) return;
    
    for (int i = 0; i < line_count; i++) {
        free(lines[i]);
    }
    free(lines);
}

long string_to_number(const char* str) {
    if (str == NULL || *str == '\0') {
        return 0;
    }
    
    // 使用strtol自动识别进制
    char* endptr;
    long value = strtol(str, &endptr, 0);
    
    // 检查转换是否成功
    if (endptr == str) {
        return 0;
    }
    
    return value;
}

COMMAND_HANDLER(handle_flash_load_load_image)
{
    uint8_t *buffer;
    size_t length;
    int retval;
    struct fileio *fileio;
    size_t write_cnt = 0;
    size_t buf_cnt = 0;
    uint8_t g_offine_bin_file[OFFLINE_FLASH_MAX_LEN] = {0};
	struct target *target = get_current_target(CMD_CTX);

    uint32_t file_size = get_offline_file_size(CMD_ARGV[0]);
    if (file_size < 0) {
        LOG_ERROR("read bin fail");
        return ERROR_TARGET_INVALID;
    }
    flash_base_addr = string_to_number(CMD_ARGV[1]);
    if (!read_offline_bin_file(g_offine_bin_file, file_size, CMD_ARGV[0])) {
        return ERROR_TARGET_INVALID;
    }
    int line_count = 0;
    char **lines = get_chip_param_value(chip_param_file_name, &line_count);
    if (lines == NULL) {
        LOG_ERROR("get param fail");
        return ERROR_TARGET_INVALID;
    }
    if (line_count < 8) { // 需要读取8个参数
        LOG_ERROR("get line param fail");
        free_string_array(lines, line_count);
        return ERROR_TARGET_INVALID;
    }
    long long burn_program_addr = string_to_number(lines[0]);
    start_func_addr = lines[1];
    g_start_timeout = string_to_number(lines[2]);
    long long sram_addr = string_to_number(lines[3]);
    char* pc_addr = lines[4];
    long long flash_write_size = string_to_number(lines[5]);
    ebreak_addr_str = lines[6];
    erase_size = string_to_number(lines[7]);

    uint32_t buffer_arr[13] = {0};
    if (target_read_buffer(target, burn_program_addr, 36, &buffer_arr)) { // burn_program_addr 表示保存的烧录程序的内存地址
        free_string_array(lines, line_count);
        LOG_ERROR("read buffer fail");
        return ERROR_TARGET_INVALID;
    }

    init_pc_func_addr = hex_to_string(buffer_arr[1]);
    erase_pc_func_addr = hex_to_string(buffer_arr[8]);
    g_write_pc_func_addr = hex_to_string(buffer_arr[7]);

    char* pc_str = "pc";
    uint8_t* data_ptr = g_offine_bin_file;
    start_opt(target);
    init_register_opt(target);
    if (file_size % erase_size) {
        erase_register_opt(target, (file_size / erase_size) + 1);
    } else {
        erase_register_opt(target, file_size / erase_size);
    }

    char* a0  = hex_to_string(flash_base_addr);
    char* a1  = "";
    char* a2  = "";
    char* func  = pc_addr; // 烧录完之后设置pc指针至指定的位置
    unsigned long a0_index = flash_base_addr;
    int num = file_size / flash_write_size;
    for (int i = 0; i < num; i++) {
        if (target_write_buffer(target, sram_addr, flash_write_size, data_ptr)) {
            free_string_array(lines, line_count);
            LOG_ERROR("write buffer fail");
            return ERROR_TARGET_INVALID;
        }

        a0 = hex_to_string(a0_index);
        a1 = hex_to_string(flash_write_size);
        a2 = hex_to_string(sram_addr);
        write_register_opt(target, a0, a1, a2);
        data_ptr += flash_write_size;
        a0_index += flash_write_size;
        free_hex_string(a0);
        free_hex_string(a1);
        free_hex_string(a2);
    }
    long value_index = file_size % flash_write_size;
    if (value_index != 0) {
        if (target_write_buffer(target, sram_addr, value_index, data_ptr)) {
            free_string_array(lines, line_count);
            LOG_ERROR("burn fail");
            return ERROR_TARGET_INVALID;
        }
        a0 = hex_to_string(a0_index);
        a1 = hex_to_string(file_size % flash_write_size);
        a2 = hex_to_string(sram_addr);
        write_register_opt(target, a0, a1, a2);
        target_halt(target);
        free_hex_string(a0);
        free_hex_string(a1);
        free_hex_string(a2);
    }
    set_riscv_register_value(target, pc_str, func);
    free_hex_string(init_pc_func_addr);
    free_hex_string(erase_pc_func_addr);
    free_hex_string(g_write_pc_func_addr);
    free_string_array(lines, line_count);
	target_call_event_callbacks(target, TARGET_EVENT_GDB_FLASH_WRITE_END);
    LOG_INFO("burn success");
    return ERROR_OK;
}

static const struct command_registration flash_offline_config_command_handlers[] = {
    {
        .name = "write_image",
        .handler = handle_flash_offline_load_image,
        .mode = COMMAND_EXEC,
        .usage = "filename",
        .help = "load offline image.",
    },
    COMMAND_REGISTRATION_DONE
};

static const struct command_registration flash_load_config_command_handlers[] = {
    {
        .name = "load_offline_image",
        .handler = handle_flash_load_load_image,
        .mode = COMMAND_EXEC,
        .usage = "filename",
        .help = "load image.",
    },
    COMMAND_REGISTRATION_DONE
};

static const struct command_registration flash_offline_command_handlers[] = {
    {
        .name = "flash_offline",
        .mode = COMMAND_ANY,
        .help = "flash offline command group",
        .chain = flash_offline_config_command_handlers,
        .usage = "",
    },
    COMMAND_REGISTRATION_DONE
};

static const struct command_registration flash_load_command_handlers[] = {
    {
        .name = "flash_load",
        .mode = COMMAND_ANY,
        .help = "flash load command group",
        .chain = flash_load_config_command_handlers,
        .usage = "",
    },
    COMMAND_REGISTRATION_DONE
};

int flash_offline_register_commands(struct command_context *cmd_ctx)
{
    return register_commands(cmd_ctx, NULL, flash_offline_command_handlers);
}

int flash_load_register_commands(struct command_context *cmd_ctx)
{
    return register_commands(cmd_ctx, NULL, flash_load_command_handlers);
}

#endif /* #ifdef HISPARK_TRACE_MODIFIES */