#include "board_init.h"

#include "lvgl.h"
#include "port/lv_port_indev.h"
#include "lvgl/src/drivers/display/renesas_glcdc/lv_renesas_glcdc.h"
#include "common_data.h"

#include "LVGL_thread.h"
#include "touch_GT911.h"

#define DIRECT_MODE 0

static void touch_init(void)
{
    fsp_err_t err;

    /* Need to initialise the Touch Controller before the LCD, as only a Single Reset line shared between them */
    err = R_SCI_B_I2C_Open(&g_i2c_master1_ctrl, &g_i2c_master1_cfg);
    if (FSP_SUCCESS != err)
    {
        __BKPT(0); //TODO: Better error handling
    }

    err = R_ICU_ExternalIrqOpen(&g_external_irq13_ctrl, &g_external_irq13_cfg);
    if (FSP_SUCCESS != err)
    {
        __BKPT(0); //TODO: Better error handling
    }

    err = init_ts(&g_i2c_master1_ctrl);
    if (FSP_SUCCESS != err)
    {
        __BKPT(0); //TODO: Better error handling
    }

    err = enable_ts(&g_i2c_master1_ctrl, &g_external_irq13_ctrl);
    if (FSP_SUCCESS != err)
    {
        __BKPT(0); //TODO: Better error handling
    }
}

void board_init(void)
{
    /* Need to initialise the Touch Controller before the LCD, as only a Single Reset line shared between them */
    touch_init();

#if DIRECT_MODE
    lv_display_t * disp = lv_renesas_glcdc_direct_create();
#else
    static uint8_t partial_draw_buf[64*1024] BSP_PLACE_IN_SECTION(".dtcm_bss") BSP_ALIGN_VARIABLE(1024);

    lv_display_t * disp = lv_renesas_glcdc_partial_create(partial_draw_buf, NULL, sizeof(partial_draw_buf));
#endif

    lv_display_set_default(disp);

    /* Enable the backlight */
    R_IOPORT_PinWrite(&g_ioport_ctrl, DISP_BLEN, BSP_IO_LEVEL_HIGH);
    R_IOPORT_PinWrite(&g_ioport_ctrl, DISP_EN, BSP_IO_LEVEL_HIGH);

    lv_port_indev_init();
}

