#pragma once
/**
 * @file lv_demo_widgets.h
 *
 */

#ifndef LV_DATA_VIEWER_H
#define LV_DATA_VIEWER_H

#ifdef __cplusplus
extern "C" {
#endif

    /*********************
     *      INCLUDES
     *********************/
#include "lvgl/demos/lv_demos.h"

#if LV_USE_DATA_VIEWER

#if LV_USE_GRID == 0
#error "LV_USE_GRID needs to be enabled"
#endif

#if LV_USE_FLEX == 0
#error "LV_USE_FLEX needs to be enabled"
#endif

     /*********************
      *      DEFINES
      *********************/


      /**********************
       *      TYPEDEFS
       **********************/

       /**********************
        * GLOBAL PROTOTYPES
        **********************/
    void lv_data_viewer(void);

    /**********************
     *      MACROS
     **********************/

#endif /*LV_USE_DEMO_WIDGETS*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_DEMO_WIDGETS_H*/
