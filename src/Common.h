// #if defined(__has_include)
// #if __has_include(<lvgl.h>)
// #include <lvgl.h>
#define USE_LVGL
// #endif
// #else
// #error "__has_include not work"
// #endif

// Refresh Rate = 18000000/(1+40+20+800)/(1+10+5+480) = 42Hz
#define LCD_PIXEL_CLOCK_HZ     (25 * 1000 * 1000)
#define LCD_WIDTH              800
#define LCD_HEIGHT             480

#define LCD_TIMING_HBP         8   // HSYNC Back Porch
#define LCD_TIMING_HFP         8   // HSYNC Front Porch
#define LCD_TIMING_HSYNC       4   // HSYNC Pulse Width

#define LCD_TIMING_VBP         8
#define LCD_TIMING_VFP         8
#define LCD_TIMING_VSYNC       4

#define LCD_PIN_BK_LIGHT       20
#define LCD_PIN_DISP_EN        42

#define LCD_PIN_HSYNC          41
#define LCD_PIN_VSYNC          40
#define LCD_PIN_DE             39
#define LCD_PIN_PCLK           43

#define LCD_PIN_DATA0          48
#define LCD_PIN_DATA1          47
#define LCD_PIN_DATA2          46
#define LCD_PIN_DATA3          45
#define LCD_PIN_DATA4          44
#define LCD_PIN_DATA5          26
#define LCD_PIN_DATA6          27
#define LCD_PIN_DATA7          28
#define LCD_PIN_DATA8          29
#define LCD_PIN_DATA9          30
#define LCD_PIN_DATA10         31
#define LCD_PIN_DATA11         53
#define LCD_PIN_DATA12         52
#define LCD_PIN_DATA13         51
#define LCD_PIN_DATA14         50
#define LCD_PIN_DATA15         49

// Touch
#define TOUCH_SDA_PIN 24
#define TOUCH_SCL_PIN 25
#define TOUCH_INT_PIN -1
#define TOUCH_RST_PIN 6

// MicroSD Card
#define SD_CS_PIN    (34)
#define SD_CD_PIN    (13)

// Sound
#define I2S_DOUT (21)
#define I2S_BCLK (22)
#define I2S_LRC  (23)

