// ----------------------------------------------------------------------------
// Copyright IBM Corp. 2010, 2012
// All Rights Reserved
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// Filename:    fsi_ipc_ccl.h
// Description: Common code layer common function definitions
// Author:      FSI IPC Team
// ----------------------------------------------------------------------------

#ifndef __FSI_IPC_CCL_H__
#define __FSI_IPC_CCL_H__

// Linux includes
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/acl.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "../fsi_ipc_common.h"

// FSI IPC defines

#define UNUSED_ARG(arg) do { (void)(arg); } while (0)

#define FSI_CIFS_RESERVED_STREAMS   4   // CIFS does not allow handles 0-2

#define FSI_BLOCK_ALIGN(x, blocksize) \
(((x) % (blocksize)) ? (((x) / (blocksize)) * (blocksize)) : (x))

#define PT_FSI_CCL_VERSION "3.3.1.102"

#define FSI_COMMAND_TIMEOUT_SEC      900 // When polling for results, number
                                         // of seconds to try before timingout
#define FSI_COMMAND_LOG_THRESHOLD_SEC 20 // In seconds, if timed responses
                                         // exceed then make log entry
#define USLEEP_INTERVAL        10000     // Parameter to usleep

#define CCL_POLLING_THREAD_HANDLE_TIMEOUT_SEC 300   // Timeout for opened handle
                                                    // to be considered old in
                                                    // polling thread path
#define CCL_ON_DEMAND_HANDLE_TIMEOUT_SEC       15   // Timeout for on-demand
                                                    // thread looking for
                                                    // handles to close

// FSI IPC getlock constants
#define FSI_IPC_GETLOCK_PTYPE                  2
#define FSI_IPC_GETLOCK_PPID                   0

#define PTFSAL_FILESYSTEM_NUMBER              77
#define FSI_IPC_MSGID_BASE               5000000

typedef enum fsi_ipc_trace_level {
  FSI_NO_LEVEL = 0,
  FSI_FATAL,
  FSI_ERR,
  FSI_WARNING,
  FSI_NOTICE,
  FSI_STAT,
  FSI_INFO,
  FSI_DEBUG,

  // this one must be last
  FSI_NUM_TRACE_LEVELS
} fsi_ipc_trace_level;

#ifndef   __GNUC__
#define __attribute__(x) /*nothing*/
#endif // __GNUC__

// Log-related functions and declarations

#define MAX_LOG_LINE_LEN 512

typedef int (*log_function_t) (int level, const char * message);
typedef int (*log_level_check_function_t) (int level);
  /*int ccl_log(const fsi_ipc_trace_level   level,
            const char                * func,
            const char                * format,
            ...);*/

// The following functions enable compile-time check with a cost of a function
// call. Ths function is empty, but due to its  __attribute__ declaration
// the compiler checks the format string which is passed to it by the
// FSI_TRACE...() mechanism.
static void
compile_time_check_func(const char * fmt, ...)
__attribute__((format(printf, 1, 2)));  // 1=format 2=params

// ----------------------------------------------------------------------------
// This is needed to make FSI_TRACE macro work
// Part of the magic of __attribute__ is this function needs to be defined,
// though it's a noop
static inline void
compile_time_check_func(const char * fmt, ...)
{
  UNUSED_ARG(fmt);
  // do nothing
}

// Our own trace macro that adds standard prefix to statements that includes
// the level and function name (by calling the wrapper ccl_log)
#define FSI_TRACE(level, ... )                                             \
{                                                                          \
  compile_time_check_func( __VA_ARGS__ );                                  \
  CCL_LOG(level, __func__, __VA_ARGS__);                                   \
}

#define FSI_TRACE_COND_RC(rc, errVal, ... )                                \
{                                                                          \
  FSI_TRACE((errVal) == (rc) ? FSI_INFO : FSI_ERR, ## __VA_ARGS__);        \
}

#define FSI_TRACE_HANDLE( handle)                                          \
{                                                                          \
  uint64_t * handlePtr = (uint64_t *) handle;                              \
  FSI_TRACE(FSI_INFO, "persistent handle: 0x%lx %lx %lx %lx",              \
            handlePtr[0], handlePtr[1], handlePtr[2], handlePtr[3]);       \
}

#define WAIT_SHMEM_ATTACH()                                                \
{                                                                          \
  while (g_shm_at_fsal == 0) {                                             \
    FSI_TRACE(FSI_INFO, "waiting for shmem attach");                       \
    sleep(1);                                                              \
  }                                                                        \
}

#define CCL_CLOSE_STYLE_NORMAL             0
#define CCL_CLOSE_STYLE_FIRE_AND_FORGET    1
#define CCL_CLOSE_STYLE_NO_INDEX           2

#define FSAL_MAX_PATH_LEN PATH_MAX

extern int       g_shm_id;              // SHM ID
extern char    * g_shm_at;              // SHM Base Address
extern char    * g_shm_at_fsal;              // SHM Base Address
extern int       g_io_req_msgq;
extern int       g_io_rsp_msgq;
extern int       g_non_io_req_msgq;
extern int       g_non_io_rsp_msgq;
extern int       g_shmem_req_msgq;
extern int       g_shmem_rsp_msgq;
extern char      g_chdir_dirpath[PATH_MAX];
extern uint64_t  g_client_pid;
extern uint64_t  g_server_pid;

extern struct file_handles_struct_t * g_fsi_handles_fsal;  // FSI client
                                                           // handles
extern struct dir_handles_struct_t  * g_fsi_dir_handles_fsal; // FSI client Dir
                                                              // handles
extern struct acl_handles_struct_t  * g_fsi_acl_handles_fsal; // FSI client ACL
                                                              // handles
extern uint64_t  g_client_trans_id;  // FSI global transaction id
extern int       g_close_trace;      // FSI global trace of io rates at close
extern int       g_multithreaded;    // ganesha = true, samba = false

// from fsi_ipc_client_statistics.c
extern struct timeval            g_next_log_time;
extern struct timeval            g_curr_log_time;
extern struct timeval            g_last_log_time;
extern struct timeval            g_last_io_completed;
extern struct timeval            g_begin_io_idle_time;
extern struct ipc_client_stats_t g_client_bytes_read;
extern struct ipc_client_stats_t g_client_bytes_written;
extern struct ipc_client_stats_t g_ipc_vfs_xfr_time;    // usecs
extern struct ipc_client_stats_t g_client_io_idle_time;  // usecs
extern uint64_t                  g_num_reads_in_progress;
extern uint64_t                  g_num_writes_in_progress;
extern uint64_t                  g_stat_log_interval;
extern char                      g_client_address[];


#define SAMBA_FSI_IPC_PARAM_NAME      "fsiparam"  // To designate as our parms
#define SAMBA_EXPORT_ID_PARAM_NAME    "exportid"  // For ExportID
#define SAMBA_STATDELTA_PARAM_NAME    "statdelta" // For Statistics Output
#define MAX_FSI_PERF_COUNT            1000        // for m_perf_xxx counters

// enum for client buffer return code state
enum e_buf_rc_state {
  BUF_RC_STATE_UNKNOWN = 0,             // default
  BUF_RC_STATE_PENDING,                 // waiting on server Rc
  BUF_RC_STATE_FILLING,                 // filling with write data
  BUF_RC_STATE_RC_NOT_PROCESSED,        // received Rc, not processed by client
  BUF_RC_STATE_RC_PROCESSED             // client processed received Rc
};

enum e_ccl_write_mode {
  CCL_WRITE_IMMEDIATE = 0,              // write should be immediately issued
  CCL_WRITE_BUFFERED                    // pwrite does not need to issue write
};


// ----------------------------------------------------------------------------
/// @struct io_buf_status_t
/// @brief  contains I/O buffer status
// ----------------------------------------------------------------------------
struct io_buf_status_t {
  char    * m_p_shmem;                  // IPC shmem pointer
  int       m_this_io_op;               // enumerated I/O operation
                                        // (read/write/other I/O)
  int       m_buf_in_use;               // used to determine available buffers
                                        // a usable buffer is not in use
                                        // and not "not allocated"
  int       m_data_valid;               // set on read when data received
  int       m_bytes_in_buf;             // number of bytes of data in buffer
  int       m_buf_use_enum;             // BufUsexxx enumeration
  int       m_buf_rc_state;             // enum return code state BufRcXxx
  uint64_t  m_trans_id;                 // transaction id
};

// --------------------------------------------------------------------------
// file statistics structure
// --------------------------------------------------------------------------
// NOTE before 81020872 this use to be linux stat struct
// now it is defined to look very similiar but it does not
// align exactly like the linust struct stat, the field names
// are kept the same but it may impact ganesha protoype
//
// typedef struct stat fsi_stat_struct;
typedef struct fsi_stat_struct__ {
  uint64_t                st_dev;          // Device
  uint64_t                st_ino;          // File serial number
  uint64_t                st_mode;         // File mode
  uint64_t                st_nlink;        // Link count
  uint64_t                st_uid;          // User ID of the file's owner
  uint64_t                st_gid;          // Group ID of the file's group
  uint64_t                st_rdev;         // Device number, if device
  uint64_t                st_size;         // Size of file, in bytes
  uint64_t                st_atime_sec;    // Time of last access  sec only
  uint64_t                st_mtime_sec;    // Time of last modification  sec
  uint64_t                st_ctime_sec;    // Time of last change  sec
  //struct timespec         st_btime;      // Birthtime  not used
  uint64_t                st_blksize;      // Optimal block size for I/O
  uint64_t                st_blocks;       // Number of 512-byte blocks
                                           // allocated
  struct PersistentHandle st_persistentHandle;
} fsi_stat_struct;


enum e_nfs_state {
  NFS_OPEN     = 1,
  NFS_CLOSE    = 2,
  CCL_CLOSING  = 4,
  CCL_CLOSE    = 8,

  IGNORE_STATE = 16
};

// ----------------------------------------------------------------------------
/// @struct file_handle_t
/// @brief  client file handle
// ----------------------------------------------------------------------------
struct file_handle_t {
  char                   m_filename[PATH_MAX];
                                              // full filename used with API
  int                    m_hndl_in_use;       // used to flag available entries
  int                    m_prev_io_op;        // enumerated I/O operation
                                              // (read/write/other I/O)
  struct io_buf_status_t m_writebuf_state[MAX_FSI_IPC_SHMEM_BUF_PER_STREAM *
                                          FSI_IPC_SHMEM_WRITEBUF_PER_BUF];
                                              // one entry per write data buffer
  int                    m_writebuf_cnt;      // how many write buffers this
                                              // handle actually uses
  int                    m_write_inuse_index; // index of the filling
                                              // write buffer (-1 if none)
  int                    m_write_inuse_bytes; // number of bytes in the
                                              // filling write buffer
  uint64_t               m_write_inuse_offset; // offset of first byte in
                                              // filling buffer
  struct io_buf_status_t m_readbuf_state [MAX_FSI_IPC_SHMEM_BUF_PER_STREAM *
                                          FSI_IPC_SHMEM_READBUF_PER_BUF];
                                              // one entry per read data buffer
  int                    m_readbuf_cnt;       // how many read buffers this
                                              // handle actually uses
  uint64_t               m_shm_handle[MAX_FSI_IPC_SHMEM_BUF_PER_STREAM];
                                              // SHM handle array
  int                    m_first_write_done;  // set if we are writing and first
                                              // write is complete
  int                    m_first_read_done;   // set if we completed first read
  int                    m_close_rsp_rcvd;    // IPC close file response
                                              // received
  int                    m_fsync_rsp_rcvd;    // IPC fsync file response
                                              // received

  int                    m_read_at_eof;       // set if at EOF - only for read
  uint64_t               m_file_loc;          // used for writes and fstat
                                              // this is the location assuming
                                              // last read or write succeeded
                                              // this is the location the next
                                              // sequential write (not pwrite)
                                              // would use as an offset
  uint64_t               m_file_flags;        // flags
  fsi_stat_struct        m_stat;
  uint64_t               m_fs_handle;         // handle
  uint64_t               m_exportId;          // export id
  int                    m_deferred_io_rc;    // deferred io return code

  int                    m_dir_not_file_flag; // set if this handle represents a
                                              // directory instead of a file
                                              // (open must issue opendir
                                              // if the entity being opened
                                              // is a directory)
  struct fsi_struct_dir_t * m_dirp;           // dir pointer
                                              // if m_dir_not_file_flag is
                                              // set
  uint64_t               m_resourceHandle;    // handle for resource management
//  pthread_mutex_t        m_io_mutex;          // used to manage multithread
//                                              // NFS io operations
  struct timeval         m_perf_pwrite_start[MAX_FSI_PERF_COUNT];
  struct timeval         m_perf_pwrite_end[MAX_FSI_PERF_COUNT];
  struct timeval         m_perf_aio_start[MAX_FSI_PERF_COUNT];
  struct timeval         m_perf_open_end;
  struct timeval         m_perf_close_end;
  uint64_t               m_perf_pwrite_count; // number of pwrite while open
  uint64_t               m_perf_pread_count;  // number of pread while open
  uint64_t               m_perf_aio_count;    // number of aio_force while open
  uint64_t               m_perf_fstat_count;  // number of fstat while open
  enum e_nfs_state       m_nfs_state;
  time_t                 m_last_io_time;      // Last time I/O was performed.
  int                    m_ftrunc_rsp_rcvd;
  uint64_t               m_eio_counter;       // number of EIOs encountered
  int                    m_sticky_rc;         // 'sticky' rc
  uint64_t               m_outstanding_io_count; // number of unfinished IOs
                                                 // on this handle
};

// ----------------------------------------------------------------------------
/// @struct file_handles_struct_t
/// @brief  contains filehandles
// ----------------------------------------------------------------------------
struct file_handles_struct_t {
  struct file_handle_t m_handle[FSI_MAX_STREAMS + FSI_CIFS_RESERVED_STREAMS];
  int                  m_count;              // maximum handle used
};

// ----------------------------------------------------------------------------
/// @struct fsi_struct_dir_t
/// @brief  fsi unique directory information
// ----------------------------------------------------------------------------
struct fsi_struct_dir_t {
  uint64_t m_dir_handle_index;
  uint64_t m_last_ino;       // last inode we responded with
  uint64_t m_exportId;
  char     dname[PATH_MAX];
  struct dirent dbuf;        // generic DIRENT buffer
};

// ----------------------------------------------------------------------------
/// @struct dir_handle_t
/// @brief  directory handle
// ----------------------------------------------------------------------------
struct dir_handle_t {
  int                     m_dir_handle_in_use; // used to flag available
                                               // entries
  uint64_t                m_fs_dir_handle;     // fsi_facade handle
  struct fsi_struct_dir_t m_fsi_struct_dir;    // directory struct
  uint64_t                m_resourceHandle;    // server resource handle
};

// ----------------------------------------------------------------------------
/// @struct dir_handles_struct_t
/// @brief  contains directory handles
// ----------------------------------------------------------------------------
struct dir_handles_struct_t {
  struct dir_handle_t m_dir_handle[FSI_MAX_STREAMS];
  int                 m_count;
};


// ----------------------------------------------------------------------------
/// @struct acl_handle_t
/// @brief  ACL handle
// ----------------------------------------------------------------------------
struct acl_handle_t {
  int        m_acl_handle_in_use; // used to flag available entries
  uint64_t   m_acl_handle;        // acl handle
  uint64_t   m_resourceHandle;    // server resource handle
};

// ----------------------------------------------------------------------------
/// @struct acl_handles_struct_t
/// @brief  contains ACL handles
// ----------------------------------------------------------------------------
struct acl_handles_struct_t {
  struct acl_handle_t m_acl_handle[FSI_MAX_STREAMS];
  int                 m_count;
};

// ----------------------------------------------------------------------------
// Structures for CCL abstraction
// ----------------------------------------------------------------------------
#define uint32 uint32_t

#define NONIO_MSG_TYPE \
  ((g_multithreaded) ? (unsigned long)pthread_self() : g_client_pid)
#define MULTITHREADED 1
#define NON_MULTITHREADED 0

// The context every call to CCL is made in
// THIS IS ALSO OFTEN REFERRED TO AS THE "CONTEXT"
typedef struct {
  uint64_t  export_id;    // export id
  uint64_t  uid;          // user id of the connecting user
  uint64_t  gid;          // group id of the connecting user
  char  client_address[256]; // address of client
  char    * export_path;  // export path name

  //TODO check on if the next fiels are used by fsal or ccl
  // next 2 fields left over from prototype -
  // do not use these if not already using
  const char * param;     // incoming parameter
  int   handle_index;     // Samba's File descriptor fsp->fh->fd
                          //  or essentially or index into our
                          // global g_fsi_handles.m_handle[] array
} ccl_context_t;


// ----------------------------------------------------------------------------
// End of defines new for CCL abstraction
// ----------------------------------------------------------------------------

// FSI IPC Statistics definitions
// ----------------------------------------------------------------------------
/// @struct ipc_client_stats_t
/// @brief  contains client statistics structure
// ----------------------------------------------------------------------------

// Statistics Logging interval of 5 minutes
#ifndef UNIT_TEST
#define FSI_IPC_CLIENT_STATS_LOG_INTERVAL 60 * 5
#else // UNIT_TEST
#define FSI_IPC_CLIENT_STATS_LOG_INTERVAL 2
#endif // UNIT_TEST

#define FSI_RETURN(result, handle)     \
{                                      \
  ccl_ipc_stats_logger(handle);        \
  return result;                       \
}

struct ipc_client_stats_t {
  uint64_t count;
  uint64_t sum;
  uint64_t sumsq;
  uint64_t min;
  uint64_t max;
  uint64_t overflow_flag;
};

#define VARIANCE(pstat)                                                 \
  ( (pstat)->count > 1  ?                                               \
    (((pstat)->sumsq - (pstat)->sum * ((pstat)->sum /(pstat)->count)) / \
      ((pstat)->count - 1)) :                                           \
    0 )

// ----------------------------------------------------------------------------
// Defines for CCL Internal Statistics
//
// Defines for IO Idle time statistic collection,  this is time we are
// idle waiting for user to send a read or write or doing other operations
// ----------------------------------------------------------------------------

#define START_IO_IDLE_CLOCK()                                                 \
{                                                                             \
  if (g_begin_io_idle_time.tv_sec != 0) {                                     \
    FSI_TRACE(FSI_ERR, "IDLE CLOCK was already started, distrust idle stat"); \
  }                                                                           \
  int rc = gettimeofday(&g_begin_io_idle_time, NULL);                         \
  if (rc != 0) {                                                              \
    FSI_TRACE(FSI_ERR, "gettimeofday rc = %d", rc);                           \
  }                                                                           \
}

#define END_IO_IDLE_CLOCK()                                                   \
{                                                                             \
  struct timeval curr_time;                                                   \
  struct timeval diff_time;                                                   \
  if (g_begin_io_idle_time.tv_sec == 0) {                                     \
    FSI_TRACE(FSI_ERR, "IDLE CLOCK already not running, distrust idle stat"); \
  }                                                                           \
  int rc = gettimeofday(&curr_time, NULL);                                    \
  if (rc != 0) {                                                              \
    FSI_TRACE(FSI_ERR, "gettimeofday rc = %d", rc);                           \
  } else {                                                                    \
    timersub(&curr_time, &g_begin_io_idle_time, &diff_time);                  \
    uint64_t delay = diff_time.tv_sec * 1000000 + diff_time.tv_usec;          \
    if (update_stats(&g_client_io_idle_time, delay)) {                        \
      FSI_TRACE(FSI_WARNING, "IO Idle time stats sum square overflow");       \
    }                                                                         \
  }                                                                           \
  memset(&g_begin_io_idle_time, 0, sizeof(g_begin_io_idle_time));             \
}

#define IDLE_STAT_READ_START()                                                \
{                                                                             \
  ccl_up_mutex_lock(&g_statistics_mutex);                                     \
  g_num_reads_in_progress++;                                                  \
  if (((g_num_reads_in_progress + g_num_writes_in_progress) == 1) &&          \
      (g_begin_io_idle_time.tv_sec != 0)) {                                   \
    END_IO_IDLE_CLOCK();                                                      \
  }                                                                           \
  ccl_up_mutex_unlock(&g_statistics_mutex);                                   \
}

#define IDLE_STAT_READ_END()                                                  \
{                                                                             \
  ccl_up_mutex_lock(&g_statistics_mutex);                                     \
  if (g_num_reads_in_progress == 0) {                                         \
    FSI_TRACE(FSI_ERR, "IO Idle read count off, distrust IDLE stat ");        \
  }                                                                           \
  g_num_reads_in_progress--;                                                  \
  if ((g_num_reads_in_progress + g_num_writes_in_progress) == 0) {            \
    START_IO_IDLE_CLOCK();                                                    \
  }                                                                           \
  ccl_up_mutex_unlock(&g_statistics_mutex);                                   \
}

#define IDLE_STAT_WRITE_START()                                               \
{                                                                             \
  ccl_up_mutex_lock(&g_statistics_mutex);                                     \
  g_num_writes_in_progress++;                                                 \
  if (((g_num_reads_in_progress + g_num_writes_in_progress) == 1) &&          \
      (g_begin_io_idle_time.tv_sec != 0)) {                                   \
    END_IO_IDLE_CLOCK();                                                      \
  }                                                                           \
  ccl_up_mutex_unlock(&g_statistics_mutex);                                   \
}

#define IDLE_STAT_WRITE_END()                                                 \
{                                                                             \
  ccl_up_mutex_lock(&g_statistics_mutex);                                     \
  if (g_num_writes_in_progress == 0) {                                        \
    FSI_TRACE(FSI_DEBUG, "IO Idle write count off, distrust IDLE stat ");     \
  }                                                                           \
  g_num_writes_in_progress--;                                                 \
  if ((g_num_reads_in_progress + g_num_writes_in_progress) == 0) {            \
    START_IO_IDLE_CLOCK();                                                    \
  }                                                                           \
  ccl_up_mutex_unlock(&g_statistics_mutex);                                   \
}

// ---------------------------------------------------------------------------
// End of CCL Internal Statistics defines
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Definitions for ACL structures for interface to ACL functions
//  - these are not used by NFS
//  - these are patterened after Sambe versions, Samba Shim layer should be
//    sure to translate these to Samba versions... for now they match Samba
//    but if Samba changes we need to detect
// ---------------------------------------------------------------------------
#define CCL_ACL_FIRST_ENTRY                     0
#define CCL_ACL_NEXT_ENTRY                      1

#define CCL_ACL_TYPE_ACCESS                     0
#define CCL_ACL_TYPE_DEFAULT                    1

// ---------------------------------------------------------------------------
// End of ACL definitions
// ---------------------------------------------------------------------------

// This determines how many times to poll when an existing opened handle
// that we are trying to reopen, but we are already closing that handle. 
// Open processing will poll until that handle is completely closed before
// opening it again. 
#define CCL_MAX_CLOSING_TO_CLOSE_POLLING_COUNT 480

// ---------------------------------------------------------------------------
// CCL Up Call ptorotypes - both the Samba VFS layer and the Ganesha PTFSAL
//     Layer provide a copy of these functions and CCL call them (up calls)
//     for the functions.
// ---------------------------------------------------------------------------
extern int ccl_up_mutex_lock(pthread_mutex_t *mutex);
extern int ccl_up_mutex_unlock(pthread_mutex_t *mutex);
extern unsigned long ccl_up_self();

// externs to globals the Samba VFS or Ganesha must provide
// dir handle mutex
extern pthread_mutex_t g_dir_mutex;
// acl handle mutex
extern pthread_mutex_t g_acl_mutex;
// file handle processing mutex
extern pthread_mutex_t g_handle_mutex;
// only one thread can parse an io at a time
extern pthread_mutex_t g_parseio_mutex;
// only one thread can change global transid at a time
extern pthread_mutex_t g_transid_mutex;
extern pthread_mutex_t g_non_io_mutex;
extern pthread_mutex_t g_statistics_mutex;
extern pthread_mutex_t g_close_mutex[FSI_MAX_STREAMS + FSI_CIFS_RESERVED_STREAMS];
// Global I/O mutex
extern pthread_mutex_t g_io_mutex;
#endif // ifndef __FSI_IPC_CCL_H__

#ifdef __cplusplus
} // extern "C"
#endif
