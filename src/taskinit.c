#include <rtx51tny.h>
#include <sys_support.h>

u8 hello[8] = {0x76, 0x79, 0x38, 0x38, 0x5c, 0, 0x6d, 0x06};

void task_init(void) _task_ INIT
{
    seg_led_init();
    button_init();

    led_print(0);
    all_seg_print(hello);

    // SYSTEM PROCESS TASK REGISTER
    os_create_task(SCHEDULER);
    os_create_task(TIME1MS);

    // USER PROCESS TASK REGISTER
    os_create_task(3);

    os_delete_task(INIT);
}

// SYSTEM PROCESS BEGIN

void scheduler(void)    _task_ SCHEDULER    { events_awake_scheduler_task(); }
void time1ms(void)      _task_ TIME1MS      { time1ms_task();                }

// SYSTEM PROCESS END

// USER PROCESS BEGIN
// You can program it to run like processes, rather than describing it in a state machine manner.

void scheduler_test(void) _task_ 3
{
    char cnt = 0;
    while (1) {
        os_event_wait(EVENT_TIME1S | EVENT_KEY1_RELEASE | EVENT_KEY2_PRESS);
        led_print(++cnt);
    }
}

// USER PROCESS END