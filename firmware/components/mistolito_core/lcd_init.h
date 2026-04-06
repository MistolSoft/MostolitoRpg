#ifndef LCD_INIT_H
#define LCD_INIT_H

#include "esp_err.h"

esp_err_t lcd_hardware_init(void);
esp_err_t lvgl_display_init(void);

#endif
