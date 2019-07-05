/* RT-Thread config file */
#ifndef __RTTHREAD_CFG_H__
#define __RTTHREAD_CFG_H__

/* RT_NAME_MAX*/
#define RT_NAME_MAX	24

/* RT_ALIGN_SIZE*/
#define RT_ALIGN_SIZE	4

/* PRIORITY_MAX */
#define RT_THREAD_PRIORITY_MAX	32

/* Tick per Second */
#define RT_TICK_PER_SECOND	    1000

/* SECTION: RT_DEBUG */
/* Thread Debug */
#define RT_DEBUG
#define RT_THREAD_DEBUG

#define RT_USING_OVERFLOW_CHECK

/* Using Hook */
#define RT_USING_HOOK

/* Using Software Timer */
#define RT_USING_TIMER_SOFT
#define RT_TIMER_THREAD_PRIO		4
#define RT_TIMER_THREAD_STACK_SIZE	512
#define RT_TIMER_TICK_PER_SECOND	RT_TICK_PER_SECOND

/* SECTION: IPC */
/* Using Semaphore*/
#define RT_USING_SEMAPHORE

/* Using Mutex */
#define RT_USING_MUTEX

/* Using Event */
#define RT_USING_EVENT

/* Using MailBox */
#define RT_USING_MAILBOX

/* Using Message Queue */
#define RT_USING_MESSAGEQUEUE

/* SECTION: Memory Management */
/* Using Memory Pool Management*/
#define RT_USING_MEMPOOL

/* Using Dynamic Heap Management */
#define RT_USING_HEAP

/* Using Small MM */
#define RT_USING_SMALL_MEM

// <bool name="RT_USING_COMPONENTS_INIT" description="Using RT-Thread components initialization" default="true" />
#define RT_USING_COMPONENTS_INIT

/* SECTION: Device System */
/* Using Device System */
#define RT_USING_DEVICE
// <bool name="RT_USING_DEVICE_IPC" description="Using device communication" default="true" />
#define RT_USING_DEVICE_IPC
// <bool name="RT_USING_SERIAL" description="Using Serial" default="true" />
#define RT_USING_SERIAL
#define RT_SERIAL_RB_BUFSZ 1024
// <bool name="RT_USING_RTC" description="Using RTC" default="true" />
#define RT_USING_RTC
// <bool name="RT_USING_SPI" description="Using SPI Device Driver Framework" default="true" />
#define RT_USING_SPI
// <bool name="RT_USING_PIN" description="Using generic GPIO device drivers" default="true" />
#define RT_USING_PIN

/* SECTION: Console options */
#define RT_USING_CONSOLE
/* the buffer size of console*/
#define RT_CONSOLEBUF_SIZE          1024
// <string name="RT_CONSOLE_DEVICE_NAME" description="The device name for console" default="uart1" />
#if (CONTIKI_SLIP_RAIDO == 0)
#define RT_CONSOLE_DEVICE_NAME      "uart1"
#else
#define RT_CONSOLE_DEVICE_NAME      "uart2"
#endif /* CONTIKI_SLIP_RAIDO */

/* SECTION: finsh, a C-Express shell */
#define RT_USING_FINSH
/* Using symbol table */
#define FINSH_USING_SYMTAB
#define FINSH_USING_DESCRIPTION
/* Using msh and finsh */
#define FINSH_USING_MSH
/* Shell default using MSH mode */
#define FINSH_USING_MSH_DEFAULT
/* command line max length */
#define FINSH_CMD_SIZE 256
/* finsh thread stack size */
#define FINSH_THREAD_STACK_SIZE 4096
/* Only enabled when firmware is released */
#ifdef RELEASE
/* Using password authentication */
#define FINSH_USING_AUTH
#define FINSH_DEFAULT_PASSWORD "61866139"
#endif /* RELEASE */

/* SECTION: device filesystem */
#define RT_USING_DFS
#define RT_USING_DFS_DEVFS
/* support fatfs */
#define RT_USING_DFS_ELMFAT
/* make sure only one thread can WR in one time, fix reentrant problem */
#define RT_DFS_ELM_REENTRANT
#define RT_DFS_ELM_WORD_ACCESS
/* using working directory */
#define DFS_USING_WORKDIR
/* number of volumes to be used */
#define RT_DFS_ELM_DRIVES           2
/* long file name.
    mode 1: use static buffer, may cause reentry problem
    mode 2: use stack, need large stack
    mode 3: use heap, the most safe one */
#define RT_DFS_ELM_USE_LFN          3
#define RT_DFS_ELM_MAX_LFN          255
#define RT_DFS_ELM_CODE_PAGE        437
/* need meet flash data sheet spec. One page = 4096 Bytes */
#define RT_DFS_ELM_MAX_SECTOR_SIZE          4096
/* the max number of mounted filesystem */
#define DFS_FILESYSTEMS_MAX         2
/* the max number of opened files       */
#define DFS_FD_MAX              4

/* SECTION: Serial Flash Universal Driver */
#define RT_USING_SFUD
//#define RT_SFUD_DEBUG
#define RT_SFUD_USING_SFDP
//#define RT_SFUD_USING_FLASH_INFO_TABLE

/* SECTION: C Runtime library setting */
// <bool name="RT_USING_LIBC" description="Using C library" default="true" />
#define RT_USING_LIBC
#define RT_USING_POSIX
// <bool name="RT_USING_PTHREADS" description="Using POSIX threads library" default="true" />
//#define RT_USING_PTHREADS

/* Using Real-YModem */
#define RT_USING_RYM

#endif
