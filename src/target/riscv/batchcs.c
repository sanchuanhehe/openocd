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
 *  RISCV-CoreSight access memory functions
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "batchcs.h"
#include "riscv.h"

memory_batch_t *memory_batch_alloc(struct target *target, size_t scans)
{
	memory_batch_t *out = calloc(1, sizeof(*out));
	if(out == NULL) {
		LOG_ERROR("memory_batch_alloc FAIL!");
		return NULL;
	}
	out->target = target;
	out->allocated_scans = scans;
	out->value_buf = malloc(sizeof(*out->value_buf) * (scans));
	out->read_keys = malloc(sizeof(*out->read_keys) * (scans));
	if(out->value_buf == NULL || out->read_keys == NULL){
		LOG_ERROR("memory_batch_alloc FAIL!");
		return NULL;
	}
	return out;
}

void memory_batch_free(memory_batch_t *batch)
{
	batch->used_scans = 0;
	free(batch->value_buf);
	free(batch->read_keys);
	free(batch);
	batch = NULL;
}

bool memory_batch_full(memory_batch_t *batch)
{
	// when use DTM, read+nop=Read, write+nop=Write,so it's 4.
	// CoreSight dress to DTM, prevent array overflow
	return batch->used_scans > (batch->allocated_scans - 4);
}

void memory_batch_write(memory_batch_t *batch, unsigned address, uint32_t value,uint64_t addr)
{
	struct riscv_info *r = riscv_info(batch->target);
	r->dmi_write_fast(batch->target, address, value);
	batch->used_scans++;
	batch->writes++;
}

/* Read 32 times,then check cmderr.Reduce the number of inspection errors,Increase execution speed.
 * Save the value of data0 value_buf[] in order. */
size_t memory_batch_read(memory_batch_t *batch, unsigned address,uint64_t addr)
{
	struct riscv_info *r = riscv_info(batch->target);
	uint32_t value;
	r->dmi_read_fast(batch->target, &value, address);
	batch->value_buf[batch->used_scans] = value;
	/* save the value in data0 into value_buf */
	/* value_buf[0]=data0_0 , value_buf[1]=data0_1 */
	batch->read_keys[batch->reads] = batch->used_scans;
	batch->used_scans++;
	batch->reads++;
	return batch->reads;
}

uint32_t memory_batch_get_dmi_read(memory_batch_t *batch, size_t key)
{
	assert(key <= batch->reads);
	size_t index = batch->read_keys[key];
	assert(index <= batch->used_scans);
	/* return value read from memory */
	return batch->value_buf[index];
}
