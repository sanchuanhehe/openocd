#ifndef BATCHCS_H
#define BATCHCS_H
/***************************************************************************
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
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

/**
 * @file
 *  RISCV-CoreSight access memory
 */

#include "target/target.h"

typedef struct memory_batch {
	struct target *target;

	size_t allocated_scans;
	size_t used_scans;
	size_t reads;
	size_t writes;
	uint32_t *value_buf;
	/* The read keys. */
	size_t *read_keys;
} memory_batch_t;

memory_batch_t *memory_batch_alloc(struct target *target, size_t scans);

void memory_batch_free(memory_batch_t *batch);

bool memory_batch_full(memory_batch_t *batch);

void memory_batch_write(memory_batch_t *batch, unsigned address, uint32_t value,uint64_t addr);

size_t memory_batch_read(memory_batch_t *batch, unsigned address,uint64_t addr);

uint32_t memory_batch_get_dmi_read(memory_batch_t *batch, size_t key);


#endif
