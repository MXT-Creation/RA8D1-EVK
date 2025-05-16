#include "ospi_ram.h"

#define ENABLE_READ_PREFETCH             (0)

#if defined(RENESAS_CORTEX_M85) && (BSP_CFG_DCACHE_ENABLED)
#define ENABLE_WRITE_COMBINATION         (1) //Writing to OSPI memory space must be in 64bit size with combination mode enabled, cached frame buffer should be ok
                                             //(Dave2D writes also writes through it's own frame buffer cache to the frame buffer)
#else
#define ENABLE_WRITE_COMBINATION         (0) //Writing to OSPI memory space must be in 64bit size with combination mode enabled, non-cached frame buffer can't guarantee that
#endif

/* IS66WVO8M8 defines */
#define OSPI_RAM_REGISTER_READ           (0xC000)
#define OSPI_RAM_REGISTER_WRITE          (0x4000)
#define ID_REGISTER_ADDRESS              (0x00000000)
#define MODE_REGISTER_ADDRESS            (0x00040000)

#define LENGTH_ZERO                      (0U)
#define LENGTH_ONE                       (1U)
#define LENGTH_TWO                       (2U)
#define LENGTH_THREE                     (3U)
#define LENGTH_FOUR                      (4U)

#define LC_OFFSET_MR                     (4U) //bits 7:4 of the Mode Register
#define LC_MASK_MR                       (0xF)
#define LATENCY_OFFSET_MR                (3)//bit 3 of the Configuration Register

#define FIXED_LATENCY                    (1)// 1 = fixed Latency, 0 = Variable

#define LC_VL_3_LC                       (0)
#define LC_VL_4_LC                       (1)
#define LC_VL_5_LC                       (2)
#define LC_VL_6_LC                       (3)
#define LC_VL_7_LC                       (4)
#define LC_VL_8_LC                       (5)

#define ISSI_MINIMUM_LATENCY_x_2         (6)
#define ISSI_MAXIMUM_LATENCY_x_2         (16)

#define PWR_ON_DEFAULT_READ_DUMMY_CYCLES (8)
#define OPTIMISED_READ_DUMMY_CYCLES      (8)
#define REGISTER_WRITE_DUMMY_CYCLES      (0) //Register writes are always 0

/* ISSI OSPI RAM */
#define ISSI_OSPI_RAM_DEVICE_ID_1_8_V          (0x930C)//1.8V part
#define ISSI_OSPI_RAM_DEVICE_ID_3_0_V          (0x932C)//3.0V part
//1.8V parts fitted to V1.0 RA8 High speed board, should have been 3.0V parts
//V1.1 RA8 High speed board will have 3.0V parts fitted

#define RAM_DEVICE_ID_MASK               (0xFFFF)
#define RAM_DEVICE_MR_MASK               (0xFFFF)

void init_ospi_ram(void)
{
    fsp_err_t err;

    /* Setup Octal RAM */
    uint32_t dev_id;
    uint32_t mode_register;

    err = R_OSPI_B_Open(&g_ospi_ram_ctrl, &g_ospi_ram_cfg);
    assert(FSP_SUCCESS == err);

    /* Disable memory mapping for CS0 */
    R_XSPI->BMCTL0 &= ~R_XSPI_BMCTL0_CH0CS0ACC_Msk;

    R_XSPI->CMCFGCS[0].CMCFG0_b.ARYAMD = 1; //Array address mode for IS66WVO8M8

    R_XSPI->CMCFGCS[0].CMCFG2_b.WRLATE = g_ospi_ram_ctrl.p_cmd_set->read_dummy_cycles;
    R_XSPI->CMCFGCS[0].CMCFG1_b.RDLATE = g_ospi_ram_ctrl.p_cmd_set->read_dummy_cycles; //Set Write latency same as read latency

    R_XSPI->LIOCFGCS_b[0].LATEMD  = 0; //Fixed Latency Mode

    R_XSPI->LIOCFGCS_b[0].WRMSKMD = 1; //Use OM_DSQ as write data mask;

#if (1 == ENABLE_READ_PREFETCH)
    R_XSPI->BMCFGCH_b[0].PREEN = 1;//Enable Read prefetch
#else
    R_XSPI->BMCFGCH_b[0].PREEN = 0;//Disable Read prefetch
#endif

#if (1 == ENABLE_WRITE_COMBINATION)
    R_XSPI->BMCFGCH_b[0].WRMD = 0;//Must be disabled if write combination is enabled, return response after writing to the Internal Write buffer

    R_XSPI->BMCFGCH_b[0].MWRSIZE = 0x0F; //64 byte
    R_XSPI->BMCFGCH_b[0].CMBTIM  = 0xFF; //PCLKA count combination timer
    R_XSPI->BMCFGCH_b[0].MWRCOMB = 1; //Enable write combination
#else
    R_XSPI->BMCFGCH_b[0].MWRCOMB = 0; //Disable write combination
    R_XSPI->BMCFGCH_b[0].WRMD = 1;//Return response after xSPI bus cycle, write combination must be disabled.
#endif

    R_XSPI->LIOCFGCS_b[0].DDRSMPEX = 1; //Expand DDR DQSM sampling window for read


    /* Re-activate memory-mapped mode in Read/Write. */
    R_XSPI->BMCTL0 |= R_XSPI_BMCTL0_CH0CS0ACC_Msk;

    spi_flash_direct_transfer_t ospi_transfer =
    {
     .command        = OSPI_RAM_REGISTER_READ,
     .address        = ID_REGISTER_ADDRESS,
     .data           = 0x00,
     .command_length = LENGTH_TWO,
     .address_length = LENGTH_FOUR,
     .data_length    = LENGTH_FOUR,
     .dummy_cycles   = PWR_ON_DEFAULT_READ_DUMMY_CYCLES
    };
    /* direct transfer read command, read device id*/
    err = R_OSPI_B_DirectTransfer(&g_ospi_ram_ctrl, &ospi_transfer, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
    assert(FSP_SUCCESS == err);

    dev_id = ospi_transfer.data & RAM_DEVICE_ID_MASK;

    /* Check the Device ID can be read */
    if ((ISSI_OSPI_RAM_DEVICE_ID_1_8_V == dev_id) || (ISSI_OSPI_RAM_DEVICE_ID_3_0_V == dev_id))
    {
        ospi_transfer.command        = OSPI_RAM_REGISTER_READ;
        ospi_transfer.address        = MODE_REGISTER_ADDRESS;
        ospi_transfer.data           = 0x00;
        ospi_transfer.command_length = LENGTH_TWO;
        ospi_transfer.address_length = LENGTH_FOUR;
        ospi_transfer.data_length    = LENGTH_TWO;
        ospi_transfer.dummy_cycles   = PWR_ON_DEFAULT_READ_DUMMY_CYCLES;

        /* direct transfer read command, read Mode Register*/
        err = R_OSPI_B_DirectTransfer(&g_ospi_ram_ctrl, &ospi_transfer, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
        assert(FSP_SUCCESS == err);

         if ((ISSI_MINIMUM_LATENCY_x_2 > g_ospi_ram_ctrl.p_cmd_set->read_dummy_cycles) || (ISSI_MAXIMUM_LATENCY_x_2 < g_ospi_ram_ctrl.p_cmd_set->read_dummy_cycles))
        {
             assert(0);
        }

         if (g_ospi_ram_ctrl.p_cmd_set->read_dummy_cycles % 2)
         {
             assert(0); //must be even
         }

        uint32_t latency_issi = (g_ospi_ram_ctrl.p_cmd_set->read_dummy_cycles/2) - 3; //Calculate the ISSI MR latency value based on the high speed latency set in the configurator

        mode_register = ospi_transfer.data & RAM_DEVICE_MR_MASK;

        mode_register = (((mode_register & 0xFF00)>>8) | ((mode_register & 0x00FF)<<8)); //swap bytes to set configuration

        mode_register &= (uint32_t)~(LC_MASK_MR<<LC_OFFSET_MR);
        mode_register |= latency_issi /*LC_VL_4_LC*/<<LC_OFFSET_MR; //bit 7:4 of the Mode Register
        mode_register |= FIXED_LATENCY<<LATENCY_OFFSET_MR; //xSPI peripheral only supports fixed latency for 8D-8D-8D profile 1

        mode_register = (((mode_register & 0xFF00)>>8) | ((mode_register & 0x00FF)<<8)); //swap bytes to set send data

        ospi_transfer.command        = OSPI_RAM_REGISTER_WRITE;
        ospi_transfer.address        = MODE_REGISTER_ADDRESS;
        ospi_transfer.data           = mode_register;
        ospi_transfer.command_length = LENGTH_TWO;
        ospi_transfer.address_length = LENGTH_FOUR;
        ospi_transfer.data_length    = LENGTH_TWO;
        ospi_transfer.dummy_cycles   = REGISTER_WRITE_DUMMY_CYCLES;

        /* direct transfer write command, write Mode Register*/
        err = R_OSPI_B_DirectTransfer(&g_ospi_ram_ctrl, &ospi_transfer, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
        assert(FSP_SUCCESS == err);

        ospi_transfer.command        = OSPI_RAM_REGISTER_READ;
        ospi_transfer.address        = MODE_REGISTER_ADDRESS;
        ospi_transfer.data           = 0x00;
        ospi_transfer.command_length = LENGTH_TWO;
        ospi_transfer.address_length = LENGTH_FOUR;
        ospi_transfer.data_length    = LENGTH_TWO;
        ospi_transfer.dummy_cycles   = OPTIMISED_READ_DUMMY_CYCLES;

        /* direct transfer read command, read Mode Register*/
        err = R_OSPI_B_DirectTransfer(&g_ospi_ram_ctrl, &ospi_transfer, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
        assert(FSP_SUCCESS == err);

        /* Check the write to the Mode Register was successful */
        if (mode_register != (ospi_transfer.data & RAM_DEVICE_MR_MASK))
        {
            assert(0);
        }
    }
    else
    {
        assert(0);
    }

    const ospi_b_extended_cfg_t* ospi_ram_cfg_extended = g_ospi_ram_cfg.p_extend;
    volatile uint32_t * ram_addr = (uint32_t *) ospi_ram_cfg_extended->p_autocalibration_preamble_pattern_addr;

    /* Write the auto-calibration preamble pattern for DOPI mode to OSPI RAM */
    *(ram_addr + 0) = 0xFFFF0000U;
    *(ram_addr + 1) = 0x000800FFU;
    *(ram_addr + 2) = 0x00FFF700U;
    *(ram_addr + 3) = 0xF700F708U;

    R_OSPI_B_AutoCalibrate(&g_ospi_ram_ctrl); //Seem to need to do the calibration twice, don't check the return value here
}
