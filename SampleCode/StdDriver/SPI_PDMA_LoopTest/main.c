/**************************************************************************//**
 * @file     main.c
 * @version  V3.00
 * @brief
 *           Demonstrate SPI data transfer with PDMA.
 *           SPI0 will be configured as Master mode and SPI1 will be configured as Slave mode.
 *           Both TX PDMA function and RX PDMA function will be enabled.
 *
 * @copyright SPDX-License-Identifier: Apache-2.0
 * @copyright Copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include "M071Q_M071V.h"

// *** <<< Use Configuration Wizard in Context Menu >>> ***
// <o> GPIO Slew Rate Control
// <0=> Basic <1=> Higher
#define SlewRateMode    0
// *** <<< end of configuration section >>> ***

#define PLL_CLOCK       72000000

#define SPI_MASTER_TX_DMA_CH 0
#define SPI_MASTER_RX_DMA_CH 1
#define SPI_SLAVE_TX_DMA_CH  2
#define SPI_SLAVE_RX_DMA_CH  3

#define TEST_COUNT      64

/* Function prototype declaration */
void SYS_Init(void);
void SPI_Init(void);
void SpiLoopTest_WithPDMA(void);

/* Global variable declaration */
uint32_t g_au32MasterToSlaveTestPattern[TEST_COUNT];
uint32_t g_au32SlaveToMasterTestPattern[TEST_COUNT];
uint32_t g_au32MasterRxBuffer[TEST_COUNT];
uint32_t g_au32SlaveRxBuffer[TEST_COUNT];

int main(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();
    /* Init System, IP clock and multi-function I/O. */
    SYS_Init();
    /* Lock protected registers */
    SYS_LockReg();

    /* Reset UART0 module */
    SYS_ResetModule(UART0_RST);

    /* Configure UART0: 115200, 8-bit word, no parity bit, 1 stop bit. */
    UART_Open(UART0, 115200);

    /* Init SPI */
    SPI_Init();

    printf("\n\n");
    printf("+--------------------------------------------------------------+\n");
    printf("|                    SPI + PDMA Sample Code                    |\n");
    printf("+--------------------------------------------------------------+\n");
    printf("\n");
    printf("Configure SPI0 as a master and SPI1 as a slave.\n");
    printf("Bit length of a transaction: 32\n");
    printf("The I/O connection for SPI0/SPI1 loopback:\n");
    printf("    SPI0_SS  (PB.4) <--> SPI1_SS(PA.4)\n    SPI0_CLK(PB.2)  <--> SPI1_CLK(PA.7)\n");
    printf("    SPI0_MISO(PB.3) <--> SPI1_MISO(PA.6)\n    SPI0_MOSI(PB.5) <--> SPI1_MOSI(PA.5)\n\n");
    printf("Please connect SPI0 with SPI1, and press any key to start transmission ...");
    getchar();
    printf("\n");

    SpiLoopTest_WithPDMA();

    printf("\n\nExit SPI driver sample code.\n");

    /* Close SPI0 */
    SPI_Close(SPI0);
    /* Close SPI1 */
    SPI_Close(SPI1);
    while(1);
}

void SYS_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Enable external 12MHz XTAL */
    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);

    /* Waiting for clock ready */
    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

    /* Switch HCLK clock source to HXT and HCLK source divide 1 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HXT, CLK_CLKDIV0_HCLK(1));

    /* Set core clock as PLL_CLOCK from PLL */
    CLK_SetCoreClock(PLL_CLOCK);

    /* Select HXT as the clock source of UART0 */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UARTSEL_HXT, CLK_CLKDIV0_UART(1));
    /* Select PCLK as the clock source of SPI0 and SPI1 */
    CLK_SetModuleClock(SPI0_MODULE, CLK_CLKSEL2_SPI0SEL_PCLK0, MODULE_NoMsk);
    CLK_SetModuleClock(SPI1_MODULE, CLK_CLKSEL2_SPI1SEL_PCLK0, MODULE_NoMsk);

    /* Enable UART peripheral clock */
    CLK_EnableModuleClock(UART0_MODULE);
    /* Enable SPI0 peripheral clock */
    CLK_EnableModuleClock(SPI0_MODULE);
    /* Enable SPI1 peripheral clock */
    CLK_EnableModuleClock(SPI1_MODULE);

    /* Enable PDMA clock source */
    CLK_EnableModuleClock(PDMA_MODULE);

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock and CyclesPerUs automatically. */
    SystemCoreClockUpdate();

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Set PD multi-function pins for UART0 RXD and TXD */
    SYS->GPD_MFPL &= ~(SYS_GPD_MFPL_PD1MFP_Msk);
    SYS->GPD_MFPL |= (SYS_GPD_MFPL_PD1MFP_UART0_TXD);
    SYS->GPD_MFPH &= ~(SYS_GPD_MFPH_PD9MFP_Msk);
    SYS->GPD_MFPH |= (SYS_GPD_MFPH_PD9MFP_UART0_RXD);

    /* Configure SPI0 related multi-function pins. GPB[5:2] : SPI0_MOSI, SPI0_SS, SPI0_MISO, SPI0_CLK. */
    SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB2MFP_Msk | SYS_GPB_MFPL_PB3MFP_Msk | SYS_GPB_MFPL_PB4MFP_Msk | SYS_GPB_MFPL_PB5MFP_Msk);
    SYS->GPB_MFPL |= (SYS_GPB_MFPL_PB2MFP_SPI0_CLK | SYS_GPB_MFPL_PB3MFP_SPI0_MISO | SYS_GPB_MFPL_PB4MFP_SPI0_SS | SYS_GPB_MFPL_PB5MFP_SPI0_MOSI);

    /* Configure SPI1 related multi-function pins. GPA[7:4] : SPI1_CLK, SPI1_MISO, SPI1_MOSI, SPI1_SS. */
    SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA4MFP_Msk | SYS_GPA_MFPL_PA5MFP_Msk | SYS_GPA_MFPL_PA6MFP_Msk | SYS_GPA_MFPL_PA7MFP_Msk);
    SYS->GPA_MFPL |= (SYS_GPA_MFPL_PA4MFP_SPI1_SS | SYS_GPA_MFPL_PA5MFP_SPI1_MOSI | SYS_GPA_MFPL_PA6MFP_SPI1_MISO | SYS_GPA_MFPL_PA7MFP_SPI1_CLK);

#if (SlewRateMode == 0)
    /* Enable SPI0 I/O basic slew rate */
    PB->SLEWCTL &= ~(GPIO_SLEWCTL_HSREN2_Msk | GPIO_SLEWCTL_HSREN3_Msk | GPIO_SLEWCTL_HSREN4_Msk | GPIO_SLEWCTL_HSREN5_Msk);

    /* Enable SPI1 I/O basic slew rate */
    PA->SLEWCTL &= ~(GPIO_SLEWCTL_HSREN4_Msk | GPIO_SLEWCTL_HSREN5_Msk | GPIO_SLEWCTL_HSREN6_Msk | GPIO_SLEWCTL_HSREN7_Msk);
#elif (SlewRateMode == 1)
    /* Enable SPI0 I/O higher slew rate */
    PB->SLEWCTL |= (GPIO_SLEWCTL_HSREN2_Msk | GPIO_SLEWCTL_HSREN3_Msk | GPIO_SLEWCTL_HSREN4_Msk | GPIO_SLEWCTL_HSREN5_Msk);

    /* Enable SPI1 I/O higher slew rate */
    PA->SLEWCTL |= (GPIO_SLEWCTL_HSREN4_Msk | GPIO_SLEWCTL_HSREN5_Msk | GPIO_SLEWCTL_HSREN6_Msk | GPIO_SLEWCTL_HSREN7_Msk);
#endif
}


void SPI_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init SPI                                                                                                */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Configure SPI0 */
    /* Configure SPI0 as a master, SPI clock rate 2MHz,
       clock idle low, 32-bit transaction, drive output on falling clock edge and latch input on rising edge. */
    SPI_Open(SPI0, SPI_MASTER, SPI_MODE_0, 32, 2000000);
    /* Enable the automatic hardware slave selection function. Select the SPI0_SS pin and configure as low-active. */
    SPI_EnableAutoSS(SPI0, SPI_SS, SPI_SS_ACTIVE_LOW);

    /* Configure SPI1 */
    /* Configure SPI1 as a slave, clock idle low, 32-bit transaction, drive output on falling clock edge and latch input on rising edge. */
    /* Configure SPI1 as a low level active device. SPI peripheral clock rate = f_PCLK0 */
    SPI_Open(SPI1, SPI_SLAVE, SPI_MODE_0, 32, (uint32_t)NULL);
}

void SpiLoopTest_WithPDMA(void)
{
    uint32_t u32DataCount, u32TestCycle;
    uint32_t u32RegValue, u32Abort;
    int32_t i32Err;


    printf("\nSPI0/1 Loop test with PDMA ");

    /* Source data initiation */
    for(u32DataCount = 0; u32DataCount < TEST_COUNT; u32DataCount++)
    {
        g_au32MasterToSlaveTestPattern[u32DataCount] = 0x55000000 | (u32DataCount + 1);
        g_au32SlaveToMasterTestPattern[u32DataCount] = 0xAA000000 | (u32DataCount + 1);
    }

    /* Reset PDMA module */
    SYS_ResetModule(PDMA_RST);

    /* Enable PDMA channels */
    PDMA_Open((1 << SPI_MASTER_TX_DMA_CH) | (1 << SPI_MASTER_RX_DMA_CH) | (1 << SPI_SLAVE_RX_DMA_CH) | (1 << SPI_SLAVE_TX_DMA_CH));


    /*=======================================================================
      SPI master PDMA TX channel configuration:
      -----------------------------------------------------------------------
        Word length = 32 bits
        Transfer Count = TEST_COUNT
        Source = g_au32MasterToSlaveTestPattern
        Source Address = Incresing
        Destination = SPI0->TX
        Destination Address = Fixed
        Burst Type = Single Transfer
    =========================================================================*/
    /* Set transfer width (32 bits) and transfer count */
    PDMA_SetTransferCnt(SPI_MASTER_TX_DMA_CH, PDMA_WIDTH_32, TEST_COUNT);
    /* Set source/destination address and attributes */
    PDMA_SetTransferAddr(SPI_MASTER_TX_DMA_CH, (uint32_t)g_au32MasterToSlaveTestPattern, PDMA_SAR_INC, (uint32_t)&SPI0->TX, PDMA_DAR_FIX);
    /* Set request source; set basic mode. */
    PDMA_SetTransferMode(SPI_MASTER_TX_DMA_CH, PDMA_SPI0_TX, FALSE, 0);
    /* Single request type. SPI only support PDMA single request type. */
    PDMA_SetBurstType(SPI_MASTER_TX_DMA_CH, PDMA_REQ_SINGLE, 0);
    /* Disable table interrupt */
    PDMA->DSCT[SPI_MASTER_TX_DMA_CH].CTL |= PDMA_DSCT_CTL_TBINTDIS_Msk;

    /*=======================================================================
      SPI master PDMA RX channel configuration:
      -----------------------------------------------------------------------
        Word length = 32 bits
        Transfer Count = TEST_COUNT
        Source = SPI0->RX
        Source Address = Fixed
        Destination = g_au32MasterRxBuffer
        Destination Address = Increasing
        Burst Type = Single Transfer
    =========================================================================*/
    /* Set transfer width (32 bits) and transfer count */
    PDMA_SetTransferCnt(SPI_MASTER_RX_DMA_CH, PDMA_WIDTH_32, TEST_COUNT);
    /* Set source/destination address and attributes */
    PDMA_SetTransferAddr(SPI_MASTER_RX_DMA_CH, (uint32_t)&SPI0->RX, PDMA_SAR_FIX, (uint32_t)g_au32MasterRxBuffer, PDMA_DAR_INC);
    /* Set request source; set basic mode. */
    PDMA_SetTransferMode(SPI_MASTER_RX_DMA_CH, PDMA_SPI0_RX, FALSE, 0);
    /* Single request type. SPI only support PDMA single request type. */
    PDMA_SetBurstType(SPI_MASTER_RX_DMA_CH, PDMA_REQ_SINGLE, 0);
    /* Disable table interrupt */
    PDMA->DSCT[SPI_MASTER_RX_DMA_CH].CTL |= PDMA_DSCT_CTL_TBINTDIS_Msk;

    /*=======================================================================
      SPI slave PDMA RX channel configuration:
      -----------------------------------------------------------------------
        Word length = 32 bits
        Transfer Count = TEST_COUNT
        Source = SPI1->RX
        Source Address = Fixed
        Destination = g_au32SlaveRxBuffer
        Destination Address = Increasing
        Burst Type = Single Transfer
    =========================================================================*/
    /* Set transfer width (32 bits) and transfer count */
    PDMA_SetTransferCnt(SPI_SLAVE_RX_DMA_CH, PDMA_WIDTH_32, TEST_COUNT);
    /* Set source/destination address and attributes */
    PDMA_SetTransferAddr(SPI_SLAVE_RX_DMA_CH, (uint32_t)&SPI1->RX, PDMA_SAR_FIX, (uint32_t)g_au32SlaveRxBuffer, PDMA_DAR_INC);
    /* Set request source; set basic mode. */
    PDMA_SetTransferMode(SPI_SLAVE_RX_DMA_CH, PDMA_SPI1_RX, FALSE, 0);
    /* Single request type. SPI only support PDMA single request type. */
    PDMA_SetBurstType(SPI_SLAVE_RX_DMA_CH, PDMA_REQ_SINGLE, 0);
    /* Disable table interrupt */
    PDMA->DSCT[SPI_SLAVE_RX_DMA_CH].CTL |= PDMA_DSCT_CTL_TBINTDIS_Msk;

    /*=======================================================================
      SPI slave PDMA TX channel configuration:
      -----------------------------------------------------------------------
        Word length = 32 bits
        Transfer Count = TEST_COUNT
        Source = g_au32SlaveToMasterTestPattern
        Source Address = Increasing
        Destination = SPI1->TX
        Destination Address = Fixed
        Burst Type = Single Transfer
    =========================================================================*/
    /* Set transfer width (32 bits) and transfer count */
    PDMA_SetTransferCnt(SPI_SLAVE_TX_DMA_CH, PDMA_WIDTH_32, TEST_COUNT);
    /* Set source/destination address and attributes */
    PDMA_SetTransferAddr(SPI_SLAVE_TX_DMA_CH, (uint32_t)g_au32SlaveToMasterTestPattern, PDMA_SAR_INC, (uint32_t)&SPI1->TX, PDMA_DAR_FIX);
    /* Set request source; set basic mode. */
    PDMA_SetTransferMode(SPI_SLAVE_TX_DMA_CH, PDMA_SPI1_TX, FALSE, 0);
    /* Single request type. SPI only support PDMA single request type. */
    PDMA_SetBurstType(SPI_SLAVE_TX_DMA_CH, PDMA_REQ_SINGLE, 0);
    /* Disable table interrupt */
    PDMA->DSCT[SPI_SLAVE_TX_DMA_CH].CTL |= PDMA_DSCT_CTL_TBINTDIS_Msk;


    /* Enable SPI slave DMA function */
    SPI_TRIGGER_TX_RX_PDMA(SPI1);
    /* Enable SPI master DMA function */
    SPI_TRIGGER_TX_RX_PDMA(SPI0);



    i32Err = 0;
    for(u32TestCycle = 0; u32TestCycle < 10000; u32TestCycle++)
    {
        if((u32TestCycle & 0x1FF) == 0)
            putchar('.');

        while(1)
        {
            /* Get interrupt status */
            u32RegValue = PDMA_GET_INT_STATUS();
            /* Check the PDMA transfer done interrupt flag */
            if(u32RegValue & PDMA_INTSTS_TDIF_Msk)
            {
                /* Check the PDMA transfer done flags */
                if((PDMA_GET_TD_STS() & ((1 << SPI_MASTER_TX_DMA_CH) | (1 << SPI_MASTER_RX_DMA_CH) | (1 << SPI_SLAVE_TX_DMA_CH) | (1 << SPI_SLAVE_RX_DMA_CH))) ==
                        ((1 << SPI_MASTER_TX_DMA_CH) | (1 << SPI_MASTER_RX_DMA_CH) | (1 << SPI_SLAVE_TX_DMA_CH) | (1 << SPI_SLAVE_RX_DMA_CH)))
                {
                    /* Clear the PDMA transfer done flags */
                    PDMA_CLR_TD_FLAG((1 << SPI_MASTER_TX_DMA_CH) | (1 << SPI_MASTER_RX_DMA_CH) | (1 << SPI_SLAVE_TX_DMA_CH) | (1 << SPI_SLAVE_RX_DMA_CH));
                    /* Disable SPI master's PDMA transfer function */
                    SPI_DISABLE_TX_RX_PDMA(SPI0);
                    /* Check the transfer data */
                    for(u32DataCount = 0; u32DataCount < TEST_COUNT; u32DataCount++)
                    {
                        if(g_au32MasterToSlaveTestPattern[u32DataCount] != g_au32SlaveRxBuffer[u32DataCount])
                        {
                            i32Err = 1;
                            break;
                        }
                        if(g_au32SlaveToMasterTestPattern[u32DataCount] != g_au32MasterRxBuffer[u32DataCount])
                        {
                            i32Err = 1;
                            break;
                        }
                    }

                    if(u32TestCycle >= 10000)
                        break;

                    /* Source data initiation */
                    for(u32DataCount = 0; u32DataCount < TEST_COUNT; u32DataCount++)
                    {
                        g_au32MasterToSlaveTestPattern[u32DataCount]++;
                        g_au32SlaveToMasterTestPattern[u32DataCount]++;
                    }
                    /* Re-trigger */
                    /* Slave PDMA TX channel configuration */
                    /* Set transfer width (32 bits) and transfer count */
                    PDMA_SetTransferCnt(SPI_SLAVE_TX_DMA_CH, PDMA_WIDTH_32, TEST_COUNT);
                    /* Set request source; set basic mode. */
                    PDMA_SetTransferMode(SPI_SLAVE_TX_DMA_CH, PDMA_SPI1_TX, FALSE, 0);

                    /* Slave PDMA RX channel configuration */
                    /* Set transfer width (32 bits) and transfer count */
                    PDMA_SetTransferCnt(SPI_SLAVE_RX_DMA_CH, PDMA_WIDTH_32, TEST_COUNT);
                    /* Set request source; set basic mode. */
                    PDMA_SetTransferMode(SPI_SLAVE_RX_DMA_CH, PDMA_SPI1_RX, FALSE, 0);

                    /* Master PDMA TX channel configuration */
                    /* Set transfer width (32 bits) and transfer count */
                    PDMA_SetTransferCnt(SPI_MASTER_TX_DMA_CH, PDMA_WIDTH_32, TEST_COUNT);
                    /* Set request source; set basic mode. */
                    PDMA_SetTransferMode(SPI_MASTER_TX_DMA_CH, PDMA_SPI0_TX, FALSE, 0);

                    /* Master PDMA RX channel configuration */
                    /* Set transfer width (32 bits) and transfer count */
                    PDMA_SetTransferCnt(SPI_MASTER_RX_DMA_CH, PDMA_WIDTH_32, TEST_COUNT);
                    /* Set request source; set basic mode. */
                    PDMA_SetTransferMode(SPI_MASTER_RX_DMA_CH, PDMA_SPI0_RX, FALSE, 0);

                    /* Enable master's DMA transfer function */
                    SPI_TRIGGER_TX_RX_PDMA(SPI0);
                    break;
                }
            }
            /* Check the DMA transfer abort interrupt flag */
            if(u32RegValue & PDMA_INTSTS_ABTIF_Msk)
            {
                /* Get the target abort flag */
                u32Abort = PDMA_GET_ABORT_STS();
                /* Clear the target abort flag */
                PDMA_CLR_ABORT_FLAG(u32Abort);
                i32Err = 1;
                break;
            }
            /* Check the DMA time-out interrupt flag */
            if(u32RegValue & 0x00000300)
            {
                /* Clear the time-out flag */
                PDMA->INTSTS = u32RegValue & 0x00000300;
                i32Err = 1;
                break;
            }
        }

        if(i32Err)
            break;
    }

    /* Disable all PDMA channels */
    PDMA_Close();

    if(i32Err)
    {
        printf(" [FAIL]\n");
    }
    else
    {
        printf(" [PASS]\n");
    }

    return;
}

/*** (C) COPYRIGHT 2016 Nuvoton Technology Corp. ***/

