typedef     unsigned char   u8;
typedef     char            i8;
typedef     unsigned int    u16;
typedef     int             i16;
typedef     unsigned long   u32;
typedef     long            i32;

// system task pid
#define     INIT            0
#define     SCHEDULER       1
#define     TIME1MS         2

// events
#define     EVENT_TIME1S        0x1
#define     EVENT_KEY1_PRESS    0x2
#define     EVENT_KEY1_RELEASE  0x4
#define     EVENT_KEY2_PRESS    0x8
#define     EVENT_KEY2_RELEASE  0x10