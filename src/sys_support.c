#include <STC15F2K60S2.H>
#include <rtx51tny.h>

#include <sys_support.h>

#define KEY_ANTISHAKE_MS 20
unsigned char k1_press_flag = 0;
unsigned char k1_press_timing = 0;
unsigned char k1_release_flag = 0;
unsigned char k1_release_timing = 0;
unsigned char k2_press_flag = 0;
unsigned char k2_press_timing = 0;
unsigned char k2_release_flag = 0;
unsigned char k2_release_timing = 0;

// SCHEDULER

data u16 events = 0;
data u16 event_wait_list[16] = {0}; // 用于存放event正在wait的task表

void os_event_wait(u16 events)
{
    // 理论上来讲应该有一个timeout的候选唤醒，但是os_wait u8的类型就算了吧
    u8 pid = os_running_task_id();
    u8 events_index;
    for (events_index = 0; events_index < 16; ++events_index) {
        if ((events >> events_index) & 1) {
            event_wait_list[events_index] |= 1 << pid;
        }
    }
    os_wait1(K_SIG);
}

void events_awake_scheduler_task(void)
{
    u8 pid;
    u8 events_index = 0;
    while (1) {
        if (event_wait_list[events_index] && ((events >> events_index) & 1)) {
            for (pid = 0; pid < 16; ++pid) {
                if ((event_wait_list[events_index] >> pid & 1)) {
                    event_wait_list[events_index] &= ~(1 << pid);
                    events &= ~(1 << events_index);
                    os_send_signal(pid);
                    break;
                }
            }
        }
        if (++ events_index > 15) events_index = 0;
        os_switch_task();
    }
}

// TIMS

void time1ms_task(void)
{
    u16 cnt = 0;
    while (1) {
        display_next();
        if (++cnt > 999){
            cnt = 0;
            events |= EVENT_TIME1S;
        }

        // 按键防抖
        if(k1_press_flag == 1){
            // k1按下
            if(k1_press_timing < KEY_ANTISHAKE_MS)
                k1_press_timing++;
            else if(P32 == 0){
                k1_press_flag = 0;
                events |= EVENT_KEY1_PRESS;
            }
        }
        if(k1_release_flag == 1){
            // k1松开
            if(k1_release_timing < KEY_ANTISHAKE_MS)
                k1_release_timing++;
            else if(P32 == 1){
                k1_release_flag = 0;
                events |= EVENT_KEY1_RELEASE;
            }
        }
        if(k2_press_flag == 1){
            // k2按下
            if(k2_press_timing < KEY_ANTISHAKE_MS)
                k2_press_timing++;
            else if(P33 == 0){
                k2_press_flag = 0;
                events |= EVENT_KEY2_PRESS;
            }
        }
        if(k2_release_flag == 1){
            // k2松开
            if(k2_release_timing < KEY_ANTISHAKE_MS) k2_release_timing++;
            else if(P33 == 1){
                k2_release_flag = 0;
                events |= EVENT_KEY2_RELEASE;
            }
        }

        os_wait(K_IVL, 1, 0);
    }
}

// DISPLAY

data u8 seg_display_buf[8] = {0,0,0,0,0,0,0,0};
data u8 led_display_buf    = 0;

void seg_led_init(void)
{
    // P0、P2口全部设为推挽输出
    P0M1 = 0x00;
    P0M0 = 0xff;
    P2M1 &= 0xf0;
    P2M0 |= 0x0f;
}

void display_next(void)
{
    static u8 display_pos;
    P23 = ~P23;
    if (P23 == 0) {
        P0 = seg_display_buf[display_pos];
        P20 = display_pos & 0x1;
        P21 = (display_pos & 0x2) >> 1;
        P22 = (display_pos & 0x4) >> 2;
    }
    else {
        P0 = led_display_buf;
        if (++display_pos > 7) display_pos = 0;
    }
}

void single_seg_print(u8 num, u8 displaychar)
{
    seg_display_buf[num] = displaychar;
}

void all_seg_print(u8 *displaychars)
{
    seg_display_buf[7] = displaychars[7];
    seg_display_buf[6] = displaychars[6];
    seg_display_buf[5] = displaychars[5];
    seg_display_buf[4] = displaychars[4];
    seg_display_buf[3] = displaychars[3];
    seg_display_buf[2] = displaychars[2];
    seg_display_buf[1] = displaychars[1];
    seg_display_buf[0] = displaychars[0];
}

void led_print(u8 displaychar)
{
    led_display_buf = displaychar;
}

// BUTTON (USING INTERRUPT TO TEST)

void button_init(void)
{
    // key1, key2
    EX1 = EX0 = EA = 1;
    PX0 = PX1 = 0;
    IE0 = IE1 = 0;          // 清除中断标志位
    IT0 = IT1 = 0;          // 0上下降沿均触发，1下降沿触发
    P3M1 |= 0x0c;
    P3M0 &= 0xf3;
}

void Int0_Routine() interrupt 0
{
    if(P32 == 0){
        k1_press_flag = 1;
        k1_press_timing = 0;
    }
    else if(P32 == 1){
        k1_release_flag = 1;
        k1_release_timing = 0;
    }
}

void Int1_Routine() interrupt 2{
    if(P33 == 0){
        k2_press_flag = 1;
        k2_press_timing = 0;
    }
    else if(P33 == 1){
        k2_release_flag = 1;
        k2_release_timing = 0;
    }
}