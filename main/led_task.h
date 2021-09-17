#ifndef __LED_TASK_H__
#define __LED_TASK_H__
#include <stdint.h>

// ACTION RGB BULB (2020)
// #define RED_GPIO           4
// #define GREEN_GPIO         12
// #define BLUE_GPIO          14
// #define WARM_GPIO          13
// #define COLD_GPIO          5

// ACTION RGB BULB (2021)
#define RED_GPIO          5
#define GREEN_GPIO        13
#define BLUE_GPIO         4
#define WARM_GPIO         12
#define COLD_GPIO         14

#define PWM_PERIOD         2000
#define LED_INVERSED       255
// some led bulbs cannot handle max brightness for a long time, so here's cap
#define MAX_BRIGHTNESS     250

typedef struct Color {
   uint8_t red;
   uint8_t green;
   uint8_t blue;
   uint8_t warm;
   uint8_t cold;
} Color;

void set_color(uint8_t r, uint8_t g, uint8_t b);
void set_warm(uint8_t w);
void set_cold(uint8_t c);
void set_color_fade(uint8_t r, uint8_t g, uint8_t b, uint8_t warm, uint8_t cold);
void set_inverse(bool inversed);
Color get_colors();
void led_task();
#endif
