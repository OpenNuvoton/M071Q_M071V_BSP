/**************************************************************************//**
 * @file     main.c
 * @version  V3.00
 * $Revision: 2 $
 * $Date: 16/10/25 4:31p $
 * @brief    Use timer0 periodic time-out interrupt event to wake up system.
 *
 * @note
 * Copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include "M071Q_M071V.h"


#define PLL_CLOCK       72000000


/*---------------------------------------------------------------------------------------------------------*/
/* Global Interface Variables Declarations                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
extern int IsDebugFifoEmpty(void);
volatile uint8_t g_u8IsTMR0WakeupFlag = 0;
volatile uint32_t g_au32TMRINTCount[4] = {0};


/**
 * @brief       Timer0 IRQ
 *
 * @param       None
 *
 * @return      None
 *
 * @details     The Timer0 default IRQ, declared in startup_NUC126.s.
 */
void TMR0_IRQHandler(void)
{
    if(TIMER_GetIntFlag(TIMER0) == 1)
    {
        /* Clear Timer0 time-out interrupt flag */
        TIMER_ClearIntFlag(TIMER0);

        g_au32TMRINTCount[0]++;
    }

    if(TIMER_GetWakeupFlag(TIMER0) == 1)
    {
        /* Clear Timer0 wake-up flag */
        TIMER_ClearWakeupFlag(TIMER0);

        g_u8IsTMR0WakeupFlag = 1;
    }
}

void SYS_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Enable HIRC clock */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);

    /* Waiting for HIRC clock ready */
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

    /* Switch HCLK clock source to HIRC */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));

    /* Enable HXT and LIRC */
    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk | CLK_PWRCTL_LIRCEN_Msk);

    /* Waiting for clock ready */
    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk | CLK_STATUS_LIRCSTB_Msk);

    /* Set core clock as PLL_CLOCK from PLL and SysTick source to HCLK/2*/
    CLK_SetCoreClock(PLL_CLOCK);
    CLK_SetSysTickClockSrc(CLK_CLKSEL0_STCLKSEL_HCLK_DIV2);

    /* Enable peripheral clock */
    CLK_EnableModuleClock(UART0_MODULE);
    CLK_EnableModuleClock(TMR0_MODULE);

    /* Peripheral clock source */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UARTSEL_PLL, CLK_CLKDIV0_UART(1));
    CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_LIRC, 0);

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Set PD multi-function pins for UART0 RXD, TXD */
    SYS->GPD_MFPL &= ~(SYS_GPD_MFPL_PD1MFP_Msk);
    SYS->GPD_MFPL |= (SYS_GPD_MFPL_PD1MFP_UART0_TXD);
    SYS->GPD_MFPH &= ~(SYS_GPD_MFPH_PD9MFP_Msk);
    SYS->GPD_MFPH |= (SYS_GPD_MFPH_PD9MFP_UART0_RXD);
}

void UART0_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART                                                                                               */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Reset IP */
    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 Baudrate */
    UART_Open(UART0, 115200);
}

/*---------------------------------------------------------------------------------------------------------*/
/*  MAIN function                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
int main(void)
{
    volatile uint32_t u32InitCount;

    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Init System, peripheral clock and multi-function I/O */
    SYS_Init();

    /* Lock protected registers */
    SYS_LockReg();

    /* Init UART0 for printf */
    UART0_Init();

    /* Use PA.0 to measure enable/disable CNTEN bit active time */
    PA->MODE = 0x1;
    PA0 = 1;

    printf("\n\nCPU @ %d Hz\n", SystemCoreClock);
    printf("+-------------------------------------------+\n");
    printf("|    Timer0 Time-out Wake-up Sample Code    |\n");
    printf("+-------------------------------------------+\n\n");

    printf("# Timer0 Settings:\n");
    printf("    - Clock source is LIRC          \n");
    printf("    - Time-out frequency is 1 Hz    \n");
    printf("    - Periodic mode                 \n");
    printf("    - Interrupt enable              \n");
    printf("    - Wake-up function enable       \n");
    printf("# System will enter to Power-down mode while Timer0 interrupt counts is reaching 3.\n");
    printf("  And will be wakeup while Timer0 interrupt counts is reaching 4.\n");
    printf("# Measure PA.0 low period to check timer counter start/stop sync time should be around 1~2 TMR_CLK.\n\n");

    /* Enable Timer0 NVIC */
    NVIC_EnableIRQ(TMR0_IRQn);

    /* Open Timer0 time-out frequency to 1 Hz in periodic mode */
    TIMER_Open(TIMER0, TIMER_PERIODIC_MODE, 1);

    /* Enable Timer0 time-out interrupt and wake-up function */
    TIMER_EnableInt(TIMER0);
    TIMER_EnableWakeup(TIMER0);

    PA0 = 0;
    /* Start Timer0 counting */
    TIMER_Start(TIMER0);
    while(TIMER_IS_ACTIVE(TIMER0) == 0);
    PA0 = 1; // Around 1~2 TMR_CLK

    u32InitCount = g_u8IsTMR0WakeupFlag = g_au32TMRINTCount[0] = 0;
    while(g_au32TMRINTCount[0] < 10)
    {
        if(g_au32TMRINTCount[0] != u32InitCount)
        {
            printf("Timer0 interrupt counts - %d\n", g_au32TMRINTCount[0]);
            if(g_au32TMRINTCount[0] == 3)
            {
                /* System enter to Power-down */
                /* To program PWRCTL register, it needs to disable register protection first. */
                SYS_UnlockReg();
                printf("\nSystem enter to power-down mode ...\n");
                /* To check if all the debug messages are finished */
                while(IsDebugFifoEmpty() == 0);
                CLK_PowerDown();

                /* Check if Timer0 time-out interrupt and wake-up flag occurred */
                while(g_u8IsTMR0WakeupFlag == 0);

                printf("System has been waken-up done. (Timer0 interrupt counts is %d)\n\n", g_au32TMRINTCount[0]);
            }
            u32InitCount = g_au32TMRINTCount[0];
        }
    }

    /* Stop Timer0 counting */
    PA0 = 0;
    TIMER_Stop(TIMER0);
    while(TIMER_IS_ACTIVE(TIMER0));
    PA0 = 1; // Around 1~2 TMR_CLK

    printf("*** PASS ***\n");

    while(1);
}

/*** (C) COPYRIGHT 2016 Nuvoton Technology Corp. ***/
