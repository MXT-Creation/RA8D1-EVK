#include "ospi_ram.h"

void ospi_reset(void)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, OSPI_RES, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
    R_IOPORT_PinWrite(&g_ioport_ctrl, OSPI_RES, BSP_IO_LEVEL_LOW);
    R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
    R_IOPORT_PinWrite(&g_ioport_ctrl, OSPI_RES, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
}

void oclk_change(bsp_clocks_octaclk_div_t divider)
{
    /* Now update the octaclk divider. */
    bsp_octaclk_settings_t octaclk_settings;
    octaclk_settings.source_clock = BSP_CLOCKS_SOURCE_CLOCK_PLL2R;
    octaclk_settings.divider      = divider;
    R_BSP_OctaclkUpdate(&octaclk_settings);
}

void ospi_change_to_max_speed(void)
{
    fsp_err_t err;

    /* Change clock speed to 166MHz. */
    oclk_change(BSP_CLOCKS_OCTACLK_DIV_2);
    R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);

    /* auto-calibrate after changing the speed of OM_SCLK */
    err = R_OSPI_B_AutoCalibrate(&g_ospi_ram_ctrl);
    assert(FSP_SUCCESS == err);

    if (1 == R_XSPI->INTS_b.DSTOCS1)
    {
        R_XSPI->INTC_b.DSTOCS1C = 1;
    }

}
