/***************************************************************************
 *   Copyright (C) 2011 by Spencer Oliver <spen@spen-soft.co.uk>           *
 *                                                                         *
 *   Written by Arnim Laeuger, 2008 (from urjtag)                          *
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
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
 ***************************************************************************/

#ifndef _FTD2XX_COMMON_H
#define _FTD2XX_COMMON_H

#if ((BUILD_FT2232_FTD2XX == 1) || (BUILD_PRESTO_FTD2XX == 1) || (BUILD_USB_BLASTER_FTD2XX == 1))
#if IS_WIN32 == 0
#include <dlfcn.h> 
#define WINAPI

typedef unsigned char            UCHAR;
typedef char                    *PCHAR;
typedef UCHAR                   *PUCHAR;
typedef unsigned short           WORD;
typedef unsigned int             DWORD;
typedef WORD                    *LPWORD;
typedef DWORD                   *LPDWORD;
typedef void                    *PVOID;
typedef PVOID                    LPVOID;
typedef unsigned int             ULONG;

typedef void                    *HMODULE;
typedef void                    *HANDLE;
#else
#include <windows.h>
#endif

typedef PVOID	FT_HANDLE;
typedef ULONG	FT_STATUS;
typedef ULONG	FT_DEVICE;

#define FT_OPEN_BY_SERIAL_NUMBER	1
#define FT_OPEN_BY_DESCRIPTION		2
#define FT_OPEN_BY_LOCATION			4

#define FT_LIST_NUMBER_ONLY			0x80000000
#define FT_LIST_BY_INDEX			0x40000000
#define FT_LIST_ALL					0x20000000

#define FT_PURGE_RX			1
#define FT_PURGE_TX			2

enum {
	FT_OK,
	FT_INVALID_HANDLE,
	FT_DEVICE_NOT_FOUND,
	FT_DEVICE_NOT_OPENED,
	FT_IO_ERROR,
	FT_INSUFFICIENT_RESOURCES,
	FT_INVALID_PARAMETER,
	FT_INVALID_BAUD_RATE,

	FT_DEVICE_NOT_OPENED_FOR_ERASE,
	FT_DEVICE_NOT_OPENED_FOR_WRITE,
	FT_FAILED_TO_WRITE_DEVICE,
	FT_EEPROM_READ_FAILED,
	FT_EEPROM_WRITE_FAILED,
	FT_EEPROM_ERASE_FAILED,
	FT_EEPROM_NOT_PRESENT,
	FT_EEPROM_NOT_PROGRAMMED,
	FT_INVALID_ARGS,
	FT_NOT_SUPPORTED,
	FT_OTHER_ERROR,
	FT_DEVICE_LIST_NOT_READY,
};

/* Mode flags */
#define POS_EDGE_OUT 0x00
#define NEG_EDGE_OUT 0x01
#define POS_EDGE_IN 0x00
#define NEG_EDGE_IN 0x04
#define MSB_FIRST 0x00
#define LSB_FIRST 0x08

static const char *ftd2xx_status_string(FT_STATUS status)
{
	switch (status) {
		case FT_OK:							return "OK";
		case FT_INVALID_HANDLE:				return "invalid handle";
		case FT_DEVICE_NOT_FOUND:			return "device not found";
		case FT_DEVICE_NOT_OPENED:			return "device not opened";
		case FT_IO_ERROR:					return "io error";
		case FT_INSUFFICIENT_RESOURCES:		return "insufficient resources";
		case FT_INVALID_PARAMETER:			return "invalid parameter";
		case FT_INVALID_BAUD_RATE:			return "invalid baud rate";

		case FT_DEVICE_NOT_OPENED_FOR_ERASE: return "device not opened for erase";
		case FT_DEVICE_NOT_OPENED_FOR_WRITE: return "device not opened for write";
		case FT_FAILED_TO_WRITE_DEVICE:		return "failed to write device";
		case FT_EEPROM_READ_FAILED:			return "eeprom read failed";
		case FT_EEPROM_WRITE_FAILED:		return "eeprom write failed";
		case FT_EEPROM_ERASE_FAILED:		return "eeprom erase failed";
		case FT_EEPROM_NOT_PRESENT:			return "eeprom not present";
		case FT_EEPROM_NOT_PROGRAMMED:		return "eeprom not programmed";
		case FT_INVALID_ARGS:				return "invalid args";
		case FT_NOT_SUPPORTED:				return "not supported";
		case FT_OTHER_ERROR:				return "other error";
	}

	return "undefined FTD2xx error";
}

#endif
#endif /* _FTD2XX_COMMON_H */
