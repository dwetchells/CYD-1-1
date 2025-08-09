/*  Rui Santos & Sara Santos - Random Nerd Tutorials - https://RandomNerdTutorials.com/esp32-lvgl-ebook/
    THIS EXAMPLE WAS TESTED WITH THE FOLLOWING HARDWARE:
    1) ESP32-2432S028R 2.8 inch 240×320 also known as the Cheap Yellow Display (CYD): https://makeradvisor.com/tools/cyd-cheap-yellow-display-esp32-2432s028r/
      SET UP INSTRUCTIONS: https://RandomNerdTutorials.com/cyd-lvgl/
    2) REGULAR ESP32 Dev Board + 2.8 inch 240x320 TFT Display: https://makeradvisor.com/tools/2-8-inch-ili9341-tft-240x320/ and https://makeradvisor.com/tools/esp32-dev-board-wi-fi-bluetooth/
      SET UP INSTRUCTIONS: https://RandomNerdTutorials.com/esp32-tft-lvgl/
    Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
    The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*/

/*  Install the "lvgl" library version 9.X by kisvegabor to interface with the TFT Display - https://lvgl.io/
    *** IMPORTANT: lv_conf.h available on the internet will probably NOT work with the examples available at Random Nerd Tutorials ***
    *** YOU MUST USE THE lv_conf.h FILE PROVIDED IN THE LINK BELOW IN ORDER TO USE THE EXAMPLES FROM RANDOM NERD TUTORIALS ***
    FULL INSTRUCTIONS AVAILABLE ON HOW CONFIGURE THE LIBRARY: https://RandomNerdTutorials.com/cyd-lvgl/ or https://RandomNerdTutorials.com/esp32-tft-lvgl/   */
#include <lvgl.h>

/*  Install the "TFT_eSPI" library by Bodmer to interface with the TFT Display - https://github.com/Bodmer/TFT_eSPI
    *** IMPORTANT: User_Setup.h available on the internet will probably NOT work with the examples available at Random Nerd Tutorials ***
    *** YOU MUST USE THE User_Setup.h FILE PROVIDED IN THE LINK BELOW IN ORDER TO USE THE EXAMPLES FROM RANDOM NERD TUTORIALS ***
    FULL INSTRUCTIONS AVAILABLE ON HOW CONFIGURE THE LIBRARY: https://RandomNerdTutorials.com/cyd-lvgl/ or https://RandomNerdTutorials.com/esp32-tft-lvgl/   */
#include <TFT_eSPI.h>

// Install the "XPT2046_Touchscreen" library by Paul Stoffregen to use the Touchscreen - https://github.com/PaulStoffregen/XPT2046_Touchscreen - Note: this library doesn't require further configuration
#include <XPT2046_Touchscreen.h>

// Touchscreen pins
#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 32  // T_DIN
#define XPT2046_MISO 39  // T_OUT
#define XPT2046_CLK 25   // T_CLK
#define XPT2046_CS 33    // T_CS

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

// Change this variable to set your unique pass code required to unlock the screen
String pass_code = "12345";

// Touchscreen coordinates: (x, y) and pressure (z)
int x, y, z;

#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

#define CYD_LED_BLUE 17

// predefie subroutine to compile code
void lv_create_unlock_screen(void);

// If logging is enabled, it will inform the user about what is happening in the library
void log_print(lv_log_level_t level, const char * buf) {
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}

// Get the Touchscreen data
void touchscreen_read(lv_indev_t * indev, lv_indev_data_t * data) {
  // Checks if Touchscreen was touched, and prints X, Y and Pressure (Z)
  if(touchscreen.tirqTouched() && touchscreen.touched()) {
    // Get Touchscreen points
    TS_Point p = touchscreen.getPoint();

    // Advanced Touchscreen calibration, LEARN MORE » https://RandomNerdTutorials.com/touchscreen-calibration/
    float alpha_x, beta_x, alpha_y, beta_y, delta_x, delta_y;

    // REPLACE WITH YOUR OWN CALIBRATION VALUES » https://RandomNerdTutorials.com/touchscreen-calibration/
    alpha_x = -0.000;
    beta_x = 0.090;
    delta_x = -35.822;
    alpha_y = 0.066;
    beta_y = 0.000;
    delta_y = -14.462;

    x = alpha_y * p.x + beta_y * p.y + delta_y;
    // clamp x between 0 and SCREEN_WIDTH - 1
    x = max(0, x);
    x = min(SCREEN_WIDTH - 1, x);

    y = alpha_x * p.x + beta_x * p.y + delta_x;
    // clamp y between 0 and SCREEN_HEIGHT - 1
    y = max(0, y);
    y = min(SCREEN_HEIGHT - 1, y);

    // Basic Touchscreen calibration points with map function to the correct width and height
    //x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    //y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);

    z = p.z;

    data->state = LV_INDEV_STATE_PRESSED;

    // Set the coordinates
    data->point.x = x;
    data->point.y = y;

    // Print Touchscreen info about X, Y and Pressure (Z) on the Serial Monitor
    Serial.print("X = ");
    Serial.print(x);
    Serial.print(" | Y = ");
    Serial.print(y);
    Serial.print(" | Pressure = ");
    Serial.print(z);
    Serial.println();
  }
  else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

static void text_area_event_handler(lv_event_t * e) {
  lv_obj_t * text_area = (lv_obj_t*) lv_event_get_target(e);
  LV_UNUSED(text_area);
  LV_LOG_USER("Enter was pressed. The current text is: %s", lv_textarea_get_text(text_area));

  if(pass_code==String(lv_textarea_get_text(text_area))){
    Serial.println(int(lv_textarea_get_text(text_area)));
    lv_create_unlock_screen();
    LV_LOG_USER("Pass code is correct.");
  }
  else {
    lv_obj_t * msg_box = lv_msgbox_create(NULL);
    lv_msgbox_add_title(msg_box, "WRONG PASS CODE");
    lv_msgbox_add_close_button(msg_box);
    LV_LOG_USER("Wrong pass code. Try again.");
  }
}

static void button_matrix_event_handler(lv_event_t * e) {
  lv_obj_t * obj = (lv_obj_t*) lv_event_get_target(e);
  lv_obj_t * text_area = (lv_obj_t*) lv_event_get_user_data(e);
  const char * txt = lv_buttonmatrix_get_button_text(obj, lv_buttonmatrix_get_selected_button(obj));

  if(lv_strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
    lv_textarea_delete_char(text_area);
  }
  else if(lv_strcmp(txt, LV_SYMBOL_OK) == 0) {
    lv_obj_send_event(text_area, LV_EVENT_READY, NULL);
  }
  else {
    lv_textarea_add_text(text_area, txt);
  }
}

// Callback that is triggered when the toggle switch changes state
static void toggle_switch_event_handler(lv_event_t * e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t * obj = (lv_obj_t*) lv_event_get_target(e);
  if(code == LV_EVENT_VALUE_CHANGED) {
    LV_UNUSED(obj);
    LV_LOG_USER("State: %s", lv_obj_has_state(obj, LV_STATE_CHECKED) ? "On" : "Off");
    // Cheap Yellow Display built-in RGB LED is controlled with inverted logic
    digitalWrite(CYD_LED_BLUE, !lv_obj_has_state(obj, LV_STATE_CHECKED));
    // If you are using a regular LED, comment previous line and uncomment the next line 
    //digitalWrite(CYD_LED_BLUE, lv_obj_has_state(obj, LV_STATE_CHECKED));
  }
}

void lv_create_lock_screen(void) {
  // Clear screen
  lv_obj_clean(lv_scr_act());

  lv_obj_t * text_area = lv_textarea_create(lv_screen_active());
  lv_textarea_set_one_line(text_area, true);
  lv_obj_align(text_area, LV_ALIGN_TOP_MID, 0, 40);
  lv_textarea_set_password_mode(text_area, true);

  lv_obj_t * text_label = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label, "ENTER PASS CODE TO UNLOCK");
  lv_obj_align_to(text_label, text_area, LV_ALIGN_TOP_MID, 0, -30);

  lv_obj_add_event_cb(text_area, text_area_event_handler, LV_EVENT_READY, text_area);
  lv_obj_add_state(text_area, LV_STATE_FOCUSED); // To be sure the cursor is visible

  static const char * keypad[]= {  "1", "2", "3", "\n",
                                   "4", "5", "6", "\n",
                                   "7", "8", "9", "\n",
                                   LV_SYMBOL_BACKSPACE, "0", LV_SYMBOL_OK, ""
                                };

  lv_obj_t * button_matrix = lv_buttonmatrix_create(lv_screen_active());
  lv_obj_set_size(button_matrix, 200, 150);
  lv_obj_align(button_matrix, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_obj_add_event_cb(button_matrix, button_matrix_event_handler, LV_EVENT_VALUE_CHANGED, text_area);
  lv_obj_remove_flag(button_matrix, LV_OBJ_FLAG_CLICK_FOCUSABLE); // To keep the text area focused on button clicks
  lv_buttonmatrix_set_map(button_matrix, keypad);
}

static void float_button_event_cb(lv_event_t * e) {
  lv_create_lock_screen();
}

void lv_create_unlock_screen(void) {
  // Clear screen
  lv_obj_clean(lv_scr_act());
  // Create a toggle switch (toggle_switch)
  lv_obj_t * toggle_switch = lv_switch_create(lv_screen_active());
  lv_obj_add_event_cb(toggle_switch, toggle_switch_event_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_flag(toggle_switch, LV_OBJ_FLAG_EVENT_BUBBLE);
  // If the LED is on, turns the toggle switch on (checked)
  if(!digitalRead(CYD_LED_BLUE)){
    lv_obj_add_state(toggle_switch, LV_STATE_CHECKED);
  }
  lv_obj_align(toggle_switch, LV_ALIGN_CENTER, 0, 0);

  // Create floating button
  lv_obj_t * float_button = lv_button_create(lv_screen_active());
  lv_obj_set_size(float_button, 40, 40);
  lv_obj_add_flag(float_button, LV_OBJ_FLAG_FLOATING);
  lv_obj_align(float_button, LV_ALIGN_BOTTOM_LEFT, 15, -15);
  lv_obj_add_event_cb(float_button, float_button_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_set_style_radius(float_button, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_image_src(float_button, LV_SYMBOL_CLOSE, 0);
  lv_obj_set_style_text_font(float_button, lv_theme_get_font_large(float_button), 0);
  lv_obj_set_style_bg_color(float_button, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
}

void setup() {
  String LVGL_Arduino = String("LVGL Library Version: ") + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.begin(115200);
  Serial.println(LVGL_Arduino);
  
  pinMode(CYD_LED_BLUE, OUTPUT);
  // Cheap Yellow Display built-in RGB LED is controlled with inverted logic
  digitalWrite(CYD_LED_BLUE, HIGH);
  // If you are using a regular LED, comment previous line and uncomment the next line 
  //digitalWrite(CYD_LED_BLUE, LOW);

  // Start LVGL
  lv_init();
  // Register print function for debugging
  lv_log_register_print_cb(log_print);

  // Start the SPI for the touchscreen and init the touchscreen
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  // Set the Touchscreen rotation in landscape mode
  // Note: in some displays, the touchscreen might be upside down, so you might need to set the rotation to 0: touchscreen.setRotation(0);
  touchscreen.setRotation(2);

  // Create a display object
  lv_display_t * disp;
  // Initialize the TFT display using the TFT_eSPI library
  disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);
  
  // Initialize an LVGL input device object (Touchscreen)
  lv_indev_t * indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  // Set the callback function to read Touchscreen input
  lv_indev_set_read_cb(indev, touchscreen_read);

  // Function to draw the lock screen with keypad
  lv_create_lock_screen();
}

void loop() {
  lv_task_handler();  // let the GUI do its work
  lv_tick_inc(5);     // tell LVGL how much time has passed
  delay(5);           // let this time pass
}