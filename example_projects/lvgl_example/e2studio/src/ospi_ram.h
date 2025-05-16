#ifndef OSPI_H_
#define OSPI_H_

#include "hal_data.h"
#include "LVGL_thread.h"

/* Flash device sector size */
#define OSPI_B_SECTOR_SIZE_4K               (0x1000)

/* Flash device address space mapping */
#define OSPI_B_CS0_START_ADDRESS            (0x80000000)
#define OSPI_B_APP_ADDRESS(sector_no)       ((uint8_t *)(OSPI_B_CS0_START_ADDRESS + ((sector_no) * OSPI_B_SECTOR_SIZE_4K)))
#define OSPI_B_SECTOR_FIRST                 (0U)

void ospi_reset(void);
void oclk_change(bsp_clocks_octaclk_div_t divider);
void ospi_change_to_max_speed(void);
void init_ospi_ram(void);

#endif /* OSPI_H_ */
