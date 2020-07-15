/***************************************************************************//**
 * @file     main.c
 * @brief    ISP tool main function
 * @version  0x32
 * @date     14, June, 2017
 *
 * @note
 * Copyright (C) 2019 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "targetdev.h"
#include "i2c_transfer.h"

#define PLLCTL_SETTING      CLK_PLLCTL_72MHz_HIRC
#define PLL_CLOCK           71884800

uint32_t u32Pclk0;
uint32_t u32Pclk1;

void ProcessHardFault(void) {}
void SH_Return(void) {}

void SYS_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Enable HIRC clock (Internal RC 22.1184MHz) and HXT clock (external XTAL 12MHz) */
    CLK->PWRCTL |= (CLK_PWRCTL_HIRCEN_Msk | CLK_PWRCTL_HXTEN_Msk);

    /* Wait for HIRC clock ready */
    while (!(CLK->STATUS & CLK_STATUS_HIRCSTB_Msk));

    /* Set core clock as PLL_CLOCK from PLL and HCLK clock divider as 1 */
    CLK->PLLCTL = PLLCTL_SETTING;
    while (!(CLK->STATUS & CLK_STATUS_PLLSTB_Msk));
    CLK->CLKSEL0 = (CLK->CLKSEL0 & (~CLK_CLKSEL0_HCLKSEL_Msk)) | CLK_CLKSEL0_HCLKSEL_PLL;
    CLK->CLKDIV0 = (CLK->CLKDIV0 & (~CLK_CLKDIV0_HCLKDIV_Msk)) | CLK_CLKDIV0_HCLK(1);

    /* Update System Core Clock */
    PllClock        = PLL_CLOCK;            // PLL
    SystemCoreClock = PLL_CLOCK / 1;        // HCLK
    CyclesPerUs     = PLL_CLOCK / 1000000;  // For CLK_SysTickDelay()

    /* Enable I2C1 peripheral clock */
    CLK->APBCLK0 |= CLK_APBCLK0_I2C1CKEN_Msk;

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock and CyclesPerUs automatically. */
    SystemCoreClockUpdate();

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Set PA multi-function pins for I2C1 SDA and SCL */
    SYS->GPC_MFPL &= ~(SYS_GPC_MFPL_PC5MFP_Msk | SYS_GPC_MFPL_PC4MFP_Msk);
    SYS->GPC_MFPL |= (SYS_GPC_MFPL_PC5MFP_I2C1_SDA | SYS_GPC_MFPL_PC4MFP_I2C1_SCL);

    /* I2C clock pin enable schmitt trigger */
    PC->SMTEN |= GPIO_SMTEN_SMTEN4_Msk;
}

int main(void)
{
    uint32_t au8CmdBuff[16];

    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Init System, peripheral clock and multi-function I/O */
    SYS_Init();

    /* Enable ISP */
    CLK->AHBCLK |= CLK_AHBCLK_ISPCKEN_Msk;
    FMC->ISPCTL |= (FMC_ISPCTL_ISPEN_Msk | FMC_ISPCTL_APUEN_Msk);

    /* Get APROM size, data flash size and address */
    g_apromSize = GetApromSize();
    GetDataFlashInfo(&g_dataFlashAddr, &g_dataFlashSize);

    if (g_dataFlashAddr < g_apromSize) {
        g_dataFlashSize = (g_apromSize - g_dataFlashAddr);
    } else {
        g_dataFlashSize = 0;
    }

    /* Init I2C */
    I2C_Init();

    /* Set Systick time-out as 300ms */
    SysTick->LOAD = 300000 * CyclesPerUs;
    SysTick->VAL   = (0x00);
    SysTick->CTRL = SysTick->CTRL | SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;

    /* Wait for CMD_CONNECT command until Systick time-out */
    while (1)
    {
        /* Wait for CMD_CONNECT command */
        if (u8I2cDataReady == 1)
        {
            goto _ISP;
        }
        /* Systick time-out, go to APROM */
        if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
        {
            goto _APROM;
        }
    }

_ISP:

    /* Parse command from master and send response back */
    while (1)
    {
        if (u8I2cDataReady == 1)
        {
            /* Get command from I2C receive buffer */
            memcpy(au8CmdBuff, au8I2cRcvBuf, 64);
            u8I2cDataReady = 0;
            /* Parse the current command */
            ParseCmd((unsigned char *)au8CmdBuff, 64);
        }
    }

_APROM:

    /* Reset system and boot from APROM */
    SYS->RSTSTS = (SYS_RSTSTS_PORF_Msk | SYS_RSTSTS_PINRF_Msk);
    FMC->ISPCTL &= ~(FMC_ISPCTL_ISPEN_Msk | FMC_ISPCTL_BS_Msk);
    SCB->AIRCR = (V6M_AIRCR_VECTKEY_DATA | V6M_AIRCR_SYSRESETREQ);

    /* Trap the CPU */
    while (1);
}
