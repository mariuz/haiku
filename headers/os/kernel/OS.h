/* OS.h
 */

#ifndef _OS_H
#define _OS_H

/**
 * @mainpage OpenBeOS Header Documentation
 *
 * @section Introduction
 * This is the header documentation as produced with doxygen. Anyone can
 * produce this using the doxygen tool and doing this
 * <pre>
 *	cd docs
 *	doxygen doxygen.conf
 * </pre>
 * The resulting files will be produced in dox/html.
 *
 * @section Updating
 * I will attempt to keep these up to date, but if they're not, just
 * give me a nudge!
 */

/**
 * @file kernel/OS.h
 * @brief Definitions, prototypes needed throughout the OS
 */

/**
 * @defgroup OpenBeOS_Headers OpenBeOS System Headers
 * @brief Headers that are available for applications
 */
 
#ifdef __cplusplus
extern "C" {
#endif

#include <ktypes.h>

/**
 * @defgroup Sys_Consts Constants
 * @ingroup OpenBeOS_Headers
 * @brief Constants defined by the system headers
 * @note these are prefixed B_ for compatability with BeOS
 * @{
 */
/**
 * @def B_OS_NAME_LENGTH
 * the maximum length of names applied to various structures
 * for example the maximum length of a thread name supplied in
 * a spawn_thread() call is determined by this value
 */
/** 
 * @def B_PAGE_SIZE
 *size of a single page of memory.
 * @note this is also the smallest area that can be requested
 */

#define B_OS_NAME_LENGTH      32
#define B_PAGE_SIZE         4096

/** @def B_CAN_INTERRUPT
 * if set the call can be interrupted
 */
/** @def B_DO_NOT_RESCHEDULE
 * explanation reqd
 */
/** @def B_CHECK_PERMISSION
 * when set the ownership will be checked, and if permission
 * is not available then the call will fail.
 */
/** @def B_TIMEOUT
 * there is a timeout given in this call
 */
/** @def B_RELATIVE_TIMEOUT
 * the timeout given in this call is relative to the time now
 */
/** @def B_ABSOLUTE_TIMEOUT
 * the timeout given in this call is a time that the timeout will expire
 */
#define B_CAN_INTERRUPT          1
#define B_DO_NOT_RESCHEDULE      2
#define B_CHECK_PERMISSION       4
#define B_TIMEOUT                8
#define	B_RELATIVE_TIMEOUT       8
#define B_ABSOLUTE_TIMEOUT      16

/** @} */

/**
 * @defgroup Sys_types Types
 * @ingroup OpenBeOS_Headers
 * @brief Definitions of basic system types.
 * @note these are not in sys/types.h as they are specific to
 *       this system
 * @note these are global to the system.
 * @{
 */

/** 
 * @typedef team_id 
 * id of an team (process) 
 */
/**
 * @typedef area_id
 * id of an area
 */

typedef int32   team_id;
typedef int32   area_id;

/** @} */

/**
 * @defgroup Areas Memory Areas
 * @brief memory areas
 * @ingroup OpenBeOS_Headers
 * @{
 */

/* Areas */
/**
 * @struct area_info
 * gives details about a particular area
 */
typedef struct area_info {
	area_id		area;
	char		name[B_OS_NAME_LENGTH];
	int foo;
	size_t		size;
	uint32		lock;
	uint32		protection;
	team_id		team;
	uint32		ram_size;
	uint32		copy_count;
	uint32		in_count;
	uint32		out_count;
	void		*address;
} area_info;

#define get_area_info(id, ainfo) \
        _get_area_info((id), (ainfo),sizeof(*(ainfo)))
#define get_next_area_info(team, cookie, ainfo) \
        _get_next_area_info((team), (cookie), (ainfo), sizeof(*(ainfo)))

/** @} */

/**
 * @defgroup Ports Messaging Ports
 * @brief A system wide messaging system
 * @ingroup OpenBeOS_Headers
 * @{
 */

/* Ports */
/**
 * @struct port_info
 * details about a port, including ownership
 */
typedef struct port_info {
	port_id		port;
	team_id		team;
	char		name[B_OS_NAME_LENGTH];
	int32		capacity; /* queue depth */
	int32		queue_count; /* # msgs waiting to be read */
	int32		total_count; /* total # msgs read so far */
} port_info;

port_id	create_port(int32, const char *);
port_id	find_port(const char *);
int     read_port(port_id, int32 *, void *, size_t);
int     read_port_etc(port_id, int32 *, void *, size_t, uint32, bigtime_t);
int     write_port(port_id, int32, const void *, size_t);
int     write_port_etc(port_id, int32, const void *, size_t, uint32, bigtime_t);
int     close_port(port_id port);
int     delete_port(port_id port);

ssize_t	port_buffer_size(port_id);
ssize_t	port_buffer_size_etc(port_id, uint32, bigtime_t);
ssize_t	port_count(port_id);
int     set_port_owner(port_id, team_id);

int     _get_port_info(port_id, port_info *, size_t);
int     _get_next_port_info(team_id, int32 *, port_info *, size_t);

#define get_port_info(port, info)    \
             _get_port_info((port), (info), sizeof(*(info)))
	
#define get_next_port_info(team, cookie, info)   \
	         _get_next_port_info((team), (cookie), (info), sizeof(*(info)))

/** @} */

/**
 * @defgroup Sems Semaphores
 * @brief used to provide synchronisation between threads
 * @ingroup OpenBeOS_Headers
 * @{
 */


/**
 * @struct sem_info
 * information on a semaphore
 */
typedef struct sem_info {
	sem_id		sem;
	proc_id		proc;
	char		name[B_OS_NAME_LENGTH];
	int32		count;
	thread_id	latest_holder;
} sem_info;

sem_id create_sem_etc(int count, const char *name, proc_id owner);
sem_id create_sem(int count, const char *name);
int    delete_sem(sem_id id);
int    delete_sem_etc(sem_id id, int return_code);
int    acquire_sem(sem_id id);
int    acquire_sem_etc(sem_id id, int count, int flags, bigtime_t timeout);
int    release_sem(sem_id id);
int    release_sem_etc(sem_id id, int count, int flags);
int    get_sem_count(sem_id id, int32* thread_count);
int    _get_sem_info(sem_id id, struct sem_info *info, size_t);
int    _get_next_sem_info(proc_id proc, uint32 *cookie, struct sem_info *info, size_t);
int    set_sem_owner(sem_id id, proc_id proc);

#define get_sem_info(sem, info)                \
            _get_sem_info((sem), (info), sizeof(*(info)))
	
#define get_next_sem_info(team, cookie, info)  \
            _get_next_sem_info((team), (cookie), (info), sizeof(*(info)))

/** @} */

/**
 * @defgroup Threads Threads
 * @brief a distinct, independantly executing task belong to a team 
 * @ingroup OpenBeOS_Headers
 * @{
 */

/* Threads */

//enum {
//	THREAD_STATE_READY = 0,   // ready to run
//	THREAD_STATE_RUNNING, // running right now somewhere
//	THREAD_STATE_WAITING, // blocked on something
//	THREAD_STATE_SUSPENDED, // suspended, not in queue
//	THREAD_STATE_FREE_ON_RESCHED, // free the thread structure upon reschedule
//	THREAD_STATE_BIRTH	// thread is being created
//};

typedef enum {
	B_THREAD_RUNNING=1,
	B_THREAD_READY,
	B_THREAD_RECEIVING,
	B_THREAD_ASLEEP,
	B_THREAD_SUSPENDED,
	B_THREAD_WAITING
} thread_state;

#define THREAD_IDLE_PRIORITY 0

#define THREAD_NUM_PRIORITY_LEVELS 64
#define THREAD_MIN_PRIORITY    (THREAD_IDLE_PRIORITY + 1)
#define THREAD_MAX_PRIORITY    (THREAD_NUM_PRIORITY_LEVELS - THREAD_NUM_RT_PRIORITY_LEVELS - 1)

#define THREAD_NUM_RT_PRIORITY_LEVELS 16
#define THREAD_MIN_RT_PRIORITY (THREAD_MAX_PRIORITY + 1)
#define THREAD_MAX_RT_PRIORITY (THREAD_NUM_PRIORITY_LEVELS - 1)

#define THREAD_LOWEST_PRIORITY    THREAD_MIN_PRIORITY
#define THREAD_LOW_PRIORITY       12
#define THREAD_MEDIUM_PRIORITY    24
#define THREAD_HIGH_PRIORITY      36
#define THREAD_HIGHEST_PRIORITY   THREAD_MAX_PRIORITY

#define THREAD_RT_LOW_PRIORITY    THREAD_MIN_RT_PRIORITY
#define THREAD_RT_HIGH_PRIORITY   THREAD_MAX_RT_PRIORITY

#define B_LOW_PRIORITY						5
#define B_NORMAL_PRIORITY					10
#define B_DISPLAY_PRIORITY					15
#define	B_URGENT_DISPLAY_PRIORITY			20
#define	B_REAL_TIME_DISPLAY_PRIORITY		100
#define	B_URGENT_PRIORITY					110
#define B_REAL_TIME_PRIORITY				120

/** information on a thread
 * @note the thread can be in any state
 */
typedef struct  {
	thread_id		thread;
	team_id			team;
	char			name[B_OS_NAME_LENGTH];
	thread_state	state;
	int32			priority;
	sem_id			sem;
	bigtime_t		user_time;
	bigtime_t		kernel_time;
	void			*stack_base;
	void			*stack_end;
} thread_info;

/** 
 * gives information on user and kernel time for a thread
 */
typedef struct {
	bigtime_t		user_time;
	bigtime_t		kernel_time;
} team_usage_info;

/** @typedef thread_func
 * the prototype for a function passed in a spawn_thread() call is
 * @code
 *  int32 some_thread_func(void *data)
 *  {
 *     ...
 *  }
 * @endcode
 */
typedef int32 (*thread_func) (void *);

/** @fn thread_id spawn_thread(thread_func func, const char *name, int32 priority, void *data)
 * creates a new thread within a team
 * @note the new thread will be created in the suspended state and will not run
 *       until a resume_thread() call is issued
 * @note the maximum length of name is B_OS_NAME_LENGTH characters
 */
thread_id spawn_thread (thread_func, const char *, int32, void *);
int       kill_thread(thread_id thread);
int       resume_thread(thread_id thread);
int       suspend_thread(thread_id thread);

thread_id find_thread(const char *);

/** @} */
#ifdef __cplusplus
}
#endif

#endif /* _OS_H */

