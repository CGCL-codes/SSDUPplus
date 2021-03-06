/* WARNING: THIS FILE IS AUTOMATICALLY GENERATED FROM A .SM FILE.
 * Changes made here will certainly be overwritten.
 */

/* 
 * (C) 2001 Clemson University and The University of Chicago 
 *
 * See COPYING in top-level directory.
 */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>

#include "pvfs2-server.h"
#include "pvfs2-internal.h"
#include "job-time-mgr.h"
#include "server-config.h"
#include "pint-security.h"


static PINT_sm_action job_timer_do_work(
	struct PINT_smcb *smcb, job_status_s *js_p);

static struct PINT_state_s ST_do_work;
static struct PINT_pjmp_tbl_s ST_do_work_pjtbl[];
static struct PINT_tran_tbl_s ST_do_work_trtbl[];

static PINT_sm_action job_timer_error(
	struct PINT_smcb *smcb, job_status_s *js_p);

static struct PINT_state_s ST_error;
static struct PINT_pjmp_tbl_s ST_error_pjtbl[];
static struct PINT_tran_tbl_s ST_error_trtbl[];

struct PINT_state_machine_s pvfs2_job_timer_sm = {
	.name = "pvfs2_job_timer_sm",
	.first_state = &ST_do_work
};

static struct PINT_state_s ST_do_work = {
	 .state_name = "do_work" ,
	 .parent_machine = &pvfs2_job_timer_sm ,
	 .flag = SM_RUN ,
	 .action.func = job_timer_do_work ,
	 .pjtbl = NULL ,
	 .trtbl = ST_do_work_trtbl 
};

static struct PINT_tran_tbl_s ST_do_work_trtbl[] = {
	{ .return_value = 0 ,
	 .next_state = &ST_do_work },
	{ .return_value = -1 ,
	 .next_state = &ST_error }
};

static struct PINT_state_s ST_error = {
	 .state_name = "error" ,
	 .parent_machine = &pvfs2_job_timer_sm ,
	 .flag = SM_RUN ,
	 .action.func = job_timer_error ,
	 .pjtbl = NULL ,
	 .trtbl = ST_error_trtbl 
};

static struct PINT_tran_tbl_s ST_error_trtbl[] = {
	{ .return_value = -1 ,

	 .flag = SM_TERM }
};

#ifndef WIN32
# 37 "src/server/job-timer.sm"
#endif


/* job_timer_error()
 *
 * cleans up any resources consumed by this state machine and ends
 * execution of the machine
 */
static PINT_sm_action job_timer_error(
        struct PINT_smcb *smcb, job_status_s *js_p)
{
    gossip_err("Error: stopping server job timer.\n");

    return(server_state_machine_complete(smcb));
}

/* job_timer_do_work()
 *
 * resets counters, updates metrices, etc- this is intended to be called
 * repeatedly on a regular interval
 */
static PINT_sm_action job_timer_do_work(
        struct PINT_smcb *smcb, job_status_s *js_p)
{
    int ret = -1;
    job_id_t tmp_id;

#if 0
    PINT_STATE_DEBUG("do_work");
#endif

    /* look for expired jobs */
    ret = job_time_mgr_expire();
    if(ret < 0)
    {
	js_p->error_code = ret;
	return SM_ACTION_COMPLETE;
    }
	
    /* post another timer */
    return(job_req_sched_post_timer(1000,
	    smcb,
	    0,
	    js_p,
	    &tmp_id,
	    server_job_context));
}

static int perm_job_timer(PINT_server_op *s_op)
{
    int ret;

    if (s_op->req->capability.op_mask & PINT_CAP_ADMIN)
    {
        ret = 0;
    }
    else
    {
        ret = -PVFS_EACCES;
    }

    return ret;
}

struct PINT_server_req_params pvfs2_job_timer_params =
{
    .string_name = "job_timer",
    .perm = perm_job_timer,
    .state_machine = &pvfs2_job_timer_sm
};




/*
 * Local variables:
 *  mode: c
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ft=c ts=8 sts=4 sw=4 expandtab
 */

