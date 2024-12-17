/***************************************************************************//**
* \file SPI_EZI2C_INT.c
* \version 3.20
*
* \brief
*  This file provides the source code to the Interrupt Service Routine for
*  the SCB Component in EZI2C mode.
*
* Note:
*
********************************************************************************
* \copyright
* Copyright 2013-2016, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include "SPI_PVT.h"
#include "SPI_EZI2C_PVT.h"
#include "cyapicallbacks.h"

#if(SPI_EZI2C_SCL_STRETCH_ENABLE_CONST)
    /*******************************************************************************
    * Function Name: SPI_EZI2C_STRETCH_ISR
    ****************************************************************************//**
    *
    *  Handles the Interrupt Service Routine for the SCB EZI2C mode.
    *  The clock stretching is used during operation.
    *
    *******************************************************************************/
    CY_ISR_PROTO(SPI_EZI2C_STRETCH_ISR)
    {
        static uint16 locBufSize;
        uint32 locIndex;
        uint32 locStatus;

        uint32 endTransfer;
        uint32 fifoIndex;
        uint32 locByte;

        uint32 locIntrCause;
        uint32 locIntrSlave;

    #ifdef SPI_EZI2C_STRETCH_ISR_ENTRY_CALLBACK
        SPI_EZI2C_STRETCH_ISR_EntryCallback();
    #endif /* SPI_EZI2C_STRETCH_ISR_ENTRY_CALLBACK */

    #if(SPI_SECONDARY_ADDRESS_ENABLE_CONST)
        /* Variable intended to be used with either buffer */
        static volatile uint8 * SPI_dataBuffer; /* Pointer to data buffer              */
        static uint16 SPI_bufSizeBuf;           /* Size of buffer in bytes             */
        static uint16 SPI_protectBuf;           /* Start index of write protected area */

        static uint8 activeAddress;
        uint32 ackResponse;

        ackResponse = SPI_EZI2C_ACK_RECEIVED_ADDRESS;
    #endif /* (SPI_SECONDARY_ADDRESS_ENABLE_CONST) */

    #if !defined (CY_REMOVE_SPI_CUSTOM_INTR_HANDLER)
        if(NULL != SPI_customIntrHandler)
        {
            SPI_customIntrHandler();
        }
    #else
        CY_SPI_CUSTOM_INTR_HANDLER();
    #endif /* !defined (CY_REMOVE_SPI_CUSTOM_INTR_HANDLER) */

        /* Make local copy of global variable */
        locIndex = SPI_EZI2C_GET_INDEX(activeAddress);

        /* Get interrupt sources */
        locIntrSlave = SPI_GetSlaveInterruptSource();
        locIntrCause = SPI_GetInterruptCause();

        /* INTR_SLAVE.I2C_ARB_LOST and INTR_SLAVE_I2C.BUS_ERROR */
        /* Handles errors on the bus. There are cases when both bits are set.
        * The error recovery is common: re-enable the scb IP. The content of the RX FIFO is lost.
        */
        if(0u != (locIntrSlave & (SPI_INTR_SLAVE_I2C_ARB_LOST |
                                  SPI_INTR_SLAVE_I2C_BUS_ERROR)))
        {
            SPI_CTRL_REG &= (uint32) ~SPI_CTRL_ENABLED; /* Disable SCB block */

        #if(SPI_CY_SCBIP_V0)
            if(0u != ((uint8) SPI_EZI2C_STATUS_BUSY & SPI_curStatus))
        #endif /* (SPI_CY_SCBIP_V0) */
            {
                SPI_curStatus &= (uint8) ~SPI_EZI2C_STATUS_BUSY;
                SPI_curStatus |= (uint8)  SPI_EZI2C_STATUS_ERR;

                /* INTR_TX_EMPTY is enabled in the address phase to receive data */
                if(0u == (SPI_GetTxInterruptMode() & SPI_INTR_TX_EMPTY))
                {
                    /* Write complete */
                    if(SPI_indexBuf1 != SPI_offsetBuf1)
                    {
                        SPI_curStatus |= (uint8) SPI_INTR_SLAVE_I2C_WRITE_STOP;
                    }
                }
                else
                {
                    /* Read complete */
                    SPI_curStatus |= (uint8) SPI_INTR_SLAVE_I2C_NACK;
                }
            }

            SPI_DISABLE_SLAVE_AUTO_DATA;

            /* Disable TX and RX interrupt sources */
            SPI_SetRxInterruptMode(SPI_NO_INTR_SOURCES);
            SPI_SetTxInterruptMode(SPI_NO_INTR_SOURCES);

        #if(SPI_CY_SCBIP_V0)
            /* Clear interrupt sources as they are not automatically cleared after SCB is disabled */
            SPI_ClearRxInterruptSource(SPI_INTR_RX_ALL);
            SPI_ClearSlaveInterruptSource(SPI_INTR_SLAVE_ALL);
        #endif /* (SPI_CY_SCBIP_V0) */

            SPI_fsmState = SPI_EZI2C_FSM_IDLE;

            SPI_CTRL_REG |= (uint32) SPI_CTRL_ENABLED;  /* Enable SCB block */
        }
        else
        {
            /* INTR_I2C_EC_WAKE_UP */
            /* Wakes up device from deep sleep */
            if(0u != (locIntrCause & SPI_INTR_CAUSE_I2C_EC))
            {
                /* Disables wakeup interrupt source but does not clear it. It is cleared in INTR_SLAVE_I2C_ADDR_MATCH */
                SPI_SetI2CExtClkInterruptMode(SPI_NO_INTR_SOURCES);
            }

            if(0u != (locIntrCause & (SPI_INTR_CAUSE_RX | SPI_INTR_CAUSE_SLAVE)))
            {
                /* INTR_RX.NOT_EMPTY */
                /* Receives data byte-by-byte. Does not use RX FIFO capabilities */
                if (0u != (SPI_GetRxInterruptSourceMasked() & SPI_INTR_RX_NOT_EMPTY))
                {
                #if(SPI_SECONDARY_ADDRESS_ENABLE_CONST)
                    /* If I2C_STOP service is delayed to I2C_ADDR_MATCH the address byte is in the RX FIFO and
                    * RX_NOT_EMPTY is enabled. The address byte has to stay into RX FIFO therefore
                    * RX.NOT_EMPTY service has to be skipped. The address byte has to be read by I2C_ADDR_MATCH.
                    */
                    if (0u == (locIntrCause & SPI_INTR_CAUSE_SLAVE))
                #endif /* (SPI_SECONDARY_ADDRESS_ENABLE_CONST) */
                    {
                        locByte = SPI_RX_FIFO_RD_REG;

                        switch(SPI_fsmState)
                        {
                        case SPI_EZI2C_FSM_BYTE_WRITE:
                            if(0u != locBufSize)
                            {
                                /* Store data byte and ACK */
                                SPI_I2C_SLAVE_GENERATE_ACK;

                                SPI_dataBuffer[locIndex] = (uint8) locByte;
                                locIndex++;
                                locBufSize--;
                            }
                            else
                            {
                                /* Discard data byte and NACK */
                                SPI_I2C_SLAVE_GENERATE_NACK;
                            }
                            break;

                    #if(SPI_SUB_ADDRESS_SIZE16_CONST)
                        case SPI_EZI2C_FSM_OFFSET_HI8:

                            SPI_I2C_SLAVE_GENERATE_ACK;

                            /* Store offset most significant byre */
                            locBufSize = (uint16) ((uint8) locByte);

                            SPI_fsmState = SPI_EZI2C_FSM_OFFSET_LO8;

                            break;
                    #endif /* (SPI_SUB_ADDRESS_SIZE16_CONST) */

                        case SPI_EZI2C_FSM_OFFSET_LO8:

                            if (SPI_SUB_ADDRESS_SIZE16)
                            {
                                /* Collect 2 bytes offset */
                                locByte = ((uint32) ((uint32) locBufSize << 8u)) | locByte;
                            }
                            

                            /* Check offset against buffer size */
                            if(locByte < (uint32) SPI_bufSizeBuf)
                            {
                                SPI_I2C_SLAVE_GENERATE_ACK;

                                /* Update local buffer index with new offset */
                                locIndex = locByte;

                                /* Get available buffer size to write */
                                locBufSize = (uint16) ((locByte < SPI_protectBuf) ?
                                                       (SPI_protectBuf - locByte) : (0u));

                            #if(SPI_CY_SCBIP_V0)

                                if(locBufSize < SPI_EZI2C_FIFO_SIZE)
                                {
                                    /* Set FSM state to receive byte by byte */
                                    SPI_fsmState = SPI_EZI2C_FSM_BYTE_WRITE;
                                }
                                /* Receive RX FIFO chunks */
                                else if(locBufSize == SPI_EZI2C_FIFO_SIZE)
                                {
                                    SPI_ENABLE_SLAVE_AUTO_DATA; /* NACK when RX FIFO is full */
                                    SPI_SetRxInterruptMode(SPI_NO_INTR_SOURCES);
                                }
                                else
                                {
                                    SPI_ENABLE_SLAVE_AUTO_DATA_ACK; /* Stretch when RX FIFO is full */
                                    SPI_SetRxInterruptMode(SPI_INTR_RX_FULL);
                                }

                            #else

                                if(SPI_SECONDARY_ADDRESS_ENABLE)
                                {
                                    /* Set FSM state to receive byte by byte.
                                    * The byte by byte receive is always chosen for two addresses. Ticket ID#175559.
                                    */
                                    SPI_fsmState = SPI_EZI2C_FSM_BYTE_WRITE;
                                }
                                else
                                {
                                    if (locBufSize < SPI_EZI2C_FIFO_SIZE)
                                    {
                                        /* Set FSM state to receive byte by byte */
                                        SPI_fsmState = SPI_EZI2C_FSM_BYTE_WRITE;
                                    }
                                    /* Receive RX FIFO chunks */
                                    else if (locBufSize == SPI_EZI2C_FIFO_SIZE)
                                    {
                                        SPI_ENABLE_SLAVE_AUTO_DATA; /* NACK when RX FIFO is full */
                                        SPI_SetRxInterruptMode(SPI_NO_INTR_SOURCES);
                                    }
                                    else
                                    {
                                        SPI_ENABLE_SLAVE_AUTO_DATA_ACK; /* Stretch when RX FIFO is full */
                                        SPI_SetRxInterruptMode(SPI_INTR_RX_FULL);
                                    }
                                }
                                

                            #endif /* (SPI_CY_SCBIP_V0) */

                                /* Store local offset into global variable */
                                SPI_EZI2C_SET_OFFSET(activeAddress, locIndex);
                            }
                            else
                            {
                                /* Discard offset byte and NACK */
                                SPI_I2C_SLAVE_GENERATE_NACK;
                            }
                            break;

                        default:
                            CYASSERT(0u != 0u); /* Should never get there */
                            break;
                        }

                        SPI_ClearRxInterruptSource(SPI_INTR_RX_NOT_EMPTY);
                    }
                }
                /* INTR_RX.FULL, INTR_SLAVE.I2C_STOP */
                /* Receive FIFO chunks: auto data ACK is enabled */
                else if (0u != (SPI_I2C_CTRL_REG & SPI_I2C_CTRL_S_READY_DATA_ACK))
                {
                    /* Slave interrupt (I2C_STOP or I2C_ADDR_MATCH) leads to completion of read.
                    * A completion event has a higher priority than the FIFO full.
                    * Read remaining data from RX FIFO.
                    */
                    if(0u != (locIntrCause & SPI_INTR_CAUSE_SLAVE))
                    {
                        /* Read remaining bytes from RX FIFO */
                        fifoIndex = SPI_GET_RX_FIFO_ENTRIES;

                        if(SPI_SECONDARY_ADDRESS_ENABLE)
                        {
                            /* Update with current address match */
                            if(SPI_CHECK_INTR_SLAVE_MASKED(SPI_INTR_SLAVE_I2C_ADDR_MATCH))
                            {
                                /* Update RX FIFO entries as address byte is there now */
                                fifoIndex = SPI_GET_RX_FIFO_ENTRIES;

                                /* If SR is valid, RX FIFO is full and address is in SHIFTER:
                                * read 8 entries and leave address in RX FIFO for further processing.
                                * If SR is invalid, address is already in RX FIFO: read (entries-1).
                                */
                                fifoIndex -= ((0u != SPI_GET_RX_FIFO_SR_VALID) ? (0u) : (1u));
                            }
                        }
                        

                        SPI_DISABLE_SLAVE_AUTO_DATA;
                        endTransfer = SPI_EZI2C_CONTINUE_TRANSFER;
                    }
                    else
                    /* INTR_RX_FULL */
                    /* Continue transfer or disable INTR_RX_FULL to catch completion event. */
                    {
                        /* Calculate buffer size available to write data into */
                        locBufSize -= (uint16) SPI_EZI2C_FIFO_SIZE;

                        if(locBufSize <= SPI_EZI2C_FIFO_SIZE)
                        {
                            /* Send NACK when RX FIFO overflow */
                            fifoIndex   = locBufSize;
                            endTransfer = SPI_EZI2C_COMPLETE_TRANSFER;
                        }
                        else
                        {
                            /* Continue  transaction */
                            fifoIndex   = SPI_EZI2C_FIFO_SIZE;
                            endTransfer = SPI_EZI2C_CONTINUE_TRANSFER;
                        }
                    }

                    for(; (0u != fifoIndex); fifoIndex--)
                    {
                        /* Store data in buffer */
                        SPI_dataBuffer[locIndex] = (uint8) SPI_RX_FIFO_RD_REG;
                        locIndex++;
                    }

                    /* Complete transfer sending NACK when RX FIFO overflow */
                    if(SPI_EZI2C_COMPLETE_TRANSFER == endTransfer)
                    {
                        SPI_ENABLE_SLAVE_AUTO_DATA_NACK;

                        /* Disable INTR_RX_FULL during last RX FIFO chunk reception */
                        SPI_SetRxInterruptMode(SPI_NO_INTR_SOURCES);
                    }

                    SPI_ClearRxInterruptSource(SPI_INTR_RX_FULL |
                                                            SPI_INTR_RX_NOT_EMPTY);
                }
                else
                {
                    /* Exit for slave interrupts which are not active for RX direction:
                    * INTR_SLAVE.I2C_ADDR_MATCH and INTR_SLAVE.I2C_STOP while byte-by-byte reception.
                    */
                }
            }

            if(0u != (locIntrCause & SPI_INTR_CAUSE_SLAVE))
            {
                /* INTR_SLAVE.I2C_STOP */
                /* Catch Stop condition: completion of write or read transfer */
            #if(!SPI_SECONDARY_ADDRESS_ENABLE_CONST)
                if(0u != (locIntrSlave & SPI_INTR_SLAVE_I2C_STOP))
            #else
                /* Prevent triggering when matched address was NACKed */
                if((0u != (locIntrSlave & SPI_INTR_SLAVE_I2C_STOP)) &&
                   (0u != ((uint8) SPI_EZI2C_STATUS_BUSY & SPI_curStatus)))
            #endif
                {
                    /* Disable TX and RX interrupt sources */
                    SPI_SetRxInterruptMode(SPI_NO_INTR_SOURCES);
                    SPI_SetTxInterruptMode(SPI_NO_INTR_SOURCES);

                    /* Set read completion mask */
                    locStatus = SPI_INTR_SLAVE_I2C_NACK;

                    /* Check if buffer content was modified: the address phase resets the locIndex */
                    if(locIndex != SPI_EZI2C_GET_OFFSET(activeAddress))
                    {
                        locStatus |= SPI_INTR_SLAVE_I2C_WRITE_STOP;
                    }

                    /* Complete read or write transaction */
                    locStatus &= locIntrSlave;
                    SPI_EZI2C_UPDATE_LOC_STATUS(activeAddress, locStatus);
                    locStatus |= (uint32)  SPI_curStatus;
                    locStatus &= (uint32) ~SPI_EZI2C_STATUS_BUSY;
                    SPI_curStatus = (uint8) locStatus;

                    SPI_fsmState = SPI_EZI2C_FSM_IDLE;

                    if(SPI_SECONDARY_ADDRESS_ENABLE)
                    {
                        /* Store local index into global variable, before address phase */
                        SPI_EZI2C_SET_INDEX(activeAddress, locIndex);
                    }
                    
                }

                /* INTR_SLAVE.I2C_ADDR_MATCH */
                /* The matched address is received: the slave starts its operation.
                * INTR_SLAVE_I2C_STOP updates the buffer index before the address phase for two addresses mode.
                * This is done to update buffer index correctly before the address phase changes it.
                */
                if(0u != (locIntrSlave & SPI_INTR_SLAVE_I2C_ADDR_MATCH))
                {
                    if(SPI_SECONDARY_ADDRESS_ENABLE)
                    {
                        /* Read address byte from RX FIFO */
                        locByte = SPI_GET_I2C_7BIT_ADDRESS(SPI_RX_FIFO_RD_REG);

                        SPI_ClearRxInterruptSource(SPI_INTR_RX_NOT_EMPTY);

                        /* Check received address against device addresses */
                        if(SPI_addrBuf1 == locByte)
                        {
                            /* Set buffer exposed to primary slave address */
                            SPI_dataBuffer = SPI_dataBuffer1;
                            SPI_bufSizeBuf = SPI_bufSizeBuf1;
                            SPI_protectBuf = SPI_protectBuf1;

                            activeAddress = SPI_EZI2C_ACTIVE_ADDRESS1;
                        }
                        else if(SPI_addrBuf2 == locByte)
                        {
                            /* Set buffer exposed to secondary slave address */
                            SPI_dataBuffer = SPI_dataBuffer2;
                            SPI_bufSizeBuf = SPI_bufSizeBuf2;
                            SPI_protectBuf = SPI_protectBuf2;

                            activeAddress = SPI_EZI2C_ACTIVE_ADDRESS2;
                        }
                        else
                        {
                            /* Address does not match */
                            ackResponse = SPI_EZI2C_NACK_RECEIVED_ADDRESS;
                        }
                    }
                    

                #if(SPI_SECONDARY_ADDRESS_ENABLE_CONST)
                    if(SPI_EZI2C_NACK_RECEIVED_ADDRESS == ackResponse)
                    {
                        /* Clear interrupt sources before NACK address */
                        SPI_ClearI2CExtClkInterruptSource(SPI_INTR_I2C_EC_WAKE_UP);
                        SPI_ClearSlaveInterruptSource(SPI_INTR_SLAVE_ALL);

                    #if(!SPI_CY_SCBIP_V0)
                        /* Disable INTR_I2C_STOP to not trigger after matched address is NACKed. Ticket ID#156094 */
                        SPI_DISABLE_INTR_SLAVE(SPI_INTR_SLAVE_I2C_STOP);
                    #endif /* (!SPI_CY_SCBIP_V0) */

                        /* NACK address byte: it does not match neither primary nor secondary */
                        SPI_I2C_SLAVE_GENERATE_NACK;
                    }
                    else
                #endif /* (SPI_SECONDARY_ADDRESS_ENABLE_CONST) */
                    {

                    #if(SPI_SCB_MODE_UNCONFIG_CONST_CFG)
                        if(!SPI_SECONDARY_ADDRESS_ENABLE)
                        {
                            /* Set buffer exposed to primary slave address */
                            SPI_dataBuffer = SPI_dataBuffer1;
                            SPI_bufSizeBuf = SPI_bufSizeBuf1;
                            SPI_protectBuf = SPI_protectBuf1;

                            activeAddress = SPI_EZI2C_ACTIVE_ADDRESS1;
                        }
                    #endif /* (SPI_SCB_MODE_UNCONFIG_CONST_CFG) */

                        /* Bus becomes busy after address is received */
                        SPI_curStatus |= (uint8) SPI_EZI2C_STATUS_BUSY;

                        /* Slave is read or written: set current offset */
                        locIndex = SPI_EZI2C_GET_OFFSET(activeAddress);

                        /* Check transaction direction */
                        if(SPI_CHECK_I2C_STATUS(SPI_I2C_STATUS_S_READ))
                        {
                            /* Calculate slave buffer size */
                            locBufSize = SPI_bufSizeBuf - (uint16) locIndex;

                            /* Clear TX FIFO to start fill from offset */
                            SPI_CLEAR_TX_FIFO;
                            SPI_SetTxInterruptMode(SPI_INTR_TX_EMPTY);
                        }
                        else
                        {
                            /* Master writes: enable reception interrupt. The FSM state was set in INTR_SLAVE_I2C_STOP */
                            SPI_SetRxInterruptMode(SPI_INTR_RX_NOT_EMPTY);
                        }

                        /* Clear interrupt sources before ACK address */
                        SPI_ClearI2CExtClkInterruptSource(SPI_INTR_I2C_EC_WAKE_UP);
                        SPI_ClearSlaveInterruptSource(SPI_INTR_SLAVE_ALL);

                    #if (!SPI_CY_SCBIP_V0)
                        /* Enable STOP to trigger after address match is ACKed. Ticket ID#156094 */
                        SPI_ENABLE_INTR_SLAVE(SPI_INTR_SLAVE_I2C_STOP);
                    #endif /* (!SPI_CY_SCBIP_V0) */

                        /* ACK the address byte */
                        SPI_I2C_SLAVE_GENERATE_ACK;
                    }
                }

                /* Clear slave interrupt sources */
                SPI_ClearSlaveInterruptSource(locIntrSlave);
            }

            /* INTR_TX.EMPTY */
            /* Transmits data to the master: loads data into the TX FIFO. The 0xFF sends out if the master reads
            * out the buffer. The address reception with a read flag clears the TX FIFO to be loaded with data.
            */
            if(0u != (SPI_GetInterruptCause() & SPI_INTR_CAUSE_TX))
            {
                /* Put data into TX FIFO until there is a room */
                do
                {
                    /* Check transmit buffer range: locBufSize calculates after address reception */
                    if(0u != locBufSize)
                    {
                        SPI_TX_FIFO_WR_REG = (uint32) SPI_dataBuffer[locIndex];
                        locIndex++;
                        locBufSize--;
                    }
                    else
                    {
                        SPI_TX_FIFO_WR_REG = SPI_EZI2C_OVFL_RETURN;
                    }
                }
                while(SPI_EZI2C_FIFO_SIZE != SPI_GET_TX_FIFO_ENTRIES);

                SPI_ClearTxInterruptSource(SPI_INTR_TX_EMPTY);
            }
        }

        /* Store local index copy into global variable */
        SPI_EZI2C_SET_INDEX(activeAddress, locIndex);

    #ifdef SPI_EZI2C_STRETCH_ISR_EXIT_CALLBACK
        SPI_EZI2C_STRETCH_ISR_ExitCallback();
    #endif /* SPI_EZI2C_STRETCH_ISR_EXIT_CALLBACK */

    }
#endif /* (SPI_EZI2C_SCL_STRETCH_ENABLE_CONST) */


#if(SPI_EZI2C_SCL_STRETCH_DISABLE_CONST)
    /*******************************************************************************
    * Function Name: SPI_EZI2C_NO_STRETCH_ISR
    ****************************************************************************//**
    *
    *  Handles the Interrupt Service Routine for the SCB EZI2C mode.
    *  Clock stretching is NOT used during operation.
    *
    *******************************************************************************/
    CY_ISR_PROTO(SPI_EZI2C_NO_STRETCH_ISR)
    {
    #if(SPI_SUB_ADDRESS_SIZE16_CONST)
        static uint8 locOffset;
    #endif /* (SPI_SUB_ADDRESS_SIZE16_CONST) */

        uint32 locByte;
        uint32 locStatus;
        uint32 locIntrSlave;
        uint32 locIntrCause;

    #ifdef SPI_EZI2C_NO_STRETCH_ISR_ENTRY_CALLBACK
        SPI_EZI2C_NO_STRETCH_ISR_EntryCallback();
    #endif /* SPI_EZI2C_NO_STRETCH_ISR_ENTRY_CALLBACK */

    #if !defined (CY_REMOVE_SPI_CUSTOM_INTR_HANDLER)
        /* Calls registered customer routine to manage interrupt sources */
        if(NULL != SPI_customIntrHandler)
        {
            SPI_customIntrHandler();
        }
    #else
        CY_SPI_CUSTOM_INTR_HANDLER();
    #endif /* !defined (CY_REMOVE_SPI_CUSTOM_INTR_HANDLER) */

        locByte = 0u;

        /* Get copy of triggered slave interrupt sources */
        locIntrSlave = SPI_GetSlaveInterruptSource();
        locIntrCause = SPI_GetInterruptCause();

        /* INTR_SLAVE.I2C_ARB_LOST and INTR_SLAVE.I2C_BUS_ERROR */
        /* Handles errors on the bus: There are cases when both bits are set.
        * The error recovery is common: re-enable the scb IP. The content of the RX FIFO is lost.
        */
        if(0u != (locIntrSlave & (SPI_INTR_SLAVE_I2C_ARB_LOST |
                                  SPI_INTR_SLAVE_I2C_BUS_ERROR)))
        {
            SPI_CTRL_REG &= (uint32) ~SPI_CTRL_ENABLED; /* Disable SCB block */

        #if (SPI_CY_SCBIP_V0)
            if(0u != (locIntrSlave & SPI_INTR_SLAVE_I2C_ADDR_MATCH))
        #endif /* (SPI_CY_SCBIP_V0) */
            {
                SPI_curStatus |= (uint8) SPI_EZI2C_STATUS_ERR;

                if(0u != (SPI_EZI2C_FSM_WRITE_MASK & SPI_fsmState))
                {
                    /* Write complete */
                    if(SPI_indexBuf1 != SPI_offsetBuf1)
                    {
                        SPI_curStatus |= (uint8) SPI_INTR_SLAVE_I2C_WRITE_STOP;
                    }
                }
                else
                {
                    /* Read complete */
                    SPI_curStatus |= (uint8) SPI_INTR_SLAVE_I2C_NACK;
                }
            }

            /* Clean-up interrupt sources */
            SPI_SetTxInterruptMode(SPI_NO_INTR_SOURCES);

        #if (SPI_CY_SCBIP_V0)
            /* Clear interrupt sources as they are not automatically cleared after SCB is disabled */
            SPI_ClearRxInterruptSource(SPI_INTR_RX_NOT_EMPTY);
            SPI_ClearSlaveInterruptSource(SPI_INTR_SLAVE_ALL);
        #endif /* (SPI_CY_SCBIP_V0) */

            SPI_fsmState = SPI_EZI2C_FSM_IDLE;

            SPI_CTRL_REG |= (uint32) SPI_CTRL_ENABLED;  /* Enable SCB block */
        }
        else
        {
            /* INTR_RX.NOT_EMPTY */
            /* The slave receives data from the master: accepts into the RX FIFO. At least one entry is available to be
            * read. The offset is written first and all the following bytes are data (expected to be put in the buffer).
            * The slave ACKs all bytes, but it discards them if they do not match the write criteria.
            * The slave NACKs the bytes in the case of an RX FIFO overflow.
            */
            if(0u != (locIntrCause & SPI_INTR_CAUSE_RX))
            {
                /* Read byte from the RX FIFO */
                locByte = SPI_RX_FIFO_RD_REG;

                switch(SPI_fsmState)
                {

                case SPI_EZI2C_FSM_BYTE_WRITE:
                    /* Check buffer index against protect area */
                    if(SPI_indexBuf1 < SPI_protectBuf1)
                    {
                        /* Stores received byte into buffer */
                        SPI_dataBuffer1[SPI_indexBuf1] = (uint8) locByte;
                        SPI_indexBuf1++;
                    }
                    else
                    {
                        /* Discard current byte and sets FSM state to discard following bytes */
                        SPI_fsmState = SPI_EZI2C_FSM_WAIT_STOP;
                    }

                    break;

            #if(SPI_SUB_ADDRESS_SIZE16_CONST)
                case SPI_EZI2C_FSM_OFFSET_HI8:

                    /* Store high byte of offset */
                    locOffset = (uint8) locByte;

                    SPI_fsmState  = SPI_EZI2C_FSM_OFFSET_LO8;

                    break;
            #endif /* (SPI_SUB_ADDRESS_SIZE16_CONST) */

                case SPI_EZI2C_FSM_OFFSET_LO8:

                    if(SPI_SUB_ADDRESS_SIZE16)
                    {
                        /* Append offset with high byte */
                        locByte = ((uint32) ((uint32) locOffset << 8u)) | locByte;
                    }
                    

                    /* Check if offset within buffer range */
                    if(locByte < (uint32) SPI_bufSizeBuf1)
                    {
                        /* Store and sets received offset */
                        SPI_offsetBuf1 = (uint16) locByte;
                        SPI_indexBuf1  = (uint16) locByte;

                        /* Move FSM to data receive state */
                        SPI_fsmState = SPI_EZI2C_FSM_BYTE_WRITE;
                    }
                    else
                    {
                        /* Reset index due to TX FIFO fill */
                        SPI_indexBuf1 = (uint16) SPI_offsetBuf1;

                        /* Discard current byte and sets FSM state to default to discard following bytes */
                        SPI_fsmState = SPI_EZI2C_FSM_WAIT_STOP;
                    }

                    break;

                case SPI_EZI2C_FSM_WAIT_STOP:
                    /* Clear RX FIFO to discard all received data */
                    SPI_CLEAR_RX_FIFO;

                    break;

                default:
                    CYASSERT(0u != 0u); /* Should never get there */
                    break;
                }

                SPI_ClearRxInterruptSource(SPI_INTR_RX_NOT_EMPTY);
            }


            /* INTR_SLAVE.I2C_START */
            /* Catches start of transfer to trigger TX FIFO update event */
            if(0u != (locIntrSlave & SPI_INTR_SLAVE_I2C_START))
            {
            #if(!SPI_CY_SCBIP_V0)
                if(SPI_EZI2C_EC_AM_ENABLE)
                {
                    /* Manage INTR_I2C_EC.WAKE_UP as slave busy status */
                    SPI_ClearI2CExtClkInterruptSource(SPI_INTR_I2C_EC_WAKE_UP);
                }
                else
                {
                    /* Manage INTR_SLAVE.I2C_ADDR_MATCH as slave busy status */
                    SPI_ClearSlaveInterruptSource(SPI_INTR_SLAVE_I2C_ADDR_MATCH);
                }
                
            #else
                /* Manage INTR_SLAVE.I2C_ADDR_MATCH as slave busy status */
                SPI_ClearSlaveInterruptSource(SPI_INTR_SLAVE_I2C_ADDR_MATCH);
            #endif /* (SPI_CY_SCBIP_V0) */

                /* Clear TX FIFO and put a byte */
                SPI_FAST_CLEAR_TX_FIFO;
                SPI_TX_FIFO_WR_REG = (uint32) SPI_dataBuffer1[SPI_offsetBuf1];

                /* Store buffer index to be handled by INTR_SLAVE.I2C_STOP */
                locByte = (uint32) SPI_indexBuf1;

                /* Update index: one byte is already in the TX FIFO */
                SPI_indexBuf1 = (uint16) SPI_offsetBuf1 + 1u;

                /* Enable INTR_TX.NOT_FULL to load TX FIFO */
                SPI_SetTxInterruptMode(SPI_INTR_TX_TRIGGER);

                /* Clear locIntrSlave after INTR.TX_TRIGGER is enabled */
                SPI_ClearSlaveInterruptSource(locIntrSlave);

                locIntrCause |= SPI_INTR_CAUSE_TX;
            }


            /* INTR_TX.TRIGGER */
            /* Transmits data to the master: loads data into the TX FIFO. The TX FIFO is loaded with data
            *  until used entries are less than SPI_TX_LOAD_SIZE. If index reaches end of the
            *  buffer the 0xFF is sent to the end of transfer.
            */
            if(0u != (locIntrCause & SPI_INTR_CAUSE_TX))
            {
                /* Put data into TX FIFO until there is room */
                do
                {
                    /* Check transmit buffer range */
                    if(SPI_indexBuf1 < SPI_bufSizeBuf1)
                    {
                        SPI_TX_FIFO_WR_REG = (uint32) SPI_dataBuffer1[SPI_indexBuf1];
                        SPI_indexBuf1++;
                    }
                    else
                    {
                        SPI_TX_FIFO_WR_REG = SPI_EZI2C_OVFL_RETURN;
                    }

                }
                while(SPI_TX_LOAD_SIZE != SPI_GET_TX_FIFO_ENTRIES);

                SPI_ClearTxInterruptSource(SPI_INTR_TX_TRIGGER);
            }


            /* INTR_SLAVE.I2C_STOP */
            /* Catch completion of write or read transfer. */
            if(0u != (locIntrSlave & SPI_INTR_SLAVE_I2C_STOP))
            {
                if(0u == (locIntrSlave & SPI_INTR_SLAVE_I2C_START))
                {
                #if(!SPI_CY_SCBIP_V0)
                    if(SPI_EZI2C_EC_AM_ENABLE)
                    {
                        /* Manage INTR_I2C_EC.WAKE_UP as slave busy status */
                        SPI_ClearI2CExtClkInterruptSource(SPI_INTR_I2C_EC_WAKE_UP);
                    }
                    
                #endif /* (!SPI_CY_SCBIP_V0) */

                    /* Manage INTR_SLAVE.I2C_ADDR_MATCH as slave busy status */
                    SPI_ClearSlaveInterruptSource(locIntrSlave);

                    /* Read current buffer index */
                    locByte = (uint32) SPI_indexBuf1;
                }

                /* Set read completion mask */
                locStatus = SPI_INTR_SLAVE_I2C_NACK;

                if((locByte != SPI_offsetBuf1) &&
                   (0u != (SPI_EZI2C_FSM_WRITE_MASK & SPI_fsmState)))
                {
                    /* Set write completion mask */
                    locStatus |= SPI_INTR_SLAVE_I2C_WRITE_STOP;
                }

                /* Set completion flags in the status variable */
                SPI_curStatus |= (uint8) (locStatus & locIntrSlave);

                SPI_fsmState = SPI_EZI2C_FSM_IDLE;
            }


        #if(!SPI_CY_SCBIP_V0)
            if(SPI_EZI2C_EC_AM_ENABLE)
            {
                /* INTR_I2C_EC.WAKE_UP */
                /* Wake up device from deep sleep on address match event. The matched address is NACKed */
                if(0u != (locIntrCause & SPI_INTR_CAUSE_I2C_EC))
                {
                    SPI_I2C_SLAVE_GENERATE_NACK; /* NACK in active mode */
                    SPI_ClearI2CExtClkInterruptSource(SPI_INTR_I2C_EC_WAKE_UP);
                }
            }
            
        #endif /* (!SPI_CY_SCBIP_V0) */
        }

    #ifdef SPI_EZI2C_NO_STRETCH_ISR_EXIT_CALLBACK
        SPI_EZI2C_NO_STRETCH_ISR_ExitCallback();
    #endif /* SPI_EZI2C_NO_STRETCH_ISR_EXIT_CALLBACK */

    }
#endif /* (SPI_EZI2C_SCL_STRETCH_DISABLE_CONST) */


/* [] END OF FILE */
