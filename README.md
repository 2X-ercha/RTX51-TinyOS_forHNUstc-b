# RTX51 TinyOS 对 hnu stc-b 单片机开发板的适配测试

## 工程目标

主要完成对 `RTOS` 在 c51 单片机上的适配情况。由于 `RTX51 TinyOS` 实质上只是一个对任务的调度系统，众多外设驱动没办法像 stm32 那用有专门的工具做外设接口封装，实际上的测试还是得回到驱动编写上。

而我目前主要在三个方面做了测试，一是最基本的 task 调度测试，由于没有显示来验证结果，这个测试也与 GPIO驱动、定时等功能测试一并进行，三是中断测试（这里选取了最简单的按键中断）。编写测试时遇到的问题将在下面说明。

功能上，我差不多是对该 RTOS 的进一步适配封装，在任务中外加了一层事件检测，使任务能够脱离单片机 bsp 的状态机式编程，从而回到 OS 的初心——进程式编程。

## 编写过程

由于调试，我还是从数码管驱动入手。传统的 bsp 编写，我们会将数码管的位选信号交给时钟中断函数处理，让时钟“嘀嗒”的时候进行一次位切换。以 1ms 的任务切换时间来讲完全是能满足人眼的视觉暂留的。而现在位选将以任务的形式加入程序中，任务在一次时间中断间隔的时间片内，显示驱动将一直占据该时间片。过快的时间切换将使段选信号来不及更新，故而需要使其像传统时间中断一样，需要加入 `os_wait(K_IVL, 1, 0);` 或者 `os_switch_task()` 使其主动放弃时间片：

```c
void display_task(void) _task_ DISPLAY
{
    u8 display_pos = 0;
    while (1) {
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
        // 此处使其主动放弃时间片
        os_switch_task();
    }
}
```

由于最开始就想拆成多文件，发现无法跨文件调用 task 。c51 的编译貌似没办法在跨文件时识别 `_task_` 标识，所以将在代码中看见一些类似于 `void xxx(void) _task_ { xxx_task(); }` 的代码（尽管我认为这很不优雅，但被迫如此）。

而后编写事件等待的封装时，我将等待唤醒的调度代码也封装为了一个单独的任务。但是这个任务却使显示的刷新率下降，肉眼可见其位选轮换。经过测试，问题在于该任务单独占用了一整个时间片而使显示无法正常 1ms 内能至少执行一次。故而有如下建议，**在不是十分要求一次性处理的死循环任务时，请在循环内至少主动放弃时间片一次**，例如：

```c
void xxx_task(void) _task_ PID
{
    // ...
    while (1) {
        // ...
        os_switch_task();
    }
}
```

## 架构细节

这里使用一个事件系统来满足任务对具体事件的等待，如在某一个任务中使用如下代码：

```c
os_event_wait(EVENT_TIME1S | EVENT_KEY1_RELEASE | EVENT_KEY2_PRESS);
```

任务将在 1s 或 key1 抬起 或 key2 按下 三种情况被重新唤醒，实质上是对 os_wait 的进一步封装。

而在专门处理事件系统的任务 `events_awake_scheduler_task` 运行时，将对事件表进行轮询。若某事件发送且其被某任务等待则唤醒对应任务：

```c
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
```

不过这里的设计并未很好的处理饿死等情况，暂时实现的比较草率。原始 RTX51 TinyOS 提供的进程系统调用也能使用。

## 接口汇总

所有的接口都在 `sys_support.h` 中被定义，在编写任务时只需引入`<rtx51tny.h>` 和 `<sys_support.h>`

除了原生提供的系统调用外，这里还新增了事件等待函数：

```c
void os_event_wait(u16 events);
```

`os_event_wait` 函数是对 os_wait 的进一步封装，用于等待指定事件

events 表示事件类型，允许使用 `|` 来同时等待多个事件，事件表如下

- `EVENT_TIME1S`，等待 1s 后唤醒，小于 256ms 请用 os_wait
- `EVENT_KEY1_PRESS`，等待 key1 按下后唤醒
- `EVENT_KEY1_RELEASE`，等待 key1 松开后唤醒
- `EVENT_KEY2_PRESS`，等待 key2 按下后唤醒
- `EVENT_KEY2_RELEASE`，等待 key2 松开后唤醒

驱动注册函数：

```c
void seg_led_init(void);
void button_init(void);
```

驱动调用函数：

```c
void single_seg_print(u8 num, u8 displaychar);
```

用于向指定的数码管打印具体编码的字符，num 表示修改的目标数码管编号，从左到右依次为 0-7；displaychar 表示为修改后要显示的字符

```c
void all_seg_print(u8 *displaychars);
```

用于修改所有数码管显示，displaychars 表示为修改后要显示的字符数组，长度指定为8

```c
void led_print(u8 displaychar);
```

用于修改 led 灯组显示，displaychar 为二进制表示的 led 显示

## 问题集

1. `_task_` 标记无法跨文件调用
2. 在不是十分要求一次性处理的死循环任务时，建议在循环内至少主动使用 `os_wait(K_IVL, 1, 0);` 或者 `os_switch_task()` 放弃时间片一次