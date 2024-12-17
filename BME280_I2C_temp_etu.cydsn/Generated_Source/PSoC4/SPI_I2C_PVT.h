/***************************************************************************//**
* \file .h
* \version 3.20
*
* \brief
*  This private file provides constants and parameter values for the
*  SCB Component in I2C mode.
*  Please do not use this file or its content in your project.
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

#if !defined(CY_SCB_I2C_PVT_SPI_H)
#define CY_SCB_I2C_PVT_SPI_H

#include "SPI_I2C.h"


/***************************************
*     Private Global Vars
***************************************/

extern volatile uint8 SPI_state; /* Current state of I2C FSM */

#if(SPI_I2C_SLAVE_CONST)
    extern volatile uint8 SPI_slStatus;          /* Slave Status */

    /* Receive buffer variables */
    extern volatile uint8 * SPI_slWrBufPtr;      /* Pointer to Receive buffer  */
    extern volatile uint32  SPI_slWrBufSize;     /* Slave Receive buffer size  */
    extern volatile uint32  SPI_slWrBufIndex;    /* Slave Receive buffer Index */

    /* Transmit buffer variables */
    extern volatile uint8 * SPI_slRdBufPtr;      /* Pointer to Transmit buffer  */
    extern volatile uint32  SPI_slRdBufSize;     /* Slave Transmit buffer size  */
    extern volatile uint32  SPI_slRdBufIndex;    /* Slave Transmit buffer Index */
    extern volatile uint32  SPI_slRdBufIndexTmp; /* Slave Transmit buffer Index Tmp */
    extern volatile uint8   SPI_slOverFlowCount; /* Slave Transmit Overflow counter */
#endif /* (SPI_I2C_SLAVE_CONST) */

#if(SPI_I2C_MASTER_CONST)
    extern volatile uint16 SPI_mstrStatus;      /* Master Status byte  */
    extern volatile uint8  SPI_mstrControl;     /* Master Control byte */

    /* Receive buffer variables */
    extern volatile uint8 * SPI_mstrRdBufPtr;   /* Pointer to Master Read buffer */
    extern volatile uint32  SPI_mstrRdBufSize;  /* Master Read buffer size       */
    extern volatile uint32  SPI_mstrRdBufIndex; /* Master Read buffer Index      */

    /* Transmit buffer variables */
    extern volatile uint8 * SPI_mstrWrBufPtr;   /* Pointer to Master Write buffer */
    extern volatile uint32  SPI_mstrWrBufSize;  /* Master Write buffer size       */
    extern volatile uint32  SPI_mstrWrBufIndex; /* Master Write buffer Index      */
    extern volatile uint32  SPI_mstrWrBufIndexTmp; /* Master Write buffer Index Tmp */
#endif /* (SPI_I2C_MASTER_CONST) */

#if (SPI_I2C_CUSTOM_ADDRESS_HANDLER_CONST)
    extern uint32 (*SPI_customAddressHandler) (void);
#endif /* (SPI_I2C_CUSTOM_ADDRESS_HANDLER_CONST) */

/***************************************
*     Private Function Prototypes
***************************************/

#if(SPI_SCB_MODE_I2C_CONST_CFG)
    void SPI_I2CInit(void);
#endif /* (SPI_SCB_MODE_I2C_CONST_CFG) */

void SPI_I2CStop(void);
void SPI_I2CSaveConfig(void);
void SPI_I2CRestoreConfig(void);

#if(SPI_I2C_MASTER_CONST)
    void SPI_I2CReStartGeneration(void);
#endif /* (SPI_I2C_MASTER_CONST) */

#endif /* (CY_SCB_I2C_PVT_SPI_H) */


/* [] END OF FILE */
