/****************************************************************************
 * @file     main.c
 * @version  V3.0
 * $Revision: 3 $
 * $Date: 17/05/04 1:12p $
 * @brief    Perform A/D Conversion with ADC continuous scan mode.
 * @note
 * Copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 *
 ******************************************************************************/
#include <stdio.h>
#include "M071Q_M071V.h"

#define PLLCTL_SETTING  CLK_PLLCTL_72MHz_HXT
#define PLL_CLOCK       72000000


/*---------------------------------------------------------------------------------------------------------*/
/* Define Function Prototypes                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void SYS_Init(void);
void UART0_Init(void);
void AdcContScanModeTest(void);


void SYS_Init(void)
{
	uint32_t u32TimeOutCnt;

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Enable HIRC clock (Internal RC 22.1184MHz) */
    CLK->PWRCTL |= CLK_PWRCTL_HIRCEN_Msk;

    /* Waiting for HIRC clock ready */
    u32TimeOutCnt = __HIRC;
    while(!(CLK->STATUS & CLK_STATUS_HIRCSTB_Msk))
		if(--u32TimeOutCnt == 0) break;


    /* Select HCLK clock source as HIRC and HCLK clock divider as 1 */
    CLK->CLKSEL0 = (CLK->CLKSEL0 & ~CLK_CLKSEL0_HCLKSEL_Msk) | CLK_CLKSEL0_HCLKSEL_HIRC;
    CLK->CLKDIV0 = (CLK->CLKDIV0 & ~CLK_CLKDIV0_HCLKDIV_Msk) | CLK_CLKDIV0_HCLK(1);

    /* Enable HXT clock (external XTAL 12MHz) */
    CLK->PWRCTL |= CLK_PWRCTL_HXTEN_Msk;

    /* Waiting for HXT clock ready */
    u32TimeOutCnt = __HIRC;
    while(!(CLK->STATUS & CLK_STATUS_HXTSTB_Msk))
		if(--u32TimeOutCnt == 0) break;

    /* Enable PLL and Set PLL frequency */
    CLK->PLLCTL = PLLCTL_SETTING;
    u32TimeOutCnt = __HIRC;
    while(!(CLK->STATUS & CLK_STATUS_PLLSTB_Msk))
		if(--u32TimeOutCnt == 0) break;

    CLK->CLKSEL0 = (CLK->CLKSEL0 & ~CLK_CLKSEL0_HCLKSEL_Msk) | CLK_CLKSEL0_HCLKSEL_PLL;

    /* Update System Core Clock */
    SystemCoreClockUpdate();

    /* Enable UART module clock */
    CLK->APBCLK0 |= CLK_APBCLK0_UART0CKEN_Msk;

    /* Enable ADC module clock */
    CLK->APBCLK0 |= CLK_APBCLK0_ADCCKEN_Msk ;

    /* Select UART module clock source as HXT */
    CLK->CLKSEL1 = (CLK->CLKSEL1 & ~CLK_CLKSEL1_UARTSEL_Msk) | CLK_CLKSEL1_UARTSEL_HXT;

    /* Select ADC module clock source */
    CLK->CLKSEL1 = (CLK->CLKSEL1 & CLK_CLKSEL1_ADCSEL_Msk) | CLK_CLKSEL1_ADCSEL_HIRC;

    /* ADC clock source is 22.1184MHz, set divider to 7, ADC clock is 22.1184/7 MHz */
    CLK->CLKDIV0  = (CLK->CLKDIV0 & ~CLK_CLKDIV0_ADCDIV_Msk) | (((7) - 1) << CLK_CLKDIV0_ADCDIV_Pos);


    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Set PD multi-function pins for UART0 RXD and TXD */
    SYS->GPD_MFPL &= ~(SYS_GPD_MFPL_PD1MFP_Msk);
    SYS->GPD_MFPL |= (SYS_GPD_MFPL_PD1MFP_UART0_TXD);
    SYS->GPD_MFPH &= ~(SYS_GPD_MFPH_PD9MFP_Msk);
    SYS->GPD_MFPH |= (SYS_GPD_MFPH_PD9MFP_UART0_RXD);

    /* Disable the GPB0 - GPB3 digital input path to avoid the leakage current. */
    GPIO_DISABLE_DIGITAL_PATH(PB, 0xF);

    /* Configure the GPB0 - GPB3 ADC analog input pins */
    SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB0MFP_Msk | SYS_GPB_MFPL_PB1MFP_Msk | SYS_GPB_MFPL_PB2MFP_Msk | SYS_GPB_MFPL_PB3MFP_Msk);
    SYS->GPB_MFPL |= SYS_GPB_MFPL_PB0MFP_ADC0_CH0 | SYS_GPB_MFPL_PB1MFP_ADC0_CH1 | SYS_GPB_MFPL_PB2MFP_ADC0_CH2 | SYS_GPB_MFPL_PB3MFP_ADC0_CH3;

}

/*---------------------------------------------------------------------------------------------------------*/
/* Init UART                                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
void UART0_Init()
{
    /* Reset UART IP */
    SYS->IPRST1 |=  SYS_IPRST1_UART0RST_Msk;
    SYS->IPRST1 &= ~SYS_IPRST1_UART0RST_Msk;

    /* Configure UART0 and set UART0 baud rate */
    UART0->BAUD = UART_BAUD_MODE2 | UART_BAUD_MODE2_DIVIDER(__HXT, 115200);
    UART0->LINE = UART_WORD_LEN_8 | UART_PARITY_NONE | UART_STOP_BIT_1;
}


/*---------------------------------------------------------------------------------------------------------*/
/* Function: ADC_GetConversionRate                                                                         */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*   None.                                                                                                 */
/*                                                                                                         */
/* Returns:                                                                                                */
/*   Return the A/D conversion rate (sample/second)                                                        */
/*                                                                                                         */
/* Description:                                                                                            */
/*   The conversion rate depends on the clock source of ADC clock.                                         */
/*   It only needs 21 ADC clocks to complete an A/D conversion.                                            */
/*---------------------------------------------------------------------------------------------------------*/
static __INLINE uint32_t ADC_GetConversionRate()
{
    uint32_t u32AdcClkSrcSel;
    uint32_t u32ClkTbl[4] = {__HXT, 0, 0, __HIRC};

    /* Set the PLL clock frequency */
    u32ClkTbl[1] = PllClock;
    /* Set the system core clock frequency */
    u32ClkTbl[2] = SystemCoreClock;
    /* Get the clock source setting */
    u32AdcClkSrcSel = ((CLK->CLKSEL1 & CLK_CLKSEL1_ADCSEL_Msk) >> CLK_CLKSEL1_ADCSEL_Pos);
    /* Return the ADC conversion rate */
    return ((u32ClkTbl[u32AdcClkSrcSel]) / (((CLK->CLKDIV0 & CLK_CLKDIV0_ADCDIV_Msk) >> CLK_CLKDIV0_ADCDIV_Pos) + 1) / 21);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function: AdcContScanModeTest                                                                           */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*   None.                                                                                                 */
/*                                                                                                         */
/* Returns:                                                                                                */
/*   None.                                                                                                 */
/*                                                                                                         */
/* Description:                                                                                            */
/*   ADC continuous scan mode test.                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
void AdcContScanModeTest()
{
    uint8_t  u8Option;
    uint32_t u32ChannelCount;
    int32_t  i32ConversionData;
    uint32_t u32TimeOutCnt;

    printf("\n\nConversion rate: %d samples/second\n", ADC_GetConversionRate());
    printf("\n");
    printf("+----------------------------------------------------------------------+\n");
    printf("|                 ADC continuous scan mode sample code                 |\n");
    printf("+----------------------------------------------------------------------+\n");

    printf("\nIn this test, software will get 2 cycles of conversion result from the specified channels.\n");

    while(1)
    {
        printf("\n\nSelect input mode:\n");
        printf("  [1] Single end input (channel 0, 1, 2 and 3)\n");
        printf("  [2] Differential input (input channel pair 0 and 1)\n");
        printf("  Other keys: exit continuous scan mode test\n");
        u8Option = getchar();
        if(u8Option == '1')
        {
            /* Set the ADC operation mode as continuous scan, input mode as single-end and enable the ADC converter */
            ADC->ADCR = (ADC_ADCR_ADMD_CONTINUOUS | ADC_ADCR_DIFFEN_SINGLE_END | ADC_ADCR_ADEN_CONVERTER_ENABLE);
            /* Enable analog input channel 0, 1, 2 and 3 */
            ADC->ADCHER = ((ADC->ADCHER & ~ADC_ADCHER_CHEN_Msk) | (0xF));

            /* Clear the A/D interrupt flag for safe */
            ADC->ADSR0 = ADC_ADSR0_ADF_Msk;

            /* Start A/D conversion */
            ADC->ADCR |= ADC_ADCR_ADST_Msk;

            /* Wait conversion done */
            u32TimeOutCnt = SystemCoreClock; /* 1 second time-out */
            while(!((ADC->ADSR0 & ADC_ADSR0_ADF_Msk) >> ADC_ADSR0_ADF_Pos))
            {
                if(--u32TimeOutCnt == 0)
                {
                    printf("Wait for ADC conversion done time-out!\n");
                    return;
                }
            }

            /* Clear the ADC interrupt flag */
            ADC->ADSR0 = ADC_ADSR0_ADF_Msk;

            for(u32ChannelCount = 0; u32ChannelCount < 4; u32ChannelCount++)
            {
                i32ConversionData = (ADC->ADDR[(u32ChannelCount)] & ADC_ADDR_RSLT_Msk) >> ADC_ADDR_RSLT_Pos;
                printf("Conversion result of channel %d: 0x%X (%d)\n", u32ChannelCount, i32ConversionData, i32ConversionData);
            }

            /* Wait conversion done */
            u32TimeOutCnt = SystemCoreClock; /* 1 second time-out */
            while(!((ADC->ADSR0 & ADC_ADSR0_ADF_Msk) >> ADC_ADSR0_ADF_Pos))
            {
                if(--u32TimeOutCnt == 0)
                {
                    printf("Wait for ADC conversion done time-out!\n");
                    return;
                }
            }

            /* Stop A/D conversion */
            ADC->ADCR &= ~ADC_ADCR_ADST_Msk;

            for(u32ChannelCount = 0; u32ChannelCount < 4; u32ChannelCount++)
            {
                i32ConversionData = (ADC->ADDR[(u32ChannelCount)] & ADC_ADDR_RSLT_Msk) >> ADC_ADDR_RSLT_Pos;
                printf("Conversion result of channel %d: 0x%X (%d)\n", u32ChannelCount, i32ConversionData, i32ConversionData);
            }

            /* Clear the ADC interrupt flag */
            ADC->ADSR0 = ADC_ADSR0_ADF_Msk;

        }
        else if(u8Option == '2')
        {
            /* Set the ADC operation mode as continuous scan, input mode as differential and enable the ADC converter */
            ADC->ADCR = (ADC_ADCR_ADMD_CONTINUOUS | ADC_ADCR_DIFFEN_DIFFERENTIAL | ADC_ADCR_ADEN_CONVERTER_ENABLE);
            /* Enable analog input channel 0 and 2 */
            ADC->ADCHER = ((ADC->ADCHER & ~ADC_ADCHER_CHEN_Msk) | (0x5));

            /* Clear the A/D interrupt flag for safe */
            ADC->ADSR0 = ADC_ADSR0_ADF_Msk;

            /* Start A/D conversion */
            ADC->ADCR |= ADC_ADCR_ADST_Msk;

            /* Wait conversion done */
            u32TimeOutCnt = SystemCoreClock; /* 1 second time-out */
            while(!((ADC->ADSR0 & ADC_ADSR0_ADF_Msk) >> ADC_ADSR0_ADF_Pos))
            {
                if(--u32TimeOutCnt == 0)
                {
                    printf("Wait for ADC conversion done time-out!\n");
                    return;
                }
            }

            /* Clear the ADC interrupt flag */
            ADC->ADSR0 = ADC_ADSR0_ADF_Msk;

            for(u32ChannelCount = 0; u32ChannelCount < 2; u32ChannelCount++)
            {
                i32ConversionData = (ADC->ADDR[(u32ChannelCount * 2)] & ADC_ADDR_RSLT_Msk) >> ADC_ADDR_RSLT_Pos;
                printf("Conversion result of differential input pair %d: 0x%X (%d)\n", u32ChannelCount, i32ConversionData, i32ConversionData);
            }

            /* Wait conversion done */
            u32TimeOutCnt = SystemCoreClock; /* 1 second time-out */
            while(!((ADC->ADSR0 & ADC_ADSR0_ADF_Msk) >> ADC_ADSR0_ADF_Pos))
            {
                if(--u32TimeOutCnt == 0)
                {
                    printf("Wait for ADC conversion done time-out!\n");
                    return;
                }
            }

            /* Stop A/D conversion */
            ADC->ADCR &= ~ADC_ADCR_ADST_Msk;

            for(u32ChannelCount = 0; u32ChannelCount < 2; u32ChannelCount++)
            {
                i32ConversionData = (ADC->ADDR[(u32ChannelCount * 2)] & ADC_ADDR_RSLT_Msk) >> ADC_ADDR_RSLT_Pos;
                printf("Conversion result of differential input pair %d: 0x%X (%d)\n", u32ChannelCount, i32ConversionData, i32ConversionData);
            }

            /* Clear the ADC interrupt flag */
            ADC->ADSR0 = ADC_ADSR0_ADF_Msk;

        }
        else
            return ;

    }
}




/*---------------------------------------------------------------------------------------------------------*/
/* MAIN function                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
int main(void)
{

    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Init System, IP clock and multi-function I/O */
    SYS_Init();

    /* Lock protected registers */
    SYS_LockReg();

    /* Init UART0 for printf */
    UART0_Init();

    /*---------------------------------------------------------------------------------------------------------*/
    /* SAMPLE CODE                                                                                             */
    /*---------------------------------------------------------------------------------------------------------*/

    printf("\nSystem clock rate: %d Hz", SystemCoreClock);

    /* Continuous scan mode test */
    AdcContScanModeTest();

    /* Reset ADC module */
    SYS->IPRST1 |= (1 << SYS_IPRST1_ADCRST_Pos) ;
    SYS->IPRST1 &= ~(1 << (SYS_IPRST1_ADCRST_Pos)) ;

    /* Disable ADC IP clock */
    CLK->APBCLK0 &= ~CLK_APBCLK0_ADCCKEN_Msk;

    /* Disable External Interrupt */
    NVIC_DisableIRQ(ADC_IRQn);

    printf("Exit ADC sample code\n");

    while(1);

}

/*** (C) COPYRIGHT 2016 Nuvoton Technology Corp. ***/
