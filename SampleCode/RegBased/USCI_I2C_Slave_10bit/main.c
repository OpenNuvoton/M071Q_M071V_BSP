/**************************************************************************//**
 * @file     main.c
 * @version  V1.00
 * $Revision: 2 $
 * $Date: 16/10/25 4:28p $
 * @brief    Show a Master how to access 10-bit address Slave
 *           This sample code needs to work with USCI_I2C_Master_10bit
 * @note
 * Copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include "M071Q_M071V.h"

#define PLLCTL_SETTING  CLK_PLLCTL_72MHz_HXT
#define PLL_CLOCK       72000000

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
uint8_t g_u8SlvData[256];
volatile uint32_t slave_buff_addr;
uint8_t g_au8RxData[4];
volatile uint16_t g_u16RecvAddr;
volatile uint8_t g_u8DataLenS;

enum UI2C_SLAVE_EVENT s_Event;

typedef void (*UI2C_FUNC)(uint32_t u32Status);

static volatile UI2C_FUNC s_UI2C0HandlerFn = NULL;
/*---------------------------------------------------------------------------------------------------------*/
/*  USCI_I2C IRQ Handler                                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
void USCI_IRQHandler(void)
{
    uint32_t u32Status;

    //UI2C0 Interrupt
    u32Status = UI2C_GET_PROT_STATUS(UI2C0);
    if(s_UI2C0HandlerFn != NULL)
        s_UI2C0HandlerFn(u32Status);
}

/*---------------------------------------------------------------------------------------------------------*/
/*  USCI_I2C0 TRx Callback Function                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
void UI2C_LB_SlaveTRx(uint32_t u32Status)
{
    if((u32Status & UI2C_PROTSTS_STARIF_Msk) == UI2C_PROTSTS_STARIF_Msk)
    {
        /* Clear START INT Flag */
        UI2C_CLR_PROT_INT_FLAG(UI2C0, UI2C_PROTSTS_STARIF_Msk);

        /* Event process */
        g_u8DataLenS = 0;
        s_Event = SLAVE_H_RD_ADDRESS_ACK;
        UI2C_SET_CONTROL_REG(UI2C0, UI2C_CTL_AA);

        /* Trigger USCI I2C */
        UI2C0->PROTCTL |= UI2C_PROTCTL_PTRG_Msk;
    }
    else if((u32Status & UI2C_PROTSTS_ACKIF_Msk) == UI2C_PROTSTS_ACKIF_Msk)
    {
        /* Clear ACK INT Flag */
        UI2C_CLR_PROT_INT_FLAG(UI2C0, UI2C_PROTSTS_ACKIF_Msk);

        /* Event process */
        if(s_Event == SLAVE_H_WR_ADDRESS_ACK)
        {
            g_u8DataLenS = 0;

            s_Event = SLAVE_L_WR_ADDRESS_ACK;
            g_u16RecvAddr = (unsigned char)(UI2C0->RXDAT);
        }
        else if(s_Event == SLAVE_H_RD_ADDRESS_ACK)
        {
            g_u8DataLenS = 0;

            UI2C0->TXDAT = g_u8SlvData[slave_buff_addr];
            slave_buff_addr++;
            g_u16RecvAddr = (unsigned char)(UI2C0->RXDAT);
        }
        else if(s_Event == SLAVE_L_WR_ADDRESS_ACK)
        {
            if((UI2C0->PROTSTS & UI2C_PROTSTS_SLAREAD_Msk) == UI2C_PROTSTS_SLAREAD_Msk)
            {
                //s_Event = SLAVE_SEND_DATA;
                UI2C0->TXDAT = g_u8SlvData[slave_buff_addr];
                slave_buff_addr++;
            }
            else
            {
                s_Event = SLAVE_GET_DATA;
            }
            g_u16RecvAddr = (unsigned char)(UI2C0->RXDAT);
        }
        else if(s_Event == SLAVE_L_RD_ADDRESS_ACK)
        {
            //s_Event = SLAVE_SEND_DATA;
            UI2C0->TXDAT = g_u8SlvData[slave_buff_addr];
            slave_buff_addr++;
        }
        else if(s_Event == SLAVE_GET_DATA)
        {
            g_au8RxData[g_u8DataLenS] = (unsigned char)(UI2C0->RXDAT);
            g_u8DataLenS++;

            if(g_u8DataLenS == 2)
            {
                slave_buff_addr = (g_au8RxData[0] << 8) + g_au8RxData[1];
            }
            if(g_u8DataLenS == 3)
            {
                g_u8SlvData[slave_buff_addr] = g_au8RxData[2];
                g_u8DataLenS = 0;
            }
        }


        UI2C_SET_CONTROL_REG(UI2C0, UI2C_CTL_AA);

        /* Trigger USCI I2C */
        UI2C0->PROTCTL |= UI2C_PROTCTL_PTRG_Msk;
    }
    else if((u32Status & UI2C_PROTSTS_NACKIF_Msk) == UI2C_PROTSTS_NACKIF_Msk)
    {
        /* Clear NACK INT Flag */
        UI2C_CLR_PROT_INT_FLAG(UI2C0, UI2C_PROTSTS_NACKIF_Msk);
        /* Event process */
        g_u8DataLenS = 0;
        s_Event = SLAVE_H_WR_ADDRESS_ACK;
        UI2C_SET_CONTROL_REG(UI2C0, UI2C_CTL_AA);

        /* Trigger USCI I2C */
        UI2C0->PROTCTL |= UI2C_PROTCTL_PTRG_Msk;


    }
    else if((u32Status & UI2C_PROTSTS_STORIF_Msk) == UI2C_PROTSTS_STORIF_Msk)
    {
        /* Clear STOP INT Flag */
        UI2C_CLR_PROT_INT_FLAG(UI2C0, UI2C_PROTSTS_STORIF_Msk);
        /* Event process */
        g_u8DataLenS = 0;
        s_Event = SLAVE_H_WR_ADDRESS_ACK;
        UI2C_SET_CONTROL_REG(UI2C0, UI2C_CTL_AA);

        /* Trigger USCI I2C */
        UI2C0->PROTCTL |= UI2C_PROTCTL_PTRG_Msk;
    }
}

void SYS_Init(void)
{
	uint32_t u32TimeOutCnt;

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Enable HIRC clock (Internal RC 22.1184MHz) */
    CLK->PWRCTL |= CLK_PWRCTL_HIRCEN_Msk;

    /* Wait for HIRC clock ready */
    u32TimeOutCnt = __HIRC;
    while(!(CLK->STATUS & CLK_STATUS_HIRCSTB_Msk))
		if(--u32TimeOutCnt == 0) break;


    /* Select HCLK clock source as HIRC and HCLK clock divider as 1 */
    CLK->CLKSEL0 = (CLK->CLKSEL0 & (~CLK_CLKSEL0_HCLKSEL_Msk)) | CLK_CLKSEL0_HCLKSEL_HXT;
    CLK->CLKDIV0 = (CLK->CLKDIV0 & (~CLK_CLKDIV0_HCLKDIV_Msk)) | CLK_CLKDIV0_HCLK(1);

    CLK->PWRCTL |= CLK_PWRCTL_HXTEN_Msk;
    u32TimeOutCnt = __HIRC;
    while(!(CLK->STATUS & CLK_STATUS_HXTSTB_Msk))
		if(--u32TimeOutCnt == 0) break;

    /* Update System Core Clock */
    SystemCoreClockUpdate();

    /* Enable UART and USCI0 module clock */
    CLK->APBCLK0 |= CLK_APBCLK0_UART0CKEN_Msk;
    CLK->APBCLK1 |= CLK_APBCLK1_USCI0CKEN_Msk;

    /* Select UART module clock source as HIRC and UART module clock divider as 1 */
    CLK->CLKSEL1 = (CLK->CLKSEL1 & (~CLK_CLKSEL1_UARTSEL_Msk)) | CLK_CLKSEL1_UARTSEL_HXT;
    CLK->CLKDIV0 = (CLK->CLKDIV0 & (~CLK_CLKDIV0_UARTDIV_Msk)) | CLK_CLKDIV0_UART(1);

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Set PD multi-function pins for UART0 RXD and TXD */
    SYS->GPD_MFPL &= ~(SYS_GPD_MFPL_PD1MFP_Msk);
    SYS->GPD_MFPL |= (SYS_GPD_MFPL_PD1MFP_UART0_TXD);
    SYS->GPD_MFPH &= ~(SYS_GPD_MFPH_PD9MFP_Msk);
    SYS->GPD_MFPH |= (SYS_GPD_MFPH_PD9MFP_UART0_RXD);

    /* Set PC multi-function pins for UI2C0_SDA(PC.5) and UI2C0_SDA(PC.4) */
    SYS->GPC_MFPL &= ~(SYS_GPC_MFPL_PC5MFP_Msk | SYS_GPC_MFPL_PC4MFP_Msk);
    SYS->GPC_MFPL |= (SYS_GPC_MFPL_PC5MFP_USCI0_DAT0 | SYS_GPC_MFPL_PC4MFP_USCI0_CLK);

    /* I2C pins enable schmitt trigger */
    PC->SMTEN |= (GPIO_SMTEN_SMTEN4_Msk | GPIO_SMTEN_SMTEN5_Msk);
}

void UI2C0_Init(uint32_t u32ClkSpeed)
{
    uint32_t u32Clkdiv;

    u32Clkdiv = (((((SystemCoreClock / 2) * 10) / (u32ClkSpeed)) + 5) / 10) - 1;

    if(u32Clkdiv < 8)
        u32Clkdiv = 8;

    /* Open UI2C0 and set clock to 100k */
    UI2C0->CTL &= ~UI2C_CTL_FUNMODE_Msk;
    UI2C0->CTL = 4 << UI2C_CTL_FUNMODE_Pos;

    /* 8-bit legnt data format configuration */
    UI2C0->LINECTL &= ~UI2C_LINECTL_DWIDTH_Msk;
    UI2C0->LINECTL |= 8 << UI2C_LINECTL_DWIDTH_Pos;

    /* MSB data format */
    UI2C0->LINECTL &= ~UI2C_LINECTL_LSB_Msk;

    /* Set UI2C0 baud rate */
    UI2C0->BRGEN &= ~UI2C_BRGEN_CLKDIV_Msk;
    UI2C0->BRGEN |= (u32Clkdiv << UI2C_BRGEN_CLKDIV_Pos);

    /* Set UI2C0 Slave Addresses */
    UI2C0->DEVADDR0 = 0x116;   /* Slave Address : 0x116 */
    UI2C0->DEVADDR1 = 0x136;   /* Slave Address : 0x136 */

    /* Set UI2C0 Slave Addresses Msk */
    UI2C0->ADDRMSK0 = 0x4;   /* Slave Address : 0x4 */
    UI2C0->ADDRMSK1 = 0x2;   /* Slave Address : 0x2 */

    /* Enable UI2C1 protocol interrupt */
    UI2C_ENABLE_PROT_INT(UI2C0, (UI2C_PROTIEN_ACKIEN_Msk | UI2C_PROTIEN_NACKIEN_Msk | UI2C_PROTIEN_STORIEN_Msk | UI2C_PROTIEN_STARIEN_Msk));
    NVIC_EnableIRQ(USCI_IRQn);

    /* Enable UI2C0 10-bit address mode */
    UI2C_ENABLE_10BIT_ADDR_MODE(UI2C0);

    /* Enable UI2C0 protocol */
    UI2C0->PROTCTL |=  UI2C_PROTCTL_PROTEN_Msk;
}

void UART_Init(void)
{
    /* Word length is 8 bits; 1 stop bit; no parity bit. */
    UART0->LINE = UART_WORD_LEN_8 | UART_PARITY_NONE | UART_STOP_BIT_1;
    /* UART peripheral clock rate 12MHz; UART bit rate 115200 bps. */
    UART0->BAUD = UART_BAUD_MODE2 | UART_BAUD_MODE2_DIVIDER(__HXT, 115200);
}

/*---------------------------------------------------------------------------------------------------------*/
/*                        Main function                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
int main()
{
    uint32_t i;

    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Init System, IP clock and multi-function I/O. */
    SYS_Init();

    /* Lock protected registers */
    SYS_LockReg();

    /* Init UART for print message */
    UART_Init();

    /*
        This sample code sets USCI_I2C bus clock to 100kHz. Then, Master accesses Slave with Byte Write
        and Byte Read operations, and check if the read data is equal to the programmed data.
    */

    printf("+-------------------------------------------------------+\n");
    printf("|  USCI_I2C Driver Sample Code for Master access        |\n");
    printf("|  10-bit address Slave                                 |\n");
    printf("|  UI2C0(Master)  <----> UI2C0(Slave)                   |\n");
    printf("+-------------------------------------------------------+\n");

    printf("\n");
    printf("Configure UI2C0 as a Slave\n");
    printf("The I/O connection for UI2C0:\n");
    printf("UI2C0_SDA(PC.5), UI2C0_SCL(PC.4)\n");

    /* Init UI2C0 baud rate */
    UI2C0_Init(100000);

    s_Event = SLAVE_H_WR_ADDRESS_ACK;
    UI2C_SET_CONTROL_REG(UI2C0, UI2C_CTL_AA);

    /* Trigger USCI_I2C0 */
    UI2C0->PROTCTL |= UI2C_PROTCTL_PTRG_Msk;

    for(i = 0; i < 0x100; i++)
        g_u8SlvData[i] = 0;

    /* I2C0 function to Slave receive/transmit data */
    s_UI2C0HandlerFn = UI2C_LB_SlaveTRx;

    printf("UI2C0 Slave is running...\n");

    while(1);
}

/*** (C) COPYRIGHT 2016 Nuvoton Technology Corp. ***/
