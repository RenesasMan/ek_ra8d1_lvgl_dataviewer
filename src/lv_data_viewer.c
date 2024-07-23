/**
 * @file lv_demo_widgets.c
 *
 */

 /*********************
  *      INCLUDES
  *********************/
#include "lv_data_viewer.h"
#include <math.h>
#include <stdio.h>
#include <stdint.h>

#if LV_USE_DATA_VIEWER

#define MY_DATA_SIZE 1024
#define CHART_FPS 20

#if LV_USE_STDLIB_MALLOC == LV_STDLIB_BUILTIN && LV_MEM_SIZE < (38ul * 1024ul)
#error Insufficient memory for lv_demo_widgets. Please set LV_MEM_SIZE to at least 38KB (38ul * 1024ul).  48KB is recommended.
#endif

  /*********************
   *      DEFINES
   *********************/

   /**********************
    *      TYPEDEFS
    **********************/
typedef enum {
    DISP_SMALL,
    DISP_MEDIUM,
    DISP_LARGE,
} disp_size_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void logs_tab_create(lv_obj_t* parent);
static void analytics_tab_create(lv_obj_t* parent);
static void chart_tab_create(lv_obj_t* parent);

static void tabview_delete_event_cb(lv_event_t* e);

/**********************
 *  STATIC VARIABLES
 **********************/
static disp_size_t disp_size;

static lv_obj_t* tv;
static lv_style_t style_text_muted;
static lv_style_t style_title;
static lv_style_t style_icon;
static lv_style_t style_bullet;


static const lv_font_t* font_large;
static const lv_font_t* font_normal;

static lv_obj_t* button_domain;
static lv_obj_t* label_button_domain;
static lv_obj_t* button_data_select;
static lv_obj_t* label_button_data_select;

#define UNFILTERED_BIT_MASK 1<<0; //the 0th bit enables the unfiltered series
#define FILTERED_BIT_MASK 1<<1; //the 1st bit enables the filtered series
#define DOMAIN_BIT_MASK 1<<2; //the 2nd bit masks which series is enabled

static uint8_t g_series_hidden_bitfield = 0b011; //default is frequency domain, unfiltered

lv_obj_t* chart_time;
lv_obj_t* chart_freq;
lv_obj_t* scale_bottom_freq;
lv_obj_t* scale_bottom_time;

lv_chart_series_t* series_fft_unfiltered;
lv_chart_series_t* series_fft_filtered;
lv_chart_series_t* series_analog_unfiltered;
lv_chart_series_t* series_analog_filtered;

/**********************
 *      MACROS
 **********************/

 /**********************
  *   GLOBAL FUNCTIONS
  **********************/

void lv_data_viewer(void)
{
    if (LV_HOR_RES <= 320) disp_size = DISP_SMALL;
    else if (LV_HOR_RES < 720) disp_size = DISP_MEDIUM;
    else disp_size = DISP_LARGE;

    font_large = LV_FONT_DEFAULT;
    font_normal = LV_FONT_DEFAULT;

    int32_t tab_h;
    if (disp_size == DISP_LARGE) {
        tab_h = 70;
#if LV_FONT_MONTSERRAT_24
        font_large = &lv_font_montserrat_24;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_24 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
#if LV_FONT_MONTSERRAT_16
        font_normal = &lv_font_montserrat_16;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_16 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
    }
    else if (disp_size == DISP_MEDIUM) {
        tab_h = 45;
#if LV_FONT_MONTSERRAT_20
        font_large = &lv_font_montserrat_20;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_20 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
#if LV_FONT_MONTSERRAT_14
        font_normal = &lv_font_montserrat_14;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_14 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
    }
    else {   /* disp_size == DISP_SMALL */
        tab_h = 45;
#if LV_FONT_MONTSERRAT_18
        font_large = &lv_font_montserrat_18;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_18 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
#if LV_FONT_MONTSERRAT_12
        font_normal = &lv_font_montserrat_12;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_12 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
    }

#if LV_USE_THEME_DEFAULT
    lv_theme_default_init(NULL, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), LV_THEME_DEFAULT_DARK,
        font_normal);
#endif

    lv_style_init(&style_text_muted);
    lv_style_set_text_opa(&style_text_muted, LV_OPA_50);

    lv_style_init(&style_title);
    lv_style_set_text_font(&style_title, font_large);

    lv_style_init(&style_icon);
    lv_style_set_text_color(&style_icon, lv_theme_get_color_primary(NULL));
    lv_style_set_text_font(&style_icon, font_large);

    lv_style_init(&style_bullet);
    lv_style_set_border_width(&style_bullet, 0);
    lv_style_set_radius(&style_bullet, LV_RADIUS_CIRCLE);

    tv = lv_tabview_create(lv_screen_active());
    lv_tabview_set_tab_bar_size(tv, tab_h);
    lv_obj_add_event_cb(tv, tabview_delete_event_cb, LV_EVENT_DELETE, NULL);

    lv_obj_set_style_text_font(lv_screen_active(), font_normal, 0);

    lv_obj_t* t1 = lv_tabview_add_tab(tv, "Graph");
    lv_obj_t* t2 = lv_tabview_add_tab(tv, "Analytics");
    lv_obj_t* t3 = lv_tabview_add_tab(tv, "Logs");

    chart_tab_create(t1);
    analytics_tab_create(t2);
    logs_tab_create(t3);

}

#define VALUE_BOX_HEIGHT 40
#define VALUE_BOX_WIDTH 40
#define VALUE_BOX_YOFFSET 10
#define VALUE_BOX_XOFFSET 0
#define VALUE_BOX_TEXT_YOFFSET 10 //the Y padding from the top of the box to the text
#define VALUE_BOX_TEXT_XOFFSET 10 //the Y padding from the top of the box to the text

static void event_chart_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* chart = lv_event_get_target(e);



    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_invalidate(chart);
    }
    if (code == LV_EVENT_REFR_EXT_DRAW_SIZE) {
        int32_t* s = lv_event_get_param(e);
        *s = LV_MAX(*s, 20);
    }
    else if (code == LV_EVENT_DRAW_TASK_ADDED) {
        lv_draw_task_t* draw_task = lv_event_get_param(e);
        lv_draw_dsc_base_t* base_dsc = draw_task->draw_dsc;

    }
    else if (code == LV_EVENT_DRAW_POST_END) {
        int32_t id = lv_chart_get_pressed_point(chart);
        if (id == LV_CHART_POINT_NONE) return;

        LV_LOG_USER("Selected point %d", (int)id);

        lv_chart_series_t* ser = lv_chart_get_series_next(chart, NULL);
        while (ser) {
            lv_point_t p;
            lv_chart_get_point_pos_by_id(chart, ser, id, &p);

            int32_t* y_array = lv_chart_get_y_array(chart, ser);
            int32_t value = y_array[id];

//            char buf[16];
//            lv_snprintf(buf, sizeof(buf), LV_SYMBOL_DUMMY"$%d", value);

            char buf[8];
            lv_snprintf(buf, sizeof(buf), "%"LV_PRIu32, value);


            lv_draw_rect_dsc_t draw_rect_dsc;
            lv_draw_rect_dsc_init(&draw_rect_dsc);
            draw_rect_dsc.bg_color = lv_color_black();
            draw_rect_dsc.bg_opa = LV_OPA_50;
            draw_rect_dsc.radius = 3;
            draw_rect_dsc.bg_image_recolor = lv_color_white();

            volatile lv_area_t text_area_bg;
            text_area_bg.x1 = chart->coords.x1 + p.x - VALUE_BOX_WIDTH / 2;
            text_area_bg.x2 = chart->coords.x1 + p.x + VALUE_BOX_WIDTH / 2;
            text_area_bg.y1 = chart->coords.y1 + p.y - VALUE_BOX_HEIGHT / 2 - VALUE_BOX_YOFFSET;
            text_area_bg.y2 = chart->coords.y1 + p.y - VALUE_BOX_HEIGHT / 2 + VALUE_BOX_YOFFSET;

            lv_area_t text_area;
            text_area.x1 = chart->coords.x1 + p.x - VALUE_BOX_WIDTH / 2 + LV_DPX(10);
            text_area.x2 = chart->coords.x1 + p.x + VALUE_BOX_WIDTH / 2;
            text_area.y1 = chart->coords.y1 + p.y - VALUE_BOX_HEIGHT / 2 - VALUE_BOX_YOFFSET;
            text_area.y2 = chart->coords.y1 + p.y - VALUE_BOX_HEIGHT / 2 + VALUE_BOX_YOFFSET;


            lv_layer_t* layer = lv_event_get_layer(e);
            lv_draw_rect(layer, &draw_rect_dsc, &text_area_bg);

            lv_draw_label_dsc_t label_dsc;
            lv_draw_label_dsc_init(&label_dsc);
            label_dsc.color = lv_color_white();
            label_dsc.font = font_normal;
            label_dsc.text = buf;
            label_dsc.text_local = true;
            //lv_style_set_text_align(&label_dsc, LV_TEXT_ALIGN_CENTER);
            lv_draw_label(layer, &label_dsc, &text_area);

            ser = lv_chart_get_series_next(chart, ser);


        }
    }
    else if (code == LV_EVENT_RELEASED) {
        lv_obj_invalidate(chart);
    }
}

#define CHART_TYPE LV_CHART_TYPE_LINE
#define CHART1_MIN 0
#define CHART1_MAX 50
#define CHART2_MIN 50
#define CHART2_MAX 100
#define CHART3_MIN 0
#define CHART3_MAX 50
#define CHART4_MIN 50
#define CHART4_MAX 100

// This function activates upon press of the Frequency/Time domain button
// This toggles the visibility and label of the F/T button domain
//

static void event_button_domain_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_VALUE_CHANGED)
    {
        if (lv_obj_has_flag(chart_time, LV_OBJ_FLAG_HIDDEN))
        {
            lv_obj_add_flag(chart_freq, LV_OBJ_FLAG_HIDDEN); //hide freq chart
            lv_obj_add_flag(scale_bottom_freq, LV_OBJ_FLAG_HIDDEN); //hide freq scale


            lv_obj_remove_flag(chart_time, LV_OBJ_FLAG_HIDDEN); //show time chart
            lv_obj_remove_flag(scale_bottom_time, LV_OBJ_FLAG_HIDDEN); //show time scale
            lv_label_set_text(label_button_domain, "Time Domain");

        }
        else
        {
            lv_obj_add_flag(chart_time, LV_OBJ_FLAG_HIDDEN); //hide time chart
            lv_obj_add_flag(scale_bottom_time, LV_OBJ_FLAG_HIDDEN); //hide time scale

            lv_obj_remove_flag(chart_freq, LV_OBJ_FLAG_HIDDEN); //show freq chart
            lv_obj_remove_flag(scale_bottom_freq, LV_OBJ_FLAG_HIDDEN); //show freq scale

            lv_label_set_text(label_button_domain, "Frequency Domain");
        }
    }
}



static void event_button_data_select_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED)
    {
        //Case 1: From all visible
        // Only show unfiltered series
        if (0b11u == g_series_hidden_bitfield)
        {
            g_series_hidden_bitfield = 0b10u;
            lv_chart_hide_series(chart_freq, series_fft_unfiltered, 0);
            lv_chart_hide_series(chart_freq, series_fft_filtered, 1);
            lv_chart_hide_series(chart_freq, series_analog_unfiltered, 0);
            lv_chart_hide_series(chart_freq, series_analog_filtered, 1);
            lv_label_set_text(label_button_data_select, "Unfiltered");
            lv_obj_set_style_bg_color(button_data_select, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
        }
        //Case 2: From all visible
        // Only show unfiltered series
        else if (0b10u == g_series_hidden_bitfield)
        {
            g_series_hidden_bitfield = 0b01;
            lv_chart_hide_series(chart_freq, series_fft_unfiltered, 1);
            lv_chart_hide_series(chart_freq, series_fft_filtered, 0);
            lv_chart_hide_series(chart_freq, series_analog_unfiltered, 1);
            lv_chart_hide_series(chart_freq, series_analog_filtered, 0);
            lv_label_set_text(label_button_data_select, "Filtered");
            lv_obj_set_style_bg_color(button_data_select, lv_palette_main(LV_PALETTE_ORANGE), LV_PART_MAIN);
        }
        else //case of 0b01
        {
            g_series_hidden_bitfield = 0b11;
            lv_chart_hide_series(chart_freq, series_fft_unfiltered, 0);
            lv_chart_hide_series(chart_freq, series_fft_filtered, 0);
            lv_chart_hide_series(chart_freq, series_analog_unfiltered, 0);
            lv_chart_hide_series(chart_freq, series_analog_filtered, 0);
            lv_label_set_text(label_button_data_select, "Filtered & Unfiltered");
            lv_obj_set_style_bg_color(button_data_select, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);
        }

        lv_chart_refresh(chart_time); /*Required after direct set*/
        lv_chart_refresh(chart_freq); /*Required after direct set*/

    }
}



void my_timer(lv_timer_t * timer)
{
//  /*Use the user_data*/
//  uint32_t * user_data = timer->user_data;
//  printf("my_timer called with user data: %d\n", *user_data);

  lv_chart_refresh(chart_time); /*Required after direct set*/
  lv_chart_refresh(chart_freq); /*Required after direct set*/
}


static void chart_tab_create(lv_obj_t* parent)
{


    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_ROW);

    /* Create the freq/time domain button w/ label */
    button_domain = lv_button_create(parent);
    lv_obj_add_event_cb(button_domain, event_button_domain_cb, LV_EVENT_ALL, NULL);
    lv_obj_align(button_domain, LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_size(button_domain, 200, 25);
    lv_obj_add_flag(button_domain, LV_OBJ_FLAG_CHECKABLE);

    label_button_domain = lv_label_create(button_domain);
    lv_label_set_text(label_button_domain, "Frequency Domain");
    lv_obj_center(label_button_domain);


    /* Create the data select button w/ label */
    button_data_select = lv_button_create(parent);
    lv_obj_add_event_cb(button_data_select, event_button_data_select_cb, LV_EVENT_ALL, NULL);
    lv_obj_align(button_data_select, LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_size(button_data_select, 200, 25);

    label_button_data_select = lv_label_create(button_data_select);
    lv_label_set_text(label_button_data_select, "Filtered & Unfiltered");
    lv_obj_center(label_button_data_select);

    //Under the buttons, create a container for the charts
    /*Create a container*/
    lv_obj_t* main_cont = lv_obj_create(parent);
    lv_obj_set_size(main_cont, 430, 750);
    //lv_obj_center(main_cont);
    lv_obj_add_flag(main_cont, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);

    /*Create a transparent wrapper for the chart and the scale.
 *Set a large width, to make it scrollable on the main container*/
    lv_obj_t* wrapper = lv_obj_create(main_cont);
    lv_obj_remove_style_all(wrapper);
    lv_obj_set_size(wrapper, lv_pct(200), lv_pct(100));
    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);

    /*Create the Frequency Domain chart*/
    chart_freq = lv_chart_create(wrapper);
    lv_obj_set_size(chart_freq, lv_pct(100), lv_pct(100));
    lv_obj_center(chart_freq);
    lv_chart_set_type(chart_freq, CHART_TYPE);
    lv_chart_set_point_count(chart_freq, MY_DATA_SIZE / 2);
    lv_obj_set_style_radius(chart_freq, 0, 0);
    //lv_obj_set_style_line_width(chart, 0, LV_PART_ITEMS);   /*Remove the lines*/
    lv_obj_set_style_width(chart_freq, 0, LV_PART_INDICATOR);   /*Remove the points*/
    lv_obj_set_style_line_opa(chart_freq, 99, LV_PART_ITEMS);
    lv_obj_set_style_line_width(chart_freq, 5, LV_PART_ITEMS);
    lv_obj_set_style_opa(chart_freq, 99, LV_PART_INDICATOR);
    lv_obj_set_style_pad_column(chart_freq, 1, 0);
    lv_obj_add_event_cb(chart_freq, event_chart_cb, LV_EVENT_ALL, NULL);
    lv_obj_refresh_ext_draw_size(chart_freq);
    lv_obj_set_flex_grow(chart_freq, 1);
    lv_obj_add_flag(chart_freq, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);

    /*Create a scale also with 100% width*/
    scale_bottom_freq = lv_scale_create(wrapper);
    lv_scale_set_mode(scale_bottom_freq, LV_SCALE_MODE_HORIZONTAL_BOTTOM);
    lv_obj_set_size(scale_bottom_freq, lv_pct(100), 25);
    lv_scale_set_total_tick_count(scale_bottom_freq, 10);
    lv_scale_set_major_tick_every(scale_bottom_freq, 1);
    lv_obj_set_style_pad_hor(scale_bottom_freq, lv_chart_get_first_point_center_offset(chart_freq), 0);

    static const char* scale_values_freq[] = { "0Hz", "1kHz", "2kHz", "3kHz", "4kHz", "5kHz", "6kHz", "7kHz", "8kHz", "9kHz", "10kHz", "11kHz", NULL };
    lv_scale_set_text_src(scale_bottom_freq, scale_values_freq);

    /*Add two data series*/
    series_fft_unfiltered = lv_chart_add_series(chart_freq, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
    series_fft_filtered = lv_chart_add_series(chart_freq, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_SECONDARY_Y);

    /*Create a chart*/
    chart_time = lv_chart_create(wrapper);
    lv_obj_set_size(chart_time, lv_pct(100), lv_pct(100));
    lv_obj_center(chart_time);
    lv_chart_set_type(chart_time, CHART_TYPE);
    lv_chart_set_point_count(chart_time, MY_DATA_SIZE);
    lv_obj_set_style_radius(chart_time, 0, 0);
    //lv_obj_set_style_line_width(chart, 0, LV_PART_ITEMS);   /*Remove the lines*/
    lv_obj_set_style_width(chart_time, 0, LV_PART_INDICATOR);   /*Remove the points*/
    lv_obj_set_style_line_opa(chart_time, 99, LV_PART_ITEMS);
    lv_obj_set_style_line_width(chart_time, 5, LV_PART_ITEMS);
    lv_obj_set_style_opa(chart_time, 99, LV_PART_INDICATOR);
    lv_obj_set_style_pad_column(chart_time, 1, 0);
    lv_obj_add_event_cb(chart_time, event_chart_cb, LV_EVENT_ALL, NULL);
    lv_obj_refresh_ext_draw_size(chart_time);
    lv_obj_set_flex_grow(chart_time, 1);

    /*Create a scale also with 100% width*/
    scale_bottom_time = lv_scale_create(wrapper);
    lv_scale_set_mode(scale_bottom_time, LV_SCALE_MODE_HORIZONTAL_BOTTOM);
    lv_obj_set_size(scale_bottom_time, lv_pct(100), 25);
    lv_scale_set_total_tick_count(scale_bottom_time, 10);
    lv_scale_set_major_tick_every(scale_bottom_time, 1);
    lv_obj_set_style_pad_hor(scale_bottom_time, lv_chart_get_first_point_center_offset(chart_time), 0);

    static const char* scale_values_time[] = { "0ms", "5ms", "10ms", "15ms", "20ms", "25ms", "30ms", "35ms", "40ms", "45ms", "50ms", "55ms", NULL };
    lv_scale_set_text_src(scale_bottom_time, scale_values_time);

    series_analog_unfiltered = lv_chart_add_series(chart_time, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_SECONDARY_Y);
    series_analog_filtered = lv_chart_add_series(chart_time, lv_palette_main(LV_PALETTE_DEEP_ORANGE), LV_CHART_AXIS_SECONDARY_Y);


    lv_chart_refresh(chart_time); /*Required after direct set*/
    lv_chart_refresh(chart_freq); /*Required after direct set*/


    static uint32_t chart_refresh_fps = CHART_FPS;
    lv_timer_t * timer = lv_timer_create(my_timer, 1000/CHART_FPS,  &chart_refresh_fps);

    lv_obj_add_flag(chart_time, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(scale_bottom_time, LV_OBJ_FLAG_HIDDEN);
}


static void logs_tab_create(lv_obj_t* parent)
{
    lv_obj_t* panel1 = lv_obj_create(parent);
    lv_obj_set_height(panel1, LV_SIZE_CONTENT);
    lv_obj_set_width(panel1, 400);

    lv_obj_t* name = lv_label_create(panel1);
    lv_label_set_text(name, "Coming Soon!");
    lv_obj_add_style(name, &style_title, 0);
}


void analytics_tab_create(lv_obj_t* parent)
{
    lv_obj_t* panel1 = lv_obj_create(parent);
    lv_obj_set_height(panel1, LV_SIZE_CONTENT);
    lv_obj_set_width(panel1, 400);

    lv_obj_t* name = lv_label_create(panel1);
    lv_label_set_text(name, "Coming Soon!");
    lv_obj_add_style(name, &style_title, 0);
}


static void tabview_delete_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_DELETE) {
        lv_style_reset(&style_text_muted);
        lv_style_reset(&style_title);
        lv_style_reset(&style_icon);
        lv_style_reset(&style_bullet);
    }
}

#endif
