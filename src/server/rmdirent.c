/* WARNING: THIS FILE IS AUTOMATICALLY GENERATED FROM A .SM FILE.
 * Changes made here will certainly be overwritten.
 */

/* 
 * (C) 2001 Clemson University and The University of Chicago 
 *
 * See COPYING in top-level directory.
 */

#include <string.h>
#include <assert.h>

#include "server-config.h"
#include "pvfs2-server.h"
#include "pvfs2-attr.h"
#include "gossip.h"
#include "pvfs2-util.h"
#include "pvfs2-internal.h"
#include "pint-util.h"
#include "pint-security.h"
#include "dist-dir-utils.h"

enum
{
    UPDATE_DIR_ATTR_REQUIRED = 133
};

static struct PINT_state_s ST_prelude;
static struct PINT_pjmp_tbl_s ST_prelude_pjtbl[];
static struct PINT_tran_tbl_s ST_prelude_trtbl[];

static PINT_sm_action rmdirent_get_dirdata_attr(
	struct PINT_smcb *smcb, job_status_s *js_p);

static struct PINT_state_s ST_get_dirdata_attr;
static struct PINT_pjmp_tbl_s ST_get_dirdata_attr_pjtbl[];
static struct PINT_tran_tbl_s ST_get_dirdata_attr_trtbl[];

static PINT_sm_action rmdirent_get_dist_dir_attr(
	struct PINT_smcb *smcb, job_status_s *js_p);

static struct PINT_state_s ST_get_dist_dir_attr;
static struct PINT_pjmp_tbl_s ST_get_dist_dir_attr_pjtbl[];
static struct PINT_tran_tbl_s ST_get_dist_dir_attr_trtbl[];

static PINT_sm_action rmdirent_get_bitmap(
	struct PINT_smcb *smcb, job_status_s *js_p);

static struct PINT_state_s ST_get_bitmap;
static struct PINT_pjmp_tbl_s ST_get_bitmap_pjtbl[];
static struct PINT_tran_tbl_s ST_get_bitmap_trtbl[];

static PINT_sm_action rmdirent_remove_directory_entry(
	struct PINT_smcb *smcb, job_status_s *js_p);

static struct PINT_state_s ST_remove_directory_entry;
static struct PINT_pjmp_tbl_s ST_remove_directory_entry_pjtbl[];
static struct PINT_tran_tbl_s ST_remove_directory_entry_trtbl[];

static PINT_sm_action rmdirent_remove_directory_entry_failure(
	struct PINT_smcb *smcb, job_status_s *js_p);

static struct PINT_state_s ST_remove_directory_entry_failure;
static struct PINT_pjmp_tbl_s ST_remove_directory_entry_failure_pjtbl[];
static struct PINT_tran_tbl_s ST_remove_directory_entry_failure_trtbl[];

static PINT_sm_action rmdirent_check_for_req_dir_update(
	struct PINT_smcb *smcb, job_status_s *js_p);

static struct PINT_state_s ST_check_for_req_dir_update;
static struct PINT_pjmp_tbl_s ST_check_for_req_dir_update_pjtbl[];
static struct PINT_tran_tbl_s ST_check_for_req_dir_update_trtbl[];

static PINT_sm_action rmdirent_update_directory_attr(
	struct PINT_smcb *smcb, job_status_s *js_p);

static struct PINT_state_s ST_update_directory_attr;
static struct PINT_pjmp_tbl_s ST_update_directory_attr_pjtbl[];
static struct PINT_tran_tbl_s ST_update_directory_attr_trtbl[];

static PINT_sm_action rmdirent_setup_resp(
	struct PINT_smcb *smcb, job_status_s *js_p);

static struct PINT_state_s ST_setup_resp;
static struct PINT_pjmp_tbl_s ST_setup_resp_pjtbl[];
static struct PINT_tran_tbl_s ST_setup_resp_trtbl[];
static struct PINT_state_s ST_final_response;
static struct PINT_pjmp_tbl_s ST_final_response_pjtbl[];
static struct PINT_tran_tbl_s ST_final_response_trtbl[];

static PINT_sm_action rmdirent_cleanup(
	struct PINT_smcb *smcb, job_status_s *js_p);

static struct PINT_state_s ST_cleanup;
static struct PINT_pjmp_tbl_s ST_cleanup_pjtbl[];
static struct PINT_tran_tbl_s ST_cleanup_trtbl[];

struct PINT_state_machine_s pvfs2_rmdirent_sm = {
	.name = "pvfs2_rmdirent_sm",
	.first_state = &ST_prelude
};

static struct PINT_state_s ST_prelude = {
	 .state_name = "prelude" ,
	 .parent_machine = &pvfs2_rmdirent_sm ,
	 .flag = SM_JUMP ,
	 .action.nested = &pvfs2_prelude_sm ,
	 .pjtbl = NULL ,
	 .trtbl = ST_prelude_trtbl 
};

static struct PINT_tran_tbl_s ST_prelude_trtbl[] = {
	{ .return_value = 0 ,
	 .next_state = &ST_get_dirdata_attr },
	{ .return_value = -1 ,
	 .next_state = &ST_final_response }
};

static struct PINT_state_s ST_get_dirdata_attr = {
	 .state_name = "get_dirdata_attr" ,
	 .parent_machine = &pvfs2_rmdirent_sm ,
	 .flag = SM_RUN ,
	 .action.func = rmdirent_get_dirdata_attr ,
	 .pjtbl = NULL ,
	 .trtbl = ST_get_dirdata_attr_trtbl 
};

static struct PINT_tran_tbl_s ST_get_dirdata_attr_trtbl[] = {
	{ .return_value = 0 ,
	 .next_state = &ST_get_dist_dir_attr },
	{ .return_value = -1 ,
	 .next_state = &ST_final_response }
};

static struct PINT_state_s ST_get_dist_dir_attr = {
	 .state_name = "get_dist_dir_attr" ,
	 .parent_machine = &pvfs2_rmdirent_sm ,
	 .flag = SM_RUN ,
	 .action.func = rmdirent_get_dist_dir_attr ,
	 .pjtbl = NULL ,
	 .trtbl = ST_get_dist_dir_attr_trtbl 
};

static struct PINT_tran_tbl_s ST_get_dist_dir_attr_trtbl[] = {
	{ .return_value = 0 ,
	 .next_state = &ST_get_bitmap },
	{ .return_value = -1 ,
	 .next_state = &ST_setup_resp }
};

static struct PINT_state_s ST_get_bitmap = {
	 .state_name = "get_bitmap" ,
	 .parent_machine = &pvfs2_rmdirent_sm ,
	 .flag = SM_RUN ,
	 .action.func = rmdirent_get_bitmap ,
	 .pjtbl = NULL ,
	 .trtbl = ST_get_bitmap_trtbl 
};

static struct PINT_tran_tbl_s ST_get_bitmap_trtbl[] = {
	{ .return_value = 0 ,
	 .next_state = &ST_remove_directory_entry },
	{ .return_value = -1 ,
	 .next_state = &ST_setup_resp }
};

static struct PINT_state_s ST_remove_directory_entry = {
	 .state_name = "remove_directory_entry" ,
	 .parent_machine = &pvfs2_rmdirent_sm ,
	 .flag = SM_RUN ,
	 .action.func = rmdirent_remove_directory_entry ,
	 .pjtbl = NULL ,
	 .trtbl = ST_remove_directory_entry_trtbl 
};

static struct PINT_tran_tbl_s ST_remove_directory_entry_trtbl[] = {
	{ .return_value = 0 ,
	 .next_state = &ST_check_for_req_dir_update },
	{ .return_value = -1 ,
	 .next_state = &ST_remove_directory_entry_failure }
};

static struct PINT_state_s ST_remove_directory_entry_failure = {
	 .state_name = "remove_directory_entry_failure" ,
	 .parent_machine = &pvfs2_rmdirent_sm ,
	 .flag = SM_RUN ,
	 .action.func = rmdirent_remove_directory_entry_failure ,
	 .pjtbl = NULL ,
	 .trtbl = ST_remove_directory_entry_failure_trtbl 
};

static struct PINT_tran_tbl_s ST_remove_directory_entry_failure_trtbl[] = {
	{ .return_value = -1 ,
	 .next_state = &ST_setup_resp }
};

static struct PINT_state_s ST_check_for_req_dir_update = {
	 .state_name = "check_for_req_dir_update" ,
	 .parent_machine = &pvfs2_rmdirent_sm ,
	 .flag = SM_RUN ,
	 .action.func = rmdirent_check_for_req_dir_update ,
	 .pjtbl = NULL ,
	 .trtbl = ST_check_for_req_dir_update_trtbl 
};

static struct PINT_tran_tbl_s ST_check_for_req_dir_update_trtbl[] = {
	{ .return_value = UPDATE_DIR_ATTR_REQUIRED ,
	 .next_state = &ST_update_directory_attr },
	{ .return_value = -1 ,
	 .next_state = &ST_setup_resp }
};

static struct PINT_state_s ST_update_directory_attr = {
	 .state_name = "update_directory_attr" ,
	 .parent_machine = &pvfs2_rmdirent_sm ,
	 .flag = SM_RUN ,
	 .action.func = rmdirent_update_directory_attr ,
	 .pjtbl = NULL ,
	 .trtbl = ST_update_directory_attr_trtbl 
};

static struct PINT_tran_tbl_s ST_update_directory_attr_trtbl[] = {
	{ .return_value = -1 ,
	 .next_state = &ST_setup_resp }
};

static struct PINT_state_s ST_setup_resp = {
	 .state_name = "setup_resp" ,
	 .parent_machine = &pvfs2_rmdirent_sm ,
	 .flag = SM_RUN ,
	 .action.func = rmdirent_setup_resp ,
	 .pjtbl = NULL ,
	 .trtbl = ST_setup_resp_trtbl 
};

static struct PINT_tran_tbl_s ST_setup_resp_trtbl[] = {
	{ .return_value = -1 ,
	 .next_state = &ST_final_response }
};

static struct PINT_state_s ST_final_response = {
	 .state_name = "final_response" ,
	 .parent_machine = &pvfs2_rmdirent_sm ,
	 .flag = SM_JUMP ,
	 .action.nested = &pvfs2_final_response_sm ,
	 .pjtbl = NULL ,
	 .trtbl = ST_final_response_trtbl 
};

static struct PINT_tran_tbl_s ST_final_response_trtbl[] = {
	{ .return_value = -1 ,
	 .next_state = &ST_cleanup }
};

static struct PINT_state_s ST_cleanup = {
	 .state_name = "cleanup" ,
	 .parent_machine = &pvfs2_rmdirent_sm ,
	 .flag = SM_RUN ,
	 .action.func = rmdirent_cleanup ,
	 .pjtbl = NULL ,
	 .trtbl = ST_cleanup_trtbl 
};

static struct PINT_tran_tbl_s ST_cleanup_trtbl[] = {
	{ .return_value = -1 ,

	 .flag = SM_TERM }
};

#ifndef WIN32
# 101 "src/server/rmdirent.sm"
#endif


static PINT_sm_action rmdirent_get_dirdata_attr(
        struct PINT_smcb *smcb, job_status_s *js_p)
{       
    struct PINT_server_op *s_op = PINT_sm_frame(smcb, PINT_FRAME_CURRENT);
    int ret = -PVFS_EINVAL;
    job_id_t tmp_id;
    memset(&(s_op->u.rmdirent.dirdata_ds_attr), 0, sizeof(PVFS_ds_attributes));
    
    gossip_debug(GOSSIP_SERVER_DEBUG, "About to retrieve attributes "
                 "for dirdata handle %llu\n", llu(s_op->req->u.rmdirent.handle));         

    ret = job_trove_dspace_getattr(
        s_op->target_fs_id, s_op->req->u.rmdirent.handle, smcb,
        &(s_op->u.rmdirent.dirdata_ds_attr),
        0, js_p, &tmp_id, server_job_context, s_op->req->hints);
        
    return ret;
}   

static PINT_sm_action rmdirent_get_dist_dir_attr(
        struct PINT_smcb *smcb, job_status_s *js_p)
{
    struct PINT_server_op *s_op = PINT_sm_frame(smcb, PINT_FRAME_CURRENT);
    int ret;
    job_id_t j_id;

    /*
      first we translate the dirdata dspace attributes into a more convenient
      server use-able format.  i.e. a PVFS_object_attr 
    */
    PVFS_ds_attr_to_object_attr(&s_op->u.rmdirent.dirdata_ds_attr,
                                &s_op->u.rmdirent.dirdata_attr);
    s_op->u.rmdirent.dirdata_attr.mask = PVFS_ATTR_COMMON_ALL;
 
    /* set up key and value structures for reading the dist_dir_attr */
    s_op->key.buffer = Trove_Common_Keys[DIST_DIR_ATTR_KEY].key;
    s_op->key.buffer_sz = Trove_Common_Keys[DIST_DIR_ATTR_KEY].size;
    if(s_op->free_val)
    {
        free(s_op->val.buffer);
    }

    s_op->val.buffer = &s_op->attr.dist_dir_attr;
    s_op->val.buffer_sz = sizeof(PVFS_dist_dir_attr);
    s_op->free_val = 0;

    js_p->error_code = 0;
    gossip_debug(GOSSIP_SERVER_DEBUG,
                 "  trying to read dist_dir_attr (coll_id = %d, "
                 "handle = %llu, key = %s (%d), val_buf = %p (%d))\n",
                 s_op->req->u.rmdirent.fs_id, llu(s_op->req->u.rmdirent.handle),
                 (char *)s_op->key.buffer, s_op->key.buffer_sz,
                 s_op->val.buffer, s_op->val.buffer_sz);

    ret = job_trove_keyval_read(
        s_op->req->u.rmdirent.fs_id, s_op->req->u.rmdirent.handle,
        &s_op->key, &s_op->val,
        0,
        NULL, smcb, 0, js_p,
        &j_id, server_job_context, s_op->req->hints);

    return ret;
}

static PINT_sm_action rmdirent_get_bitmap(
        struct PINT_smcb *smcb, job_status_s *js_p)
{       
    struct PINT_server_op *s_op = PINT_sm_frame(smcb, PINT_FRAME_CURRENT);
    int ret;
    PVFS_object_attr *attr_p;
    job_id_t j_id;
    
    attr_p = &s_op->attr;
    
    if(js_p->error_code == -TROVE_ENOENT)
    {
        gossip_debug(GOSSIP_SERVER_DEBUG,
            "rmdirent: no DIST_DIR_ATTR key present in dirdata handle!!\n");
        attr_p->dist_dir_bitmap = NULL;
        attr_p->dirdata_handles = NULL;
        return SM_ACTION_COMPLETE;
    }   
    
    assert(attr_p->dist_dir_attr.num_servers > 0);

    gossip_debug(GOSSIP_SERVER_DEBUG,
            "rmdirent: get dist-dir-attr for dirdata handle %llu "
            "with tree_height=%d, num_servers=%d, bitmap_size=%d, "
            "split_size=%d, server_no=%d and branch_level=%d\n",
            llu(s_op->req->u.rmdirent.handle),
            attr_p->dist_dir_attr.tree_height,
            attr_p->dist_dir_attr.num_servers,
            attr_p->dist_dir_attr.bitmap_size,
            attr_p->dist_dir_attr.split_size,
            attr_p->dist_dir_attr.server_no,
            attr_p->dist_dir_attr.branch_level);

    /* allocate space for dirdata bitmap */
    attr_p->dirdata_handles = NULL;
    attr_p->dist_dir_bitmap =
        malloc(attr_p->dist_dir_attr.bitmap_size *
                sizeof(PVFS_dist_dir_bitmap_basetype));
    if(!attr_p->dist_dir_bitmap)
    {
        js_p->error_code = -PVFS_ENOMEM;
        return SM_ACTION_COMPLETE;
    }

    /* set up attr->mask */
    attr_p->mask |= PVFS_ATTR_DISTDIR_ATTR;

    /* set up key and value structures for reading dirdata bitmap */
    s_op->key.buffer = Trove_Common_Keys[DIST_DIRDATA_BITMAP_KEY].key;
    s_op->key.buffer_sz = Trove_Common_Keys[DIST_DIRDATA_BITMAP_KEY].size;
    if(s_op->free_val)
    {
        free(s_op->val.buffer);
    }

    s_op->val.buffer = attr_p->dist_dir_bitmap;
    s_op->val.buffer_sz =
        attr_p->dist_dir_attr.bitmap_size *
        sizeof(PVFS_dist_dir_bitmap_basetype);
    s_op->free_val = 0; /* will be freed in PINT_free_object_attr*/

    js_p->error_code = 0;
    gossip_debug(GOSSIP_SERVER_DEBUG,
                 "  trying to read dirdata bitmap (coll_id = %d, "
                 "handle = %llu, key = %s (%d), val_buf = %p (%d))\n",
                 s_op->req->u.rmdirent.fs_id, llu(s_op->req->u.rmdirent.handle),
                 (char *)s_op->key.buffer, s_op->key.buffer_sz,
                 s_op->val.buffer, s_op->val.buffer_sz);

    ret = job_trove_keyval_read(
        s_op->req->u.rmdirent.fs_id, s_op->req->u.rmdirent.handle,
        &s_op->key, &s_op->val,
        0,
        NULL, smcb, 0, js_p,
        &j_id, server_job_context, s_op->req->hints);

    return ret;
}

/*
 * Function: rmdirent_remove_directory_entry
 *
 * Synopsis: posts a trove keyval remove to remove the directory entry
 * from the dirdata object.
 *           
 */
static int rmdirent_remove_directory_entry(
        struct PINT_smcb *smcb, job_status_s *js_p)
{
    struct PINT_server_op *s_op = PINT_sm_frame(smcb, PINT_FRAME_CURRENT);
    int ret = -PVFS_EINVAL;
    job_id_t j_id;
    TROVE_ds_flags flags;
    int i;
    unsigned char *c;
    PVFS_object_attr *attr_p;
    
    attr_p = &s_op->attr;
    
    /* gossip bitmap, since jump from get_bitmap*/
    gossip_debug(GOSSIP_SERVER_DEBUG, 
            "rmdirent: dist_dir_bitmap as:\n");
    for(i = attr_p->dist_dir_attr.bitmap_size - 1;
            i >= 0 ; i--)
    {       
        c = (unsigned char *)(attr_p->dist_dir_bitmap + i);
        gossip_debug(GOSSIP_SERVER_DEBUG,
                " i=%d : %02x %02x %02x %02x\n",
                i, c[3], c[2], c[1], c[0]);
    }           
    gossip_debug(GOSSIP_SERVER_DEBUG, "\n");
    

    /* make sure the entry belongs to the dirdata handle*/
    PVFS_dist_dir_hash_type dirdata_hash; 
    int dirdata_server_index;
    
    /* find the hash value and the dist dir bucket */
    dirdata_hash = PINT_encrypt_dirdata(s_op->req->u.rmdirent.entry);
    gossip_debug(GOSSIP_SERVER_DEBUG,
        "rmdirent: encrypt dirent %s into hash value %llu.\n",
            s_op->req->u.rmdirent.entry,
            llu(dirdata_hash));
            
    dirdata_server_index =
        PINT_find_dist_dir_bucket(dirdata_hash,
            &attr_p->dist_dir_attr,
            attr_p->dist_dir_bitmap);
    gossip_debug(GOSSIP_SERVER_DEBUG,
        "rmdirent: selecting bucket No.%d from dist_dir_bitmap.\n",
            dirdata_server_index);
            
    if(dirdata_server_index !=
            attr_p->dist_dir_attr.server_no)
    {       
        gossip_debug(GOSSIP_SERVER_DEBUG,
             "rmdirent: error: WRONG dirdata object for the dirent! Returning error.\n");    
 
        /* return an error to tell the client to try again */
        js_p->error_code = -PVFS_EAGAIN;
        return SM_ACTION_COMPLETE;
    }   
    else
    {
        gossip_debug(GOSSIP_SERVER_DEBUG,
                "rmdirent: Correct dirdata object!\n");
    }           
    
    /* start removing entry */
    PINT_ACCESS_DEBUG(s_op, GOSSIP_ACCESS_DEBUG, "rmdirent entry: %s\n",
        s_op->req->u.rmdirent.entry);
        
    /* set up key and structure for keyval remove */
    s_op->key.buffer = s_op->req->u.rmdirent.entry;
    s_op->key.buffer_sz = strlen(s_op->req->u.rmdirent.entry) + 1;

    s_op->val.buffer = &s_op->u.rmdirent.entry_handle;
    s_op->val.buffer_sz = sizeof(PVFS_handle);

    gossip_debug(GOSSIP_SERVER_DEBUG, "  removing entry %s from dirdata "
                 "object (handle = %llu)\n", s_op->req->u.rmdirent.entry,
                 llu(s_op->req->u.rmdirent.handle));

    /* set the sync flag */
    flags = TROVE_SYNC;

    /* Also keep track of the keyval entries removed on this handle.  Because
     * we're doing a remove, this should decrement the count.
     */
    flags |= TROVE_KEYVAL_HANDLE_COUNT | TROVE_KEYVAL_DIRECTORY_ENTRY;

    ret = job_trove_keyval_remove(
        s_op->req->u.rmdirent.fs_id, s_op->req->u.rmdirent.handle,
        &s_op->key, 
        &s_op->val,
        flags,
        NULL, smcb, 0, js_p, &j_id, server_job_context, s_op->req->hints);

    /* 
     * Removing an entry causes an update of directory timestamps
     */
    s_op->u.rmdirent.dir_attr_update_required = 1;
    return ret;
}

static PINT_sm_action rmdirent_check_for_req_dir_update(
        struct PINT_smcb *smcb, job_status_s *js_p)
{
    struct PINT_server_op *s_op = PINT_sm_frame(smcb, PINT_FRAME_CURRENT);
    if ((js_p->error_code == 0) &&
        (s_op->u.rmdirent.dir_attr_update_required))
    {
        js_p->error_code = UPDATE_DIR_ATTR_REQUIRED;
    }
    return SM_ACTION_COMPLETE;
}

static PINT_sm_action rmdirent_update_directory_attr(
        struct PINT_smcb *smcb, job_status_s *js_p)
{
    struct PINT_server_op *s_op = PINT_sm_frame(smcb, PINT_FRAME_CURRENT);
    int ret = -1;
    job_id_t j_id;
    PVFS_object_attr tmp_attr, *tmp_attr_ptr = &tmp_attr;
    PVFS_object_attr *dspace_attr = NULL;
    PVFS_ds_attributes *ds_attr = NULL;

    if (js_p->error_code != UPDATE_DIR_ATTR_REQUIRED)
    {
        PVFS_perror_gossip("previous keyval remove failed",
                           js_p->error_code);
        return SM_ACTION_COMPLETE;
    }

    memset(&tmp_attr, 0, sizeof(PVFS_object_attr));
    dspace_attr = &s_op->u.rmdirent.dirdata_attr;
    dspace_attr->mask |= (PVFS_ATTR_COMMON_ATIME | PVFS_ATTR_COMMON_MTIME | PVFS_ATTR_COMMON_CTIME);

    PVFS_object_attr_overwrite_setable(tmp_attr_ptr, dspace_attr);
    ds_attr = &(s_op->u.rmdirent.dirdata_ds_attr);
    PVFS_object_attr_to_ds_attr(tmp_attr_ptr, ds_attr);

    /* setting dspace */
    ret = job_trove_dspace_setattr(
        s_op->req->u.rmdirent.fs_id, s_op->req->u.rmdirent.handle,
        ds_attr,
        TROVE_SYNC,
        smcb, 0, js_p, &j_id, server_job_context, s_op->req->hints);

    return ret;
}

/* Function: rmdirent_read_directory_entry_failure
 *
 * The purpose of this state is simply to parse the error value from a
 * failed direntry remove operation (trove_keyval_remove), convert it
 * into an error to return to the user if possible, and print an error
 * if something unexpected has occurred.
 *
 * NOTE: we should only ever hit this state with PVFS_EAGAIN if a client
 * has old information after a directory has split. Otherwise, nothing
 * else should be modifying trove in such a way that the keyval isn't
 * there at this point, because the keyval read has to have succeeded
 * for us to get this far.
 *
 * This state always returns 1, and it allows another function to
 * handle actually returning the error value.
 */
static PINT_sm_action rmdirent_remove_directory_entry_failure(
        struct PINT_smcb *smcb, job_status_s *js_p)
{
    switch (js_p->error_code)
    {
	case -TROVE_ENOENT:
	    js_p->error_code = -PVFS_ENOENT;
	    break;
        case -PVFS_EAGAIN:
            gossip_debug(GOSSIP_SERVER_DEBUG, "mismatch dirdata!!\n");
            break;
	default:
            gossip_lerr("unexpected error %d\n", js_p->error_code);
	    break;
    }

    return SM_ACTION_COMPLETE;
}

static PINT_sm_action rmdirent_setup_resp(
        struct PINT_smcb *smcb, job_status_s *js_p)
{
    struct PINT_server_op *s_op = PINT_sm_frame(smcb, PINT_FRAME_CURRENT);
    /* Set the handle if it was removed */
    if(js_p->error_code == 0)
    {
	/*
          we return the handle from the directory entry in the
          response
        */
	s_op->resp.u.rmdirent.entry_handle =
            s_op->u.rmdirent.entry_handle;
	gossip_debug(GOSSIP_SERVER_DEBUG,
		     "  succeeded; returning handle %llu in response\n",
		     llu(s_op->resp.u.rmdirent.entry_handle));
    }
    else
    {
	gossip_debug(GOSSIP_SERVER_DEBUG, "  sending error response\n");
    }

    /* NOTE: we _intentionally_ leave the error_code field the way that
     * we found it, so that later states can use it to set the resp.status
     * field.
     */
    return SM_ACTION_COMPLETE;
}

static PINT_sm_action rmdirent_cleanup(
        struct PINT_smcb *smcb, job_status_s *js_p)
{
    struct PINT_server_op *s_op = PINT_sm_frame(smcb, PINT_FRAME_CURRENT);
    
    PINT_free_object_attr(&s_op->attr);
    return(server_state_machine_complete(smcb));
}

static int perm_rmdirent(PINT_server_op *s_op)
{
    int ret;

    if (s_op->req->capability.op_mask & PINT_CAP_WRITE &&
        s_op->req->capability.op_mask & PINT_CAP_EXEC)
    {
        ret = 0;
    }
    else
    {
        ret = -PVFS_EACCES;
    }

    return ret;
}

PINT_GET_OBJECT_REF_DEFINE(rmdirent);

struct PINT_server_req_params pvfs2_rmdirent_params =
{
    .string_name = "rmdirent",
    .perm = perm_rmdirent,
    .access_type = PINT_server_req_modify,
    .sched_policy = PINT_SERVER_REQ_SCHEDULE,
    .get_object_ref = PINT_get_object_ref_rmdirent,
    .state_machine = &pvfs2_rmdirent_sm
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
