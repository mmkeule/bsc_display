// Copyright (c) 2022 Tobias Himmler
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "display.h"
#include "i2c.h"
#include "data.h"
#include "PanelLan.h"

// BOARD_SC01_PLUS, BOARD_SC02, BOARD_SC05, BOARD_KC01, BOARD_BC02, BOARD_SC07
//Achtung, bei änderung des Boards auch in der defines.h die i2c Ports anpassen
PanelLan lcd(BOARD_SC01_PLUS);

//#define LGFX_AUTODETECT // Autodetect board
#define LGFX_USE_V1     // set to use new version of library

#include <LovyanGFX.hpp> // main library
#include <lvgl.h>
#include "lv_conf.h"

// Variables for touch x,y
#ifdef DRAW_ON_SCREEN
static int32_t x, y;
#endif

//static LGFX lcd;

static const uint16_t screenWidth = 480;
static const uint16_t screenHeight = 320;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10];

//Settings
uint8_t u8_mPowersaveTime = 5;
static struct data_s *lDataDisp;

lv_obj_t * tabHome;
lv_obj_t * tabZellSpg;
lv_obj_t * tabSerBmsOverview;
lv_obj_t * tabBTBmsOverview;
lv_obj_t * tabInfo;

lv_obj_t * kachelAlarme;
lv_obj_t * kachelBmsError;
lv_obj_t * kachelInverter;
lv_obj_t * kachelInverter2;

lv_obj_t * relaisState[6];

// Function declaration
void display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
void touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);

void createScreens(void);


void displayInit()
{
  lcd.begin(); // init LovyanGFX
  lv_init();  // init lvgl

  // Setting display to landscape
  if (lcd.width() < lcd.height()) lcd.setRotation(lcd.getRotation() ^ 1);

  // LVGL; Setting up buffer to use for display
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 10);

  // LVGL; Setup + init display device driver
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = display_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  // LVGL; Setup + init input device driver
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = touchpad_read;
  lv_indev_drv_register(&indev_drv);


  lDataDisp=getData();
  createScreens();
}


unsigned int offTimer=0;
unsigned long currentMillis;
unsigned long previousMillis1000;
void displayRunCyclic()
{
  lv_timer_handler(); 

  currentMillis = millis();
  if(currentMillis - previousMillis1000 >=1000)
  {
    if(offTimer<(u8_mPowersaveTime*60)) offTimer++;
    if(offTimer>=(u8_mPowersaveTime*60)) lcd.sleep();

    previousMillis1000 = currentMillis;
  }
  
}


// Display callback to flush the buffer to screen
void display_flush(lv_disp_drv_t * disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  lcd.startWrite();
  lcd.setAddrWindow(area->x1, area->y1, w, h);
  lcd.pushPixels((uint16_t *)&color_p->full, w * h, true);
  lcd.endWrite();

  lv_disp_flush_ready(disp);
}


// Touchpad callback to read the touchpad 
void touchpad_read(lv_indev_drv_t * indev_driver, lv_indev_data_t * data)
{
  uint16_t touchX, touchY;
  bool touched = lcd.getTouch(&touchX, &touchY);

  if (!touched)
  {
    data->state = LV_INDEV_STATE_REL;
  }
  else
  {
    data->state = LV_INDEV_STATE_PR;

    //Set the coordinates
    data->point.x = touchX;
    data->point.y = touchY;
    //Serial.printf("Touch (x,y): (%03d,%03d)\n",touchX,touchY);

    if(offTimer>=(u8_mPowersaveTime*60)) lcd.wakeup(); 
    offTimer=0;
  }
}

static void draw_part_event_cb(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    lv_obj_draw_part_dsc_t * dsc = lv_event_get_draw_part_dsc(e);
    /*If the cells are drawn...*/
    if(dsc->part == LV_PART_ITEMS) {
        uint32_t row = dsc->id /  lv_table_get_col_cnt(obj);
        uint32_t col = dsc->id - row * lv_table_get_col_cnt(obj);

        /*Make the texts in the first cell center aligned*/
        if(row == 0) {
            dsc->label_dsc->align = LV_TEXT_ALIGN_CENTER;
            dsc->rect_dsc->bg_color = lv_color_mix(lv_palette_main(LV_PALETTE_BLUE), dsc->rect_dsc->bg_color, LV_OPA_20);
            dsc->rect_dsc->bg_opa = LV_OPA_COVER;
        }
        /*In the first column align the texts to the right*/
        else if(col == 0) {
            dsc->label_dsc->align = LV_TEXT_ALIGN_RIGHT;
        }

        /*MAke every 2nd row grayish*/
        if((row != 0 && row % 2) == 0) {
            dsc->rect_dsc->bg_color = lv_color_mix(lv_palette_main(LV_PALETTE_GREY), dsc->rect_dsc->bg_color, LV_OPA_10);
            dsc->rect_dsc->bg_opa = LV_OPA_COVER;
        }
    }
}

static void scroll_begin_event(lv_event_t* e)
{
    /*Disable the scroll animations. Triggered when a tab button is clicked */
    if (lv_event_get_code(e) == LV_EVENT_SCROLL_BEGIN) {
        lv_anim_t* a = (lv_anim_t *)lv_event_get_param(e);
        if (a)  a->time = 0;
     
    }
}


void createScreens(void)
{
  lv_obj_t * tabview;
  tabview = lv_tabview_create(lv_scr_act(), LV_DIR_LEFT, 60);
  lv_obj_clear_flag(lv_tabview_get_content(tabview), LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(lv_tabview_get_content(tabview), scroll_begin_event,LV_EVENT_SCROLL_BEGIN, NULL);

  lv_obj_t * tab_btns = lv_tabview_get_tab_btns(tabview);
  lv_obj_set_style_bg_color(tab_btns, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
  lv_obj_set_style_text_color(tab_btns, lv_palette_lighten(LV_PALETTE_GREY, 5), 0);
  lv_obj_set_style_border_side(tab_btns, LV_BORDER_SIDE_RIGHT, LV_PART_ITEMS | LV_STATE_CHECKED);

  tabHome = lv_tabview_add_tab(tabview, "Home");
  lv_obj_set_scrollbar_mode(tabHome, LV_SCROLLBAR_MODE_OFF);
  tabSerBmsOverview = lv_tabview_add_tab(tabview, "Serial\n BMS");
  tabBTBmsOverview = lv_tabview_add_tab(tabview, "  BT\nBMS");
  tabZellSpg = lv_tabview_add_tab(tabview, "Cell\nSpg.");
  tabInfo = lv_tabview_add_tab(tabview, "Info");

  lv_obj_clear_flag(lv_tabview_get_content(tabview), LV_OBJ_FLAG_SCROLLABLE);


  //Styles
  //line
  static lv_style_t style_line;
  lv_style_init(&style_line);
  lv_style_set_line_width(&style_line, 2);
  lv_style_set_line_color(&style_line, lv_palette_main(LV_PALETTE_BLUE));

  //line2
  static lv_style_t style_line2;
  lv_style_init(&style_line2);
  lv_style_set_line_width(&style_line2, 1);
  lv_style_set_line_color(&style_line2, lv_palette_main(LV_PALETTE_GREY));

  //Fontsytle: big
  static lv_style_t style_font1;
  lv_style_init(&style_font1);
  lv_style_set_text_font(&style_font1, &lv_font_montserrat_24);

  //Sytle: Kachel
  static lv_style_t style_kachel;
  lv_style_init(&style_kachel);
  lv_style_set_border_width(&style_kachel,0);
  lv_style_set_radius(&style_kachel,0);
  lv_style_set_bg_color(&style_kachel,LV_COLOR_MAKE(0xe0, 0xee, 0xee));

  //Fontsytle: Kachel
  static lv_style_t style_fontKachel;
  lv_style_init(&style_fontKachel);
  lv_style_set_text_font(&style_fontKachel, &lv_font_montserrat_16);


  //Objects
  lv_obj_t * label;
  lv_obj_t * line1;
  lv_obj_t * kachel;


  /****************************************
   * Tab HOME
   ****************************************/
  label = lv_label_create(tabHome);
  lv_label_set_recolor(label, true);
  lv_obj_add_style(label, &style_font1, 0);
  lv_label_set_text(label, "#2196F3 Battery safety controller#");
  lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);
  
  
  //Kachel 1
  kachelAlarme = lv_obj_create(tabHome);  
  lv_obj_set_style_pad_left(kachelAlarme,10,0);
  lv_obj_set_style_pad_right(kachelAlarme,10,0);
  lv_obj_set_style_pad_top(kachelAlarme,10,0);
  lv_obj_set_style_pad_bottom(kachelAlarme,10,0);
  lv_obj_add_style(kachelAlarme, &style_kachel,0);
  lv_obj_set_size(kachelAlarme, 150, 95);
  lv_obj_align(kachelAlarme, LV_ALIGN_CENTER,-90,-45);

  label = lv_label_create(kachelAlarme);
  lv_obj_add_style(label, &style_fontKachel, 0);
  lv_label_set_recolor(label, true);
  lv_label_set_text_fmt(label, "#252850 Trigger#");
  lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);

  label = lv_label_create(kachelAlarme);
  lv_obj_add_style(label, &style_fontKachel, 0);
  lv_label_set_text_fmt(label, "1..5\n6..10");
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 20);

  label = lv_label_create(kachelAlarme);
  lv_obj_add_style(label, &style_fontKachel, 0);
  lv_label_set_text_fmt(label, "%d  %d  %d  %d  %d\n%d  %d  %d  %d  %d",0,0,0,0,0,0,0,0,0,0);
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 40, 20);

  //Kachel 2
  kachelBmsError = lv_obj_create(tabHome);
  lv_obj_set_style_pad_left(kachelBmsError,10,0);
  lv_obj_set_style_pad_right(kachelBmsError,10,0);
  lv_obj_set_style_pad_top(kachelBmsError,10,0);
  lv_obj_set_style_pad_bottom(kachelBmsError,10,0);
  lv_obj_add_style(kachelBmsError, &style_kachel, 0);
  lv_obj_set_size(kachelBmsError, 150, 95);
  lv_obj_align(kachelBmsError, LV_ALIGN_CENTER, 90, -45);

  label = lv_label_create(kachelBmsError);
  lv_obj_add_style(label, &style_fontKachel, 0);
  lv_label_set_recolor(label, true);
  lv_label_set_text_fmt(label, "#252850 BMS Status#");
  lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);

  label = lv_label_create(kachelBmsError);
  lv_obj_add_style(label, &style_fontKachel, 0);
  lv_label_set_text_fmt(label, "---");
  lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 30);

  //Kachel 3
  kachelInverter = lv_obj_create(tabHome);
  lv_obj_set_style_pad_left(kachelInverter,10,0);
  lv_obj_set_style_pad_right(kachelInverter,10,0);
  lv_obj_set_style_pad_top(kachelInverter,10,0);
  lv_obj_set_style_pad_bottom(kachelInverter,10,0);
  lv_obj_add_style(kachelInverter, &style_kachel, 0);
  lv_obj_set_size(kachelInverter, 150, 95);
  lv_obj_align(kachelInverter, LV_ALIGN_CENTER, -90, 60);

  label = lv_label_create(kachelInverter);
  lv_obj_add_style(label, &style_fontKachel, 0);
  lv_label_set_recolor(label, true);
  lv_label_set_text_fmt(label, "#252850 Wechselrichter#");
  lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);

  label = lv_label_create(kachelInverter);
  lv_obj_add_style(label, &style_fontKachel, 0);
  lv_label_set_text_fmt(label, "Spg.\nStrom\nSoC");
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 20);

  label = lv_label_create(kachelInverter);
  lv_obj_add_style(label, &style_fontKachel, 0);
  lv_label_set_text_fmt(label, "%.2f V\n%.2f A\n%d %%",0.0,0.0,0);
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 65, 20);

  //Kachel 4
  kachelInverter2 = lv_obj_create(tabHome);
  lv_obj_set_style_pad_left(kachelInverter2,10,0);
  lv_obj_set_style_pad_right(kachelInverter2,10,0);
  lv_obj_set_style_pad_top(kachelInverter2,10,0);
  lv_obj_set_style_pad_bottom(kachelInverter2,10,0);
  lv_obj_add_style(kachelInverter2, &style_kachel, 0);
  lv_obj_set_size(kachelInverter2, 150, 95);
  lv_obj_align(kachelInverter2, LV_ALIGN_CENTER, 90, 60);

  label = lv_label_create(kachelInverter2);
  lv_obj_add_style(label, &style_fontKachel, 0);
  lv_label_set_recolor(label, true);
  lv_label_set_text_fmt(label, "#252850 Wechselrichter#");
  lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);

  label = lv_label_create(kachelInverter2);
  lv_obj_add_style(label, &style_fontKachel, 0);
  lv_label_set_recolor(label, true);
  lv_label_set_text_fmt(label, "#252850 max#");
  lv_obj_align(label, LV_ALIGN_TOP_MID, 2, 15);

  label = lv_label_create(kachelInverter2);
  lv_obj_add_style(label, &style_fontKachel, 0);
  lv_label_set_text_fmt(label, "Lade.\nEntl.");
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 8, 33);

  label = lv_label_create(kachelInverter2);
  lv_obj_add_style(label, &style_fontKachel, 0);
  lv_label_set_text_fmt(label, "%.2f A\n%.2f A",0,0);
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 75, 33);

  //Relais
  for(uint8_t i=0;i<6;i++)
  {
    uint16_t xpos = 10+(i*62);
    relaisState[i] = lv_obj_create(tabHome);  
    lv_obj_set_scrollbar_mode(relaisState[i], LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_style(relaisState[i], &style_kachel,0);
    lv_obj_set_size(relaisState[i], 55, 25);
    lv_obj_align(relaisState[i], LV_ALIGN_BOTTOM_LEFT, xpos, 12);
    
    label = lv_label_create(relaisState[i]);
    lv_obj_set_scrollbar_mode(label, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_style(label, &style_fontKachel, 0);
    lv_label_set_text_fmt(label, "Rel %i",i+1);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  }

  
  /****************************************
   * Tab Serial-BMS Overview
   ****************************************/
  uint16_t xPos=0, yPos=0, bmsNr=0;

  label = lv_label_create(tabSerBmsOverview);
  lv_label_set_text_fmt(label, "\n\nSpg. (V)\nCur. (A)\nSoC (%%)\nMax Cell\n(mV)\nMin Cell\n(mV)\nMax Cell\nDiff (mV)\nTemp °C\nBalance\nFehler");
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 0);
    
  for(uint8_t n=5;n<8;n++)
  {
    xPos=44*(n-5)+70;
    if(n>4)xPos+=2;
    label = lv_label_create(tabSerBmsOverview);
    lv_label_set_text_fmt(label, "S%d",bmsNr);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, xPos, yPos);
    bmsNr++;
  }

  //Draw line horizontal
  line1 = lv_line_create(tabSerBmsOverview);
  static lv_point_t line_points4[] = {{0, 22}, {198, 22}};
  lv_line_set_points(line1, line_points4, 2);
  lv_obj_add_style(line1, &style_line, 0);

  //Draw line vertical
  line1 = lv_line_create(tabSerBmsOverview);
  static lv_point_t line_points5[] = {{66, 0}, {66, 228}};
  lv_line_set_points(line1, line_points5, 2);   
  lv_obj_add_style(line1, &style_line, 0);



  /****************************************
   * Tab BT-BMS Overview
   ****************************************/
  label = lv_label_create(tabBTBmsOverview);
  lv_label_set_text_fmt(label, "\n\nSpg. (V)\nCur. (A)\nSoC (%%)\nMax Cell\n(mV)\nMin Cell\n(mV)\nMax Cell\nDiff (mV)\nTemp °C\nBalance\nFehler");
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 0);
  
  yPos=0;
  bmsNr=0;
  for(uint8_t n=0;n<5;n++)
  {
    xPos=44*n+70;
    if(n>4)xPos+=2;
    label = lv_label_create(tabBTBmsOverview);
    lv_label_set_text_fmt(label, "Bt%d",bmsNr);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, xPos, yPos);
    bmsNr++;
  }

  //Draw line horizontal
  line1 = lv_line_create(tabBTBmsOverview);
  static lv_point_t line_points6[] = {{0, 22}, {286, 22}};
  lv_line_set_points(line1, line_points6, 2);   
  lv_obj_add_style(line1, &style_line, 0);

  //Draw line vertical
  line1 = lv_line_create(tabBTBmsOverview);
  static lv_point_t line_points7[] = {{66, 0}, {66, 228}};
  lv_line_set_points(line1, line_points7, 2);   
  lv_obj_add_style(line1, &style_line, 0);


  /****************************************
   * Tab Zellspannungen
   ****************************************/
  label = lv_label_create(tabZellSpg);
  lv_label_set_text_fmt(label, "%s\n\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d","mV",1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16);
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 0);

  //Bluetooth
  uint8_t n;
  bmsNr=0;
  for(n=0;n<5;n++)
  {
    xPos=44*n+33;
    if(n>4)xPos+=8;
    label = lv_label_create(tabZellSpg);
    lv_label_set_text_fmt(label, "Bt%d",bmsNr);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, xPos, 0);
    bmsNr++;
  }

  //Serial
  for(uint8_t s=n;s<8;s++)
  {
    xPos=44*s+33;
    if(s>4)xPos+=8;
    label = lv_label_create(tabZellSpg);
    lv_label_set_text_fmt(label, "S%d",bmsNr-5);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, xPos, 0);
    bmsNr++;
  }

  //Draw line top horizontal
  line1 = lv_line_create(tabZellSpg);
  static lv_point_t line_points[] = {{0, 22}, {385, 22}};
  lv_line_set_points(line1, line_points, 2);   
  lv_obj_add_style(line1, &style_line, 0);

  //Draw line left vertical
  line1 = lv_line_create(tabZellSpg);
  static lv_point_t line_points2[] = {{28, 0}, {28, 285}};
  lv_line_set_points(line1, line_points2, 2);   
  lv_obj_add_style(line1, &style_line, 0);

  //Draw line right vertical
  line1 = lv_line_create(tabZellSpg);
  static lv_point_t line_points3[] = {{253, 0}, {253, 285}};
  lv_line_set_points(line1, line_points3, 2);   
  lv_obj_add_style(line1, &style_line2, 0);


  /****************************************
   * Tab Info
   ****************************************/
  
  //Headline
  label = lv_label_create(tabInfo);
  lv_label_set_recolor(label, true);
  lv_obj_add_style(label, &style_font1, 0);
  lv_label_set_text(label, "#2196F3 Battery safety controller#");
  lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);
 
  //Display
  label = lv_label_create(tabInfo);
  lv_label_set_text_fmt(label, "Display Firmware Version: %s",BSCD_FW_VERSION);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, -60);

  //----------------------
  /* TODO Integrieren mit spezial Display FW
  //*** IP-Address ***
  label = lv_label_create(tabInfo);
  lv_label_set_text(label, "IP Adresse:");
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 60);

  label = lv_label_create(tabInfo);
  lv_label_set_text_fmt(label, "%s","---");
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 150, 60);

  //WLAN-Mode
  label = lv_label_create(tabInfo);
  lv_label_set_text(label, "WLAN Mode:");
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 80);

  label = lv_label_create(tabInfo);
  lv_label_set_text_fmt(label, "%s","---");
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 150, 80);
  
  //*** Firmware Version ***
  label = lv_label_create(tabInfo);
  lv_label_set_text(label, "Firmware-Version ");
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 110);

  //Underlined
  line1 = lv_line_create(tabInfo);
  static lv_point_t line_points8[] = {{0, 125}, {130, 125}};
  lv_line_set_points(line1, line_points8, 2);   
  lv_obj_add_style(line1, &style_line2, 0);

  /* TODO Integrieren mit spezial Display FW
  //Mainboard
  label = lv_label_create(tabInfo);
  lv_label_set_text(label, "Mainboard: " );
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 130);

  label = lv_label_create(tabInfo);
  lv_label_set_text_fmt(label, "%s","---");
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 150, 130);


  //Display
  label = lv_label_create(tabInfo);
  lv_label_set_text(label, "Display: ");
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 150);

  label = lv_label_create(tabInfo);
  lv_label_set_text_fmt(label, "%s",BSCD_FW_VERSION);
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 150, 150);
*/
  //----------------------

  //Footer
  label = lv_label_create(tabInfo);
  lv_label_set_text(label, "https://github.com/shining-man/bsc_fw\n          https://www.BSC-Shop.com");
  lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, 0);
}


void displayNewBscData()
{
  if(!hasNewDisplayData()) return;

  lv_obj_t *label;
  uint8_t u8_lObjCnt;
  bool bo_lBmsHasError=false;

  /****************************************
   * Tab Home Overview
   ****************************************/
  //Kachel1; Alarme 
  label = lv_obj_get_child(kachelAlarme, 2);
  uint16_t u16_lAlarme = lDataDisp->bscAlarms;
  uint8_t alms[10];
  for(uint8_t i=0;i<10;i++)
  {
    if((u16_lAlarme & (1<<i)) == (1<<i)) alms[i]=1;
    else alms[i]=0;
  }
  lv_label_set_text_fmt(label, "%d  %d  %d  %d  %d\n%d  %d  %d  %d  %d",
    alms[0],alms[1],alms[2],alms[3],alms[4],alms[5],alms[6],alms[7],alms[8],alms[9]);

  //Kachel2; BMS Status
  label = lv_obj_get_child(kachelBmsError, 1);
  lv_label_set_recolor(label, true);
  if(bo_lBmsHasError) lv_label_set_text_fmt(label, "#FF0000 Error#");
  else lv_label_set_text_fmt(label, "#00FF00 OK#");
  //lv_label_set_text(labelDisp, "test");

  //Kachel3; Inverter 
  label = lv_obj_get_child(kachelInverter, 2);
  lv_label_set_text_fmt(label, "%.2f V\n%.2f A\n%d %%",lDataDisp->inverterVoltage/100.0,lDataDisp->inverterCurrent/10.0,lDataDisp->inverterSoc);

  //Kachel4; Inverter 2
  label = lv_obj_get_child(kachelInverter2, 3);
  lv_label_set_text_fmt(label, "%d A\n%d A\n",lDataDisp->inverterChargeCurrent,lDataDisp->inverterDischargeCurrent);

  //Relais
  uint8_t u8_lRelais = lDataDisp->bscRelais;
  uint8_t u8_lRelNr=0;
  for(uint8_t i=0;i<6;i++)
  {
    if((u8_lRelais>>i)&0x1) lv_obj_set_style_bg_color(relaisState[u8_lRelNr],LV_COLOR_MAKE(0xff, 0x00, 0x00),LV_PART_MAIN);
    else lv_obj_set_style_bg_color(relaisState[u8_lRelNr],LV_COLOR_MAKE(0x00, 0xff, 0x00),LV_PART_MAIN);
    u8_lRelNr++;
  }


  /****************************************
   * Tab Serial-BMS Overview
   ****************************************/
  u8_lObjCnt=1;
  String str_lIsBalance, str_lError;
  
  for(uint8_t i=5;i<8;i++)
  {
    str_lIsBalance="AUS";
    str_lError="#00ff00 OK#";
    if(lDataDisp->bmsIsBalancingActive[i]>0)str_lIsBalance="EIN";
    if(lDataDisp->bmsErrors[i]>0)
    {
      str_lError="#ff0000 ERR#";
      bo_lBmsHasError=true;
    }

    label = lv_obj_get_child(tabSerBmsOverview, u8_lObjCnt);

    if((lDataDisp->bmsMaxCellVoltage[i] != UINT16_MAX) && (lDataDisp->bmsMaxCellVoltage[i] != 0))       //Gerät verfügbar
    {
      lv_label_set_recolor(label, true);
      lv_label_set_text_fmt(label, "S%d\n\n%.1f\n%.1f\n%d\n%d\n\n%d\n\n%d\n\n%.1f\n%s\n%s", u8_lObjCnt-1,
      lDataDisp->bmsTotalVoltage[i]/100.0, lDataDisp->bmsTotalCurrent[i]/100.0, lDataDisp->bmsChargePercentage[i], lDataDisp->bmsMaxCellVoltage[i],
      lDataDisp->bmsMinCellVoltage[i], lDataDisp->bmsMaxCellDifferenceVoltage[i], lDataDisp->bmsTemperature[i][0]/100.0, str_lIsBalance.c_str(),
      str_lError.c_str());
    }
    else                                                              //Gerät nicht verfügbar -> Spalte ausblenden
    {
      lv_label_set_text_fmt(label, "S%d", u8_lObjCnt-1);               //Kopfzeile setzen
    }

    u8_lObjCnt++;
  }


  /****************************************
   * Tab BT-BMS Overview
   ****************************************/
  u8_lObjCnt=1;

  for(uint8_t i=0;i<5;i++)
  {
    str_lIsBalance="AUS";
    str_lError="#00ff00 OK#";
    if(lDataDisp->bmsIsBalancingActive[i]>0)str_lIsBalance="EIN";
    if(lDataDisp->bmsErrors[i]>0)
    {
      str_lError="#ff0000 ERR#";
      bo_lBmsHasError=true;
    }

    label = lv_obj_get_child(tabBTBmsOverview, u8_lObjCnt);
  
    //TODO Online/Offline-Flag nutzen (noch nicht in i2c drin auf Display-Seite)
    if((lDataDisp->bmsMaxCellVoltage[i] != UINT16_MAX) && (lDataDisp->bmsMaxCellVoltage[i] != 0))       //Gerät verfügbar
    {
      lv_label_set_recolor(label, true);
      lv_label_set_text_fmt(label, "Bt%d\n\n%.1f\n%.1f\n%d\n%d\n\n%d\n\n%d\n\n%.1f\n%s\n%s", u8_lObjCnt-1,
      lDataDisp->bmsTotalVoltage[i]/100.0, lDataDisp->bmsTotalCurrent[i]/100.0, lDataDisp->bmsChargePercentage[i], lDataDisp->bmsMaxCellVoltage[i],
      lDataDisp->bmsMinCellVoltage[i], lDataDisp->bmsMaxCellDifferenceVoltage[i], lDataDisp->bmsTemperature[i][0]/100.0, str_lIsBalance.c_str(),
      str_lError.c_str());
    }
    else                                                              //Gerät nicht verfügbar -> Spalte ausblenden
    {
      lv_label_set_text_fmt(label, "Bt%d", u8_lObjCnt-1);               //Kopfzeile setzen
    }

    u8_lObjCnt++;
  }


  /****************************************
   * Tab Zellspannungen Overview
   ****************************************/
  u8_lObjCnt=1;
  String DevType = "Bt";
  uint8_t DevID = u8_lObjCnt-1;

  for(uint8_t i=0;i<8;i++)
  {
    if(i>4)                                                                       //Serial BMS werden mit "S" benannt und ab 0 gezählt
    {
      DevType = "S";
      DevID = (u8_lObjCnt-1)-5;
    }
    else
    {
      DevID = (u8_lObjCnt-1);
    }

    label = lv_obj_get_child(tabZellSpg, u8_lObjCnt);

    if((lDataDisp->bmsCellVoltage[i][0] != UINT16_MAX) && (lDataDisp->bmsCellVoltage[i][0] != 0))       //Gerät verfügbar
    {      
      lv_label_set_text_fmt(label, "%s%d\n\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d", DevType.c_str(), DevID,
      lDataDisp->bmsCellVoltage[i][0], lDataDisp->bmsCellVoltage[i][1], lDataDisp->bmsCellVoltage[i][2], lDataDisp->bmsCellVoltage[i][3],
      lDataDisp->bmsCellVoltage[i][4], lDataDisp->bmsCellVoltage[i][5], lDataDisp->bmsCellVoltage[i][6], lDataDisp->bmsCellVoltage[i][7],
      lDataDisp->bmsCellVoltage[i][8], lDataDisp->bmsCellVoltage[i][9], lDataDisp->bmsCellVoltage[i][10], lDataDisp->bmsCellVoltage[i][11],
      lDataDisp->bmsCellVoltage[i][12], lDataDisp->bmsCellVoltage[i][13], lDataDisp->bmsCellVoltage[i][14], lDataDisp->bmsCellVoltage[i][15]);
    }
    else                                                                          //Gerät nicht verfügbar -> Spalte ausblenden
    {
      lv_label_set_text_fmt(label, "%s%d", DevType.c_str(), DevID);               //Kopfzeile setzen
    }

    u8_lObjCnt++;
  }
  

  /****************************************
   * Tab BT-BMS Overview
   ****************************************/
  /* TODO Integrieren mit spezial Display FW
  //IP-Adress
  label = lv_obj_get_child(tabInfo, 3);
  lv_label_set_text_fmt(label, "%s",lDataDisp->bscIpAdr);

  //WLAN-Mode
  label = lv_obj_get_child(tabInfo, 4);
  lv_label_set_text_fmt(label, "%s",lDataDisp->bscWlanMode);        

  //Mainboard FW-Version
  label = lv_obj_get_child(tabInfo, 5);
  lv_label_set_text_fmt(label, "%s",lDataDisp->bscFwVersion);
*/
  //Displaytimeout
  u8_mPowersaveTime=lDataDisp->displayTimeout;
}