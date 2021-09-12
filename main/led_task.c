#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "led_task.h"
#include "driver/pwm.h"

#define TAG "led_task"

// pwm pin number
const uint32_t pin_num[5] = {
  RED_GPIO,
  GREEN_GPIO,
  BLUE_GPIO,
  WARM_GPIO,
  COLD_GPIO
};

// duties table, real_duty = duties[x]/PERIOD
uint32_t duties[5];

// phase table, delay = (phase[x]/360)*PERIOD
// set phase for RGB and White to reduce power usage
float phase[5] = {
  90, 90, 90, -90, -90
};

// cie1931 lookup table
const uint16_t conv_table[256] = {
      0,    1,    2,    3,    3,    4,    5,    6,    7,    8,    9,   10,   10,   11,   12,   13,
     14,   15,   16,   17,   17,   18,   19,   20,   21,   22,   23,   24,   25,   26,   27,   29,
     30,   31,   32,   34,   35,   36,   38,   39,   41,   42,   44,   45,   47,   49,   51,   52,
     54,   56,   58,   60,   62,   64,   66,   68,   70,   72,   75,   77,   79,   82,   84,   86,
     89,   92,   94,   97,  100,  102,  105,  108,  111,  114,  117,  120,  123,  126,  130,  133,
    136,  140,  143,  147,  150,  154,  158,  161,  165,  169,  173,  177,  181,  185,  189,  194,
    198,  202,  207,  211,  216,  220,  225,  230,  235,  240,  244,  249,  255,  260,  265,  270,
    276,  281,  287,  292,  298,  304,  309,  315,  321,  327,  333,  340,  346,  352,  359,  365,
    372,  378,  385,  392,  399,  406,  413,  420,  427,  434,  442,  449,  457,  464,  472,  480,
    488,  496,  504,  512,  520,  528,  537,  545,  554,  562,  571,  580,  589,  598,  607,  616,
    626,  635,  645,  654,  664,  674,  683,  693,  703,  714,  724,  734,  745,  755,  766,  777,
    787,  798,  809,  821,  832,  843,  855,  866,  878,  890,  901,  913,  926,  938,  950,  962,
    975,  988, 1000, 1013, 1026, 1039, 1052, 1066, 1079, 1092, 1106, 1120, 1134, 1148, 1162, 1176,
   1190, 1205, 1219, 1234, 1248, 1263, 1278, 1293, 1309, 1324, 1339, 1355, 1371, 1387, 1403, 1419,
   1435, 1451, 1468, 1484, 1501, 1518, 1535, 1552, 1569, 1586, 1604, 1621, 1639, 1657, 1675, 1693,
   1711, 1729, 1748, 1766, 1785, 1804, 1823, 1842, 1861, 1881, 1900, 1920, 1940, 1960, 1980, 2000,
  };

Color color;
Color last_color;

bool color_equal(Color c1, Color c2){
  if(c1.red == c2.red && c1.green == c2.green && c1.blue == c2.blue && c1.warm == c2.warm && c1.cold == c2.cold){
    return true;
  }
  return false;
}

Color get_colors(){
  return color;
}

void set_warm(uint8_t w){
  color.warm = w;
}

void set_cold(uint8_t c){
  color.cold = c;
}

void set_color(uint8_t r, uint8_t g, uint8_t b){
  color.red = r;
  color.green = g;
  color.blue = b;
}

void apply_color_duty(){
  duties[0] = conv_table[LED_INVERSED-color.red];
  duties[1] = conv_table[LED_INVERSED-color.green];
  duties[2] = conv_table[LED_INVERSED-color.blue];
  duties[3] = conv_table[LED_INVERSED-color.warm];
  duties[4] = conv_table[LED_INVERSED-color.cold];
}

void led_task(){
  apply_color_duty();
  pwm_init(PWM_PERIOD, duties, 5, pin_num);
  pwm_set_phases(phase);
  pwm_start();

  while(1){
    if(!color_equal(color, last_color)){
      apply_color_duty();
      pwm_set_duties(duties);
      pwm_start();
      last_color = color;
    }
    vTaskDelay(10 / portTICK_RATE_MS);
  }
}
