/***************************************************************************//**
* \file SPI_UART.c
* \version 3.20
*
* \brief
*  This file provides the source code to the API for the SCB Component in
*  UART mode.
*
* Note:
*
*******************************************************************************
* \copyright
* Copyright 2013-2016, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include "SPI_PVT.h"
#include "SPI_SPI_UART_PVT.h"
#include "cyapicallbacks.h"

#if (SPI_UART_WAKE_ENABLE_CONST && SPI_UART_RX_WAKEUP_IRQ)
    /**
    * \addtogroup group_globals
    * \{
    */
    /** This global variable determines whether to enable Skip Start
    * functionality when SPI_Sleep() function is called:
    * 0 – disable, other values – enable. Default value is 1.
    * It is only available when Enable wakeup from Deep Sleep Mode is enabled.
    */
    uint8 SPI_skipStart = 1u;
    /** \} globals */
#endif /* (SPI_UART_WAKE_ENABLE_CONST && SPI_UART_RX_WAKEUP_IRQ) */

#if(SPI_SCB_MODE_UNCONFIG_CONST_CFG)

    /***************************************
    *  Configuration Structure Initialization
    ***************************************/

    const SPI_UART_INIT_STRUCT SPI_configUart =
    {
        SPI_UART_SUB_MODE,
        SPI_UART_DIRECTION,
        SPI_UART_DATA_BITS_NUM,
        SPI_UART_PARITY_TYPE,
        SPI_UART_STOP_BITS_NUM,
        SPI_UART_OVS_FACTOR,
        SPI_UART_IRDA_LOW_POWER,
        SPI_UART_MEDIAN_FILTER_ENABLE,
        SPI_UART_RETRY_ON_NACK,
        SPI_UART_IRDA_POLARITY,
        SPI_UART_DROP_ON_PARITY_ERR,
        SPI_UART_DROP_ON_FRAME_ERR,
        SPI_UART_WAKE_ENABLE,
        0u,
        NULL,
        0u,
        NULL,
        SPI_UART_MP_MODE_ENABLE,
        SPI_UART_MP_ACCEPT_ADDRESS,
        SPI_UART_MP_RX_ADDRESS,
        SPI_UART_MP_RX_ADDRESS_MASK,
        (uint32) SPI_SCB_IRQ_INTERNAL,
        SPI_UART_INTR_RX_MASK,
        SPI_UART_RX_TRIGGER_LEVEL,
        SPI_UART_INTR_TX_MASK,
        SPI_UART_TX_TRIGGER_LEVEL,
        (uint8) SPI_UART_BYTE_MODE_ENABLE,
        (uint8) SPI_UART_CTS_ENABLE,
        (uint8) SPI_UART_CTS_POLARITY,
        (uint8) SPI_UART_RTS_POLARITY,
        (uint8) SPI_UART_RTS_FIFO_LEVEL
    };


    /*******************************************************************************
    * Function Name: SPI_UartInit
    ****************************************************************************//**
    *
    *  Configures the SPI for UART operation.
    *
    *  This function is intended specifically to be used when the SPI
    *  configuration is set to “Unconfigured SPI” in the customizer.
    *  After initializing the SPI in UART mode using this function,
    *  the component can be enabled using the SPI_Start() or
    * SPI_Enable() function.
    *  This function uses a pointer to a structure that provides the configuration
    *  settings. This structure contains the same information that would otherwise
    *  be provided by the customizer settings.
    *
    *  \param config: pointer to a structure that contains the following list of
    *   fields. These fields match the selections available in the customizer.
    *   Refer to the customizer for further description of the settings.
    *
    *******************************************************************************/
    void SPI_UartInit(const SPI_UART_INIT_STRUCT *config)
    {
        uint32 pinsConfig;

        if (NULL == config)
        {
            CYASSERT(0u != 0u); /* Halt execution due to bad function parameter */
        }
        else
        {
            /* Get direction to configure UART pins: TX, RX or TX+RX */
            pinsConfig  = config->direction;

        #if !(SPI_CY_SCBIP_V0 || SPI_CY_SCBIP_V1)
            /* Add RTS and CTS pins to configure */
            pinsConfig |= (0u != config->rtsRxFifoLevel) ? (SPI_UART_RTS_PIN_ENABLE) : (0u);
            pinsConfig |= (0u != config->enableCts)      ? (SPI_UART_CTS_PIN_ENABLE) : (0u);
        #endif /* !(SPI_CY_SCBIP_V0 || SPI_CY_SCBIP_V1) */

            /* Configure pins */
            SPI_SetPins(SPI_SCB_MODE_UART, config->mode, pinsConfig);

            /* Store internal configuration */
            SPI_scbMode       = (uint8) SPI_SCB_MODE_UART;
            SPI_scbEnableWake = (uint8) config->enableWake;
            SPI_scbEnableIntr = (uint8) config->enableInterrupt;

            /* Set RX direction internal variables */
            SPI_rxBuffer      =         config->rxBuffer;
            SPI_rxDataBits    = (uint8) config->dataBits;
            SPI_rxBufferSize  = (uint8) config->rxBufferSize;

            /* Set TX direction internal variables */
            SPI_txBuffer      =         config->txBuffer;
            SPI_txDataBits    = (uint8) config->dataBits;
            SPI_txBufferSize  = (uint8) config->txBufferSize;

            /* Configure UART interface */
            if(SPI_UART_MODE_IRDA == config->mode)
            {
                /* OVS settings: IrDA */
                SPI_CTRL_REG  = ((0u != config->enableIrdaLowPower) ?
                                                (SPI_UART_GET_CTRL_OVS_IRDA_LP(config->oversample)) :
                                                (SPI_CTRL_OVS_IRDA_OVS16));
            }
            else
            {
                /* OVS settings: UART and SmartCard */
                SPI_CTRL_REG  = SPI_GET_CTRL_OVS(config->oversample);
            }

            SPI_CTRL_REG     |= SPI_GET_CTRL_BYTE_MODE  (config->enableByteMode)      |
                                             SPI_GET_CTRL_ADDR_ACCEPT(config->multiprocAcceptAddr) |
                                             SPI_CTRL_UART;

            /* Configure sub-mode: UART, SmartCard or IrDA */
            SPI_UART_CTRL_REG = SPI_GET_UART_CTRL_MODE(config->mode);

            /* Configure RX direction */
            SPI_UART_RX_CTRL_REG = SPI_GET_UART_RX_CTRL_MODE(config->stopBits)              |
                                        SPI_GET_UART_RX_CTRL_POLARITY(config->enableInvertedRx)          |
                                        SPI_GET_UART_RX_CTRL_MP_MODE(config->enableMultiproc)            |
                                        SPI_GET_UART_RX_CTRL_DROP_ON_PARITY_ERR(config->dropOnParityErr) |
                                        SPI_GET_UART_RX_CTRL_DROP_ON_FRAME_ERR(config->dropOnFrameErr);

            if(SPI_UART_PARITY_NONE != config->parity)
            {
               SPI_UART_RX_CTRL_REG |= SPI_GET_UART_RX_CTRL_PARITY(config->parity) |
                                                    SPI_UART_RX_CTRL_PARITY_ENABLED;
            }

            SPI_RX_CTRL_REG      = SPI_GET_RX_CTRL_DATA_WIDTH(config->dataBits)       |
                                                SPI_GET_RX_CTRL_MEDIAN(config->enableMedianFilter) |
                                                SPI_GET_UART_RX_CTRL_ENABLED(config->direction);

            SPI_RX_FIFO_CTRL_REG = SPI_GET_RX_FIFO_CTRL_TRIGGER_LEVEL(config->rxTriggerLevel);

            /* Configure MP address */
            SPI_RX_MATCH_REG     = SPI_GET_RX_MATCH_ADDR(config->multiprocAddr) |
                                                SPI_GET_RX_MATCH_MASK(config->multiprocAddrMask);

            /* Configure RX direction */
            SPI_UART_TX_CTRL_REG = SPI_GET_UART_TX_CTRL_MODE(config->stopBits) |
                                                SPI_GET_UART_TX_CTRL_RETRY_NACK(config->enableRetryNack);

            if(SPI_UART_PARITY_NONE != config->parity)
            {
               SPI_UART_TX_CTRL_REG |= SPI_GET_UART_TX_CTRL_PARITY(config->parity) |
                                                    SPI_UART_TX_CTRL_PARITY_ENABLED;
            }

            SPI_TX_CTRL_REG      = SPI_GET_TX_CTRL_DATA_WIDTH(config->dataBits)    |
                                                SPI_GET_UART_TX_CTRL_ENABLED(config->direction);

            SPI_TX_FIFO_CTRL_REG = SPI_GET_TX_FIFO_CTRL_TRIGGER_LEVEL(config->txTriggerLevel);

        #if !(SPI_CY_SCBIP_V0 || SPI_CY_SCBIP_V1)
            SPI_UART_FLOW_CTRL_REG = SPI_GET_UART_FLOW_CTRL_CTS_ENABLE(config->enableCts) | \
                                            SPI_GET_UART_FLOW_CTRL_CTS_POLARITY (config->ctsPolarity)  | \
                                            SPI_GET_UART_FLOW_CTRL_RTS_POLARITY (config->rtsPolarity)  | \
                                            SPI_GET_UART_FLOW_CTRL_TRIGGER_LEVEL(config->rtsRxFifoLevel);
        #endif /* !(SPI_CY_SCBIP_V0 || SPI_CY_SCBIP_V1) */

            /* Configure interrupt with UART handler but do not enable it */
            CyIntDisable    (SPI_ISR_NUMBER);
            CyIntSetPriority(SPI_ISR_NUMBER, SPI_ISR_PRIORITY);
            (void) CyIntSetVector(SPI_ISR_NUMBER, &SPI_SPI_UART_ISR);

            /* Configure WAKE interrupt */
        #if(SPI_UART_RX_WAKEUP_IRQ)
            CyIntDisable    (SPI_RX_WAKE_ISR_NUMBER);
            CyIntSetPriority(SPI_RX_WAKE_ISR_NUMBER, SPI_RX_WAKE_ISR_PRIORITY);
            (void) CyIntSetVector(SPI_RX_WAKE_ISR_NUMBER, &SPI_UART_WAKEUP_ISR);
        #endif /* (SPI_UART_RX_WAKEUP_IRQ) */

            /* Configure interrupt sources */
            SPI_INTR_I2C_EC_MASK_REG = SPI_NO_INTR_SOURCES;
            SPI_INTR_SPI_EC_MASK_REG = SPI_NO_INTR_SOURCES;
            SPI_INTR_SLAVE_MASK_REG  = SPI_NO_INTR_SOURCES;
            SPI_INTR_MASTER_MASK_REG = SPI_NO_INTR_SOURCES;
            SPI_INTR_RX_MASK_REG     = config->rxInterruptMask;
            SPI_INTR_TX_MASK_REG     = config->txInterruptMask;
        
            /* Configure TX interrupt sources to restore. */
            SPI_IntrTxMask = LO16(SPI_INTR_TX_MASK_REG);

            /* Clear RX buffer indexes */
            SPI_rxBufferHead     = 0u;
            SPI_rxBufferTail     = 0u;
            SPI_rxBufferOverflow = 0u;

            /* Clear TX buffer indexes */
            SPI_txBufferHead = 0u;
            SPI_txBufferTail = 0u;
        }
    }

#else

    /*******************************************************************************
    * Function Name: SPI_UartInit
    ****************************************************************************//**
    *
    *  Configures the SCB for the UART operation.
    *
    *******************************************************************************/
    void SPI_UartInit(void)
    {
        /* Configure UART interface */
        SPI_CTRL_REG = SPI_UART_DEFAULT_CTRL;

        /* Configure sub-mode: UART, SmartCard or IrDA */
        SPI_UART_CTRL_REG = SPI_UART_DEFAULT_UART_CTRL;

        /* Configure RX direction */
        SPI_UART_RX_CTRL_REG = SPI_UART_DEFAULT_UART_RX_CTRL;
        SPI_RX_CTRL_REG      = SPI_UART_DEFAULT_RX_CTRL;
        SPI_RX_FIFO_CTRL_REG = SPI_UART_DEFAULT_RX_FIFO_CTRL;
        SPI_RX_MATCH_REG     = SPI_UART_DEFAULT_RX_MATCH_REG;

        /* Configure TX direction */
        SPI_UART_TX_CTRL_REG = SPI_UART_DEFAULT_UART_TX_CTRL;
        SPI_TX_CTRL_REG      = SPI_UART_DEFAULT_TX_CTRL;
        SPI_TX_FIFO_CTRL_REG = SPI_UART_DEFAULT_TX_FIFO_CTRL;

    #if !(SPI_CY_SCBIP_V0 || SPI_CY_SCBIP_V1)
        SPI_UART_FLOW_CTRL_REG = SPI_UART_DEFAULT_FLOW_CTRL;
    #endif /* !(SPI_CY_SCBIP_V0 || SPI_CY_SCBIP_V1) */

        /* Configure interrupt with UART handler but do not enable it */
    #if(SPI_SCB_IRQ_INTERNAL)
        CyIntDisable    (SPI_ISR_NUMBER);
        CyIntSetPriority(SPI_ISR_NUMBER, SPI_ISR_PRIORITY);
        (void) CyIntSetVector(SPI_ISR_NUMBER, &SPI_SPI_UART_ISR);
    #endif /* (SPI_SCB_IRQ_INTERNAL) */

        /* Configure WAKE interrupt */
    #if(SPI_UART_RX_WAKEUP_IRQ)
        CyIntDisable    (SPI_RX_WAKE_ISR_NUMBER);
        CyIntSetPriority(SPI_RX_WAKE_ISR_NUMBER, SPI_RX_WAKE_ISR_PRIORITY);
        (void) CyIntSetVector(SPI_RX_WAKE_ISR_NUMBER, &SPI_UART_WAKEUP_ISR);
    #endif /* (SPI_UART_RX_WAKEUP_IRQ) */

        /* Configure interrupt sources */
        SPI_INTR_I2C_EC_MASK_REG = SPI_UART_DEFAULT_INTR_I2C_EC_MASK;
        SPI_INTR_SPI_EC_MASK_REG = SPI_UART_DEFAULT_INTR_SPI_EC_MASK;
        SPI_INTR_SLAVE_MASK_REG  = SPI_UART_DEFAULT_INTR_SLAVE_MASK;
        SPI_INTR_MASTER_MASK_REG = SPI_UART_DEFAULT_INTR_MASTER_MASK;
        SPI_INTR_RX_MASK_REG     = SPI_UART_DEFAULT_INTR_RX_MASK;
        SPI_INTR_TX_MASK_REG     = SPI_UART_DEFAULT_INTR_TX_MASK;
    
        /* Configure TX interrupt sources to restore. */
        SPI_IntrTxMask = LO16(SPI_INTR_TX_MASK_REG);

    #if(SPI_INTERNAL_RX_SW_BUFFER_CONST)
        SPI_rxBufferHead     = 0u;
        SPI_rxBufferTail     = 0u;
        SPI_rxBufferOverflow = 0u;
    #endif /* (SPI_INTERNAL_RX_SW_BUFFER_CONST) */

    #if(SPI_INTERNAL_TX_SW_BUFFER_CONST)
        SPI_txBufferHead = 0u;
        SPI_txBufferTail = 0u;
    #endif /* (SPI_INTERNAL_TX_SW_BUFFER_CONST) */
    }
#endif /* (SPI_SCB_MODE_UNCONFIG_CONST_CFG) */


/*******************************************************************************
* Function Name: SPI_UartPostEnable
****************************************************************************//**
*
*  Restores HSIOM settings for the UART output pins (TX and/or RTS) to be
*  controlled by the SCB UART.
*
*******************************************************************************/
void SPI_UartPostEnable(void)
{
#if (SPI_SCB_MODE_UNCONFIG_CONST_CFG)
    #if (SPI_TX_SDA_MISO_PIN)
        if (SPI_CHECK_TX_SDA_MISO_PIN_USED)
        {
            /* Set SCB UART to drive the output pin */
            SPI_SET_HSIOM_SEL(SPI_TX_SDA_MISO_HSIOM_REG, SPI_TX_SDA_MISO_HSIOM_MASK,
                                           SPI_TX_SDA_MISO_HSIOM_POS, SPI_TX_SDA_MISO_HSIOM_SEL_UART);
        }
    #endif /* (SPI_TX_SDA_MISO_PIN_PIN) */

    #if !(SPI_CY_SCBIP_V0 || SPI_CY_SCBIP_V1)
        #if (SPI_RTS_SS0_PIN)
            if (SPI_CHECK_RTS_SS0_PIN_USED)
            {
                /* Set SCB UART to drive the output pin */
                SPI_SET_HSIOM_SEL(SPI_RTS_SS0_HSIOM_REG, SPI_RTS_SS0_HSIOM_MASK,
                                               SPI_RTS_SS0_HSIOM_POS, SPI_RTS_SS0_HSIOM_SEL_UART);
            }
        #endif /* (SPI_RTS_SS0_PIN) */
    #endif /* !(SPI_CY_SCBIP_V0 || SPI_CY_SCBIP_V1) */

#else
    #if (SPI_UART_TX_PIN)
         /* Set SCB UART to drive the output pin */
        SPI_SET_HSIOM_SEL(SPI_TX_HSIOM_REG, SPI_TX_HSIOM_MASK,
                                       SPI_TX_HSIOM_POS, SPI_TX_HSIOM_SEL_UART);
    #endif /* (SPI_UART_TX_PIN) */

    #if (SPI_UART_RTS_PIN)
        /* Set SCB UART to drive the output pin */
        SPI_SET_HSIOM_SEL(SPI_RTS_HSIOM_REG, SPI_RTS_HSIOM_MASK,
                                       SPI_RTS_HSIOM_POS, SPI_RTS_HSIOM_SEL_UART);
    #endif /* (SPI_UART_RTS_PIN) */
#endif /* (SPI_SCB_MODE_UNCONFIG_CONST_CFG) */

    /* Restore TX interrupt sources. */
    SPI_SetTxInterruptMode(SPI_IntrTxMask);
}


/*******************************************************************************
* Function Name: SPI_UartStop
****************************************************************************//**
*
*  Changes the HSIOM settings for the UART output pins (TX and/or RTS) to keep
*  them inactive after the block is disabled. The output pins are controlled by
*  the GPIO data register. Also, the function disables the skip start feature
*  to not cause it to trigger after the component is enabled.
*
*******************************************************************************/
void SPI_UartStop(void)
{
#if(SPI_SCB_MODE_UNCONFIG_CONST_CFG)
    #if (SPI_TX_SDA_MISO_PIN)
        if (SPI_CHECK_TX_SDA_MISO_PIN_USED)
        {
            /* Set GPIO to drive output pin */
            SPI_SET_HSIOM_SEL(SPI_TX_SDA_MISO_HSIOM_REG, SPI_TX_SDA_MISO_HSIOM_MASK,
                                           SPI_TX_SDA_MISO_HSIOM_POS, SPI_TX_SDA_MISO_HSIOM_SEL_GPIO);
        }
    #endif /* (SPI_TX_SDA_MISO_PIN_PIN) */

    #if !(SPI_CY_SCBIP_V0 || SPI_CY_SCBIP_V1)
        #if (SPI_RTS_SS0_PIN)
            if (SPI_CHECK_RTS_SS0_PIN_USED)
            {
                /* Set output pin state after block is disabled */
                SPI_uart_rts_spi_ss0_Write(SPI_GET_UART_RTS_INACTIVE);

                /* Set GPIO to drive output pin */
                SPI_SET_HSIOM_SEL(SPI_RTS_SS0_HSIOM_REG, SPI_RTS_SS0_HSIOM_MASK,
                                               SPI_RTS_SS0_HSIOM_POS, SPI_RTS_SS0_HSIOM_SEL_GPIO);
            }
        #endif /* (SPI_RTS_SS0_PIN) */
    #endif /* !(SPI_CY_SCBIP_V0 || SPI_CY_SCBIP_V1) */

#else
    #if (SPI_UART_TX_PIN)
        /* Set GPIO to drive output pin */
        SPI_SET_HSIOM_SEL(SPI_TX_HSIOM_REG, SPI_TX_HSIOM_MASK,
                                       SPI_TX_HSIOM_POS, SPI_TX_HSIOM_SEL_GPIO);
    #endif /* (SPI_UART_TX_PIN) */

    #if (SPI_UART_RTS_PIN)
        /* Set output pin state after block is disabled */
        SPI_rts_Write(SPI_GET_UART_RTS_INACTIVE);

        /* Set GPIO to drive output pin */
        SPI_SET_HSIOM_SEL(SPI_RTS_HSIOM_REG, SPI_RTS_HSIOM_MASK,
                                       SPI_RTS_HSIOM_POS, SPI_RTS_HSIOM_SEL_GPIO);
    #endif /* (SPI_UART_RTS_PIN) */

#endif /* (SPI_SCB_MODE_UNCONFIG_CONST_CFG) */

#if (SPI_UART_WAKE_ENABLE_CONST)
    /* Disable skip start feature used for wakeup */
    SPI_UART_RX_CTRL_REG &= (uint32) ~SPI_UART_RX_CTRL_SKIP_START;
#endif /* (SPI_UART_WAKE_ENABLE_CONST) */

    /* Store TX interrupt sources (exclude level triggered). */
    SPI_IntrTxMask = LO16(SPI_GetTxInterruptMode() & SPI_INTR_UART_TX_RESTORE);
}


/*******************************************************************************
* Function Name: SPI_UartSetRxAddress
****************************************************************************//**
*
*  Sets the hardware detectable receiver address for the UART in the
*  Multiprocessor mode.
*
*  \param address: Address for hardware address detection.
*
*******************************************************************************/
void SPI_UartSetRxAddress(uint32 address)
{
     uint32 matchReg;

    matchReg = SPI_RX_MATCH_REG;

    matchReg &= ((uint32) ~SPI_RX_MATCH_ADDR_MASK); /* Clear address bits */
    matchReg |= ((uint32)  (address & SPI_RX_MATCH_ADDR_MASK)); /* Set address  */

    SPI_RX_MATCH_REG = matchReg;
}


/*******************************************************************************
* Function Name: SPI_UartSetRxAddressMask
****************************************************************************//**
*
*  Sets the hardware address mask for the UART in the Multiprocessor mode.
*
*  \param addressMask: Address mask.
*   - Bit value 0 – excludes bit from address comparison.
*   - Bit value 1 – the bit needs to match with the corresponding bit
*     of the address.
*
*******************************************************************************/
void SPI_UartSetRxAddressMask(uint32 addressMask)
{
    uint32 matchReg;

    matchReg = SPI_RX_MATCH_REG;

    matchReg &= ((uint32) ~SPI_RX_MATCH_MASK_MASK); /* Clear address mask bits */
    matchReg |= ((uint32) (addressMask << SPI_RX_MATCH_MASK_POS));

    SPI_RX_MATCH_REG = matchReg;
}


#if(SPI_UART_RX_DIRECTION)
    /*******************************************************************************
    * Function Name: SPI_UartGetChar
    ****************************************************************************//**
    *
    *  Retrieves next data element from receive buffer.
    *  This function is designed for ASCII characters and returns a char where
    *  1 to 255 are valid characters and 0 indicates an error occurred or no data
    *  is present.
    *  - RX software buffer is disabled: Returns data element retrieved from RX
    *    FIFO.
    *  - RX software buffer is enabled: Returns data element from the software
    *    receive buffer.
    *
    *  \return
    *   Next data element from the receive buffer. ASCII character values from
    *   1 to 255 are valid. A returned zero signifies an error condition or no
    *   data available.
    *
    *  \sideeffect
    *   The errors bits may not correspond with reading characters due to
    *   RX FIFO and software buffer usage.
    *   RX software buffer is enabled: The internal software buffer overflow
    *   is not treated as an error condition.
    *   Check SPI_rxBufferOverflow to capture that error condition.
    *
    *******************************************************************************/
    uint32 SPI_UartGetChar(void)
    {
        uint32 rxData = 0u;

        /* Reads data only if there is data to read */
        if (0u != SPI_SpiUartGetRxBufferSize())
        {
            rxData = SPI_SpiUartReadRxData();
        }

        if (SPI_CHECK_INTR_RX(SPI_INTR_RX_ERR))
        {
            rxData = 0u; /* Error occurred: returns zero */
            SPI_ClearRxInterruptSource(SPI_INTR_RX_ERR);
        }

        return (rxData);
    }


    /*******************************************************************************
    * Function Name: SPI_UartGetByte
    ****************************************************************************//**
    *
    *  Retrieves the next data element from the receive buffer, returns the
    *  received byte and error condition.
    *   - The RX software buffer is disabled: returns the data element retrieved
    *     from the RX FIFO. Undefined data will be returned if the RX FIFO is
    *     empty.
    *   - The RX software buffer is enabled: returns data element from the
    *     software receive buffer.
    *
    *  \return
    *   Bits 7-0 contain the next data element from the receive buffer and
    *   other bits contain the error condition.
    *   - SPI_UART_RX_OVERFLOW - Attempt to write to a full
    *     receiver FIFO.
    *   - SPI_UART_RX_UNDERFLOW	Attempt to read from an empty
    *     receiver FIFO.
    *   - SPI_UART_RX_FRAME_ERROR - UART framing error detected.
    *   - SPI_UART_RX_PARITY_ERROR - UART parity error detected.
    *
    *  \sideeffect
    *   The errors bits may not correspond with reading characters due to
    *   RX FIFO and software buffer usage.
    *   RX software buffer is enabled: The internal software buffer overflow
    *   is not treated as an error condition.
    *   Check SPI_rxBufferOverflow to capture that error condition.
    *
    *******************************************************************************/
    uint32 SPI_UartGetByte(void)
    {
        uint32 rxData;
        uint32 tmpStatus;

        if (SPI_CHECK_RX_SW_BUFFER)
        {
            SPI_DisableInt();
        }
        

        if (0u != SPI_SpiUartGetRxBufferSize())
        {
            /* Enables interrupt to receive more bytes: at least one byte is in
            * buffer.
            */
            if (SPI_CHECK_RX_SW_BUFFER)
            {
                SPI_EnableInt();
            }
            

            /* Get received byte */
            rxData = SPI_SpiUartReadRxData();
        }
        else
        {
            /* Reads a byte directly from RX FIFO: underflow is raised in the
            * case of empty. Otherwise the first received byte will be read.
            */
            rxData = SPI_RX_FIFO_RD_REG;


            /* Enables interrupt to receive more bytes. */
            if (SPI_CHECK_RX_SW_BUFFER)
            {

                /* The byte has been read from RX FIFO. Clear RX interrupt to
                * not involve interrupt handler when RX FIFO is empty.
                */
                SPI_ClearRxInterruptSource(SPI_INTR_RX_NOT_EMPTY);

                SPI_EnableInt();
            }
            
        }

        /* Get and clear RX error mask */
        tmpStatus = (SPI_GetRxInterruptSource() & SPI_INTR_RX_ERR);
        SPI_ClearRxInterruptSource(SPI_INTR_RX_ERR);

        /* Puts together data and error status:
        * MP mode and accept address: 9th bit is set to notify mark.
        */
        rxData |= ((uint32) (tmpStatus << 8u));

        return (rxData);
    }


    #if !(SPI_CY_SCBIP_V0 || SPI_CY_SCBIP_V1)
        /*******************************************************************************
        * Function Name: SPI_UartSetRtsPolarity
        ****************************************************************************//**
        *
        *  Sets active polarity of RTS output signal.
        *  Only available for PSoC 4100 BLE / PSoC 4200 BLE / PSoC 4100M / PSoC 4200M /
        *  PSoC 4200L / PSoC 4000S / PSoC 4100S / PSoC Analog Coprocessor devices.
        *
        *  \param polarity: Active polarity of RTS output signal.
        *   - SPI_UART_RTS_ACTIVE_LOW  - RTS signal is active low.
        *   - SPI_UART_RTS_ACTIVE_HIGH - RTS signal is active high.
        *
        *******************************************************************************/
        void SPI_UartSetRtsPolarity(uint32 polarity)
        {
            if(0u != polarity)
            {
                SPI_UART_FLOW_CTRL_REG |= (uint32)  SPI_UART_FLOW_CTRL_RTS_POLARITY;
            }
            else
            {
                SPI_UART_FLOW_CTRL_REG &= (uint32) ~SPI_UART_FLOW_CTRL_RTS_POLARITY;
            }
        }


        /*******************************************************************************
        * Function Name: SPI_UartSetRtsFifoLevel
        ****************************************************************************//**
        *
        *  Sets level in the RX FIFO for RTS signal activation.
        *  While the RX FIFO has fewer entries than the RX FIFO level the RTS signal
        *  remains active, otherwise the RTS signal becomes inactive.
        *  Only available for PSoC 4100 BLE / PSoC 4200 BLE / PSoC 4100M / PSoC 4200M /
        *  PSoC 4200L / PSoC 4000S / PSoC 4100S / PSoC Analog Coprocessor devices.
        *
        *  \param level: Level in the RX FIFO for RTS signal activation.
        *   The range of valid level values is between 0 and RX FIFO depth - 1.
        *   Setting level value to 0 disables RTS signal activation.
        *
        *******************************************************************************/
        void SPI_UartSetRtsFifoLevel(uint32 level)
        {
            uint32 uartFlowCtrl;

            uartFlowCtrl = SPI_UART_FLOW_CTRL_REG;

            uartFlowCtrl &= ((uint32) ~SPI_UART_FLOW_CTRL_TRIGGER_LEVEL_MASK); /* Clear level mask bits */
            uartFlowCtrl |= ((uint32) (SPI_UART_FLOW_CTRL_TRIGGER_LEVEL_MASK & level));

            SPI_UART_FLOW_CTRL_REG = uartFlowCtrl;
        }
    #endif /* !(SPI_CY_SCBIP_V0 || SPI_CY_SCBIP_V1) */

#endif /* (SPI_UART_RX_DIRECTION) */


#if(SPI_UART_TX_DIRECTION)
    /*******************************************************************************
    * Function Name: SPI_UartPutString
    ****************************************************************************//**
    *
    *  Places a NULL terminated string in the transmit buffer to be sent at the
    *  next available bus time.
    *  This function is blocking and waits until there is a space available to put
    *  requested data in transmit buffer.
    *
    *  \param string: pointer to the null terminated string array to be placed in the
    *   transmit buffer.
    *
    *******************************************************************************/
    void SPI_UartPutString(const char8 string[])
    {
        uint32 bufIndex;

        bufIndex = 0u;

        /* Blocks the control flow until all data has been sent */
        while(string[bufIndex] != ((char8) 0))
        {
            SPI_UartPutChar((uint32) string[bufIndex]);
            bufIndex++;
        }
    }


    /*******************************************************************************
    * Function Name: SPI_UartPutCRLF
    ****************************************************************************//**
    *
    *  Places byte of data followed by a carriage return (0x0D) and line feed
    *  (0x0A) in the transmit buffer.
    *  This function is blocking and waits until there is a space available to put
    *  all requested data in transmit buffer.
    *
    *  \param txDataByte: the data to be transmitted.
    *
    *******************************************************************************/
    void SPI_UartPutCRLF(uint32 txDataByte)
    {
        SPI_UartPutChar(txDataByte);  /* Blocks control flow until all data has been sent */
        SPI_UartPutChar(0x0Du);       /* Blocks control flow until all data has been sent */
        SPI_UartPutChar(0x0Au);       /* Blocks control flow until all data has been sent */
    }


    #if !(SPI_CY_SCBIP_V0 || SPI_CY_SCBIP_V1)
        /*******************************************************************************
        * Function Name: SPISCB_UartEnableCts
        ****************************************************************************//**
        *
        *  Enables usage of CTS input signal by the UART transmitter.
        *  Only available for PSoC 4100 BLE / PSoC 4200 BLE / PSoC 4100M / PSoC 4200M /
        *  PSoC 4200L / PSoC 4000S / PSoC 4100S / PSoC Analog Coprocessor devices.
        *
        *******************************************************************************/
        void SPI_UartEnableCts(void)
        {
            SPI_UART_FLOW_CTRL_REG |= (uint32)  SPI_UART_FLOW_CTRL_CTS_ENABLE;
        }


        /*******************************************************************************
        * Function Name: SPI_UartDisableCts
        ****************************************************************************//**
        *
        *  Disables usage of CTS input signal by the UART transmitter.
        *  Only available for PSoC 4100 BLE / PSoC 4200 BLE / PSoC 4100M / PSoC 4200M /
        *  PSoC 4200L / PSoC 4000S / PSoC 4100S / PSoC Analog Coprocessor devices.
        *
        *******************************************************************************/
        void SPI_UartDisableCts(void)
        {
            SPI_UART_FLOW_CTRL_REG &= (uint32) ~SPI_UART_FLOW_CTRL_CTS_ENABLE;
        }


        /*******************************************************************************
        * Function Name: SPI_UartSetCtsPolarity
        ****************************************************************************//**
        *
        *  Sets active polarity of CTS input signal.
        *  Only available for PSoC 4100 BLE / PSoC 4200 BLE / PSoC 4100M / PSoC 4200M /
        *  PSoC 4200L / PSoC 4000S / PSoC 4100S / PSoC Analog Coprocessor devices.
        *
        *  \param polarity: Active polarity of CTS output signal.
        *   - SPI_UART_CTS_ACTIVE_LOW  - CTS signal is active low.
        *   - SPI_UART_CTS_ACTIVE_HIGH - CTS signal is active high.
        *
        *******************************************************************************/
        void SPI_UartSetCtsPolarity(uint32 polarity)
        {
            if (0u != polarity)
            {
                SPI_UART_FLOW_CTRL_REG |= (uint32)  SPI_UART_FLOW_CTRL_CTS_POLARITY;
            }
            else
            {
                SPI_UART_FLOW_CTRL_REG &= (uint32) ~SPI_UART_FLOW_CTRL_CTS_POLARITY;
            }
        }
    #endif /* !(SPI_CY_SCBIP_V0 || SPI_CY_SCBIP_V1) */

#endif /* (SPI_UART_TX_DIRECTION) */


#if (SPI_UART_WAKE_ENABLE_CONST)
    /*******************************************************************************
    * Function Name: SPI_UartSaveConfig
    ****************************************************************************//**
    *
    *  Clears and enables an interrupt on a falling edge of the Rx input. The GPIO
    *  interrupt does not track in the active mode, therefore requires to be 
    *  cleared by this API.
    *
    *******************************************************************************/
    void SPI_UartSaveConfig(void)
    {
    #if (SPI_UART_RX_WAKEUP_IRQ)
        /* Set SKIP_START if requested (set by default). */
        if (0u != SPI_skipStart)
        {
            SPI_UART_RX_CTRL_REG |= (uint32)  SPI_UART_RX_CTRL_SKIP_START;
        }
        else
        {
            SPI_UART_RX_CTRL_REG &= (uint32) ~SPI_UART_RX_CTRL_SKIP_START;
        }
        
        /* Clear RX GPIO interrupt status and pending interrupt in NVIC because
        * falling edge on RX line occurs while UART communication in active mode.
        * Enable interrupt: next interrupt trigger should wakeup device.
        */
        SPI_CLEAR_UART_RX_WAKE_INTR;
        SPI_RxWakeClearPendingInt();
        SPI_RxWakeEnableInt();
    #endif /* (SPI_UART_RX_WAKEUP_IRQ) */
    }


    /*******************************************************************************
    * Function Name: SPI_UartRestoreConfig
    ****************************************************************************//**
    *
    *  Disables the RX GPIO interrupt. Until this function is called the interrupt
    *  remains active and triggers on every falling edge of the UART RX line.
    *
    *******************************************************************************/
    void SPI_UartRestoreConfig(void)
    {
    #if (SPI_UART_RX_WAKEUP_IRQ)
        /* Disable interrupt: no more triggers in active mode */
        SPI_RxWakeDisableInt();
    #endif /* (SPI_UART_RX_WAKEUP_IRQ) */
    }


    #if (SPI_UART_RX_WAKEUP_IRQ)
        /*******************************************************************************
        * Function Name: SPI_UART_WAKEUP_ISR
        ****************************************************************************//**
        *
        *  Handles the Interrupt Service Routine for the SCB UART mode GPIO wakeup
        *  event. This event is configured to trigger on a falling edge of the RX line.
        *
        *******************************************************************************/
        CY_ISR(SPI_UART_WAKEUP_ISR)
        {
        #ifdef SPI_UART_WAKEUP_ISR_ENTRY_CALLBACK
            SPI_UART_WAKEUP_ISR_EntryCallback();
        #endif /* SPI_UART_WAKEUP_ISR_ENTRY_CALLBACK */

            SPI_CLEAR_UART_RX_WAKE_INTR;

        #ifdef SPI_UART_WAKEUP_ISR_EXIT_CALLBACK
            SPI_UART_WAKEUP_ISR_ExitCallback();
        #endif /* SPI_UART_WAKEUP_ISR_EXIT_CALLBACK */
        }
    #endif /* (SPI_UART_RX_WAKEUP_IRQ) */
#endif /* (SPI_UART_RX_WAKEUP_IRQ) */


/* [] END OF FILE */
