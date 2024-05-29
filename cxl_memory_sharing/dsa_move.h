#ifndef __DSA_MOVE_H__
#define __DSA_MOVE_H__

#include "dml/dml.h"
#include <stddef.h>
// customer can define the job number and move size
#define DSA_JOB_NUM 1
#define DSA_MOVE_BLOCK 16384 //16k once

dml_job_t ** init_dsa(int job_num);
void dsa_move_data(dml_job_t ** jobs, int job_num, void * src, void * dst, size_t src_size, size_t block_size);
#endif
