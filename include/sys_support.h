#ifndef _SYS_SUPPORT_H
#define _SYS_SUPPORT_H

#include <define.h>

// 系统调用
extern data u16 events;

void display_next(void);

// 以下用于task注册

void seg_led_init(void);
void button_init(void);
void events_awake_scheduler_task(void);
void time1ms_task(void);

// 以下提供给其它task调用修改

/*
 * os_event_wait 函数是对 os_wait 的进一步封装，用于等待指定事件
 * 参数说明如下：events 表示事件类型，允许使用 `|` 来同时等待多个事件，事件表如下
 * - `EVENT_TIME1S`，等待 1s 后唤醒，小于 256ms 请用 os_wait
 * - `EVENT_KEY1_PRESS`，等待 key1 按下后唤醒
 * - `EVENT_KEY1_RELEASE`，等待 key1 松开后唤醒
 * - `EVENT_KEY2_PRESS`，等待 key2 按下后唤醒
 * - `EVENT_KEY2_RELEASE`，等待 key2 松开后唤醒
 */
void os_event_wait(u16 events);

/*
 * single_seg_print 函数用于向指定的数码管打印具体编码的字符
 * 参数说明如下：num 表示修改的目标数码管编号，从左到右依次为 0-7；displaychar 表示为修改后要显示的字符
 */
void single_seg_print(u8 num, u8 displaychar);
/*
 * all_seg_print 函数用于修改所有数码管显示
 * 参数说明如下：displaychars 表示为修改后要显示的字符数组，长度指定为8
 */
void all_seg_print(u8 *displaychars);
/*
 * led_print 函数用于修改 led 灯组显示
 * 参数说明如下：displaychar 为二进制表示的 led 显示
 */
void led_print(u8 displaychar);

#endif