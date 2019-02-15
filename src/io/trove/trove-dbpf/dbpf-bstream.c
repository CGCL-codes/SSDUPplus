/*
 * (C) 2001 Clemson University and The University of Chicago
 *
 * See COPYING in top-level directory.
 */

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <assert.h>
#include <errno.h>
#include <pthread.h>

#include "gossip.h"
#include "pvfs2-debug.h"
#include "trove.h"
#include "trove-internal.h"
#include "dbpf.h"
#include "dbpf-op-queue.h"
#include "dbpf-bstream.h"
#include "dbpf-attr-cache.h"
#include "pint-event.h"
#include "dbpf-open-cache.h"
#include "dbpf-sync.h"

#include "dbpf-alt-aio.h"

#include "my_avltree.h"
#include "../../../server/pvfs2-server.h"
#include <dirent.h>
#include <time.h>  
#include <sys/time.h>
#include <math.h>
static long avl_cost = 0;
static long group_cost = 0;
static long array_cost = 0;
#define false 0
#define true 1
typedef int bool;

typedef struct ssd_cache_fd
{
    int fd;
    int ori_fd;
    int is_open;
    int write_back_fd;
    char cache[10];
    struct ssd_cache_fd * next;
} ssd_cache_fd;
typedef struct write_back_use
{
    int fd;
    int ssd_fd;
    struct write_back_use *next;
    char cache[10];
} write_back_use;
extern gen_mutex_t dbpf_attr_cache_mutex;
long long newstructcost = 0;
#define AIOCB_ARRAY_SZ 64

#define times 64
//#define GETOFFSET 1
#define USINGSSD 1
//#define GETRF 1
#define ONLYHDD 1
//#define ONLYSSD 1
//#define BB 1
//#define DIRECTFLUSH 1
//#define RFOFSINGLEJOB 1
#define QOS 1
extern int TROVE_max_concurrent_io;
static int s_dbpf_ios_in_progress = 0;

struct offset_strct{
	int64_t offset;
	int64_t size;
};

struct offsetnode //record voffset fd size of request 
{
        int fd;
        int count;
        long long offset[128];  
        struct offsetnode *next;
        int size;
};
struct offsetlist //record request of multi file
{
        int count;
        int fd_num;
        struct offsetnode* offset_fd;
}offset_list;

struct offset_strct offset_array[128];
int64_t size_diff[128];
struct offset_strct offset_ori[128];
int k_ori = 0;
int notzero = 0;
int i_tt = 0;
int is_write_ssd = 0;
volatile int is_write_hhd = 1;
volatile int hdd_overhead_low = 0;
int flush_done = 0;
volatile int ssd1_full = 0;
volatile int ssd2_full = 0;
float percent=0.0;
int is_ssd = 0;
int is_hhd = 0;
int total_is_ssd = 0;
int total_is_hhd = 0;
int ret_array[1024];
off_t avl_offset_array[204800];
off_t avl_offset_array_ssd[204800];
char avl_cachename[204800][10];
TROVE_size avl_size_array[204800];
int avl_index = 0;
int merge = 0;
TROVE_size wb_total=0;
TROVE_size avl_total_size=0;

static int _offset_count = 0;

int per_list_len = 0;
float per,temp_per;
volatile float level=0.5;
float percent_list[100000];
int mod_flag=0;
long long int ssdsizesum=0;
int is_hdd=0;
static int in_dead_lock=0;
pthread_mutex_t noderlock_mutex;
int init_mutexlock=0;
struct timeval avl_start, avl_end;
struct timeval group_start, group_end;
struct timeval open_start, open_end;
struct timeval write_start, write_end;
unsigned long avl_diff=0, group_diff=0, open_diff=0, write_diff=0;
float rate,total_rate;
float percsum=0.0,percavg;
int clearflag=0;
float clearcount=0.0,clearperlistcount=0.0;
#ifdef BB
volatile int bb_full = 0;
#endif
#ifdef QOS
int writehddfd = 0;
int writessdfd = 0;
#endif
/********************************/
static dbpf_op_queue_p s_dbpf_io_ready_queue = NULL;
static gen_mutex_t s_dbpf_io_mutex = GEN_MUTEX_INITIALIZER;
static gen_mutex_t dbpf_update_size_lock = GEN_MUTEX_INITIALIZER;


static gen_mutex_t dbpf_freeblock_lock = GEN_MUTEX_INITIALIZER;
static gen_cond_t dbpf_cond = GEN_COND_INITIALIZER;

static int block_num=1;
static int flush_num=1;

static int free_block_total=2;
static char to_buffer_region[PATH_MAX] = {0};
static TROVE_size r_threshold = 0;
static TROVE_size b_threshold = 0;
static int rs_length = 0;
static float high_w = 0.0;
static float low_w = 0.0;
static TROVE_size size_p=0, size_p1=0, size_p2=0;

static bool is_writeback_exist =false;
static gen_mutex_t  dbpf_writeback_thread = GEN_MUTEX_INITIALIZER;

static gen_mutex_t  dbpf_rdwr_lock = GEN_MUTEX_INITIALIZER;

static bool full_flag1 = false;
static bool full_flag2 = false;
static bool cr_mod = false;
static gen_mutex_t dbpf_update_offset_map_lock = GEN_MUTEX_INITIALIZER;

static struct mavlnode *nodew= NULL;
static struct mavlnode *noder= NULL;
volatile int noderlock = 0;
static int is_reading= 0;


static ssd_cache_fd *ssd_fd =NULL, *link_ssd_fd = NULL, *link_ssd_fd_1 = NULL, *link_ssd_fd_2 = NULL; 
//static int write_count = 0;
static void mylog(char *message);
static int write_back(void *fd);
static void in_order_travel_test(struct mavlnode *n);
float randomfactor(struct offsetlist *offset_list);
void print(struct offsetlist *offset_list);
void clear(struct offsetlist *offset_list);
void insert(struct offsetlist *offset_list, long long offset, int fd, int size);
void shell_insert(long long array[], int size);
void *get_io_occupy();
int cost = 0;

int partition(int arra[], int arrb[],int arrc[], int low, int high){
    int keya, keyb, keyc;
    keya = arra[low];
    keyb = arrb[low];
    keyc = arrc[low];
    while(low<high){
        while(low <high && arra[high]>= keya )
            high--;
        if(low<high)
        {
            arra[low++] = arra[high];
            arrb[low-1] = arrb[high];
	    arrc[low-1] = arrc[high];
        }
        while( low<high && arra[low]<=keya )
            low++;
        if(low<high)
        {
            arra[high--] = arra[low];
            arrb[high+1] = arrb[low];
	    arrc[high+1] = arrc[low];
        }
    }
    arra[low] = keya;
    arrb[low] = keyb;
    arrc[low] = keyc;
    return low;
}

void quick_sort(int arra[], int arrb[], int arrc[],int start, int end){
    int pos;
    if (start<end){
        pos = partition(arra, arrb,arrc, start, end);
        quick_sort(arra,arrb,arrc,start,pos-1);
        quick_sort(arra,arrb,arrc,pos+1,end);
    }
    return;
}

static int open_fd_ssd(
    ssd_cache_fd** ssd_fd ,ssd_cache_fd** link_ssd_fd,  TROVE_offset *filesize,
    enum open_cache_open_type,
    TROVE_handle handle,
    struct aiocb **aiocb_ptr_array,
    int write_back_fd);

static size_t direct_aligned_write_ssd(int fd, 
                                    void *buf,
                                    off_t buf_offset,
                                    size_t size,
                                    off_t write_offset);
static void  write_ssd(int fd,
			TROVE_handle handle,
			struct aiocb **aiocbp,
			int aio_count,
			TROVE_size file_size);

static void aio_progress_notification_ssd(dbpf_queued_op_t *cur_op);
static struct dbpf_aio_ops aio_ops;

static int issue_or_delay_io_operation(
    dbpf_queued_op_t *cur_op, struct aiocb **aiocb_ptr_array,
    int aiocb_inuse_count, struct sigevent *sig, int dec_first);
static void start_delayed_ops_if_any(int dec_first);

#ifdef __PVFS2_TROVE_AIO_THREADED__
static char *list_proc_state_strings[] __attribute__((unused)) = {
    "LIST_PROC_INITIALIZED",
    "LIST_PROC_INPROGRESS ",
    "LIST_PROC_ALLCONVERTED",
    "LIST_PROC_ALLPOSTED",
};
#endif

#ifndef __PVFS2_TROVE_AIO_THREADED__
static int dbpf_bstream_rw_list_op_svc(struct dbpf_op *op_p);
#endif
static int dbpf_bstream_flush_op_svc(struct dbpf_op *op_p);

#ifdef __PVFS2_TROVE_AIO_THREADED__
#include "dbpf-thread.h"
#include "pvfs2-internal.h"

extern pthread_cond_t dbpf_op_completed_cond;
extern dbpf_op_queue_p dbpf_completion_queue_array[TROVE_MAX_CONTEXTS];
extern gen_mutex_t dbpf_completion_queue_array_mutex[TROVE_MAX_CONTEXTS];


static int open_fd_ssd(
    ssd_cache_fd **ssd_fd, ssd_cache_fd** link_ssd_fd, TROVE_offset *file_size,
    enum open_cache_open_type type,
    TROVE_handle handle,
    struct aiocb **aiocb_ptr_array,
    int write_back_fd)
{
    ssd_cache_fd * start_ssd_fd, *end_ssd_fd, *insert_ssd_fd;
    int flags = 0;
    int mode = 0;
    char filename[PATH_MAX] = {0};
    struct stat statbuf;
    char realname[20],cache[20]="/cache-";
    sprintf(realname,"%d",aiocb_ptr_array[0]->aio_fildes); 
    //snprintf(realname,256,"%s",realname);
    //gossip_err("%s\n",realname);
    strcat(cache,realname);
    if(strlen(to_buffer_region)==0)snprintf(to_buffer_region,PATH_MAX,my_wb_info.buff_path_1);
    snprintf(filename,PATH_MAX,to_buffer_region);
    strcat(filename,cache);

    flags = O_RDWR;

    if(type == DBPF_FD_BUFFERED_WRITE ||
       type == DBPF_FD_DIRECT_WRITE)
    {
        flags |= O_CREAT;
        mode = TROVE_FD_MODE;
    }
    if(*link_ssd_fd==NULL)
    {
        *link_ssd_fd = (ssd_cache_fd *)malloc(sizeof(ssd_cache_fd));
        *ssd_fd = (ssd_cache_fd *)malloc(sizeof(ssd_cache_fd));
        if((*link_ssd_fd) == NULL){ gossip_err("link_ssd_fd is not created!\n");}
        (*link_ssd_fd)->ori_fd = -1;
        (*link_ssd_fd)->fd = -1;
        (*link_ssd_fd)->is_open = 0;
        (*link_ssd_fd)->write_back_fd= -1;
	//(*link_ssd_fd)->cache = "\0";
        (*link_ssd_fd)->next = NULL;
        
    }
    for(start_ssd_fd=*link_ssd_fd;start_ssd_fd!=NULL;start_ssd_fd=start_ssd_fd->next)
    {
        if(aiocb_ptr_array[0]->aio_fildes == start_ssd_fd->ori_fd)
        {
            (*ssd_fd)->fd = start_ssd_fd->fd;
            (*ssd_fd)->ori_fd = start_ssd_fd->ori_fd;
            (*ssd_fd)->is_open = start_ssd_fd->is_open;
            (*ssd_fd)->write_back_fd= start_ssd_fd->write_back_fd;
	    strcpy((*ssd_fd)->cache,start_ssd_fd->cache);
            (*ssd_fd)->next = NULL;
            break;
        }
        if(start_ssd_fd->next==NULL)
            end_ssd_fd = start_ssd_fd;
    }
    if(start_ssd_fd==NULL)
    {
        insert_ssd_fd = (ssd_cache_fd *)malloc(sizeof(ssd_cache_fd));
        insert_ssd_fd->fd = DBPF_OPEN(filename, flags, mode);
        //gossip_err("filename = %s %d\n",filename, insert_ssd_fd->fd);
        insert_ssd_fd->is_open = 1;
        insert_ssd_fd->ori_fd = aiocb_ptr_array[0]->aio_fildes;
        insert_ssd_fd->write_back_fd= write_back_fd;
	strcpy(insert_ssd_fd->cache,cache);
        insert_ssd_fd->next = NULL;
        //gossip_err("1\n");
        end_ssd_fd->next = insert_ssd_fd;
        //gossip_err("2\n");
        (*ssd_fd)->fd = insert_ssd_fd->fd;
        (*ssd_fd)->ori_fd = insert_ssd_fd->ori_fd;
        (*ssd_fd)->is_open = insert_ssd_fd->is_open;
        (*ssd_fd)->write_back_fd= write_back_fd;
	strcpy((*ssd_fd)->cache,cache);
        (*ssd_fd)->next = NULL;
        //gossip_err("in here\n"); 
        
    }
/*
	if(*ssd_fd==NULL)
	{
	    *ssd_fd = (ssd_cache_fd *)malloc(sizeof(ssd_cache_fd));
		(*ssd_fd)->fd = -1;
		(*ssd_fd)->is_open = 0;
    }
	if((*ssd_fd) == NULL){ gossip_err("ssd_fd is not created!\n");}
	if((*ssd_fd)->is_open !=1)
	{
         (*ssd_fd)->fd = DBPF_OPEN(filename, flags, mode);
		 (*ssd_fd)->is_open = 1;
	}
*/
    if(stat(filename,&statbuf)!=-1) *file_size = (TROVE_size)statbuf.st_size;
    return (((*ssd_fd)->fd < 0) ? -trove_errno_to_trove_error(errno) : 0);
}


static size_t direct_aligned_write_ssd(int fd, 
                                    void *buf, 
                                    off_t buf_offset,
                                    size_t size, 
                                    off_t write_offset)
{
    int ret;

	ret = dbpf_pwrite_ssd(fd, (((char *)buf) + buf_offset), size,write_offset);
    if(ret < 0)
    {
        gossip_err("dbpf_direct_write: failed to perform aligned write\n");
        return ret;
    }

    return ret;
}

static void write_ssd(int fd,
                            TROVE_handle handle,
                            struct aiocb **aiocb_ptr_array,
                            int aiocb_inuse_count,
			                 TROVE_size file_size
                    )
{
    //write_count++;
    //gossip_err("write_count=%d\n",write_count);
    //gossip_err("fd in write_ssd = %d, aiocb_ptr[0].fd = %d\n", fd, aiocb_ptr_array[0]->aio_fildes);
    int j=0,ret=0;
    gettimeofday(&write_start,NULL);
    for(j=0;j<aiocb_inuse_count;j++)
    {
        size_p += aiocb_ptr_array[j]->aio_nbytes/1024;

        //if(j!=0){ret = open_fd_ssd(&ssd_fd,&link_ssd_fd,&file_size,DBPF_FD_DIRECT_WRITE,handle,aiocb_ptr_array);}
        if(aiocb_ptr_array[j]->aio_nbytes==0)continue;
        int write_ret;
        Offset_pair_t d;
        u_int32_t flags;
        char db_name[PATH_MAX] = {0};
        char db_handle[PATH_MAX] = {0};
        char message[100] = {0};
        struct stat statbuf;
        FILE *avl;
        enum MAVLRES tmp;
        gen_mutex_lock(&dbpf_update_offset_map_lock);
        gettimeofday(&avl_start,NULL);
        file_size+=128;
        d = (Offset_pair_t)malloc(sizeof(struct Offset_pair));
        d->access_size = (TROVE_size)aiocb_ptr_array[j]->aio_nbytes;
        d->original_offset = (TROVE_offset)aiocb_ptr_array[j]->aio_offset;
        d->new_offset = file_size;
        //d->new_offset = (TROVE_offset)aiocb_ptr_array[j]->aio_offset;
	strcmp(d->cache,ssd_fd->cache);
    	tmp = mavlinsert(&nodew,d);
        gettimeofday(&avl_end,NULL);
        avl_diff+=1000000*(avl_end.tv_sec-avl_start.tv_sec)+avl_end.tv_usec-avl_start.tv_usec;
        ssdsizesum+=aiocb_ptr_array[j]->aio_nbytes;
        gen_mutex_unlock(&dbpf_update_offset_map_lock);
        //gettimeofday(&write_end,NULL);
        //write_diff+=1000000*(write_end.tv_sec-write_start.tv_sec)+write_end.tv_usec-write_start.tv_usec;
        //gossip_err("write cost = %ld\n", write_diff);
        write_ret = direct_aligned_write_ssd(
            fd, aiocb_ptr_array[j]->aio_buf, 0, aiocb_ptr_array[j]->aio_nbytes,file_size);//aiocb_ptr_array[j]->aio_offset);
            ret_array[j] = write_ret;
    }
}

static void aio_progress_notification_ssd(dbpf_queued_op_t *cur_op)
{
    struct dbpf_op *op_p = NULL;
    int  i, aiocb_inuse_count, state = 0;
    struct aiocb *aiocb_p = NULL, *aiocb_ptr_array[AIOCB_ARRAY_SZ] = {0};
    //struct aiocb  *aiocb_ptr_array[AIOCB_ARRAY_SZ] = {0};
    PVFS_size eor = -1;
    int j;
    TROVE_ds_attributes attr;
    TROVE_object_ref ref;
    int sync_required = 0;
    int ret =0;

    assert(cur_op);

    op_p = &cur_op->op;
    assert(op_p);

    aiocb_p = op_p->u.b_rw_list.aiocb_array;
    gen_mutex_lock(&cur_op->mutex);
    state = cur_op->op.state;
    //state = OP_COMPLETED;
    gen_mutex_unlock(&cur_op->mutex);
    assert(state != OP_COMPLETED);
    for (i = 0; i < op_p->u.b_rw_list.aiocb_array_count; i++)
    {
        if (aiocb_p[i].aio_lio_opcode == LIO_NOP)
        {
            continue;
        }

        /* aio_error gets the "errno" value of the individual op */
        ret = ret_array[i];
        //ret = op_p->u.b_rw_list.aio_ops->aio_error(&aiocb_p[i]);
        if (ret == aiocb_p[i].aio_nbytes)
        {
            /* aio_return gets the return value of the individual op */
            *(op_p->u.b_rw_list.out_size_p) += ret;

            aiocb_p[i].aio_lio_opcode = LIO_NOP;
        }
        else
        {
            ret = -trove_errno_to_trove_error(ret);
            goto final_threaded_aio_cleanup;
        }
    }
   if (op_p->u.b_rw_list.list_proc_state == LIST_PROC_ALLPOSTED)
    {
        ret = 0;

      final_threaded_aio_cleanup:

        if ((op_p->type == BSTREAM_WRITE_AT) ||
            (op_p->type == BSTREAM_WRITE_LIST))
        {

            DBPF_AIO_SYNC_IF_NECESSARY(
                op_p, op_p->u.b_rw_list.fd, ret);
            for(j=0; j<op_p->u.b_rw_list.stream_array_count; j++)
            {
                if(eor < op_p->u.b_rw_list.stream_offset_array[j] +
                    op_p->u.b_rw_list.stream_size_array[j])
                {
                    eor = op_p->u.b_rw_list.stream_offset_array[j] +
                        op_p->u.b_rw_list.stream_size_array[j];
                }
            }

            ref.fs_id = op_p->coll_p->coll_id;
            ref.handle = op_p->handle;

            gen_mutex_lock(&dbpf_update_size_lock);
            ret = dbpf_dspace_attr_get(op_p->coll_p, ref, &attr);
            if(ret != 0)
            {
                gen_mutex_unlock(&dbpf_update_size_lock);
                goto error_in_cleanup;
            }

            if(eor > attr.u.datafile.b_size)
            {
                attr.u.datafile.b_size = eor;
                ret = dbpf_dspace_attr_set(op_p->coll_p, ref, &attr);
                if(ret != 0)
                {
                    gen_mutex_unlock(&dbpf_update_size_lock);
                    goto error_in_cleanup;
                }
                if(op_p->flags & TROVE_SYNC)
                {
                    sync_required = 1;
                }
            }
            gen_mutex_unlock(&dbpf_update_size_lock);
        }

error_in_cleanup:

        dbpf_open_cache_put(&op_p->u.b_rw_list.open_ref);
        op_p->u.b_rw_list.fd = -1;

                //cur_op->state = ret;
                cur_op->state = 0;
        /* this is a macro defined in dbpf-thread.h */

        if(sync_required)
        {
            int outcount;
           
            free(cur_op->op.u.b_rw_list.aiocb_array);
            cur_op->op.u.b_rw_list.aiocb_array = NULL;

            dbpf_queued_op_init(cur_op,
                                DSPACE_SETATTR,
                                ref.handle,
                                cur_op->op.coll_p,
                                dbpf_dspace_setattr_op_svc,
                                cur_op->op.user_ptr,
                                TROVE_SYNC,
                                cur_op->op.context_id);
            cur_op->op.state = OP_IN_SERVICE;
            dbpf_sync_coalesce(cur_op, 0, &outcount);
        }
        else
        {
            dbpf_queued_op_complete(cur_op, OP_COMPLETED);
        }

        //start_delayed_ops_if_any(1);
    }

}

static void aio_progress_notification(union sigval sig)
{
    dbpf_queued_op_t *cur_op = NULL;
    struct dbpf_op *op_p = NULL;
    int ret, i, aiocb_inuse_count, state = 0;
    struct aiocb *aiocb_p = NULL, *aiocb_ptr_array[AIOCB_ARRAY_SZ] = {0};
    PVFS_size eor = -1;
    int j;
    TROVE_ds_attributes attr;
    TROVE_object_ref ref;
    int sync_required = 0;

    cur_op = (dbpf_queued_op_t *)sig.sival_ptr;
    assert(cur_op);

    op_p = &cur_op->op;
    assert(op_p);

    gossip_debug(
        GOSSIP_TROVE_DEBUG," --- aio_progress_notification called "
        "with handle %llu (%p)\n", llu(op_p->handle), cur_op);

    aiocb_p = op_p->u.b_rw_list.aiocb_array;
    assert(aiocb_p);

    gen_mutex_lock(&cur_op->mutex);
    state = cur_op->op.state;
    gen_mutex_unlock(&cur_op->mutex);

    assert(state != OP_COMPLETED);

    /*
      we should iterate through the ops here to determine the
      error/return value of the op based on individual request
      error/return values.  they're ignored for now, however.
    */
    for (i = 0; i < op_p->u.b_rw_list.aiocb_array_count; i++)
    {
        if (aiocb_p[i].aio_lio_opcode == LIO_NOP)
        {
            continue;
        }

        /* aio_error gets the "errno" value of the individual op */
        ret = op_p->u.b_rw_list.aio_ops->aio_error(&aiocb_p[i]);
        if (ret == 0)
        {
            /* aio_return gets the return value of the individual op */
            ret = op_p->u.b_rw_list.aio_ops->aio_return(&aiocb_p[i]);

            gossip_debug(GOSSIP_TROVE_DEBUG, "%s: %s complete: "
                         "aio_return() says %d [fd = %d]\n",
                         __func__, dbpf_op_type_to_str(op_p->type),
                         ret, op_p->u.b_rw_list.fd);

            *(op_p->u.b_rw_list.out_size_p) += ret;

            /* mark as a NOP so we ignore it from now on */
            aiocb_p[i].aio_lio_opcode = LIO_NOP;
        }
        else
        {
            gossip_debug(GOSSIP_TROVE_DEBUG, "error %d (%s) from "
                         "aio_error/aio_return on block %d; "
                         "skipping\n", ret, strerror(ret), i);

            ret = -trove_errno_to_trove_error(ret);
            goto final_threaded_aio_cleanup;
        }
    }

    if (op_p->u.b_rw_list.list_proc_state == LIST_PROC_ALLPOSTED)
    {
        ret = 0;

      final_threaded_aio_cleanup:

        if ((op_p->type == BSTREAM_WRITE_AT) ||
            (op_p->type == BSTREAM_WRITE_LIST))
        {
            DBPF_AIO_SYNC_IF_NECESSARY(
                op_p, op_p->u.b_rw_list.fd, ret);

            /* TODO: need similar logic for non-threaded aio case too */

            /* calculate end of request */
            for(j=0; j<op_p->u.b_rw_list.stream_array_count; j++)
            {
                if(eor < op_p->u.b_rw_list.stream_offset_array[j] + 
                    op_p->u.b_rw_list.stream_size_array[j])
                {
                    eor = op_p->u.b_rw_list.stream_offset_array[j] + 
                        op_p->u.b_rw_list.stream_size_array[j];
                }
            }

            ref.fs_id = op_p->coll_p->coll_id;
            ref.handle = op_p->handle;

            gen_mutex_lock(&dbpf_update_size_lock);
            ret = dbpf_dspace_attr_get(op_p->coll_p, ref, &attr);
            if(ret != 0)
            {
                gen_mutex_unlock(&dbpf_update_size_lock);
                goto error_in_cleanup;
            }

            if(eor > attr.u.datafile.b_size)
            {
                /* set the size of the file */
                attr.u.datafile.b_size = eor;
                ret = dbpf_dspace_attr_set(op_p->coll_p, ref, &attr);
                if(ret != 0)
                {
                    gen_mutex_unlock(&dbpf_update_size_lock);
                    goto error_in_cleanup;
                }
                if(op_p->flags & TROVE_SYNC)
                {
                    sync_required = 1;
                }
            }
            gen_mutex_unlock(&dbpf_update_size_lock);
        }

error_in_cleanup:

        dbpf_open_cache_put(&op_p->u.b_rw_list.open_ref);
        op_p->u.b_rw_list.fd = -1;
        
	cur_op->state = ret;
        /* this is a macro defined in dbpf-thread.h */

        if(sync_required)
        {
            int outcount;

            gossip_debug(GOSSIP_TROVE_DEBUG, 
                "aio updating size for handle %llu\n", llu(ref.handle));

            /* If we updated the size, then convert cur_op into a setattr.
             * Note that we are not actually going to perform a setattr.
             * We just want the coalescing path to treat it like a setattr
             * so that the size update is synced before we complete.
             */

            /* We need to free the aiocb_array in this case, since the
             * dbpf_queued_op_free function won't know to do that anymore
             */
            free(cur_op->op.u.b_rw_list.aiocb_array);
            cur_op->op.u.b_rw_list.aiocb_array = NULL;

            dbpf_queued_op_init(cur_op,
                                DSPACE_SETATTR,
                                ref.handle,
                                cur_op->op.coll_p,
                                dbpf_dspace_setattr_op_svc,
                                cur_op->op.user_ptr,
                                TROVE_SYNC,
                                cur_op->op.context_id);
            cur_op->op.state = OP_IN_SERVICE;
            dbpf_sync_coalesce(cur_op, 0, &outcount);
        }
        else
        {
            dbpf_queued_op_complete(cur_op, OP_COMPLETED);
        }

        /* if sync is not required, then dbpf_queued_op_complete executes and will issue a cond_signal. if
         * the signal'd thread executes before the following gossip_debug statement, then cur_op is un-
         * defined, causing the gossip_debug statement to seg fault.  So, we check for existence first!
        */

        /* if the signal'd thread executes op_p can also go away
         * causing the list_proc_state access to segfault. there isn't really
         * much debugging information to be had by accessing cur_op or
         * op_p, the key is that delayed ops are starting. */
        gossip_debug(GOSSIP_TROVE_DEBUG,"*** starting delayed ops if any.\n");

        start_delayed_ops_if_any(1);
    }
    else
    {
        gossip_debug(GOSSIP_TROVE_DEBUG, "*** issuing more aio requests "
                     "(state is %s)\n", 
                     list_proc_state_strings[
                     op_p->u.b_rw_list.list_proc_state]);

        /* no operations in progress; convert and post some more */
        op_p->u.b_rw_list.aiocb_array_count = AIOCB_ARRAY_SZ;
        op_p->u.b_rw_list.aiocb_array = aiocb_p;

        /* convert listio arguments into aiocb structures */
        aiocb_inuse_count = op_p->u.b_rw_list.aiocb_array_count;
        ret = dbpf_bstream_listio_convert(
            op_p->u.b_rw_list.fd,
            op_p->u.b_rw_list.opcode,
            op_p->u.b_rw_list.mem_offset_array,
            op_p->u.b_rw_list.mem_size_array,
            op_p->u.b_rw_list.mem_array_count,
            op_p->u.b_rw_list.stream_offset_array,
            op_p->u.b_rw_list.stream_size_array,
            op_p->u.b_rw_list.stream_array_count,
            aiocb_p,
            &aiocb_inuse_count,
            &op_p->u.b_rw_list.lio_state);

        if (ret == 1)
        {
            op_p->u.b_rw_list.list_proc_state = LIST_PROC_ALLCONVERTED;
        }

        op_p->u.b_rw_list.sigev.sigev_notify = SIGEV_THREAD;
        op_p->u.b_rw_list.sigev.sigev_notify_attributes = NULL;
        op_p->u.b_rw_list.sigev.sigev_notify_function =
            aio_progress_notification;
        op_p->u.b_rw_list.sigev.sigev_value.sival_ptr = (void *)cur_op;

        /* mark the unused with LIO_NOPs */
        for(i = aiocb_inuse_count;
            i < op_p->u.b_rw_list.aiocb_array_count; i++)
        {
            /* mark these as NOPs and we'll ignore them */
            aiocb_p[i].aio_lio_opcode = LIO_NOP;
        }

        for(i = 0; i < aiocb_inuse_count; i++)
        {
            aiocb_ptr_array[i] = &aiocb_p[i];
        }

        assert(cur_op == op_p->u.b_rw_list.sigev.sigev_value.sival_ptr);

        if (op_p->u.b_rw_list.list_proc_state == LIST_PROC_ALLCONVERTED)
        {
            op_p->u.b_rw_list.list_proc_state = LIST_PROC_ALLPOSTED;
        }

        ret = issue_or_delay_io_operation(
            cur_op, aiocb_ptr_array, aiocb_inuse_count,
            &op_p->u.b_rw_list.sigev, 1);

        if (ret)
        {
            gossip_lerr("issue_or_delay_io_operation() returned "
                        "%d\n", ret);
        }
    }
}
#endif /* __PVFS2_TROVE_AIO_THREADED__ */

static void start_delayed_ops_if_any(int dec_first)
{
    int ret = 0;
    dbpf_queued_op_t *cur_op = NULL;
    int i = 0, aiocb_inuse_count = 0;
    struct aiocb *aiocbs = NULL, *aiocb_ptr_array[AIOCB_ARRAY_SZ] = {0};

    gen_mutex_lock(&s_dbpf_io_mutex);
    if (dec_first)
    {
        s_dbpf_ios_in_progress--;
    }
    gossip_debug(GOSSIP_TROVE_DEBUG, "DBPF I/O ops in progress: %d\n",
        s_dbpf_ios_in_progress);

    if (s_dbpf_io_ready_queue == NULL)
    {
        s_dbpf_io_ready_queue = dbpf_op_queue_new();
    }
    assert(s_dbpf_io_ready_queue);

    if (!dbpf_op_queue_empty(s_dbpf_io_ready_queue))
    {
        cur_op = dbpf_op_queue_shownext(s_dbpf_io_ready_queue);
        assert(cur_op);
#ifndef __PVFS2_TROVE_AIO_THREADED__
        assert(cur_op->op.state == OP_INTERNALLY_DELAYED);
#endif
        assert((cur_op->op.type == BSTREAM_READ_AT) ||
               (cur_op->op.type == BSTREAM_READ_LIST) ||
               (cur_op->op.type == BSTREAM_WRITE_AT) ||
               (cur_op->op.type == BSTREAM_WRITE_LIST));
        dbpf_op_queue_remove(cur_op);

        gossip_debug(GOSSIP_TROVE_DEBUG, "starting delayed I/O "
                     "operation %p (%d in progress)\n", cur_op,
                     s_dbpf_ios_in_progress);

        aiocbs = cur_op->op.u.b_rw_list.aiocb_array;
        assert(aiocbs);

        for(i = 0; i < AIOCB_ARRAY_SZ; i++)
        {
            if (aiocbs[i].aio_lio_opcode != LIO_NOP)
            {
                aiocb_inuse_count++;
            }
        }

        for(i = 0; i < aiocb_inuse_count; i++)
        {
            aiocb_ptr_array[i] = &aiocbs[i];
        }

        if(gossip_debug_enabled(GOSSIP_TROVE_DEBUG))
        {

            gossip_debug(GOSSIP_TROVE_DEBUG,
                         "lio_listio called with %d following aiocbs:\n", 
                         aiocb_inuse_count);
            for(i=0; i<aiocb_inuse_count; i++)
            {
                gossip_debug(
                    GOSSIP_TROVE_DEBUG,
                    "aiocb_ptr_array[%d]: fd: %d, off: %lld, "
                    "bytes: %d, buf: %p, type: %d\n",
                    i, 
                    aiocb_ptr_array[i]->aio_fildes,
                    lld(aiocb_ptr_array[i]->aio_offset),
                    (int)aiocb_ptr_array[i]->aio_nbytes,
                    aiocb_ptr_array[i]->aio_buf,
                    (int)aiocb_ptr_array[i]->aio_lio_opcode);
            }
        }

        ret = cur_op->op.u.b_rw_list.aio_ops->lio_listio(
            LIO_NOWAIT, aiocb_ptr_array, aiocb_inuse_count,
            &cur_op->op.u.b_rw_list.sigev);

        if (ret != 0)
        {
            gossip_lerr("lio_listio() returned %d\n", ret);
            dbpf_open_cache_put(&cur_op->op.u.b_rw_list.open_ref);
            goto error_exit;
        }
        s_dbpf_ios_in_progress++;

        gossip_debug(GOSSIP_TROVE_DEBUG, "%s: lio_listio posted %p "
                     "(handle %llu, ret %d))\n", __func__, cur_op,
                     llu(cur_op->op.handle), ret);

#ifndef __PVFS2_TROVE_AIO_THREADED__
        /*
          to continue making progress on this previously delayed I/O
          operation, we need to re-add it back to the normal dbpf
          operation queue so that the calling thread can continue to
          call the service method (state flag is updated as well)
        */
        dbpf_queued_op_queue_nolock(cur_op);
#endif
    }
  error_exit:
    gen_mutex_unlock(&s_dbpf_io_mutex);
}

static int issue_or_delay_io_operation(
    dbpf_queued_op_t *cur_op, struct aiocb **aiocb_ptr_array,
    int aiocb_inuse_count, struct sigevent *sig, int dec_first)
{
    int ret = -TROVE_EINVAL, op_delayed = 0;
    int i;
    assert(cur_op);

    gen_mutex_lock(&s_dbpf_io_mutex);
    if (dec_first)
    {
        s_dbpf_ios_in_progress--;
    }
    if (s_dbpf_ios_in_progress < TROVE_max_concurrent_io)
    {
        s_dbpf_ios_in_progress++;
    }
    else
    {
        if (s_dbpf_io_ready_queue == NULL)
        {
            s_dbpf_io_ready_queue = dbpf_op_queue_new();
            if (!s_dbpf_io_ready_queue)
            {
                return -TROVE_ENOMEM;
            }
        }
        assert(s_dbpf_io_ready_queue);
        dbpf_op_queue_add(s_dbpf_io_ready_queue, cur_op);

        op_delayed = 1;
#ifndef __PVFS2_TROVE_AIO_THREADED__
        /*
          setting this state flag tells the caller not to re-add this
          operation to the normal dbpf-op queue because it will be
          started automatically (internally) on completion of other
          I/O operations
        */
        gen_mutex_lock(&cur_op->mutex);
        cur_op->op.state = OP_INTERNALLY_DELAYED;
        gen_mutex_unlock(&cur_op->mutex);
#endif

        gossip_debug(GOSSIP_TROVE_DEBUG, "delayed I/O operation %p "
                     "(%d already in progress)\n",
                     cur_op, s_dbpf_ios_in_progress);
    }

    gossip_debug(GOSSIP_TROVE_DEBUG, "DBPF I/O ops in progress: %d\n",
        s_dbpf_ios_in_progress);

    gen_mutex_unlock(&s_dbpf_io_mutex);

    if (!op_delayed)
    {
        if(gossip_debug_enabled(GOSSIP_TROVE_DEBUG))
        {

            gossip_debug(GOSSIP_TROVE_DEBUG,
                         "lio_listio called with the following aiocbs:\n");
            for(i=0; i<aiocb_inuse_count; i++)
            {
                gossip_debug(GOSSIP_TROVE_DEBUG,
                             "aiocb_ptr_array[%d]: fd: %d, "
                             "off: %lld, bytes: %d, buf: %p, type: %d\n",
                             i, aiocb_ptr_array[i]->aio_fildes,
                             lld(aiocb_ptr_array[i]->aio_offset),
                             (int)aiocb_ptr_array[i]->aio_nbytes,
                             aiocb_ptr_array[i]->aio_buf,
                             (int)aiocb_ptr_array[i]->aio_lio_opcode);
            }
        }

        ret = cur_op->op.u.b_rw_list.aio_ops->lio_listio(
            LIO_NOWAIT, aiocb_ptr_array,
            aiocb_inuse_count, sig);
        if (ret != 0)
        {
            s_dbpf_ios_in_progress--;
            gossip_lerr("lio_listio() returned %d\n", ret);
            dbpf_open_cache_put(&cur_op->op.u.b_rw_list.open_ref);
            return -trove_errno_to_trove_error(errno);
        }
        if ( cur_op )
        {
           gossip_debug(GOSSIP_TROVE_DEBUG, "%s: lio_listio posted %p "
                        "(handle %llu, ret %d)\n", __func__, cur_op,
                        llu(cur_op->op.handle), ret);
        }
    }
    return 0;
}

int dbpf_bstream_read_at(TROVE_coll_id coll_id,
                         TROVE_handle handle,
                         void *buffer,
                         TROVE_size *inout_size_p,
                         TROVE_offset offset,
                         TROVE_ds_flags flags,
                         TROVE_vtag_s *vtag,
                         void *user_ptr,
                         TROVE_context_id context_id,
                         TROVE_op_id *out_op_id_p,
                         PVFS_hint  hints)
{
    return -TROVE_ENOSYS;
}

int dbpf_bstream_write_at(TROVE_coll_id coll_id,
                          TROVE_handle handle,
                          void *buffer,
                          TROVE_size *inout_size_p,
                          TROVE_offset offset,
                          TROVE_ds_flags flags,
                          TROVE_vtag_s *vtag,
                          void *user_ptr,
                          TROVE_context_id context_id,
                          TROVE_op_id *out_op_id_p,
                          PVFS_hint  hints)
{
    return -TROVE_ENOSYS;
}

int dbpf_bstream_flush(TROVE_coll_id coll_id,
                       TROVE_handle handle,
                       TROVE_ds_flags flags,
                       void *user_ptr,
                       TROVE_context_id context_id,
                       TROVE_op_id *out_op_id_p,
                       PVFS_hint  hints)
{
    dbpf_queued_op_t *q_op_p = NULL;
    struct dbpf_collection *coll_p = NULL;

    coll_p = dbpf_collection_find_registered(coll_id);
    if (coll_p == NULL)
    {
        return -TROVE_EINVAL;
    }

    q_op_p = dbpf_queued_op_alloc();
    if (q_op_p == NULL)
    {
        return -TROVE_ENOMEM;
    }

    /* initialize all the common members */
    dbpf_queued_op_init(q_op_p,
                        BSTREAM_FLUSH,
                        handle,
                        coll_p,
                        dbpf_bstream_flush_op_svc,
                        user_ptr,
                        flags,
                        context_id);
    q_op_p->op.hints = hints;
    *out_op_id_p = dbpf_queued_op_queue(q_op_p);
    return 0;
}

/* returns 1 on completion, -TROVE_errno on error, 0 on not done */
static int dbpf_bstream_flush_op_svc(struct dbpf_op *op_p)
{
    int ret = -TROVE_EINVAL, got_fd = 0;
    struct open_cache_ref tmp_ref;

    ret = dbpf_open_cache_get(
        op_p->coll_p->coll_id, op_p->handle,
        DBPF_FD_BUFFERED_WRITE, &tmp_ref);
    if (ret < 0)
    {
        goto return_error;
    }
    got_fd = 1;

    ret = DBPF_SYNC(tmp_ref.fd);
    if (ret != 0)
    {
        ret = -trove_errno_to_trove_error(errno);
        goto return_error;
    }
    dbpf_open_cache_put(&tmp_ref);
    return 1;

return_error:
    if (got_fd)
    {
        dbpf_open_cache_put(&tmp_ref);
    }
    return ret;
}

int dbpf_bstream_validate(TROVE_coll_id coll_id,
                          TROVE_handle handle,
                          TROVE_ds_flags flags,
                          TROVE_vtag_s *vtag,
                          void *user_ptr,
                          TROVE_context_id context_id,
                          TROVE_op_id *out_op_id_p,
                          PVFS_hint  hints)
{
    return -TROVE_ENOSYS;
}

static int dbpf_bstream_read_list(TROVE_coll_id coll_id,
                                  TROVE_handle handle,
                                  char **mem_offset_array,
                                  TROVE_size *mem_size_array,
                                  int mem_count,
                                  TROVE_offset *stream_offset_array,
                                  TROVE_size *stream_size_array,
                                  int stream_count,
                                  TROVE_size *out_size_p,
                                  TROVE_ds_flags flags,
                                  TROVE_vtag_s *vtag,
                                  void *user_ptr,
                                  TROVE_context_id context_id,
                                  TROVE_op_id *out_op_id_p,
                                  PVFS_hint  hints)
{
    return dbpf_bstream_rw_list(coll_id,
                                handle,
                                mem_offset_array,
                                mem_size_array,
                                mem_count,
                                stream_offset_array,
                                stream_size_array,
                                stream_count,
                                out_size_p,
                                flags,
                                vtag,
                                user_ptr,
                                context_id,
                                out_op_id_p,
                                LIO_READ,
                                &aio_ops,
                                hints);
}

static int dbpf_bstream_write_list(TROVE_coll_id coll_id,
                                   TROVE_handle handle,
                                   char **mem_offset_array,
                                   TROVE_size *mem_size_array,
                                   int mem_count,
                                   TROVE_offset *stream_offset_array,
                                   TROVE_size *stream_size_array,
                                   int stream_count,
                                   TROVE_size *out_size_p,
                                   TROVE_ds_flags flags,
                                   TROVE_vtag_s *vtag,
                                   void *user_ptr,
                                   TROVE_context_id context_id,
                                   TROVE_op_id *out_op_id_p,
                                   PVFS_hint  hints)
{
    return dbpf_bstream_rw_list(coll_id,
                                handle,
                                mem_offset_array,
                                mem_size_array,
                                mem_count,
                                stream_offset_array,
                                stream_size_array,
                                stream_count,
                                out_size_p,
                                flags,
                                vtag,
                                user_ptr,
                                context_id,
                                out_op_id_p,
                                LIO_WRITE,
                                &aio_ops,
                                hints);
}

/* dbpf_bstream_rw_list()
 *
 * Handles queueing of both read and write list operations
 *
 * opcode parameter should be LIO_READ or LIO_WRITE
 */
inline int dbpf_bstream_rw_list(TROVE_coll_id coll_id,
                                TROVE_handle handle,
                                char **mem_offset_array,
                                TROVE_size *mem_size_array,
                                int mem_count,
                                TROVE_offset *stream_offset_array,
                                TROVE_size *stream_size_array,
                                int stream_count,
                                TROVE_size *out_size_p,
                                TROVE_ds_flags flags,
                                TROVE_vtag_s *vtag,
                                void *user_ptr,
                                TROVE_context_id context_id,
                                TROVE_op_id *out_op_id_p,
                                int opcode,
                                struct dbpf_aio_ops * aio_ops,
                                PVFS_hint  hints)
{
    int ret = -TROVE_EINVAL;
    int fake_ret;
    dbpf_queued_op_t *q_op_p = NULL;
    struct dbpf_collection *coll_p = NULL;
    enum dbpf_op_type tmp_type;
    PINT_event_type event_type;
    int i,j;
    PVFS_size count_mem;

	int fd_ssd;
	TROVE_size file_size;
	pthread_t tid;
	b_threshold = my_wb_info.buff_threshold;
    rs_length = my_wb_info.rs_length;
    high_w = my_wb_info.high_watermark;
    low_w = my_wb_info.low_watermark;
	char message[100] = {0};
	pthread_attr_t attr;

#ifdef __PVFS2_TROVE_AIO_THREADED__
    struct dbpf_op *op_p = NULL;
    int aiocb_inuse_count = 0;
    struct aiocb *aiocb_p = NULL, *aiocb_ptr_array[AIOCB_ARRAY_SZ] = {0};
#endif

    coll_p = dbpf_collection_find_registered(coll_id);
    if (coll_p == NULL)
    {
        return -TROVE_EINVAL;
    }

    q_op_p = dbpf_queued_op_alloc();
    if (q_op_p == NULL)
    {
        return -TROVE_ENOMEM;
    }

    if (opcode == LIO_READ)
    {
        tmp_type = BSTREAM_READ_LIST;
        event_type = trove_dbpf_read_event_id;
    }
    else
    {
        tmp_type = BSTREAM_WRITE_LIST;
        event_type = trove_dbpf_write_event_id;
    }

    /* initialize all the common members */
    dbpf_queued_op_init(q_op_p,
                        tmp_type,
                        handle,
                        coll_p,
#ifdef __PVFS2_TROVE_AIO_THREADED__
                        NULL,
#else
                        dbpf_bstream_rw_list_op_svc,
#endif
                        user_ptr,
                        flags,
                        context_id);

#if PINT_EVENT_ENABLED
    count_mem = 0;
    for(i = 0; i < mem_count; ++i)
    {
        count_mem += mem_size_array[i];
    }
#endif

    q_op_p->event_type = event_type;

    PINT_EVENT_START(event_type, dbpf_pid, NULL, &q_op_p->event_id,
                     PINT_HINT_GET_CLIENT_ID(hints),
                     PINT_HINT_GET_REQUEST_ID(hints),
                     PINT_HINT_GET_RANK(hints),
                     PINT_HINT_GET_HANDLE(hints),
                     handle,
                     PINT_HINT_GET_OP_ID(hints),
                     count_mem);

    if(gossip_debug_enabled(GOSSIP_TROVE_DEBUG))
    {
        PVFS_size count_stream = 0;
        count_mem = 0;
        gossip_debug(GOSSIP_TROVE_DEBUG, 
                     "dbpf_bstream_rw_list: mem_count: %d, stream_count: %d\n",
                     mem_count,
                     stream_count);
        for(i = 0; i < mem_count; ++i)
        {
            gossip_debug(
                GOSSIP_TROVE_DEBUG,
                "dbpf_bstream_rw_list: mem_offset: %p, mem_size: %Ld\n",
                mem_offset_array[i], lld(mem_size_array[i]));
            count_mem += mem_size_array[i];
        }

        for(i = 0; i < stream_count; ++i)
        {
            gossip_debug(GOSSIP_TROVE_DEBUG,
                         "dbpf_bstream_rw_list: "
                         "stream_offset: %Ld, stream_size: %Ld\n",
                         lld(stream_offset_array[i]), lld(stream_size_array[i]));
            count_stream += stream_size_array[i];
        }
        
        if(count_mem != count_stream)
        {
            gossip_debug(GOSSIP_TROVE_DEBUG,
                         "dbpf_bstream_rw_list: "
                         "mem_count: %Ld != stream_count: %Ld\n",
                         lld(count_mem), lld(count_stream));
        }
    }
                         
    /* initialize op-specific members */
    q_op_p->op.u.b_rw_list.fd = -1;
    q_op_p->op.u.b_rw_list.opcode = opcode;
    q_op_p->op.u.b_rw_list.mem_array_count = mem_count;
    q_op_p->op.u.b_rw_list.mem_offset_array = mem_offset_array;
    q_op_p->op.u.b_rw_list.mem_size_array = mem_size_array;
    q_op_p->op.u.b_rw_list.stream_array_count = stream_count;
    q_op_p->op.u.b_rw_list.stream_offset_array = stream_offset_array;
    q_op_p->op.u.b_rw_list.stream_size_array = stream_size_array;
    q_op_p->op.hints = hints;
    q_op_p->op.u.b_rw_list.aio_ops = aio_ops;

    /* initialize the out size to 0 */
    *out_size_p = 0;
    q_op_p->op.u.b_rw_list.out_size_p = out_size_p;
    q_op_p->op.u.b_rw_list.aiocb_array_count = 0;
    q_op_p->op.u.b_rw_list.aiocb_array = NULL;
#ifndef __PVFS2_TROVE_AIO_THREADED__
    q_op_p->op.u.b_rw_list.queued_op_ptr = (void *)q_op_p;
#endif

    /* initialize list processing state (more op-specific members) */
    q_op_p->op.u.b_rw_list.lio_state.mem_ct = 0;
    q_op_p->op.u.b_rw_list.lio_state.stream_ct = 0;
    q_op_p->op.u.b_rw_list.lio_state.cur_mem_size = mem_size_array[0];
    q_op_p->op.u.b_rw_list.lio_state.cur_mem_off = mem_offset_array[0];
    q_op_p->op.u.b_rw_list.lio_state.cur_stream_size =
        stream_size_array[0];
    q_op_p->op.u.b_rw_list.lio_state.cur_stream_off =
        stream_offset_array[0];

    q_op_p->op.u.b_rw_list.list_proc_state = LIST_PROC_INITIALIZED;

    ret = dbpf_open_cache_get(
        coll_id, handle, 
        (opcode == LIO_WRITE) ? DBPF_FD_BUFFERED_WRITE : DBPF_FD_BUFFERED_READ, 
        &q_op_p->op.u.b_rw_list.open_ref);
    if (ret < 0)
    {
        dbpf_queued_op_free(q_op_p);
        gossip_ldebug(GOSSIP_TROVE_DEBUG,
                      "warning: useless error value: %d\n", ret);
        return ret;
    }
    q_op_p->op.u.b_rw_list.fd = q_op_p->op.u.b_rw_list.open_ref.fd;

    /*
      if we're doing an i/o write, remove the cached attribute for
      this handle if it's present
    */
    if (opcode == LIO_WRITE)
    {
        TROVE_object_ref ref = {handle, coll_id};
        gen_mutex_lock(&dbpf_attr_cache_mutex);
        dbpf_attr_cache_remove(ref);
        gen_mutex_unlock(&dbpf_attr_cache_mutex);
    }

#ifndef __PVFS2_TROVE_AIO_THREADED__

    *out_op_id_p = dbpf_queued_op_queue(q_op_p);

#else
    op_p = &q_op_p->op;

    /*
      instead of queueing the op like most other trove operations,
      we're going to issue the system aio calls here to begin being
      serviced immediately.  We'll check progress in the
      aio_progress_notification callback method; this array is freed
      in dbpf-op.c:dbpf_queued_op_free
    */
    aiocb_p = (struct aiocb *)malloc(
        (AIOCB_ARRAY_SZ * sizeof(struct aiocb)));
    if (aiocb_p == NULL)
    {
        dbpf_open_cache_put(&q_op_p->op.u.b_rw_list.open_ref);
        return -TROVE_ENOMEM;
    }

    memset(aiocb_p, 0, (AIOCB_ARRAY_SZ * sizeof(struct aiocb)));
    for(i = 0; i < AIOCB_ARRAY_SZ; i++)
    {
        aiocb_p[i].aio_lio_opcode = LIO_NOP;
        aiocb_p[i].aio_sigevent.sigev_notify = SIGEV_NONE;
    }

    op_p->u.b_rw_list.aiocb_array_count = AIOCB_ARRAY_SZ;
    op_p->u.b_rw_list.aiocb_array = aiocb_p;
    op_p->u.b_rw_list.list_proc_state = LIST_PROC_INPROGRESS;

    /* convert listio arguments into aiocb structures */
    aiocb_inuse_count = op_p->u.b_rw_list.aiocb_array_count;
    ret = dbpf_bstream_listio_convert(
        op_p->u.b_rw_list.fd,
        op_p->u.b_rw_list.opcode,
        op_p->u.b_rw_list.mem_offset_array,
        op_p->u.b_rw_list.mem_size_array,
        op_p->u.b_rw_list.mem_array_count,
        op_p->u.b_rw_list.stream_offset_array,
        op_p->u.b_rw_list.stream_size_array,
        op_p->u.b_rw_list.stream_array_count,
        aiocb_p,
        &aiocb_inuse_count,
        &op_p->u.b_rw_list.lio_state);

    if (ret == 1)
    {
        op_p->u.b_rw_list.list_proc_state = LIST_PROC_ALLCONVERTED;
    }

    op_p->u.b_rw_list.sigev.sigev_notify = SIGEV_THREAD;
    op_p->u.b_rw_list.sigev.sigev_notify_attributes = NULL;
    op_p->u.b_rw_list.sigev.sigev_notify_function =
        aio_progress_notification;
    op_p->u.b_rw_list.sigev.sigev_value.sival_ptr = (void *)q_op_p;

    /* mark unused with LIO_NOPs */
    for(i = aiocb_inuse_count;
        i < op_p->u.b_rw_list.aiocb_array_count; i++)
    {
        aiocb_p[i].aio_lio_opcode = LIO_NOP;
    }

    for(i = 0; i < aiocb_inuse_count; i++)
    {
        aiocb_ptr_array[i] = &aiocb_p[i];
    }

    assert(q_op_p == op_p->u.b_rw_list.sigev.sigev_value.sival_ptr);

    if (op_p->u.b_rw_list.list_proc_state == LIST_PROC_ALLCONVERTED)
    {
        op_p->u.b_rw_list.list_proc_state = LIST_PROC_ALLPOSTED;
    }

    gen_mutex_lock(&q_op_p->mutex);
    q_op_p->op.state = OP_IN_SERVICE;
    gen_mutex_unlock(&q_op_p->mutex);

    id_gen_fast_register(&q_op_p->op.id, q_op_p);
    *out_op_id_p = q_op_p->op.id;
    //gossip_err("%llu %llu\n", size_p, b_threshold);
    if(size_p>b_threshold)
    {
#ifdef BB
        bb_full = 1;
#endif
	    
        gossip_err("noderlock=%d,in_deadlock=%d\n",noderlock,in_dead_lock);
        if(is_writeback_exist)
        {
            ;//is_hdd=1;
        }
        
        while(noderlock!=0)
        {
            
            in_dead_lock=1;// && is_hdd==0);// gossip_err("is in the deadlock!\n");
            gossip_err("now dead lock is 1\n");
            break;
        }
	    
	while(noderlock!=0);
        if(noderlock==0)
        {
            in_dead_lock=0;
            noder = nodew;
            noderlock = 1;
            nodew = NULL;

	    size_p = 0;
	    for(j=0;j<aiocb_inuse_count;j++)
	    {
		size_p += aiocb_ptr_array[j]->aio_nbytes/1024;
	    }
	    switch(block_num)
            {
                case 1:
                    full_flag1 = true;
                    block_num = 2;
		    link_ssd_fd = link_ssd_fd_2;
                    snprintf(to_buffer_region,PATH_MAX,my_wb_info.buff_path_2);
                    gossip_err("ssd_1 is full! new buffer is %s\n",to_buffer_region);
                    break;
                case 2:
                    full_flag2 = true;
                    block_num = 1;
		    link_ssd_fd = link_ssd_fd_1;
                    snprintf(to_buffer_region,PATH_MAX,my_wb_info.buff_path_1);
                    gossip_err("ssd_2 is full! new buffer is %s\n",to_buffer_region);
                    break;
            }
	}
            /*
	    */
    }

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    int i_t;
    if(!is_writeback_exist)
    {
            pthread_create(&tid,&attr,write_back,&(q_op_p->op.u.b_rw_list.open_ref.fd));
            pthread_attr_destroy(&attr);
            
    }
    //else gossip_err("w:write back exist!\n");

    //ret = open_fd_ssd(&ssd_fd,&link_ssd_fd,&file_size,DBPF_FD_DIRECT_WRITE,op_p->handle, aiocb_ptr_array,q_op_p->op.u.b_rw_list.open_ref.fd);
    if(block_num==1)
        ret = open_fd_ssd(&ssd_fd,&link_ssd_fd_1,&file_size,DBPF_FD_DIRECT_WRITE,op_p->handle, aiocb_ptr_array,q_op_p->op.u.b_rw_list.open_ref.fd);
    else
        ret = open_fd_ssd(&ssd_fd,&link_ssd_fd_2,&file_size,DBPF_FD_DIRECT_WRITE,op_p->handle, aiocb_ptr_array,q_op_p->op.u.b_rw_list.open_ref.fd);
    //gossip_err("open cost = %ld\n",open_diff);
    if (ret<0)
    {
        return ret;
    }
    //gossip_err("ret=%d\n",ret);
    //gossip_err("to_buffer_regin=%s, ret=%d\n",to_buffer_region,ret);
    //gossip_err("size_p=%lld,threshold=%lld\n",size_p,b_threshold);
    gettimeofday(&group_start,NULL);
    struct timeval time1, time2;
    for(j=0;j<aiocb_inuse_count;j++)
    {
        //offset_array[i_tt].offset = aiocb_ptr_array[j]->aio_offset;   
        //offset_array[i_tt].size = aiocb_ptr_array[j]->aio_nbytes;
        //i_tt++; 
        //gossip_err("offset=%ld,size=%d\n",aiocb_ptr_array[j]->aio_offset,aiocb_ptr_array[j]->aio_nbytes);
        
#ifdef USINGSSD
        gettimeofday(&time1, NULL);
        insert(&offset_list, aiocb_ptr_array[j]->aio_offset, aiocb_ptr_array[j]->aio_fildes, aiocb_ptr_array[j]->aio_nbytes);
        gettimeofday(&time2, NULL);
#endif
#ifdef GETOFFSET
        gossip_err("%ld %d %d %d\n",aiocb_ptr_array[j]->aio_offset, aiocb_ptr_array[j]->aio_fildes, aiocb_ptr_array[j]->aio_nbytes, aiocb_ptr_array[j]->aio_reqprio);
#endif
        //gossip_err("aio_fd = %d, q_op_p.fd = %d\n", aiocb_ptr_array[j]->aio_fildes, q_op_p->op.u.b_rw_list.open_ref.fd);
    }
    newstructcost += (time2.tv_sec-time1.tv_sec)*1000000 + time2.tv_usec-time1.tv_usec;
#ifdef QOS
    static int counth = 0, counts = 0;
    int tempi;
    for(tempi=0; tempi<aiocb_inuse_count; tempi++)
    {
        if(aiocb_ptr_array[tempi]->aio_fildes == writehddfd)
        {
            ret = issue_or_delay_io_operation(q_op_p, aiocb_ptr_array, aiocb_inuse_count, &op_p->u.b_rw_list.sigev, 0);
            counth++;
        }
        else if(aiocb_ptr_array[tempi]->aio_fildes == writessdfd)
        {
            write_ssd(ssd_fd->fd,op_p->handle,aiocb_ptr_array,aiocb_inuse_count,file_size);
            aio_progress_notification_ssd(q_op_p);
            counts++;
        }
        if(offset_list.count >= rs_length)
        {
            clear(&offset_list);
        }
    }
    //gossip_err("h = %d, s = %d\n", counth, counts);
#endif
#ifndef QOS
#ifdef USINGSSD
    if(offset_list.count >= rs_length)
    {
        /*
    	}*/
        gettimeofday(&time1, NULL);
        percent = randomfactor(&offset_list);
        //print(&offset_list);
        gettimeofday(&time2, NULL);
        newstructcost += (time2.tv_sec-time1.tv_sec)*1000000 + time2.tv_usec-time1.tv_usec;
#ifdef GETRF
        gossip_err("%f\n", percent);
#endif
        clear(&offset_list);
        percsum+=percent;
    	per = percent;
        if(fabs(per-level)>0.2)
        {
            clearcount+=1;
            if(clearflag==0)
                clearflag=1;
        }
        if(clearperlistcount>10)
        {
            if(clearcount/clearperlistcount>=0.8)
            {
                per_list_len=0;
                percsum = per;
            }
            clearcount=0;
            clearflag=0;
            clearperlistcount=0;
        }
        if(clearflag)
            clearperlistcount+=1;
        percent_list[per_list_len++] = per;
        int k;
        for(j=0;j<per_list_len-1;j++)
        {
            if(per<=percent_list[j])
            {
                for(k=j;k<per_list_len;k++)
                {
                    temp_per = percent_list[k];
                    percent_list[k] = per;
                    per = temp_per;
                }
                break;
            }
        }
        percavg=percsum/per_list_len;
        level = percent_list[(int)((1-percavg)*per_list_len)];
    	total_rate=(float)(total_is_ssd)/(total_is_ssd+total_is_hhd);
    	rate=(float)(is_ssd)/(is_ssd+is_hhd);
    }
#endif
    gettimeofday(&group_end,NULL);
    group_diff+=1000000*(group_end.tv_sec-group_start.tv_sec)+group_end.tv_usec-group_start.tv_usec;
    //if(i_tt==0)
    //    gossip_err("group cost = %ld, avl cost = %ld, percent = %lf\n",group_diff,avl_diff,percent);
    mod_flag=my_wb_info.flag;
    //gossip_err("percnetage = %lf\n", percent);
    if(mod_flag)
    {
        high_w=level;
        low_w=level;
    }
    //gossip_err("ssdsziesum=%lld\n",ssdsizesum);
    if(percent>0.45 && percent > high_w && is_write_hhd)
    //if(percent > high_w && is_write_hhd)
    //if(percent > level)
    {
        is_write_hhd = 0;
    }
    else if((percent < low_w && is_write_hhd==0)||percent<0.3)
    //else if((percent < low_w && is_write_hhd==0))
    //else
    {
	   is_write_hhd = 1;
    }
#ifdef ONLYHDD
    is_write_hhd = 1;
#endif
#ifdef ONLYSSD
    is_write_hhd = 0;
#endif
#ifdef BB
    if(bb_full) is_write_hhd = 1;
    else is_write_hhd = 0;
#endif
    //gossip_err("new struct cost = %llu\n", newstructcost);
    if(full_flag1 && full_flag2) is_write_hhd = 1;
    gettimeofday(&open_start,NULL);
    if(is_write_hhd)
    {
	    is_hhd++;
            total_is_hhd++;
        ret = issue_or_delay_io_operation(
            q_op_p, aiocb_ptr_array, aiocb_inuse_count,
            &op_p->u.b_rw_list.sigev, 0);
        //gossip_err("write hdd\n");
    }
    else
    {
    	    //gossip_err("file_size=%ld, offset=%ld, rqsize=%ld\n",file_size, aiocb_ptr_array[0]->aio_offset,aiocb_ptr_array[0]->aio_nbytes);
   	    write_ssd(ssd_fd->fd,op_p->handle,aiocb_ptr_array,aiocb_inuse_count,file_size);
	    aio_progress_notification_ssd(q_op_p); 
	    is_ssd++;
            total_is_ssd++;
        //gossip_err("write ssd\n");
    }
    if(rate>0.5)
	    cr_mod = true;
	//gettimeofday(&open_end,NULL);
	//open_diff+=1000000*(open_end.tv_sec-open_start.tv_sec)+open_end.tv_usec-open_start.tv_usec;
	//gossip_err("total cost = %ld\n", open_diff);
    //gossip_err("rate=%f, count=%d\n",rate,aiocb_inuse_count);
    /*
    if(is_ssd+is_hhd == 4000)
    {
        is_ssd=0;
        is_hhd=0;
    }
    */
    gossip_err("is_ssd=%d,is_hdd=%d,rate=%.4f\n",is_ssd,is_hhd,rate);
    /*
    char isssd[12],ishdd[12],isall[24];
    sprintf(isssd,"%d",is_ssd);
    sprintf(ishdd,"%d",is_hhd);
    strcat(isall,isssd);
    strcat(isall," ");

    strcat(isall,ishdd);
    mylog(isall);
    */
/*
    if (ret)
    {
        return ret;
    }
*/
#endif
#endif
    return 0;
}

static int dbpf_bstream_cancel(
    TROVE_coll_id coll_id,
    TROVE_op_id cancel_id,
    TROVE_context_id context_id)
{
    dbpf_queued_op_t *cur_op = NULL;
    int state, ret;

    cur_op = id_gen_fast_lookup(cancel_id);
    if (cur_op == NULL)
    {
        gossip_err("Invalid operation to test against\n");
        return -TROVE_EINVAL;
    }

    /* check the state of the current op to see if it's completed */
    gen_mutex_lock(&cur_op->mutex);
    state = cur_op->op.state;
    gen_mutex_unlock(&cur_op->mutex);

    gossip_debug(GOSSIP_TROVE_DEBUG, "got cur_op %p\n", cur_op);

    switch(state)
    {
        case OP_QUEUED:
        {
            gossip_debug(GOSSIP_TROVE_DEBUG,
                         "op %p is queued: handling\n", cur_op);

            /* dequeue and complete the op in canceled state */
            cur_op->op.state = OP_IN_SERVICE;
            dbpf_queued_op_put_and_dequeue(cur_op);
            assert(cur_op->op.state == OP_DEQUEUED);

	    cur_op->state = 0;
            /* this is a macro defined in dbpf-thread.h */
            dbpf_queued_op_complete(cur_op, OP_CANCELED);

            gossip_debug(
                GOSSIP_TROVE_DEBUG, "op %p is canceled\n", cur_op);
            ret = 0;
        }
        break;
        case OP_IN_SERVICE:
        {
            /*
              for bstream i/o op, try an aio_cancel.  for other ops,
              there's not much we can do other than let the op
              complete normally
            */
            if (((cur_op->op.type == BSTREAM_READ_LIST) ||
                (cur_op->op.type == BSTREAM_WRITE_LIST)) &&
                (cur_op->op.u.b_rw_list.aio_ops != NULL))
            {
#if 0
                ret = aio_cancel(cur_op->op.u.b_rw_list.fd,
                                 cur_op->op.u.b_rw_list.aiocb_array);
#endif
                ret = cur_op->op.u.b_rw_list.aio_ops->aio_cancel(
                    cur_op->op.u.b_rw_list.fd,
                    cur_op->op.u.b_rw_list.aiocb_array);
                gossip_debug(
                    GOSSIP_TROVE_DEBUG, "aio_cancel returned %s\n",
                    ((ret == AIO_CANCELED) ? "CANCELED" :
                     "NOT CANCELED"));
                /*
                  NOTE: the normal aio notification method takes care
                  of completing the op and moving it to the completion
                  queue
                */
            }
            else
            {
                gossip_debug(
                    GOSSIP_TROVE_DEBUG, "op is in service: ignoring "
                    "operation type %d\n", cur_op->op.type);
            }
            ret = 0;
        }
        break;
        case OP_COMPLETED:
        case OP_CANCELED:
            /* easy cancelation case; do nothing */
            gossip_debug(
                GOSSIP_TROVE_DEBUG, "op is completed: ignoring\n");
            ret = 0;
            break;
        default:
            gossip_err("Invalid dbpf_op state found (%d)\n", state);
            assert(0);
    }
    return ret;
}

/* dbpf_bstream_rw_list_op_svc()
 *
 * This function is used to service both read_list and write_list
 * operations.  State maintained in the struct dbpf_op (pointed to by
 * op_p) keeps up with which type of operation this is via the
 * "opcode" field in the b_rw_list member.
 *
 * NOTE: This method will NEVER be called if
 * __PVFS2_TROVE_AIO_THREADED__ is defined.  Instead, progress is
 * monitored and pushed using aio_progress_notification callback
 * method.
 *
 * Assumptions:
 * - FD has been retrieved from open cache, is refct'd so it won't go
 *   away
 * - lio_state in the op is valid
 * - opcode is set to LIO_READ or LIO_WRITE (corresponding to a
 *   read_list or write_list, respectively)
 *
 * This function is responsible for alloating and deallocating the
 * space reserved for the aiocb array.
 *
 * Outline:
 * - look to see if we have an aiocb array
 *   - if we don't, allocate one
 *   - if we do, then check on progress of each operation (having
 *     array implies that we have put some things in service)
 *     - if we got an error, ???
 *     - if op is finished, mark w/NOP
 *
 * - look to see if there are unfinished but posted operations
 *   - if there are, return 0
 *   - if not, and we are in the LIST_PROC_ALLPOSTED state,
 *     then we're done!
 *   - otherwise convert some more elements and post them.
 * 
 */
#ifndef __PVFS2_TROVE_AIO_THREADED__
static int dbpf_bstream_rw_list_op_svc(struct dbpf_op *op_p)
{
    int ret = -TROVE_EINVAL, i = 0, aiocb_inuse_count = 0;
    int op_in_progress_count = 0;
    struct aiocb *aiocb_p = NULL, *aiocb_ptr_array[AIOCB_ARRAY_SZ];

	gossip_err("%s: called\n", __func__);
	
    if (op_p->u.b_rw_list.list_proc_state == LIST_PROC_INITIALIZED)
    {
        /*
          first call; need to allocate aiocb array and ptr array;
          this array is freed in dbpf-op.c:dbpf_queued_op_free
        */
        aiocb_p = malloc(AIOCB_ARRAY_SZ * sizeof(struct aiocb));
        if (aiocb_p == NULL)
        {
            return -TROVE_ENOMEM;
        }

        memset(aiocb_p, 0, AIOCB_ARRAY_SZ * sizeof(struct aiocb));
        for(i = 0; i < AIOCB_ARRAY_SZ; i++)
        {
            aiocb_p[i].aio_lio_opcode = LIO_NOP;
            aiocb_p[i].aio_sigevent.sigev_notify = SIGEV_NONE;
        }

        op_p->u.b_rw_list.aiocb_array_count = AIOCB_ARRAY_SZ;
        op_p->u.b_rw_list.aiocb_array = aiocb_p;
        op_p->u.b_rw_list.list_proc_state = LIST_PROC_INPROGRESS;
        op_p->u.b_rw_list.sigev.sigev_notify = SIGEV_NONE;
    }
    else
    {
        /* operations potentially in progress */
        aiocb_p = op_p->u.b_rw_list.aiocb_array;

        /* check to see how we're progressing on previous operations */
        for(i = 0; i < op_p->u.b_rw_list.aiocb_array_count; i++)
        {
            if (aiocb_p[i].aio_lio_opcode == LIO_NOP)
            {
                continue;
            }

            /* gets the "errno" value of the individual op */
            ret = op_p->u.b_rw_list.aio_ops->aio_error(&aiocb_p[i]);
            if (ret == 0)
            {
                /*
                  this particular operation completed w/out error.
                  gets the return value of the individual op
                */
                ret = op_p->u.b_rw_list.aio_ops->aio_return(&aiocb_p[i]);

                gossip_debug(GOSSIP_TROVE_DEBUG, "%s: %s complete: "
                             "aio_return() ret %d (fd %d)\n",
                             __func__,
                             ((op_p->type == BSTREAM_WRITE_LIST) ||
                              (op_p->type == BSTREAM_WRITE_AT) ?
                              "WRITE" : "READ"), ret,
                             op_p->u.b_rw_list.fd);

                /* aio_return doesn't seem to return bytes read/written if 
                 * sigev_notify == SIGEV_NONE, so we set the out size 
                 * from what's requested.  For reads we just leave as zero,
                 * which ends up being OK,
                 * since the amount read (if past EOF its less than requested)
                 * is determined from the bstream size.
                 */
                if(op_p->type == BSTREAM_WRITE_LIST || 
                   op_p->type == BSTREAM_WRITE_AT)
                {
                    *(op_p->u.b_rw_list.out_size_p) += aiocb_p[i].aio_nbytes;
                }

                /* mark as a NOP so we ignore it from now on */
                aiocb_p[i].aio_lio_opcode = LIO_NOP;
            }
            else if (ret != EINPROGRESS)
            {
                gossip_err("%s: aio_error on block %d, skipping: %s\n",
                           __func__, i, strerror(ret));
                ret = -trove_errno_to_trove_error(ret);
                goto final_aio_cleanup;
            }
            else
            {
                /* otherwise the operation is still in progress; skip it */
                op_in_progress_count++;
            }
        }
    }

    /* if we're not done with the last set of operations, break out */
    if (op_in_progress_count > 0)
    {
        return 0;
    }
    else if (op_p->u.b_rw_list.list_proc_state == LIST_PROC_ALLPOSTED)
    {
        /* we've posted everything, and it all completed */
        ret = 1;

      final_aio_cleanup:
        if ((op_p->type == BSTREAM_WRITE_AT) ||
            (op_p->type == BSTREAM_WRITE_LIST))
        {
            DBPF_AIO_SYNC_IF_NECESSARY(op_p, op_p->u.b_rw_list.fd, ret);
        }

        dbpf_open_cache_put(&op_p->u.b_rw_list.open_ref);
        op_p->u.b_rw_list.fd = -1;

        start_delayed_ops_if_any(1);
        return ret;
    }
    else
    {
        /* no operations in progress; convert and post some more */
        aiocb_inuse_count = op_p->u.b_rw_list.aiocb_array_count;
        ret = dbpf_bstream_listio_convert(
            op_p->u.b_rw_list.fd,
            op_p->u.b_rw_list.opcode,
            op_p->u.b_rw_list.mem_offset_array,
            op_p->u.b_rw_list.mem_size_array,
            op_p->u.b_rw_list.mem_array_count,
            op_p->u.b_rw_list.stream_offset_array,
            op_p->u.b_rw_list.stream_size_array,
            op_p->u.b_rw_list.stream_array_count,
            aiocb_p,
            &aiocb_inuse_count,
            &op_p->u.b_rw_list.lio_state);

        if (ret == 1)
        {
            op_p->u.b_rw_list.list_proc_state = LIST_PROC_ALLCONVERTED;
        }
        
        /* mark unused with LIO_NOPs */
        for(i = aiocb_inuse_count;
            i < op_p->u.b_rw_list.aiocb_array_count; i++)
        {
            aiocb_p[i].aio_lio_opcode = LIO_NOP;
        }

        for(i = 0; i < aiocb_inuse_count; i++)
        {
            aiocb_ptr_array[i] = &aiocb_p[i];
        }

        if (op_p->u.b_rw_list.list_proc_state == LIST_PROC_ALLCONVERTED)
        {
            op_p->u.b_rw_list.list_proc_state = LIST_PROC_ALLPOSTED;
        }

        /*
          we use a reverse mapped ptr for I/O operations in order to
          access the queued op from the op.  this is only useful for
          the delayed io operation scheme.  it's initialized in
          dbpf_bstream_rw_list
        */
        assert(op_p->u.b_rw_list.queued_op_ptr);

        ret = issue_or_delay_io_operation(
            (dbpf_queued_op_t *)op_p->u.b_rw_list.queued_op_ptr,
            aiocb_ptr_array, aiocb_inuse_count,
            &op_p->u.b_rw_list.sigev, 1);

        if (ret)
        {
            return ret;
        }
        return 0;
    }
}

#endif

inline int dbpf_pread(int fd, void *buf, size_t count, off_t offset)
{
    int ret = 0;
    int ret_size = 0;

    do
    {
        ret = pread(fd, ((char *)buf) + ret_size, 
                    count - ret_size, offset + ret_size);
        if (ret)
        {
            ret_size += ret;
        }
    } while( (ret == -1 && errno == EINTR) || (ret_size < count && ret > 0) );

    if(ret < 0)
    {
        return ret;
    }
    return ret_size;
}
inline int dbpf_pwrite_ssd(int fd, const void *buf, size_t count,off_t offset)
{
    int ret = 0;
    int ret_size = 0;
    do
    {
        //ret = write(fd, ((char *)buf) + ret_size, 
        //             count - ret_size);
        ret = pwrite(fd, ((char *)buf) + ret_size, 
                     count - ret_size,offset + ret_size);
        if (ret)
        {
            ret_size += ret;
        }
    } while( (ret == -1 && errno == EINTR)  || (ret_size < count && ret > 0) );
    if(ret < 0)
    {
        return -trove_errno_to_trove_error(errno);
    }
    return ret_size;
}

inline int dbpf_pwrite(int fd, const void *buf, size_t count, off_t offset)
{
    int ret = 0;
    int ret_size = 0;
    do
    {
        ret = pwrite(fd, ((char *)buf) + ret_size, 
                     count - ret_size, offset + ret_size);
        if (ret)
        {
            ret_size += ret;
        }
    } while( (ret == -1 && errno == EINTR)  || (ret_size < count && ret > 0) );

    if(ret < 0)
    {
        return -trove_errno_to_trove_error(errno);
    }
    return ret_size;
}

static struct dbpf_aio_ops aio_ops =
{
    aio_read,
    aio_write,
    lio_listio,
    aio_error,
    aio_return,
    aio_cancel,
    aio_suspend,
    aio_fsync
};

static int dbpf_bstream_resize_op_svc(struct dbpf_op *op_p)
{
    int ret;
    TROVE_ds_attributes attr;
    TROVE_object_ref ref;
    dbpf_queued_op_t *q_op_p;
    struct open_cache_ref open_ref;
    PVFS_size tmpsize;

    q_op_p = (dbpf_queued_op_t *)op_p->u.b_resize.queued_op_ptr;

    ref.fs_id = op_p->coll_p->coll_id;
    ref.handle = op_p->handle;

    gen_mutex_lock(&dbpf_update_size_lock);
    ret = dbpf_dspace_attr_get(op_p->coll_p, ref, &attr);
    if(ret != 0)
    {
        gen_mutex_unlock(&dbpf_update_size_lock);
        return ret;
    }

    tmpsize = op_p->u.b_resize.size;
    attr.u.datafile.b_size = tmpsize;

    ret = dbpf_dspace_attr_set(op_p->coll_p, ref, &attr);
    if(ret < 0)
    {
        gen_mutex_unlock(&dbpf_update_size_lock);
        return ret;
    }
    gen_mutex_unlock(&dbpf_update_size_lock);

    /* setup op for sync coalescing */
    dbpf_queued_op_init(q_op_p,
                        DSPACE_SETATTR,
                        ref.handle,
                        q_op_p->op.coll_p,
                        dbpf_dspace_setattr_op_svc,
                        q_op_p->op.user_ptr,
                        TROVE_SYNC,
                        q_op_p->op.context_id);
    q_op_p->op.state = OP_IN_SERVICE;

    /* truncate file after attributes are set */
    ret = dbpf_open_cache_get(
        op_p->coll_p->coll_id, op_p->handle,
        DBPF_FD_BUFFERED_WRITE,
        &open_ref);
    if(ret < 0)
    {
        return ret;
    }

    ret = DBPF_RESIZE(open_ref.fd, tmpsize);
    if(ret < 0)
    {
        return(ret);
    }

    dbpf_open_cache_put(&open_ref);

    return DBPF_OP_COMPLETE;
}

int dbpf_bstream_resize(TROVE_coll_id coll_id,
                        TROVE_handle handle,
                        TROVE_size *inout_size_p,
                        TROVE_ds_flags flags,
                        TROVE_vtag_s *vtag,
                        void *user_ptr,
                        TROVE_context_id context_id,
                        TROVE_op_id *out_op_id_p,
                        PVFS_hint hints)
{
    dbpf_queued_op_t *q_op_p = NULL;
    struct dbpf_collection *coll_p = NULL;

    coll_p = dbpf_collection_find_registered(coll_id);
    if (coll_p == NULL)
    {
        return -TROVE_EINVAL;
    }

    q_op_p = dbpf_queued_op_alloc();
    if (q_op_p == NULL)
    {
        return -TROVE_ENOMEM;
    }

    /* initialize all the common members */
    dbpf_queued_op_init(q_op_p,
                        BSTREAM_RESIZE,
                        handle,
                        coll_p,
                        dbpf_bstream_resize_op_svc,
                        user_ptr,
                        flags,
                        context_id);

    /* initialize the op-specific members */
    q_op_p->op.u.b_resize.size = *inout_size_p;
    q_op_p->op.u.b_resize.queued_op_ptr = q_op_p;
    *out_op_id_p = dbpf_queued_op_queue(q_op_p);

    return 0;
}

struct TROVE_bstream_ops dbpf_bstream_ops =
{
    dbpf_bstream_read_at,
    dbpf_bstream_write_at,
    dbpf_bstream_resize,
    dbpf_bstream_validate,
    dbpf_bstream_read_list,
    dbpf_bstream_write_list,
    dbpf_bstream_flush,
    dbpf_bstream_cancel
};

static int write_back(void *parm)
{
    ssd_cache_fd * start_ssd_fd, *end_ssd_fd;
    struct stat statbuf;
    off_t read_offset = 0;
    TROVE_size  total_size=0,file_size=0;
    int file_num = 0;
    char buffer_region[100] = {0};
    int ret;
    char *wb_buf = NULL;
    int i;
    write_back_use * write_back_fd, *temp, *start, *end;
    TROVE_size buf_size=0;   
    int fd = *((int *)parm);
    Offset_pair_t offset_data;
    char message[100] = {0};
    int have_read = 0;

    char cache_name[40] = {0};
    off_t offset=0;
    char test_buf[20] = "this is a test!";

	
    if(is_writeback_exist)return 1;
	
    gen_mutex_lock(&dbpf_writeback_thread);
    is_writeback_exist = true;
    gen_mutex_unlock(&dbpf_writeback_thread);

    write_back_fd = (write_back_use*)malloc(sizeof(write_back_use));
    write_back_fd->fd = -1;
    write_back_fd->ssd_fd = -1;
    write_back_fd->next = NULL;
    temp = write_back_fd;
    //pthread_mutex_lock(&noderlock_mutex);
    if(full_flag1)
    {
        gossip_err("ssd_1 is full!\n");
        snprintf(buffer_region,100,my_wb_info.buff_path_1);
        strcpy(cache_name,buffer_region);
        int wbi = 0;
        for(start_ssd_fd=link_ssd_fd_1->next;start_ssd_fd!=NULL;start_ssd_fd=start_ssd_fd->next)
        {
            char cid[5], cache[20]="/cache-";
            sprintf(cid,"%d",start_ssd_fd->ori_fd);
            strcat(cache,cid);
            strcat(cache_name,cache);
            gossip_err("cache_name:%s\n",cache_name);
            write_back_use * insert;
            insert = (write_back_use*)malloc(sizeof(write_back_use));
            insert->fd = start_ssd_fd->write_back_fd;
            insert->ssd_fd = open(cache_name,O_RDWR);
            temp->next = insert;
            temp = insert;
            memset(cache_name,0,sizeof(cache_name));
            strcpy(cache_name,buffer_region);
            if(stat(cache_name,&statbuf)!=-1) file_size = (TROVE_size)statbuf.st_size;
        }
	flush_num = 1;
    }
    else if(full_flag2)
    {
        gossip_err("ssd_2 is full!\n");
        snprintf(buffer_region,100,my_wb_info.buff_path_2);
        strcpy(cache_name,buffer_region);
        for(start_ssd_fd=link_ssd_fd_2->next;start_ssd_fd!=NULL;start_ssd_fd=start_ssd_fd->next)
        {
            char cid[5], cache[20]="/cache-";
            sprintf(cid,"%d",start_ssd_fd->ori_fd);
            strcat(cache,cid);
            strcat(cache_name,cache);
            gossip_err("cache_name:%s\n",cache_name);
            write_back_use * insert;
            insert = (write_back_use*)malloc(sizeof(write_back_use));
            insert->fd = start_ssd_fd->write_back_fd;
            insert->ssd_fd = open(cache_name,O_RDWR);
            temp->next = insert;
            temp = insert;
            memset(cache_name,0,sizeof(cache_name));
            strcpy(cache_name,buffer_region);
            if(stat(cache_name,&statbuf)!=-1) file_size = (TROVE_size)statbuf.st_size;
        }
        flush_num = 2;

    }
    else 
    {
	gen_mutex_lock(&dbpf_writeback_thread);
    	is_writeback_exist = false;
    	gen_mutex_unlock(&dbpf_writeback_thread);
	return 1;
    }
	
    for(i=0;i<204800;i++){
        avl_offset_array[i] = -1;
        avl_offset_array_ssd[i] = -1;
	avl_size_array[i] = -1;
	//avl_cachename[i]=;
	}	
    avl_index = 0;
    merge = 0;
    if(noder==NULL)gossip_err("write back error\n");
    gettimeofday(&avl_start,NULL);
    in_order_travel_test(noder);
    gettimeofday(&avl_end,NULL);
    avl_diff+=1000000*(avl_end.tv_sec-avl_start.tv_sec)+avl_end.tv_usec-avl_start.tv_usec;
    gossip_err("avl cost : %ld\n",avl_diff);
    wb_buf = (char *)malloc(sizeof(char) * avl_size_array[0]);
    avl_index = 0;

    gossip_err("now is writing back!\n");
    float static ppp=0.0;
    int static pll=0;
    int endrun;
    /*
    while(per_list_len<500)
    {
	if(cr_mod)
	    break;
    	if(mod_flag==0)
	        break;
        if(ppp!=percent){
            ppp=percent;
            gossip_err("percent=%f\n",ppp);
        }
        if(pll!=per_list_len){
            pll=per_list_len;
            gossip_err("per_list_len=%d\n",pll);
        }
        if(in_dead_lock==1){
            gossip_err("in dead lock so exit\n");
            break;
        }
	    if(percent>0.9){
	        gossip_err("percent>0.7 exit\n");
            break;
    	}
        endrun=is_hdd+is_ssd;
        //sleep(1);
        if(endrun==is_hdd+is_ssd){
            gossip_err("end run exit\n");
            //break;
	    }

    }*/
    is_ssd=0;
    is_hhd=0;
    rate=0.0;
    flush_done = 0;
    pthread_t io_overhead;
    pthread_create(&io_overhead, NULL, get_io_occupy, NULL);
    gossip_err("start flushing!per_list_len=%d,percent=%f\n",per_list_len,percent);
    hdd_overhead_low = 0;
    struct timeval t1, t2;
    float wait_cost = 0.0;
    while(avl_index<204800 && avl_offset_array[avl_index]!=-1 && avl_size_array[avl_index!=-1])
    {
	//while(percent<0.43 && !in_dead_lock);
	
	//gossip_err("percent=%f\n",percent);
        /*
	while(percent < 0.5){
		if(in_dead_lock || per_list_len>510){
			gossip_err("in_dead——lock=1 \n") ;
			break ; 
		}
		if(in_dead_lock)gossip_err("in_dead_lock=%d\n",in_dead_lock);
	}*/
        gettimeofday(&t1, NULL);
        while(is_write_hhd)// && hdd_overhead_low==0)
        {
#ifdef BB
            break;
#endif
#ifdef DIRECTFLUSH
            break;
#endif
            usleep(1000);
            if(hdd_overhead_low)
                break;
        }
        gettimeofday(&t2, NULL);
        wait_cost += t2.tv_sec-t1.tv_sec+(t2.tv_usec-t1.tv_usec)/1000000;
        //while(is_write_hhd && level<0.7);
        for(start=write_back_fd->next;start!=NULL;start=start->next)
        {
	    if(strcmp(start->cache, avl_cachename[avl_index])==0)
		break;
	}
            ret = pread(start->ssd_fd,wb_buf, avl_size_array[avl_index],avl_offset_array_ssd[avl_index]);
            ret = pwrite(start->fd,wb_buf, avl_size_array[avl_index],avl_offset_array[avl_index]);
            free(wb_buf);
            avl_index++;
            wb_buf = (char *)malloc(sizeof(char) * avl_size_array[avl_index]);
            total_size+=ret;
            read_offset+=ret;
        
    }
    gossip_err("write back is over!, wait cost is %f\n", wait_cost);
#ifdef BB
    bb_full = 0;
#endif
    flush_done = 1; 
    free(wb_buf);
    if(noder==NULL)gossip_err("noder is NULL\n");
    destroy_avl(&noder);
    noder = NULL;
    //pthread_mutex_unlock(&noderlock_mutex);
    wb_total = 0;

    avl_total_size=0;

    gen_mutex_lock(&dbpf_writeback_thread);
    is_writeback_exist = false;
    gen_mutex_unlock(&dbpf_writeback_thread);
    is_hdd=0;
    free_block_total++;
    if(flush_num==2)
    {
        
        gossip_err("Be flushed was ssd2, now to_buffer_region is %s!\n",to_buffer_region);
        full_flag2=false;
        snprintf(buffer_region,100,my_wb_info.buff_path_2);
        strcpy(cache_name,buffer_region);
        for(start_ssd_fd=link_ssd_fd_2;start_ssd_fd!=NULL;start_ssd_fd=start_ssd_fd->next)
        {
            char cid[5], cache[20]="/cache-";
            if(start_ssd_fd->fd == -1)
                continue;
            sprintf(cid,"%d",start_ssd_fd->ori_fd);
            strcat(cache,cid);
            strcat(cache_name,cache);
            gossip_err("cache_name:%s\n",cache_name);
            unlink(cache_name);
            memset(cache_name,0,sizeof(cache_name));
            strcpy(cache_name,buffer_region);
        }
        if(noder==NULL)gossip_err("noder is null\n");
        if(noderlock==0)gossip_err("noderlock is 0\n");
        write_back_use * wdel;// = write_back_fd->next;
        while(write_back_fd)
        {    
             wdel=write_back_fd->next;
             if(wdel==NULL)
                break;
             close(wdel->ssd_fd);
             free(wdel);
             write_back_fd = wdel;
        }    
        ssd_cache_fd* del = link_ssd_fd_2->next;
        ssd_cache_fd* tmp;// = del==NULL?NULL:link_ssd_fd_1->next->next;
        while(del)
        {    
            tmp=del->next;
            if(tmp==NULL)
                break;
            close(del->fd);
            free(del);
            del = tmp; 
                           
        }
        //free(link_ssd_fd_1)    
        link_ssd_fd_2->next=NULL;
        //free(ssd_fd);
        //ssd_fd = NULL;
        gossip_err("2 exit write back success!\n");


   }
    else if(flush_num==1)
    {
        gossip_err("Be flushed was ssd1, now to_buffer_region is %s, is_hdd=%d!\n",to_buffer_region,is_hdd);
        full_flag1=false;
        snprintf(buffer_region,100,my_wb_info.buff_path_1);
        strcpy(cache_name,buffer_region);
        for(start_ssd_fd=link_ssd_fd_1;start_ssd_fd!=NULL;start_ssd_fd=start_ssd_fd->next)
        {
            char cid[5], cache[20]="/cache-";
            if(start_ssd_fd->fd == -1)
                continue;
            sprintf(cid,"%d",start_ssd_fd->ori_fd);
            strcat(cache,cid);
            strcat(cache_name,cache);
            gossip_err("cache_name:%s\n",cache_name);
            unlink(cache_name);
            memset(cache_name, 0, sizeof(cache_name));
            strcpy(cache_name,buffer_region);
        }

        if(noder==NULL)gossip_err("noder is null\n");
        if(noderlock==0)gossip_err("noderlock is 0\n");
        
        gossip_err("start clear write_back!\n");
        write_back_use * wdel;// = write_back_fd->next;
        while(write_back_fd)
        {

            gossip_err("clearing write_back!\n");
	        wdel=write_back_fd->next;
            if(wdel==NULL)
                break;
            close(wdel->ssd_fd);
            free(wdel);
            write_back_fd = wdel;
        }
       gossip_err("clear write_back succeed!\n");
        ssd_cache_fd* del = link_ssd_fd_1->next;
        ssd_cache_fd* tmp;// = del==NULL?NULL:link_ssd_fd_1->next->next;
        while(del)
        {
	       tmp=del->next;
           if(tmp==NULL)
                break;
           close(del->fd);
           free(del);
           del = tmp;
        }
       gossip_err("clear link_ssd_fd_1 succeed!\n");
       //free(link_ssd_fd_1)    
       link_ssd_fd_1->next=NULL;
       //free(ssd_fd);
       //ssd_fd = NULL;
       gossip_err("1 exit write back success!\n");
    }
    	
    noderlock=0;
    return 1;
}

static void in_order_travel_test(struct mavlnode *n)
{
	if(n)
	{
		in_order_travel_test(n->left);
		if(avl_index!=0 && avl_offset_array[avl_index-1] + n->d->access_size == n->d->original_offset){
			avl_size_array[avl_index-1] += n->d->access_size;
			merge++;
		}
		else{
			avl_offset_array[avl_index] = n->d->original_offset;
			avl_offset_array_ssd[avl_index] = n->d->new_offset;
			avl_size_array[avl_index] = n->d->access_size;
			strcpy(avl_cachename[avl_index],n->d->cache);
			avl_index++;
		}
		in_order_travel_test(n->right);		
	}
}


static void mylog(char *message)
{
	FILE *fd;
	fd = fopen("/tmp/mylog.log", "a+");
	fprintf(fd,message);
	fclose(fd);
}
void shell_insert(long long array[], int size)
{
    int gap = size/2;
    int i, j;
    while(gap)
    {
        for(i=gap-1;i>=0;i--)
        {
            for(j=i;j<size;j+=gap)
            {
                long long needinsert = array[j];
                int k=j-gap;
                for(;k>=0 && array[k]>needinsert;k-=gap)
                    array[k+gap] = array[k];
                array[k+gap] = needinsert;
            }
        }
        gap /= 2;
    }
}

void insert(struct offsetlist *offset_list, long long offset, int fd, int size)
{
    struct offsetnode *head = offset_list->offset_fd;
    struct offsetnode *pre = head;
    if(head==NULL)
    {
        offset_list->offset_fd = (struct offsetnode*)malloc(sizeof(struct offsetnode));
        offset_list->offset_fd->fd = fd;
        offset_list->offset_fd->count = 0;
        offset_list->offset_fd->size = size;
        offset_list->offset_fd->offset[offset_list->offset_fd->count++] = offset;
        offset_list->offset_fd->next = NULL;
        offset_list->fd_num = 0;
        offset_list->fd_num++;
#ifdef QOS
        if(writehddfd==0) writehddfd = fd;
#endif
    }
    else
    {
#ifdef RFOFSINGLEJOB
        head->offset[head->count++] = offset;
#endif 
#ifndef RFOFSINGLEJOB
        while(head)
        {
            if(head->fd==fd) break;
            pre = head;
            head = head->next;
        }
        if(head==NULL)
        {
            pre->next = (struct offsetnode*)malloc(sizeof(struct offsetnode));
            pre->next->fd = fd;
            pre->next->count = 0;
            pre->next->size = size;
            pre->next->offset[pre->next->count++] = offset;
            pre->next->next = NULL;
            offset_list->fd_num++;
#ifdef QOS
            if(writessdfd==0) writessdfd = fd;
#endif
        }
        else
        {
            head->offset[head->count++] = offset;
        }
#endif
    }
    offset_list->count++;
}
void clear(struct offsetlist *offset_list)
{
    struct offsetnode *head = offset_list->offset_fd;
    while(head)
    {
        head->count = 0;
        head = head->next;
    }
    offset_list->count = 0;
    offset_list->fd_num = 0;
}

void print(struct offsetlist *offset_list)
{
    struct offsetnode *head = offset_list->offset_fd;
    while(head)
    {
        int i = 0;
        for(;i<head->count; i++)
            gossip_err("fd = %d, offset = %llu\n", head->fd, head->offset[i]);
        head = head->next;
    }
}
float randomfactor(struct offsetlist *offset_list)
{
    struct offsetnode *head = offset_list->offset_fd;
    int num = 0;
    struct timeval time1, time2;
    while(head)
    {
        gettimeofday(&time1, NULL);
        //quick_sort(head->offset, 0, head->count-1);
        //bubble_sort(head->offset, head->count);
        shell_insert(head->offset, head->count);
        //halve_insert_sort(head->offset, head->count);
        gettimeofday(&time2, NULL);
        cost += 1000000*(time2.tv_sec-time1.tv_sec)+time2.tv_usec-time1.tv_usec;
        head = head->next;
    }
    head = offset_list->offset_fd;
    //int test = 0;
    while(head)
    {
        int i = 0;
        int c = 0;
        for(;i<head->count-1; i++)
        {
            //float diff = (float)(head->offset[i+1]-head->offset[i]-head->size)/(float)head->size;
            //gossip_err("diff = %f\n", diff);
            if((head->offset[i+1]-head->offset[i]-head->size)/head->size > 1)
            {
                num += 1;
                c += 1;
            }
            /*
            else if(diff < 0)
            {
                test += 1;
            }*/
        }
        head = head->next;
        float r = (float)c/(i+1);
        //printf("%d,%d,%f,", c, i, r);
        if(head) num += 1;

    }
    //gossip_err("%d\n", test);
    float rf = (float)num/127;
    return rf;
}

void *get_io_occupy()
{
    char cmd[] = "iostat -dx 1";
    char buffer[1024];
    char a[20], a1[20];
    float arr[20], arr1[20];
    FILE *pipe = popen(cmd, "r");
    if(!pipe) return -1;
    fgets(buffer, sizeof(buffer), pipe);
    fgets(buffer, sizeof(buffer), pipe);
    fgets(buffer, sizeof(buffer), pipe);
    fgets(buffer, sizeof(buffer), pipe);
    fgets(buffer, sizeof(buffer), pipe);
    while(1)
    {
        fgets(buffer, sizeof(buffer), pipe);
        fgets(buffer, sizeof(buffer), pipe);
        fgets(buffer, sizeof(buffer), pipe);
        sscanf(buffer, "%s %f %f %f %f %f %f %f %f %f %f %f %f %f ",a1,&arr1[0],&arr1[1],&arr1[2],&arr1[3],&arr1[4],&arr1[5],&arr1[6],&arr1[7],&arr1[8],&arr1[9],&arr1[10],&arr1[11],&arr1[12]);
        fgets(buffer, sizeof(buffer), pipe);
        sscanf(buffer, "%s %f %f %f %f %f %f %f %f %f %f %f %f %f ",a,&arr[0],&arr[1],&arr[2],&arr[3],&arr[4],&arr[5],&arr[6],&arr[7],&arr[8],&arr[9],&arr[10],&arr[11],&arr[12]);
        //printf("%s %f %f %f %f %f %f %f %f %f %f %f %f\n",a,arr[0],arr[1],arr[2],arr[3],arr[4],arr[5],arr[6],arr[7],arr[8],arr[9],arr[10], arr[11]);
        /*
        if(arr[10]<10) 
        {
            hdd_overhead_low = 1;
            gossip_err("hdd_overhead_low = %d\n", hdd_overhead_low);
        }*/
        if(arr1[5]<100 && arr[5]<100)
        {
            hdd_overhead_low = 1;
        }
        //else hdd_overhead_low = 0;
        if(flush_done) break;
        gossip_err("sda : %.2f\n", arr[10]);
    }
    pclose(pipe);
}
/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */
