#include "LCD.h"
#include <Arduino.h>

#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_ldo_regulator.h"

static const char *TAG = "LCD";

static esp_lcd_panel_handle_t panel_handle = NULL;

static void setLDO_VO4_to_3V3() {
	esp_ldo_channel_handle_t ldo4_handle = NULL;
	esp_ldo_channel_config_t ldo_vo4 = {
		.chan_id = 4,
		.voltage_mv = 3300,
	};
	esp_err_t err = esp_ldo_acquire_channel(&ldo_vo4, &ldo4_handle);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to acquire LDO_CHANNEL_VO4: %s", esp_err_to_name(err));
		return;
	}
	ESP_LOGI(TAG, "Acquired LDO_CHANNEL_VO4 and set to 3.3V");
}

void LCD::initRGBInterface() {
	esp_lcd_rgb_panel_config_t panel_config = {
		.clk_src = LCD_CLK_SRC_DEFAULT,
		.timings = {
			.pclk_hz = LCD_PIXEL_CLOCK_HZ,
			.h_res = LCD_WIDTH,
			.v_res = LCD_HEIGHT,
			.hsync_pulse_width = LCD_TIMING_HSYNC,
			.hsync_back_porch = LCD_TIMING_HBP,
			.hsync_front_porch = LCD_TIMING_HFP,
			.vsync_pulse_width = LCD_TIMING_VSYNC,
			.vsync_back_porch = LCD_TIMING_VBP,
			.vsync_front_porch = LCD_TIMING_VFP,
			.flags = {
				.hsync_idle_low = false,
				.vsync_idle_low = false,
				.de_idle_high = false,
				.pclk_active_neg = true,
				.pclk_idle_high = false,
			},
		},
		.data_width = 16,
		.bits_per_pixel = 16,
		.num_fbs = 1, // --- Frambuffer
		.bounce_buffer_size_px = 0,
		.dma_burst_size = 64,
		.hsync_gpio_num = LCD_PIN_HSYNC,
		.vsync_gpio_num = LCD_PIN_VSYNC,
		.de_gpio_num = LCD_PIN_DE,
		.pclk_gpio_num = LCD_PIN_PCLK,
		.disp_gpio_num = LCD_PIN_DISP_EN,
		.data_gpio_nums = {
		LCD_PIN_DATA0,
		LCD_PIN_DATA1,
		LCD_PIN_DATA2,
		LCD_PIN_DATA3,
		LCD_PIN_DATA4,
		LCD_PIN_DATA5,
		LCD_PIN_DATA6,
		LCD_PIN_DATA7,
		LCD_PIN_DATA8,
		LCD_PIN_DATA9,
		LCD_PIN_DATA10,
		LCD_PIN_DATA11,
		LCD_PIN_DATA12,
		LCD_PIN_DATA13,
		LCD_PIN_DATA14,
		LCD_PIN_DATA15,
		},
		.flags = {
			.disp_active_low = false,
			.refresh_on_demand = false,
			.fb_in_psram = true, // allocate frame buffer in PSRAM
			.double_fb = false,
			.no_fb = false,
			.bb_invalidate_cache = false,
		}
	};
	ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));

	ESP_LOGI(TAG, "Initialize RGB LCD panel");
	ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
	ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
	setLDO_VO4_to_3V3();
	this->on();
}

LCD::LCD() {}

void LCD::begin(uint8_t rotation, uint8_t brightness) {
	this->rotation = rotation;
	this->brightness = constrain(brightness, 0, 100);
	
	pinMode(LCD_PIN_BK_LIGHT, OUTPUT);
	analogWrite(LCD_PIN_BK_LIGHT, 0);

	this->initRGBInterface();
	this->setRotation(rotation);
	this->on();
}

int LCD::getWidth() {
	return this->lcd_width;
}

int LCD::getHeight() {
	return this->lcd_height;
}

void LCD::setRotation(int m) {
	this->rotation = m;
}

uint8_t LCD::getRotation() {
	return this->rotation;
}

// Display OFF
void LCD::off() {
	esp_lcd_panel_disp_on_off(panel_handle, false);
	analogWrite(LCD_PIN_BK_LIGHT, 0);
}
 
// Display ON
void LCD::on() {
	esp_lcd_panel_disp_on_off(panel_handle, true);
	this->setBrightness(this->brightness);
}

void LCD::setBrightness(int level) {
	level = constrain(level, 0, 100);
	analogWrite(LCD_PIN_BK_LIGHT, map(level, 0, 100, 0, 255));
	this->brightness = level;
}

int LCD::getBrightness() {
	return this->brightness;
}

void LCD::drawBitmap(int x_start, int y_start, int x_end, int y_end, uint16_t* color_data) {
	esp_lcd_panel_draw_bitmap(panel_handle, x_start, y_start, x_end, y_end, color_data);
}

void LCD::drawPixel(uint16_t x, uint16_t y, uint16_t color) {
  if (x >= this->lcd_width) return;
  if (y >= this->lcd_height) return;

  uint8_t px_map[] = {
    (uint8_t)((color >> 8) & 0xFF),
    (uint8_t)(color & 0xFF)
  };
  esp_lcd_panel_draw_bitmap(panel_handle, x, y, x, y, px_map);
}

uint16_t LCD::color565(uint8_t red, uint8_t green, uint8_t blue) {
  return ((red & 0b11111000) << 8) | ((green & 0b11111100) << 3) | (blue >> 3);
}

uint32_t LCD::color24to16(uint32_t color888) {
  return this->color565((color888 >> 16) & 0xFF, (color888 >> 8) & 0xFF, color888 & 0xFF);
}

void LCD::fillRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    if (x1 >= this->lcd_width || y1 >= this->lcd_height) return;
    if (x2 >= this->lcd_width) x2 = this->lcd_width - 1;
    if (y2 >= this->lcd_height) y2 = this->lcd_height - 1;
    if (x2 < x1 || y2 < y1) return;

    uint32_t width = (x2 - x1) + 1;
    // uint32_t height = (y2 - y1) + 1;

    uint16_t * line_buffer = (uint16_t *) heap_caps_malloc(width * sizeof(uint16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!line_buffer) return;

    for (uint32_t i = 0; i < width; i++) {
        line_buffer[i] = color;
    }

    for (uint16_t y = y1; y <= y2; y++) {
        esp_lcd_panel_draw_bitmap(panel_handle, x1, y, x2 + 1, y + 1, line_buffer);
    }

    free(line_buffer);
}

void LCD::fillScreen(uint16_t color) {
    this->fillRect(0, 0, this->lcd_width - 1, this->lcd_height - 1, color);
}

// Draw line
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End   X coordinate
// y2:End   Y coordinate
// color:color 
void LCD::drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
	int i;
	int dx,dy;
	int sx,sy;
	int E;

	/* distance between two points */
	dx = ( x2 > x1 ) ? x2 - x1 : x1 - x2;
	dy = ( y2 > y1 ) ? y2 - y1 : y1 - y2;

	/* direction of two point */
	sx = ( x2 > x1 ) ? 1 : -1;
	sy = ( y2 > y1 ) ? 1 : -1;

	/* inclination < 1 */
	if ( dx > dy ) {
		E = -dx;
		for ( i = 0 ; i <= dx ; i++ ) {
			this->drawPixel(x1, y1, color);
			x1 += sx;
			E += 2 * dy;
			if ( E >= 0 ) {
			y1 += sy;
			E -= 2 * dx;
		}
	}

	/* inclination >= 1 */
	} else {
		E = -dy;
		for ( i = 0 ; i <= dy ; i++ ) {
			this->drawPixel(x1, y1, color);
			y1 += sy;
			E += 2 * dx;
			if ( E >= 0 ) {
				x1 += sx;
				E -= 2 * dy;
			}
		}
	}
}

// Draw rectangle
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End   X coordinate
// y2:End   Y coordinate
// color:color
void LCD::drawRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
	this->drawLine(x1, y1, x2, y1, color);
	this->drawLine(x2, y1, x2, y2, color);
	this->drawLine(x2, y2, x1, y2, color);
	this->drawLine(x1, y2, x1, y1, color);
}

// Draw rectangle with angle
// xc:Center X coordinate
// yc:Center Y coordinate
// w:Width of rectangle
// h:Height of rectangle
// angle:Angle of rectangle
// color:color

//When the origin is (0, 0), the point (x1, y1) after rotating the point (x, y) by the angle is obtained by the following calculation.
// x1 = x * cos(angle) - y * sin(angle)
// y1 = x * sin(angle) + y * cos(angle)
void LCD::drawRectAngle(uint16_t xc, uint16_t yc, uint16_t w, uint16_t h, uint16_t angle, uint16_t color) {
	double xd,yd,rd;
	int x1,y1;
	int x2,y2;
	int x3,y3;
	int x4,y4;
	rd = -angle * M_PI / 180.0;
	xd = 0.0 - w/2;
	yd = h/2;
	x1 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
	y1 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

	yd = 0.0 - yd;
	x2 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
	y2 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

	xd = w/2;
	yd = h/2;
	x3 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
	y3 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

	yd = 0.0 - yd;
	x4 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
	y4 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

	this->drawLine(x1, y1, x2, y2, color);
	this->drawLine(x1, y1, x3, y3, color);
	this->drawLine(x2, y2, x4, y4, color);
	this->drawLine(x3, y3, x4, y4, color);
}

// Draw triangle
// xc:Center X coordinate
// yc:Center Y coordinate
// w:Width of triangle
// h:Height of triangle
// angle:Angle of triangle
// color:color

//When the origin is (0, 0), the point (x1, y1) after rotating the point (x, y) by the angle is obtained by the following calculation.
// x1 = x * cos(angle) - y * sin(angle)
// y1 = x * sin(angle) + y * cos(angle)
void LCD::drawTriangle(uint16_t xc, uint16_t yc, uint16_t w, uint16_t h, uint16_t angle, uint16_t color) {
	double xd,yd,rd;
	int x1,y1;
	int x2,y2;
	int x3,y3;
	rd = -angle * M_PI / 180.0;
	xd = 0.0;
	yd = h/2;
	x1 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
	y1 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

	xd = w/2;
	yd = 0.0 - yd;
	x2 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
	y2 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

	xd = 0.0 - w/2;
	x3 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
	y3 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

	this->drawLine(x1, y1, x2, y2, color);
	this->drawLine(x1, y1, x3, y3, color);
	this->drawLine(x2, y2, x3, y3, color);
}

// Draw circle
// x0:Central X coordinate
// y0:Central Y coordinate
// r:radius
// color:color
void LCD::drawCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color) {
	int x;
	int y;
	int err;
	int old_err;

	x=0;
	y=-r;
	err=2-2*r;
	do{
		this->drawPixel(x0-x, y0+y, color); 
		this->drawPixel(x0-y, y0-x, color); 
		this->drawPixel(x0+x, y0-y, color); 
		this->drawPixel(x0+y, y0+x, color); 
		if ((old_err=err)<=x)	err+=++x*2+1;
		if (old_err>y || err>x) err+=++y*2+1;	 
	} while(y<0);
}

// Draw circle of filling
// x0:Central X coordinate
// y0:Central Y coordinate
// r:radius
// color:color
void LCD::fillCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color) {
	int x;
	int y;
	int err;
	int old_err;
	int ChangeX;

	x=0;
	y=-r;
	err=2-2*r;
	ChangeX=1;
	do{
		if(ChangeX) {
			this->drawLine(x0-x, y0-y, x0-x, y0+y, color);
			this->drawLine(x0+x, y0-y, x0+x, y0+y, color);
		} // endif
		ChangeX=(old_err=err)<=x;
		if (ChangeX)			err+=++x*2+1;
		if (old_err>y || err>x) err+=++y*2+1;
	} while(y<=0);
} 

// Draw rectangle with round corner
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End   X coordinate
// y2:End   Y coordinate
// r:radius
// color:color
void LCD::drawRoundRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t r, uint16_t color) {
	int x;
	int y;
	int err;
	int old_err;
	unsigned char temp;

	if(x1>x2) {
		temp=x1; x1=x2; x2=temp;
	} // endif
	  
	if(y1>y2) {
		temp=y1; y1=y2; y2=temp;
	} // endif

	if (x2-x1 < r) return; // Add 20190517
	if (y2-y1 < r) return; // Add 20190517

	x=0;
	y=-r;
	err=2-2*r;

	do{
		if(x) {
			this->drawPixel(x1+r-x, y1+r+y, color); 
			this->drawPixel(x2-r+x, y1+r+y, color); 
			this->drawPixel(x1+r-x, y2-r-y, color); 
			this->drawPixel(x2-r+x, y2-r-y, color);
		} // endif 
		if ((old_err=err)<=x)	err+=++x*2+1;
		if (old_err>y || err>x) err+=++y*2+1;	 
	} while(y<0);

	this->drawLine(x1+r,y1  ,x2-r,y1	,color);
	this->drawLine(x1+r,y2  ,x2-r,y2	,color);
	this->drawLine(x1  ,y1+r,x1  ,y2-r,color);
	this->drawLine(x2  ,y1+r,x2  ,y2-r,color);  
} 

// Draw arrow
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End   X coordinate
// y2:End   Y coordinate
// w:Width of the botom
// color:color
// Thanks http://k-hiura.cocolog-nifty.com/blog/2010/11/post-2a62.html
void LCD::drawArrow(uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1,uint16_t w,uint16_t color) {
	double Vx= x1 - x0;
	double Vy= y1 - y0;
	double v = sqrt(Vx*Vx+Vy*Vy);
	//	 printf("v=%f\n",v);
	double Ux= Vx/v;
	double Uy= Vy/v;

	uint16_t L[2],R[2];
	L[0]= x1 - Uy*w - Ux*v;
	L[1]= y1 + Ux*w - Uy*v;
	R[0]= x1 + Uy*w - Ux*v;
	R[1]= y1 - Ux*w - Uy*v;
	//printf("L=%d-%d R=%d-%d\n",L[0],L[1],R[0],R[1]);

	//lcdDrawLine(x0,y0,x1,y1,color);
	this->drawLine(x1, y1, L[0], L[1], color);
	this->drawLine(x1, y1, R[0], R[1], color);
	this->drawLine(L[0], L[1], R[0], R[1], color);
}


// Draw arrow of filling
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End   X coordinate
// y2:End   Y coordinate
// w:Width of the botom
// color:color
void LCD::fillArrow(uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1,uint16_t w,uint16_t color) {
	double Vx= x1 - x0;
	double Vy= y1 - y0;
	double v = sqrt(Vx*Vx+Vy*Vy);
	//printf("v=%f\n",v);
	double Ux= Vx/v;
	double Uy= Vy/v;

	uint16_t L[2],R[2];
	L[0]= x1 - Uy*w - Ux*v;
	L[1]= y1 + Ux*w - Uy*v;
	R[0]= x1 + Uy*w - Ux*v;
	R[1]= y1 - Ux*w - Uy*v;
	//printf("L=%d-%d R=%d-%d\n",L[0],L[1],R[0],R[1]);

	this->drawLine(x0, y0, x1, y1, color);
	this->drawLine(x1, y1, L[0], L[1], color);
	this->drawLine(x1, y1, R[0], R[1], color);
	this->drawLine(L[0], L[1], R[0], R[1], color);

	int ww;
	for(ww=w-1;ww>0;ww--) {
		L[0]= x1 - Uy*ww - Ux*v;
		L[1]= y1 + Ux*ww - Uy*v;
		R[0]= x1 + Uy*ww - Ux*v;
		R[1]= y1 - Ux*ww - Uy*v;
		//printf("Fill>L=%d-%d R=%d-%d\n",L[0],L[1],R[0],R[1]);
		this->drawLine(x1, y1, L[0], L[1], color);
		this->drawLine(x1, y1, R[0], R[1], color);
	}
}

#ifdef USE_LVGL
#include "LVGLHelper.h"

static bool IRAM_ATTR notify_lvgl_flush_ready(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *event_data, void *user_ctx) {
    lv_display_t *disp = (lv_display_t *) user_ctx;
    lv_display_flush_ready(disp);
    return false;
}

static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
	esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)lv_display_get_user_data(disp);
	int offsetx1 = area->x1;
	int offsetx2 = area->x2;
	int offsety1 = area->y1;
	int offsety2 = area->y2;

	// pass the draw buffer to the driver
	esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1,  offsety2 + 1, px_map);
}

unsigned long last_touch_on_display = 0;

void display_inp_feedback(lv_indev_t *indev_driver, uint8_t event) {
	if ((event == LV_EVENT_CLICKED) || (event == LV_EVENT_KEY)) {
		last_touch_on_display = millis();
	}
}

void LCD::useLVGL() {
	ESP_LOGI(TAG, "Initialize LVGL library");
	lv_init();

	/*Set a tick source so that LVGL will know how much time elapsed. */
	lv_tick_set_cb((lv_tick_get_cb_t)millis);
	lv_delay_set_cb(delay);
	// lv_log_register_print_cb( my_print );

	// create a lvgl display
	lv_display_t *display = lv_display_create(this->lcd_width, this->lcd_height);
	// associate the rgb panel handle to the display
	lv_display_set_user_data(display, panel_handle);
	// set color depth
	lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);

	// create draw buffers
	void *buf1 = NULL;
	// void *buf2 = NULL;
	ESP_LOGI(TAG, "Allocate LVGL draw buffers");
	// it's recommended to allocate the draw buffer from internal memory, for better performance
	size_t draw_buffer_sz = LCD_WIDTH * 120 * 2; // 30 Lines
	void *raw_buf1 = heap_caps_malloc(draw_buffer_sz + 64, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
	buf1 = lv_draw_buf_align(raw_buf1, LV_COLOR_FORMAT_RGB565);
	assert(buf1);
	// set LVGL draw buffers and partial mode
	lv_display_set_buffers(display, buf1, NULL, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);

	// set the callback which can copy the rendered image to an area of the display
	lv_display_set_flush_cb(display, lvgl_flush_cb);
	
	ESP_LOGI(TAG, "Register event callbacks");
	esp_lcd_rgb_panel_event_callbacks_t cbs = {
		.on_color_trans_done = notify_lvgl_flush_ready,
	};
	ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &cbs, display));
}

void LCD::loop() {
	{ // UI update
		static unsigned long timer = 0;
		if ((millis() < timer) || (timer == 0) || ((millis() - timer) >= 5)) {
			timer = millis();
			lv_timer_handler();
		}
	}

	{ // Auto sleep
		static uint8_t state = 0;
		if (this->auto_sleep_after_sec > 0) { // enable auto sleep
			if (state == 0) {
				if ((millis() - last_touch_on_display) >=
					(this->auto_sleep_after_sec * 1000)) {
					this->off();
					state = 1;
				}
			} else if (state == 1) {
				if ((millis() - last_touch_on_display) <
					(this->auto_sleep_after_sec * 1000)) {
					lv_obj_invalidate(lv_scr_act());
					lv_timer_handler();
					this->on();
					state = 0;
				}
			}
		} else {			  // disable auto sleep
			if (state != 0) { // but now in sleep
				lv_obj_invalidate(lv_scr_act());
				lv_timer_handler();
				this->on();
				state = 0;
			}
		}
	}

	if (xSafeUpdateQueue) { // Safe UI update
		SafeUpdateParam_t safe_update_item;
		while (xQueueReceive(xSafeUpdateQueue, &safe_update_item, 0) ==
			   pdPASS) {
			if (safe_update_item.cb) {
				safe_update_item.cb(safe_update_item.user_data);
			}
			if (safe_update_item.sync_event_group_handle) {
				xEventGroupSetBits(safe_update_item.sync_event_group_handle, BIT0);
			}
		}
	}
}
#endif

void LCD::enableAutoSleep(uint32_t timeout_in_sec) {
	this->auto_sleep_after_sec = timeout_in_sec;
}

void LCD::disableAutoSleep() { this->auto_sleep_after_sec = 0; }

LCD Display;
