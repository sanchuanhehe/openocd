#ifndef FLASH_OFFLINE_LOAD_H
#define FLASH_OFFLINE_LOAD_H

#ifdef HISPARK_TRACE_MODIFIES
#define FLASH_OFFLINE_LOAD_HEAD_LEN        48
#define FLASH_OFFLINE_LOAD_ONCE_LEN        1024

int flash_offline_register_commands(struct command_context *cmd_ctx);
int flash_load_register_commands(struct command_context *cmd_ctx);
#endif /* #ifdef HISPARK_TRACE_MODIFIES */

#endif /* #ifndef FLASH_OFFLINE_LOAD_H */