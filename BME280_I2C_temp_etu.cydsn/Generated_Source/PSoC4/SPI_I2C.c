/***************************************************************************//**
* \file SPI_I2C.c
* \version 3.20
*
* \brief
*  This file provides the source code to the API for the SCB Component in
*  I2C mode.
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
#include "SPI_I2C_PVT.h"


/***************************************
*      I2C Private Vars
***************************************/

volatile uint8 SPI_state;  /* Current state of I2C FSM */

#if(SPI_SCB_MODE_UNCONFIG_CONST_CFG)

    /***************************************
    *  Configuration Structure Initialization
    ***************************************/

    /* Constant configuration of I2C */
    const SPI_I2C_INIT_STRUCT SPI_configI2C =
    {
        SPI_I2C_MODE,
        SPI_I2C_OVS_FACTOR_LOW,
        SPI_I2C_OVS_FACTOR_HIGH,
        SPI_I2C_MEDIAN_FILTER_ENABLE,
        SPI_I2C_SLAVE_ADDRESS,
        SPI_I2C_SLAVE_ADDRESS_MASK,
        SPI_I2C_ACCEPT_ADDRESS,
        SPI_I2C_WAKE_ENABLE,
        SPI_I2C_BYTE_MODE_ENABLE,
        SPI_I2C_DATA_RATE,
        SPI_I2C_ACCEPT_GENERAL_CALL,
    };

    /*******************************************************************************
    * Function Name: SPI_I2CInit
    ****************************************************************************//**
    *
    *
    *  Configures the SPI for I2C operation.
    *
    *  This function is intended specifically to be used when the SPI 
    *  configuration is set to “Unconfigured SPI” in the customizer. 
    *  After initializing the SPI in I2C mode using this function, 
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
    void SPI_I2CInit(const SPI_I2C_INIT_STRUCT *config)
    {
        uint32 medianFilter;
        uint32 locEnableWake;

        if(NULL == config)
        {
            CYASSERT(0u != 0u); /* Halt execution due to bad function parameter */
        }
        else
        {
            /* Configure pins */
            SPI_SetPins(SPI_SCB_MODE_I2C, SPI_DUMMY_PARAM,
                                     SPI_DUMMY_PARAM);

            /* Store internal configuration */
            SPI_scbMode       = (uint8) SPI_SCB_MODE_I2C;
            SPI_scbEnableWake = (uint8) config->enableWake;
            SPI_scbEnableIntr = (uint8) SPI_SCB_IRQ_INTERNAL;

            SPI_mode          = (uint8) config->mode;
            SPI_acceptAddr    = (uint8) config->acceptAddr;

        #if (SPI_CY_SCBIP_V0)
            /* Adjust SDA filter settings. Ticket ID#150521 */
            SPI_SET_I2C_CFG_SDA_FILT_TRIM(SPI_EC_AM_I2C_CFG_SDA_FILT_TRIM);
        #endif /* (SPI_CY_SCBIP_V0) */

            /* Adjust AF and DF filter settings. Ticket ID#176179 */
            if (((SPI_I2C_MODE_SLAVE != config->mode) &&
                 (config->dataRate <= SPI_I2C_DATA_RATE_FS_MODE_MAX)) ||
                 (SPI_I2C_MODE_SLAVE == config->mode))
            {
                /* AF = 1, DF = 0 */
                SPI_I2C_CFG_ANALOG_FITER_ENABLE;
                medianFilter = SPI_DIGITAL_FILTER_DISABLE;
            }
            else
            {
                /* AF = 0, DF = 1 */
                SPI_I2C_CFG_ANALOG_FITER_DISABLE;
                medianFilter = SPI_DIGITAL_FILTER_ENABLE;
            }

        #if (!SPI_CY_SCBIP_V0)
            locEnableWake = (SPI_I2C_MULTI_MASTER_SLAVE) ? (0u) : (config->enableWake);
        #else
            locEnableWake = config->enableWake;
        #endif /* (!SPI_CY_SCBIP_V0) */

            /* Configure I2C interface */
            SPI_CTRL_REG     = SPI_GET_CTRL_BYTE_MODE  (config->enableByteMode) |
                                            SPI_GET_CTRL_ADDR_ACCEPT(config->acceptAddr)     |
                                            SPI_GET_CTRL_EC_AM_MODE (locEnableWake);

            SPI_I2C_CTRL_REG = SPI_GET_I2C_CTRL_HIGH_PHASE_OVS(config->oversampleHigh) |
                    SPI_GET_I2C_CTRL_LOW_PHASE_OVS (config->oversampleLow)                          |
                    SPI_GET_I2C_CTRL_S_GENERAL_IGNORE((uint32)(0u == config->acceptGeneralAddr))    |
                    SPI_GET_I2C_CTRL_SL_MSTR_MODE  (config->mode);

            /* Configure RX direction */
            SPI_RX_CTRL_REG      = SPI_GET_RX_CTRL_MEDIAN(medianFilter) |
                                                SPI_I2C_RX_CTRL;
            SPI_RX_FIFO_CTRL_REG = SPI_CLEAR_REG;

            /* Set default address and mask */
            SPI_RX_MATCH_REG    = ((SPI_I2C_SLAVE) ?
                                                (SPI_GET_I2C_8BIT_ADDRESS(config->slaveAddr) |
                                                 SPI_GET_RX_MATCH_MASK(config->slaveAddrMask)) :
                                                (SPI_CLEAR_REG));


            /* Configure TX direction */
            SPI_TX_CTRL_REG      = SPI_I2C_TX_CTRL;
            SPI_TX_FIFO_CTRL_REG = SPI_CLEAR_REG;

            /* Configure interrupt with I2C handler but do not enable it */
            CyIntDisable    (SPI_ISR_NUMBER);
            CyIntSetPriority(SPI_ISR_NUMBER, SPI_ISR_PRIORITY);
            (void) CyIntSetVector(SPI_ISR_NUMBER, &SPI_I2C_ISR);

            /* Configure interrupt sources */
        #if(!SPI_CY_SCBIP_V1)
            SPI_INTR_SPI_EC_MASK_REG = SPI_NO_INTR_SOURCES;
        #endif /* (!SPI_CY_SCBIP_V1) */

            SPI_INTR_I2C_EC_MASK_REG = SPI_NO_INTR_SOURCES;
            SPI_INTR_RX_MASK_REG     = SPI_NO_INTR_SOURCES;
            SPI_INTR_TX_MASK_REG     = SPI_NO_INTR_SOURCES;

            SPI_INTR_SLAVE_MASK_REG  = ((SPI_I2C_SLAVE) ?
                            (SPI_GET_INTR_SLAVE_I2C_GENERAL(config->acceptGeneralAddr) |
                             SPI_I2C_INTR_SLAVE_MASK) : (SPI_CLEAR_REG));

            SPI_INTR_MASTER_MASK_REG = ((SPI_I2C_MASTER) ?
                                                     (SPI_I2C_INTR_MASTER_MASK) :
                                                     (SPI_CLEAR_REG));

            /* Configure global variables */
            SPI_state = SPI_I2C_FSM_IDLE;

            /* Internal slave variables */
            SPI_slStatus        = 0u;
            SPI_slRdBufIndex    = 0u;
            SPI_slWrBufIndex    = 0u;
            SPI_slOverFlowCount = 0u;

            /* Internal master variables */
            SPI_mstrStatus     = 0u;
            SPI_mstrRdBufIndex = 0u;
            SPI_mstrWrBufIndex = 0u;
        }
    }

#else

    /*******************************************************************************
    * Function Name: SPI_I2CInit
    ****************************************************************************//**
    *
    *  Configures the SCB for the I2C operation.
    *
    *******************************************************************************/
    void SPI_I2CInit(void)
    {
    #if(SPI_CY_SCBIP_V0)
        /* Adjust SDA filter settings. Ticket ID#150521 */
        SPI_SET_I2C_CFG_SDA_FILT_TRIM(SPI_EC_AM_I2C_CFG_SDA_FILT_TRIM);
    #endif /* (SPI_CY_SCBIP_V0) */

        /* Adjust AF and DF filter settings. Ticket ID#176179 */
        SPI_I2C_CFG_ANALOG_FITER_ENABLE_ADJ;

        /* Configure I2C interface */
        SPI_CTRL_REG     = SPI_I2C_DEFAULT_CTRL;
        SPI_I2C_CTRL_REG = SPI_I2C_DEFAULT_I2C_CTRL;

        /* Configure RX direction */
        SPI_RX_CTRL_REG      = SPI_I2C_DEFAULT_RX_CTRL;
        SPI_RX_FIFO_CTRL_REG = SPI_I2C_DEFAULT_RX_FIFO_CTRL;

        /* Set default address and mask */
        SPI_RX_MATCH_REG     = SPI_I2C_DEFAULT_RX_MATCH;

        /* Configure TX direction */
        SPI_TX_CTRL_REG      = SPI_I2C_DEFAULT_TX_CTRL;
        SPI_TX_FIFO_CTRL_REG = SPI_I2C_DEFAULT_TX_FIFO_CTRL;

        /* Configure interrupt with I2C handler but do not enable it */
        CyIntDisable    (SPI_ISR_NUMBER);
        CyIntSetPriority(SPI_ISR_NUMBER, SPI_ISR_PRIORITY);
    #if(!SPI_I2C_EXTERN_INTR_HANDLER)
        (void) CyIntSetVector(SPI_ISR_NUMBER, &SPI_I2C_ISR);
    #endif /* (SPI_I2C_EXTERN_INTR_HANDLER) */

        /* Configure interrupt sources */
    #if(!SPI_CY_SCBIP_V1)
        SPI_INTR_SPI_EC_MASK_REG = SPI_I2C_DEFAULT_INTR_SPI_EC_MASK;
    #endif /* (!SPI_CY_SCBIP_V1) */

        SPI_INTR_I2C_EC_MASK_REG = SPI_I2C_DEFAULT_INTR_I2C_EC_MASK;
        SPI_INTR_SLAVE_MASK_REG  = SPI_I2C_DEFAULT_INTR_SLAVE_MASK;
        SPI_INTR_MASTER_MASK_REG = SPI_I2C_DEFAULT_INTR_MASTER_MASK;
        SPI_INTR_RX_MASK_REG     = SPI_I2C_DEFAULT_INTR_RX_MASK;
        SPI_INTR_TX_MASK_REG     = SPI_I2C_DEFAULT_INTR_TX_MASK;

        /* Configure global variables */
        SPI_state = SPI_I2C_FSM_IDLE;

    #if(SPI_I2C_SLAVE)
        /* Internal slave variable */
        SPI_slStatus        = 0u;
        SPI_slRdBufIndex    = 0u;
        SPI_slWrBufIndex    = 0u;
        SPI_slOverFlowCount = 0u;
    #endif /* (SPI_I2C_SLAVE) */

    #if(SPI_I2C_MASTER)
    /* Internal master variable */
        SPI_mstrStatus     = 0u;
        SPI_mstrRdBufIndex = 0u;
        SPI_mstrWrBufIndex = 0u;
    #endif /* (SPI_I2C_MASTER) */
    }
#endif /* (SPI_SCB_MODE_UNCONFIG_CONST_CFG) */


/*******************************************************************************
* Function Name: SPI_I2CStop
****************************************************************************//**
*
*  Resets the I2C FSM into the default state.
*
*******************************************************************************/
void SPI_I2CStop(void)
{
    SPI_state = SPI_I2C_FSM_IDLE;
}


#if(SPI_I2C_WAKE_ENABLE_CONST)
    /*******************************************************************************
    * Function Name: SPI_I2CSaveConfig
    ****************************************************************************//**
    *
    *  Enables SPI_INTR_I2C_EC_WAKE_UP interrupt source. This interrupt
    *  triggers on address match and wakes up device.
    *
    *******************************************************************************/
    void SPI_I2CSaveConfig(void)
    {
    #if (!SPI_CY_SCBIP_V0)
        #if (SPI_I2C_MULTI_MASTER_SLAVE_CONST && SPI_I2C_WAKE_ENABLE_CONST)
            /* Enable externally clocked address match if it was not enabled before.
            * This applicable only for Multi-Master-Slave. Ticket ID#192742 */
            if (0u == (SPI_CTRL_REG & SPI_CTRL_EC_AM_MODE))
            {
                /* Enable external address match logic */
                SPI_Stop();
                SPI_CTRL_REG |= SPI_CTRL_EC_AM_MODE;
                SPI_Enable();
            }
        #endif /* (SPI_I2C_MULTI_MASTER_SLAVE_CONST) */

        #if (SPI_SCB_CLK_INTERNAL)
            /* Disable clock to internal address match logic. Ticket ID#187931 */
            SPI_SCBCLK_Stop();
        #endif /* (SPI_SCB_CLK_INTERNAL) */
    #endif /* (!SPI_CY_SCBIP_V0) */

        SPI_SetI2CExtClkInterruptMode(SPI_INTR_I2C_EC_WAKE_UP);
    }


    /*******************************************************************************
    * Function Name: SPI_I2CRestoreConfig
    ****************************************************************************//**
    *
    *  Disables SPI_INTR_I2C_EC_WAKE_UP interrupt source. This interrupt
    *  triggers on address match and wakes up device.
    *
    *******************************************************************************/
    void SPI_I2CRestoreConfig(void)
    {
        /* Disable wakeup interrupt on address match */
        SPI_SetI2CExtClkInterruptMode(SPI_NO_INTR_SOURCES);

    #if (!SPI_CY_SCBIP_V0)
        #if (SPI_SCB_CLK_INTERNAL)
            /* Enable clock to internal address match logic. Ticket ID#187931 */
            SPI_SCBCLK_Start();
        #endif /* (SPI_SCB_CLK_INTERNAL) */
    #endif /* (!SPI_CY_SCBIP_V0) */
    }
#endif /* (SPI_I2C_WAKE_ENABLE_CONST) */


/* [] END OF FILE */
