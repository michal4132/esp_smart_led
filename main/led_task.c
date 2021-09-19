#include "led_task.h"
#include "driver/pwm.h"
#include "driver/hw_timer.h"

#define TAG "led_task"

Color color;
Color last_color;
Color target_color;
bool led_inversed;
bool fade_done = true;

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

void set_inverse(bool inversed){
  led_inversed = inversed;
}

bool ICACHE_FLASH_ATTR color_black(Color c){
  if(c.red == 0 && c.green == 0 && c.blue == 0 && c.brightness == 0){
    return true;
  }
  return false;
}

bool ICACHE_FLASH_ATTR color_equal(Color c1, Color c2){
  if(c1.red == c2.red && c1.green == c2.green && c1.blue == c2.blue && c1.brightness == c2.brightness && c1.temperature == c2.temperature){
    return true;
  }
  return false;
}

Color get_colors(){
  return color;
}

void ICACHE_FLASH_ATTR set_color(uint8_t r, uint8_t g, uint8_t b){
  color.red = r;
  color.green = g;
  color.blue = b;
}

void ICACHE_FLASH_ATTR apply_color(){
  if(led_inversed){
    duties[0] = conv_table[LED_INVERSED-color.red];
    duties[1] = conv_table[LED_INVERSED-color.green];
    duties[2] = conv_table[LED_INVERSED-color.blue];
  }else{
    duties[0] = conv_table[color.red];
    duties[1] = conv_table[color.green];
    duties[2] = conv_table[color.blue];
  }

  //https://www.desmos.com/calculator/vdxo6sj9uq
  //warm = \frac{2000}{255}z-\left(\frac{2000z}{255\cdot255}x\right)
  //cold = \left(\frac{2000z}{255\cdot255}x\right)
  int16_t warm = (float)(7.8431372549)*color.brightness - ((2000*color.brightness*color.temperature)/(65025));
  if(warm > 2000){
    warm = 2000;
  }else if(warm < 0){
    warm = 0;
  }
  duties[3] = warm;

  int16_t cold = (2000*color.brightness*color.temperature)/(65025);
  if(cold > 2000){
    cold = 2000;
  }else if(cold < 0){
    cold = 0;
  }
  duties[4] = cold;

  pwm_set_duties(duties);
  pwm_start();
  last_color = color;
}

void ICACHE_FLASH_ATTR set_white(uint8_t brightness, uint8_t temperature){
  if(brightness > MAX_BRIGHTNESS){
    brightness = MAX_BRIGHTNESS;
  }
  color.brightness = brightness;
  color.temperature = temperature;
}

void set_color_fade(uint8_t r, uint8_t g, uint8_t b, uint8_t temperature, uint8_t brightness){
  Color c;
  c.red = r;
  c.green = g;
  c.blue = b;

  if(brightness > MAX_BRIGHTNESS){
    brightness = MAX_BRIGHTNESS;
  }
  c.brightness = brightness;
  c.temperature = temperature;

  target_color = c;
  fade_done = false;
}

void ICACHE_FLASH_ATTR fade_color(){
  int16_t r_diff = target_color.red - color.red;
  int16_t g_diff = target_color.green - color.green;
  int16_t b_diff = target_color.blue - color.blue;

  int16_t brightness_diff = target_color.brightness - color.brightness;
  int16_t temperature_diff = target_color.temperature - color.temperature;

  if(r_diff > 0){
    color.red+=1;
  }else if(r_diff < 0){
    color.red-=1;
  }
  if(g_diff > 0){
    color.green+=1;
  }else if(g_diff < 0){
    color.green-=1;
  }
  if(b_diff > 0){
    color.blue+=1;
  }else if(b_diff < 0){
    color.blue-=1;
  }

  if(brightness_diff > 0){
    color.brightness+=1;
  }else if(brightness_diff < 0){
    color.brightness-=1;
  }
  if(temperature_diff > 0){
    color.temperature+=1;
  }else if(temperature_diff < 0){
    color.temperature-=1;
  }

  if(r_diff == 0 && g_diff == 0 && b_diff == 0 && brightness_diff == 0 && temperature_diff == 0){
    fade_done = true;
  }
}

void ICACHE_FLASH_ATTR led_timer_callback(){
  if(!fade_done){
    fade_color();
  }

  if(!color_equal(color, last_color)){
    apply_color();
    if(color_black(color)){
      pwm_stop(0x00); // set all pins low
    }
  }
}

void led_task_init(){
  pwm_init(PWM_PERIOD, duties, 5, pin_num);
  pwm_set_phases(phase);
  apply_color();

  hw_timer_init(led_timer_callback, NULL);
  hw_timer_alarm_us(16667, true); // 60fps
//  hw_timer_deinit();
}
