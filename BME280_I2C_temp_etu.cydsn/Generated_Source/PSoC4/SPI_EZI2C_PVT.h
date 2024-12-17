/***************************************************************************//**
* \file .h
* \version 3.20
*
* \brief
*  This private file provides constants and parameter values for the
*  SCB Component in _EZI2C mode.
*  Do not use this file or its content in your project.
*
* Note:
*
********************************************************************************
* \copyright
* Copyright 2013-2016, Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#if !defined(CY_SCB_EZI2C_PVT_SPI_H)
#define CY_SCB_EZI2C_PVT_SPI_H

#include "SPI_EZI2C.h"


/***************************************
*      EZI2C Private Vars
***************************************/

extern volatile uint8 SPI_curStatus;
extern uint8 SPI_fsmState;

/* Variables intended to be used with Buffer 1: Primary slave address */
extern volatile uint8 * SPI_dataBuffer1;
extern uint16 SPI_bufSizeBuf1;
extern uint16 SPI_protectBuf1;
extern uint16 SPI_offsetBuf1;
extern uint16 SPI_indexBuf1;

#if(SPI_SECONDARY_ADDRESS_ENABLE_CONST)
    extern uint8 SPI_addrBuf1;
    extern uint8 SPI_addrBuf2;

    /* Variables intended to be used with Buffer 2: Primary slave address */
    extern volatile uint8 * SPI_dataBuffer2;
    extern uint16 SPI_bufSizeBuf2;
    extern uint16 SPI_protectBuf2;
    extern uint16 SPI_offsetBuf2;
    extern uint16 SPI_indexBuf2;
#endif /* (SPI_SECONDARY_ADDRESS_ENABLE_CONST) */


/***************************************
*     Private Function Prototypes
***************************************/

#if(SPI_SCB_MODE_EZI2C_CONST_CFG)
    void SPI_EzI2CInit(void);
#endif /* (SPI_SCB_MODE_EZI2C_CONST_CFG) */

void SPI_EzI2CStop(void);
#if(SPI_EZI2C_WAKE_ENABLE_CONST)
    void SPI_EzI2CSaveConfig(void);
    void SPI_EzI2CRestoreConfig(void);
#endif /* (SPI_EZI2C_WAKE_ENABLE_CONST) */

#endif /* (CY_SCB__EZI2C_PVT_SPI_H) */


/* [] END OF FILE */
