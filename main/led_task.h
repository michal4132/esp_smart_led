#ifndef __LED_TASK_H__
#define __LED_TASK_H__
#include <stdint.h>
#include <stdbool.h>

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

// ACTION RGB BULB small (2021)
// #define RED_GPIO          4
// #define GREEN_GPIO        3
// #define BLUE_GPIO         14
// #define WARM_GPIO         5
// #define COLD_GPIO         12

#define PWM_PERIOD         2000
#define LED_INVERSED       255
// some led bulbs cannot handle max brightness for a long time, so here's cap
#define MAX_PWM_DUTY 1350 // 1370/2000 // max duty should be less than pwm period

typedef struct Color {
   uint8_t red;
   uint8_t green;
   uint8_t blue;
   uint8_t brightness;
   uint8_t temperature;
} Color;

void set_color(uint8_t r, uint8_t g, uint8_t b);
void set_white(uint8_t temperature, uint8_t brightness);
void set_color_fade(uint8_t r, uint8_t g, uint8_t b, uint8_t temperature, uint8_t brightness);
void set_inverse(bool inversed);
Color get_colors();
void led_task_init();
#endif
