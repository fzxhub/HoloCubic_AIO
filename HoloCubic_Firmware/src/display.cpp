#include "display.h"
#include "network.h"
#include "lv_port_indev.h"
#include "lv_demo_encoder.h"
#include "common.h"

static lv_disp_buf_t disp_buf;
static lv_color_t buf[LV_HOR_RES_MAX * 10];

void my_print(lv_log_level_t level, const char *file, uint32_t line, const char *fun, const char *dsc)
{
    Serial.printf("%s@%d %s->%s\r\n", file, line, fun, dsc);
    Serial.flush();
}

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft->setAddrWindow(area->x1, area->y1, w, h);
    tft->startWrite();
    // tft->writePixels(&color_p->full, w * h);
    tft->pushColors(&color_p->full, w * h, true);
    tft->endWrite();

    lv_disp_flush_ready(disp);
}

void Display::init()
{
    ledcSetup(LCD_BL_PWM_CHANNEL, 5000, 8);
    ledcAttachPin(LCD_BL_PIN, LCD_BL_PWM_CHANNEL);

    lv_init();

    lv_log_register_print_cb(my_print); /* register print function for debugging */

    tft->begin(); /* TFT init */
    tft->fillScreen(TFT_BLACK);
    // tft->fillScreen(BLACK);
    // tft->setRotation(2); /* mirror 修改反转，如果加上分光棱镜需要改为4镜像*/
    tft->setRotation(4); /* mirror 修改反转，如果加上分光棱镜需要改为4镜像*/

    lv_disp_buf_init(&disp_buf, buf, NULL, LV_HOR_RES_MAX * 10);

    /*Initialize the display*/
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 240;
    disp_drv.ver_res = 240;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);
}

void Display::routine()
{
    lv_task_handler();
}

void Display::setBackLight(float duty)
{
    duty = constrain(duty, 0, 1);
    duty = 1 - duty;
    ledcWrite(LCD_BL_PWM_CHANNEL, (int)(duty * 255));
}
