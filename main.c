/**************************************************************************//**
 * @file     main.c
 * @version  V2.00
 * $Revision: 3 $
 * $Date: 15/04/10 10:26a $
 * @brief    NUC230_240 Series SWD & Flash Programming Sample Code
 *
 * @note
 * Copyright (C) 2014 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include "NUC230_240.h"

#include "error.h"
#include "swd_host.h"
#include "target_config.h"

#define PLL_CLOCK           72000000


extern error_t target_flash_init(void);
extern error_t target_flash_uninit(void);
extern error_t target_flash_program_page(uint32_t addr, const uint8_t *buf, uint32_t size);
extern error_t target_flash_erase_sector(uint32_t addr);
extern error_t target_flash_erase_chip(void);

extern uint32_t UserImageBase;
extern uint32_t UserImageLimit;


void SYS_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Enable Internal RC 22.1184MHz clock */
    CLK_EnableXtalRC(CLK_PWRCON_OSC22M_EN_Msk);

    /* Waiting for Internal RC clock ready */
    CLK_WaitClockReady(CLK_CLKSTATUS_OSC22M_STB_Msk);

    /* Switch HCLK clock source to Internal RC and HCLK source divide 1 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLK_S_HIRC, CLK_CLKDIV_HCLK(1));

    /* Enable external XTAL 12MHz clock */
    CLK_EnableXtalRC(CLK_PWRCON_XTL12M_EN_Msk);

    /* Waiting for external XTAL clock ready */
    CLK_WaitClockReady(CLK_CLKSTATUS_XTL12M_STB_Msk);

    /* Set core clock as PLL_CLOCK from PLL */
    CLK_SetCoreClock(PLL_CLOCK);

    /* Enable UART module clock */
    CLK_EnableModuleClock(UART0_MODULE);

    /* Select UART module clock source */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART_S_HXT, CLK_CLKDIV_UART(1));

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Set GPB multi-function pins for UART0 RXD and TXD */
    SYS->GPB_MFP &= ~(SYS_GPB_MFP_PB0_Msk | SYS_GPB_MFP_PB1_Msk);
    SYS->GPB_MFP |= (SYS_GPB_MFP_PB0_UART0_RXD | SYS_GPB_MFP_PB1_UART0_TXD);
}

void UART0_Init()
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART                                                                                               */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Reset UART0 */
    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 Baudrate */
    UART_Open(UART0, 115200);
}

/*---------------------------------------------------------------------------------------------------------*/
/* MAIN function                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
int main(void)
{
    uint32_t u32PID;
    uint32_t u32Addr;
    uint32_t u32ImageSize;

    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Init System, peripheral clock and multi-function I/O */
    SYS_Init();

    /* Lock protected registers */
    SYS_LockReg();

    /* Init UART0 for printf */
    UART0_Init();

    printf("\n\nCPU @ %d Hz\n", SystemCoreClock);
    printf("+-------------------------------------------------+\n");
    printf("|    SWD & Flash Programming Sample Code          |\n");
    printf("+-------------------------------------------------+\n\n");
    printf("  >> Use PC.1 as SWD_CLK Pin and PC.2 as SWD IO Pin (IO_Config.h) << \n");
    printf("     Press any key to start...\n\n");
    getchar();

#ifndef __TARGET_M2351_NS__
    if (!target_set_state(RESET_PROGRAM))
    {
        return 0;
    }

    if (!swd_read_memory(0x40000000, (uint8_t *)&u32PID, 4))
    {
        return 0;
    }
    
    printf(" The Target PID is 0x%X\n", u32PID);
#endif

    printf(" Start Programming Image (LoadImage.s).\n");

    if (target_flash_init() != ERROR_SUCCESS)
    {
        return 0;
    }    

    for (u32Addr = target_device.flash_start; u32Addr < target_device.flash_end; u32Addr += target_device.sector_size)
    {
        if (target_flash_erase_sector(u32Addr) != ERROR_SUCCESS)
        {
            return 0;
        }
    }

    printf(" Erase Done.\n");

    u32ImageSize = (uint32_t)&UserImageLimit - (uint32_t)&UserImageBase;

    if (target_flash_program_page(target_device.flash_start, (const uint8_t *)&UserImageBase, u32ImageSize) != ERROR_SUCCESS)
    {
        return 0;
    }

    printf(" Program Done.\n");
    
    if (target_flash_uninit() != ERROR_SUCCESS)
    {
        return 0;
    }

    while(1);
}

/*** (C) COPYRIGHT 2014 Nuvoton Technology Corp. ***/
