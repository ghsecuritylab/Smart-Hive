#define LOG_TAG    "app_task"

#include "app_task.h"
#include <stdlib.h>
#include <stdio.h>
#include <board.h>
#include <elog.h>
#include <elog_flash.h>
#include <easyflash.h>
#include <delay_conf.h>
#include <cpuusage.h>
#include <utils.h>
#include <shell.h>
#include <finsh.h>
#include <cm_backtrace.h>
#include <dfs_elm.h>
#include <dfs_fs.h>
#include <spi_flash.h>
#include <spi_flash_sfud.h>
#include <partition.h>

#define SOFTWARE_VERSION     "1.07.15"

static uint8_t cpu_usage_major, cpu_usage_minor;
static rt_uint32_t total_mem, used_mem, max_used_mem;

#define thread_sys_monitor_prio                30
ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t thread_sys_monitor_stack[512];
struct rt_thread thread_sys_monitor;
rt_spi_flash_device_t nor_flash;

static void rtt_user_assert_hook(const char* ex, const char* func, rt_size_t line);
static void elog_user_assert_hook(const char* ex, const char* func, size_t line);
static rt_err_t exception_hook(void *context);

/**
 * ϵͳ����߳�
 * @param parameter parameter
 */
void thread_entry_sys_monitor(void* parameter)
{
    while (1)
    {
        rt_memory_info(&total_mem, &used_mem, &max_used_mem);
//        rt_kprintf("Total_Mem:%ld  Used_Mem:%ld  Max_Used_Mem:%ld\n",total_mem,used_mem,max_used_mem);
        IWDG_Feed(); //ι��
    }
}

/**
 * ϵͳ��ʼ���߳�
 * @param parameter parameter
 */
void sys_init_thread(void* parameter){
    set_system_status(SYSTEM_STATUS_INIT);

#ifdef RT_USING_COMPONENTS_INIT
    /* RT-Thread �����ʼ�� */
    rt_components_init();
#endif

#if defined(RT_USING_FINSH) && !defined(RT_USING_POSIX)
    /* ���� finsh ����̨�豸���� */
    finsh_set_device(RT_CONSOLE_DEVICE_NAME);
#endif

    /* ��ʼ�� nor_flash Flash �豸 */
    if ((nor_flash = rt_sfud_flash_probe("nor_flash", "spi20")) == NULL) {
        /* Flash ��ʼ��ʧ�� */
        set_system_status(SYSTEM_STATUS_FAULT);
        return;
    }

    /* ��ʼ�� Flash �ϵĸ��� ���� */
    extern int rt_partition_init(const char* flash_device, const struct rt_partition* parts, rt_size_t num);
    static const struct rt_partition partition[] = RT_PARTITION_DEFAULT_TABLE;
    rt_partition_init("nor_flash", partition, sizeof(partition) / sizeof(struct rt_partition));

    /* ��ʼ�� EasyFlash ģ�� */
    easyflash_init();
    
    /* ��ʼ����־ϵͳ */
    elog_init();
    elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_ALL & ~ELOG_FMT_P_INFO);
    elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_ALL & ~ELOG_FMT_P_INFO);
    elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_ALL & ~ELOG_FMT_P_INFO);
    elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_ALL & ~(ELOG_FMT_FUNC | ELOG_FMT_P_INFO));
    elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_ALL & ~(ELOG_FMT_FUNC | ELOG_FMT_P_INFO));

#ifdef DEBUG
    elog_set_filter_lvl(ELOG_LVL_VERBOSE);
#elif RELEASE
    elog_set_filter_lvl(ELOG_LVL_INFO);
#endif

#ifdef ELOG_COLOR_ENABLE
    elog_set_text_color_enabled(true);
#endif
    /* ��ʼ��EasyLogger��Flash��� */
    elog_flash_init();
    /* ����EasyLogger */
    elog_start();
    
    extern void flash_read_normalized(void); //��ʼ����ȡ ���ز���
    flash_read_normalized(); 
    
    /* ����EasyLogger���Թ��� */
    elog_assert_set_hook(elog_user_assert_hook);

    /* ����Ӳ���쳣���� */
    rt_hw_exception_install(exception_hook);

    /* ����RTT���Թ��� */
    rt_assert_set_hook(rtt_user_assert_hook);

    /* CmBacktrace �����ʼ�� */
    cm_backtrace_init("ART-6LoWPAN", HARDWARE_VERSION, SOFTWARE_VERSION);

    /* ���� nor-flash �� fatfs ������Ϊ��Ŀ¼ */
    if (dfs_mount("fatfs", "/", "elm", 0, 0) == 0) {
        log_i("File System initialized!");
    } else {
        log_e("File System initialization failed!");
    }

    extern void contiki_init(void);
    contiki_init();

    set_system_status(SYSTEM_STATUS_RUN);

    log_i("ART-6LoWPAN initialize success. Firmware version is %s", SOFTWARE_VERSION);
}

static void rtt_user_assert_hook(const char* ex, const char* func, rt_size_t line) {
    rt_enter_critical();

#ifdef ELOG_ASYNC_OUTPUT_ENABLE
    elog_async_enabled(false);
#endif

    elog_output_lock_enabled(false);
    elog_flash_lock_enabled(false);
    elog_a("rtt", "(%s) has assert failed at %s:%ld.", ex, func, line);
    cm_backtrace_assert(cmb_get_sp());
    elog_flash_flush();
    while(1);
}

static void elog_user_assert_hook(const char* ex, const char* func, size_t line) {
    rt_enter_critical();

#ifdef ELOG_ASYNC_OUTPUT_ENABLE
    elog_async_enabled(false);
#endif

    elog_output_lock_enabled(false);
    elog_flash_lock_enabled(false);
    elog_a("elog", "(%s) has assert failed at %s:%ld.\n", ex, func, line);
    cm_backtrace_assert(cmb_get_sp());
    elog_flash_flush();
    while(1);
}

static rt_err_t exception_hook(void *context) {
    extern long list_thread(void);
    uint8_t _continue = 1;

    rt_enter_critical();

#ifdef ELOG_ASYNC_OUTPUT_ENABLE
    elog_async_enabled(false);
#endif

    elog_output_lock_enabled(false);
    elog_flash_lock_enabled(false);

#ifdef RT_USING_FINSH
    list_thread();
#endif

    cm_backtrace_fault(*((uint32_t *)(cmb_get_sp() + sizeof(uint32_t) * 8)), cmb_get_sp() + sizeof(uint32_t) * 9);
    elog_flash_flush();
    while (_continue == 1);

    return RT_EOK;
}

/**
 * RT-Thread �� kprintf ��ӡ��Ϣ����ĵײ�ӿ�
 *
 * @param str ��ӡ��Ϣ
 */
void rt_hw_console_output(const char *str) {
    extern void output_log_to_console_or_flash(bool console, bool flash, const char *log, size_t size);
    /* ͬʱ�����ӡ��Ϣ������̨�� flash �� */
    output_log_to_console_or_flash(true, true, str, rt_strlen(str));
}

int rt_application_init(void)
{
    rt_thread_t thread = NULL;
    rt_thread_init(&thread_sys_monitor,
                   "sys_monitor",
                   thread_entry_sys_monitor,
                   RT_NULL,
                   thread_sys_monitor_stack,
                   sizeof(thread_sys_monitor_stack),
                   thread_sys_monitor_prio, 5);
    rt_thread_startup(&thread_sys_monitor);

    thread = rt_thread_create("sys init", sys_init_thread, NULL, 2048, 10, 10);
    if (thread != NULL) {
        rt_thread_startup(thread);
    }

    return 0;
}

/**
 * This function will startup RT-Thread RTOS.
 */
void rtthread_startup(void)
{
    /* init board */
    rt_hw_board_init();

    /* show version */
    rt_show_version();

    /* init tick */
    rt_system_tick_init();

    /* init kernel object */
    rt_system_object_init();

    /* init timer system */
    rt_system_timer_init();

#ifdef RT_USING_HEAP
    /* init memory system */
    rt_system_heap_init((void*)STM32_SRAM_BEGIN, (void*)STM32_SRAM_END);
#endif

    /* init scheduler system */
    rt_system_scheduler_init();

    /* initialize timer */
    rt_system_timer_init();

    /* init timer thread */
    rt_system_timer_thread_init();

    /* Add CPU usage to system */
    cpu_usage_init();

    /* init application */
    rt_application_init();

    /* init idle thread */
    rt_thread_idle_init();

    /* start scheduler */
    rt_system_scheduler_start();

    /* never reach here */
    return;
}
void sys_reboot_entry(void* parameter)
{
    static rt_uint32_t sys_cnt = 0;
    
    while(1)
    {
        rt_thread_delay(RT_TICK_PER_SECOND);   //1s      
        sys_cnt++;
        if(sys_cnt >= 60*60*4) //6Сʱ   1-5    2-10    3-20    4-30     5-1h
        { 
            sys_cnt = 0;
            NVIC_SystemReset();
            
        }
    }
  
}

int sys_reboot_thread_init(void)
{
  
    rt_thread_t reboot_tid;
		/*������̬�߳� */
    reboot_tid = rt_thread_create("reboot",//�߳�����
                    sys_reboot_entry,		
                    RT_NULL,					
                    1024,						
                    30,										 	
                    10);										 

    if (reboot_tid != RT_NULL){				
        rt_thread_startup(reboot_tid);
    }
    return 0;
}
INIT_APP_EXPORT(sys_reboot_thread_init);

int main(void) {
    /* disable interrupt first */
    rt_hw_interrupt_disable();

    /* startup RT-Thread RTOS */
    rtthread_startup();

    return 0;
}
