/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef OPENOCD_JTAG_DRIVERS_CMSIS_DAP_H
#define OPENOCD_JTAG_DRIVERS_CMSIS_DAP_H

#include <stdint.h>

struct cmsis_dap_backend;
struct cmsis_dap_backend_data;
struct command_registration;

#ifdef HISPARK_TRACE_MODIFIES
#define MAXVARNUM 60
#define HISPARK_OFFLINE_ONCE_SEND_DATA    256
#define OFFLINE_STOP_STATUS_DONE          1
#define OFFLINE_STOP_STATUS_FAIL          2
#endif /* HISPARK_TRACE_MODIFIES */

struct cmsis_dap {
	struct cmsis_dap_backend_data *bdata;
	const struct cmsis_dap_backend *backend;
	uint16_t packet_size;
	int packet_count;
	uint8_t *packet_buffer;
	uint16_t packet_buffer_size;
	uint8_t *command;
	uint8_t *response;
	uint16_t caps;
	uint8_t mode;
	uint32_t swo_buf_sz;
	bool trace_enabled;
};

struct cmsis_dap_backend {
	const char *name;
	int (*open)(struct cmsis_dap *dap, uint16_t vids[], uint16_t pids[], const char *serial,
	    char *usbDeviceName);
	void (*close)(struct cmsis_dap *dap);
	int (*read)(struct cmsis_dap *dap, int timeout_ms);
	int (*write)(struct cmsis_dap *dap, int len, int timeout_ms);
	int (*packet_buffer_alloc)(struct cmsis_dap *dap, unsigned int pkt_sz);
};

#ifdef HISPARK_TRACE_MODIFIES
int cmsis_dap_cmd_DAP_Var_Monitor(unsigned int periodUs, unsigned int varNum, unsigned int *varAddress, unsigned int *varNumBytes);
int cmsis_dap_cmd_DAP_Read_Var(unsigned int numOfRequestedData, unsigned char *numOfResponseData, unsigned char **data);
int cmsis_dap_cmd_DAP_Stop_VarMonitor(void);
int cmsis_dap_cmd_DAP_Write_Var(unsigned int address, unsigned int length, const uint8_t *buffer);
int cmsis_dap_cmd_DAP_Puase_VarMonitor(unsigned char status);
int cmsis_dap_cmd_DAP_Var_Config(unsigned int ahbChannelFlag);

int cmsis_dap_cmd_write_offline_head(uint8_t *buffer, uint32_t len);
int cmsis_dap_cmd_write_offline_data(uint8_t *buffer, uint32_t len);
int cmsis_dap_cmd_write_offline_stop(uint8_t status);
#endif /* HISPARK_TRACE_MODIFIES */
extern const struct cmsis_dap_backend cmsis_dap_hid_backend;
extern const struct cmsis_dap_backend cmsis_dap_usb_backend;
extern const struct command_registration cmsis_dap_usb_subcommand_handlers[];

#define REPORT_ID_SIZE   1

#endif
