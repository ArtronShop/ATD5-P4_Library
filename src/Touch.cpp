#include <Arduino.h>
#include <Wire.h>
#include "LCD.h"
#include "Touch.h"
#include "indev/lv_indev.h"

#include <lvgl.h>

static const char * TAG = "Touch";

#define GT911_ADDR 0x5D

GT911::GT911() { }

bool GT911::readReg(uint16_t reg, uint8_t *data, uint8_t len) {
    Wire.beginTransmission(GT911_ADDR);
    Wire.write(reg >> 8);
    Wire.write(reg & 0xFF);
    if (Wire.endTransmission(false) != 0) {
        ESP_LOGE(TAG, "Write error !");
        return false;
    }

    uint8_t count = Wire.requestFrom(GT911_ADDR, len);
    if (count != len) {
        ESP_LOGE(TAG, "Read error !");
        return false;
    }

    for (uint8_t i = 0; i < len; i++) {
        data[i] = Wire.read();
    }

    return true;
}

bool GT911::writeReg(uint16_t reg, uint8_t *data, uint8_t len) {
    Wire.beginTransmission(GT911_ADDR);
    Wire.write(reg >> 8);
    Wire.write(reg & 0xFF);
    for (uint8_t i = 0; i < len; i++) {
        Wire.write(data[i]);
    }
    return Wire.endTransmission() == 0;
}

bool GT911::writeReg(uint16_t reg, uint8_t data) {
    return this->writeReg(reg, &data, 1);
}

void GT911::begin() {
    Wire.begin(TOUCH_SDA_PIN, TOUCH_SCL_PIN, (uint32_t) 400E3);

    pinMode(TOUCH_RST_PIN, OUTPUT);
	
	// Reset
    digitalWrite(TOUCH_RST_PIN, LOW);
    delay(20);
    digitalWrite(TOUCH_RST_PIN, HIGH);
    delay(50);
}

uint8_t GT911::read(uint16_t *cx, uint16_t *cy) {
    uint8_t touch_info;
    if (!this->readReg(0x814E, &touch_info, 1)) {
        ESP_LOGE(TAG, "Read error !");
        return 0;
    }

    if ((touch_info & 0x80) == 0) { // buffer status are set
        return 0; // not ready and data is not valid
    }

    if (!this->writeReg(0x814E, 0x00)) {
        ESP_LOGE(TAG, "Write error !");
        return 0;
    }

    uint8_t touch_point = touch_info & 0x0F;
    if ((touch_point <= 0) || (touch_point > 5)) {
        return 0;
    }

    // Read Coordinate
    uint8_t data[4];
    if (!this->readReg(0x8150, data, 4)) {
        ESP_LOGE(TAG, "Read error !");
        return 0;
    }

    // Process Data
    uint16_t x = (((uint16_t)data[1]&0x0F)<<8)|data[0];
    uint16_t y = (((uint16_t)data[3]&0x0F)<<8)|data[2];

    uint8_t m = Display.getRotation();
    if (m == 0) {
        *cx = x;
        *cy = y;
    } else if (m == 1) {
        *cx = y;
        *cy = Display.getHeight() - x;
    } else if (m == 2) {
        *cx = Display.getWidth() - y;
        *cy = x;
    } else if (m == 3) {
        *cx = Display.getHeight() - x;
        *cy = Display.getWidth() - y;
    } else {
        ESP_LOGE(TAG, "invalid rotation %d", m);
        return 0;
    }

    return touch_point;
}

#ifdef USE_LVGL
static void touchpad_read(lv_indev_t * indev, lv_indev_data_t * data) {
  GT911 * touch = (GT911 *) lv_indev_get_user_data(indev);

  uint8_t touchPoint = touch->read((uint16_t*)(&data->point.x), (uint16_t*)(&data->point.y));
  if (touchPoint > 0) {
    ESP_LOGV(TAG, "X: %d, Y: %d", data->point.x, data->point.y);
  }
  data->state = touchPoint > 0 ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

lv_indev_t * lvgl_indev = NULL;

static void indev_event_cb(lv_event_t * e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_indev_t * indev = (lv_indev_t *)lv_event_get_target(e);
	
  extern void beep_inp_feedback(lv_indev_t *indev, uint8_t event) ;
  beep_inp_feedback(indev, code) ;
	  
  extern void display_inp_feedback(lv_indev_t *indev_driver, uint8_t event) ;
  display_inp_feedback(indev, code) ;
}

void GT911::useLVGL() {
  lv_indev_t * indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); /*Touchpad should have POINTER type*/
  lv_indev_set_read_cb(indev, touchpad_read);
  lv_indev_set_user_data(indev, this);
  
  lv_indev_add_event_cb(indev, indev_event_cb, LV_EVENT_ALL, NULL);
}
#endif

GT911 Touch;
