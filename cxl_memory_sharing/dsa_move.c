#include "dsa_move.h"
#include <stdio.h>
#include <stdlib.h>
dml_job_t ** init_dsa(int job_num)
{
    dml_job_t * dml_job_ptr;
    uint32_t size;
    dml_path_t execution_path = DML_PATH_HW;
    dml_status_t status = dml_get_job_size(execution_path, &size);
    if (DML_STATUS_OK != status) {
        printf("An error (%u) occured during getting job size.\n", status);
        return NULL;
    }

    dml_job_t ** job = (dml_job_t **) malloc(sizeof(dml_job_t *) * job_num);
    
    for (int i = 0;i < job_num ;i++) 
    {
    	job[i] = NULL;
    }
    for (int i = 0;i < job_num ;i++) 
    {
    	dml_job_ptr = (dml_job_t *)malloc(size);
    	status = dml_init_job(execution_path, dml_job_ptr);
    	if (DML_STATUS_OK != status) {
            printf("An error (%u) occured during job initialization.\n", status);
            goto dml_error;
	}

	job[i] = dml_job_ptr;
    }
    return job;

dml_error:
    for(int i=0; i< job_num; i++)
    {
	if(job[i] != NULL) {
	  dml_finalize_job(job[i]);
	}
    }
    free(job);
    return NULL;
}

void dsa_move_data(dml_job_t ** jobs, int job_num, void * src, void * dst, size_t src_size, size_t block_size)
{
    uint8_t * source = (uint8_t *)src;
    uint8_t * dest = (uint8_t *)dst;
    int used_job_num = 0;
    size_t max_move = block_size * job_num;
    int i;
    dml_status_t status;
    while (src_size > max_move) 
    {
        for (i = 0; i < job_num ; i++) 
        {
	    jobs[i]->operation              = DML_OP_MEM_MOVE;
            jobs[i]->source_first_ptr       = source;
            jobs[i]->destination_first_ptr  = dest;
            jobs[i]->source_length          = block_size;
            jobs[i]->flags = DML_FLAG_BLOCK_ON_FAULT;

            status = dml_submit_job(jobs[i]);
            if (DML_STATUS_OK != status) 
	    {
               goto dml_error;
	    }
	    source += block_size;
	    dest += block_size;
	    src_size -= block_size;
	}
		
	// wait all the job to be done
	for (i = 0; i < job_num; i++) 
	{
	    status = dml_wait_job(jobs[i], DML_WAIT_MODE_BUSY_POLL);	
	    if (DML_STATUS_OK != status) 
	    {
	    	goto dml_error;	
	    }
	}
    }

    for (i = 0; i < job_num && src_size > block_size; i++) 
    {
        jobs[i]->operation              = DML_OP_MEM_MOVE;
        jobs[i]->source_first_ptr       = source;
        jobs[i]->destination_first_ptr  = dest;
        jobs[i]->source_length          = block_size;
        jobs[i]->flags = DML_FLAG_BLOCK_ON_FAULT;

        status = dml_submit_job(jobs[i]);
        if (DML_STATUS_OK != status) 
	{
           goto dml_error;
	}
	source += block_size;
	dest += block_size;
	src_size -= block_size;
	if(src_size < block_size) 
	{ 
	   i++; 
	   break;
	}
    } 
    
    if (src_size > 0) {
        jobs[i]->operation              = DML_OP_MEM_MOVE;
        jobs[i]->source_first_ptr       = source;
        jobs[i]->destination_first_ptr  = dest;
        jobs[i]->source_length          = src_size;
        jobs[i]->flags = DML_FLAG_BLOCK_ON_FAULT;

        status = dml_submit_job(jobs[i]);
        if (DML_STATUS_OK != status) 
	{
           goto dml_error;
	}
	i++;
    }
    used_job_num = i;
    for (i = 0;i < used_job_num ; i++)
    {
	status = dml_wait_job(jobs[i], DML_WAIT_MODE_BUSY_POLL);	
	if (DML_STATUS_OK != status) 
	{
	    goto dml_error;	
	}
    }

    return;
 dml_error:
    printf("dml meet error and the error code =%d\n", status);
    for( i=0; i< job_num ; i++ )
    {
	dml_finalize_job(jobs[i]);
    }
}

