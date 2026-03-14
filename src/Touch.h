#ifndef __TOUCH_H__
#define __TOUCH_H__

#include <stdint.h>
#include "Common.h"

class GT911 {
    private:
        bool readReg(uint16_t reg, uint8_t *data, uint8_t len) ;
        bool writeReg(uint16_t reg, uint8_t *data, uint8_t len) ;
        bool writeReg(uint16_t reg, uint8_t data) ;

    public: 
        GT911() ;

        void begin() ;
        uint8_t read(uint16_t *, uint16_t *) ;

#ifdef USE_LVGL
        void useLVGL() ;
#endif

};

extern GT911 Touch;
#endif
