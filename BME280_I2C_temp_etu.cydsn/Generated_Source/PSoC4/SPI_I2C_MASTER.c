/***************************************************************************//**
* \file SPI_I2C_MASTER.c
* \version 3.20
*
* \brief
*  This file provides the source code to the API for the SCB Component in
*  I2C Master mode.
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

#if(SPI_I2C_MASTER_CONST)

/***************************************
*      I2C Master Private Vars
***************************************/

/* Master variables */
volatile uint16 SPI_mstrStatus;      /* Master Status byte  */
volatile uint8  SPI_mstrControl;     /* Master Control byte */

/* Receive buffer variables */
volatile uint8 * SPI_mstrRdBufPtr;   /* Pointer to Master Read buffer */
volatile uint32  SPI_mstrRdBufSize;  /* Master Read buffer size       */
volatile uint32  SPI_mstrRdBufIndex; /* Master Read buffer Index      */

/* Transmit buffer variables */
volatile uint8 * SPI_mstrWrBufPtr;   /* Pointer to Master Write buffer */
volatile uint32  SPI_mstrWrBufSize;  /* Master Write buffer size       */
volatile uint32  SPI_mstrWrBufIndex; /* Master Write buffer Index      */
volatile uint32  SPI_mstrWrBufIndexTmp; /* Master Write buffer Index Tmp */

#if (!SPI_CY_SCBIP_V0 && \
    SPI_I2C_MULTI_MASTER_SLAVE_CONST && SPI_I2C_WAKE_ENABLE_CONST)
    static void SPI_I2CMasterDisableEcAm(void);
#endif /* (!SPI_CY_SCBIP_V0) */


/*******************************************************************************
* Function Name: SPI_I2CMasterWriteBuf
****************************************************************************//**
*
*  Automatically writes an entire buffer of data to a slave device.
*  Once the data transfer is initiated by this function, further data transfer
*  is handled by the included ISR.
*  Enables the I2C interrupt and clears SPI_I2C_MSTAT_WR_CMPLT 
*  status.
*
*  \param slaveAddr: 7-bit slave address.
*  \param xferData: Pointer to buffer of data to be sent.
*  \param cnt: Size of buffer to send.
*  \param mode: Transfer mode defines:
*  (1) Whether a start or restart condition is generated at the beginning 
*  of the transfer, and
*  (2) Whether the transfer is completed or halted before the stop 
*  condition is generated on the bus.Transfer mode, mode constants 
*  may be ORed together.
*  - SPI_I2C_MODE_COMPLETE_XFER - Perform complete transfer 
*    from Start to Stop.
*  - SPI_I2C_MODE_REPEAT_START - Send Repeat Start instead 
*    of Start. A Stop is generated after transfer is completed unless 
*    NO_STOP is specified.
*  - SPI_I2C_MODE_NO_STOP Execute transfer without a Stop.
*    The following transfer expected to perform ReStart.
*
* \return
*  Error status.
*  - SPI_I2C_MSTR_NO_ERROR - Function complete without error. 
*    The master started the transfer.
*  - SPI_I2C_MSTR_BUS_BUSY - Bus is busy. Nothing was sent on
*    the bus. The attempt has to be retried.
*  - SPI_I2C_MSTR_NOT_READY - Master is not ready for to start
*    transfer. A master still has not completed previous transaction or a 
*    slave operation is in progress (in multi-master-slave configuration).
*    Nothing was sent on the bus. The attempt has to be retried.
*
* \globalvars
*  SPI_mstrStatus  - used to store current status of I2C Master.
*  SPI_state       - used to store current state of software FSM.
*  SPI_mstrControl - used to control master end of transaction with
*  or without the Stop generation.
*  SPI_mstrWrBufPtr - used to store pointer to master write buffer.
*  SPI_mstrWrBufIndex - used to current index within master write
*  buffer.
*  SPI_mstrWrBufSize - used to store master write buffer size.
*
*******************************************************************************/
uint32 SPI_I2CMasterWriteBuf(uint32 slaveAddress, uint8 * wrData, uint32 cnt, uint32 mode)
{
    uint32 errStatus;

    errStatus = SPI_I2C_MSTR_NOT_READY;

    if(NULL != wrData)  /* Check buffer pointer */
    {
        /* Check FSM state and bus before generating Start/ReStart condition */
        if(SPI_CHECK_I2C_FSM_IDLE)
        {
            SPI_DisableInt();  /* Lock from interruption */

            /* Check bus state */
            errStatus = SPI_CHECK_I2C_STATUS(SPI_I2C_STATUS_BUS_BUSY) ?
                            SPI_I2C_MSTR_BUS_BUSY : SPI_I2C_MSTR_NO_ERROR;
        }
        else if(SPI_CHECK_I2C_FSM_HALT)
        {
            SPI_mstrStatus &= (uint16) ~SPI_I2C_MSTAT_XFER_HALT;
                              errStatus  = SPI_I2C_MSTR_NO_ERROR;
        }
        else
        {
            /* Unexpected FSM state: exit */
        }
    }

    /* Check if master is ready to start  */
    if(SPI_I2C_MSTR_NO_ERROR == errStatus) /* No error proceed */
    {
    #if (!SPI_CY_SCBIP_V0 && \
        SPI_I2C_MULTI_MASTER_SLAVE_CONST && SPI_I2C_WAKE_ENABLE_CONST)
            SPI_I2CMasterDisableEcAm();
    #endif /* (!SPI_CY_SCBIP_V0) */

        /* Set up write transaction */
        SPI_state = SPI_I2C_FSM_MSTR_WR_ADDR;
        SPI_mstrWrBufIndexTmp = 0u;
        SPI_mstrWrBufIndex    = 0u;
        SPI_mstrWrBufSize     = cnt;
        SPI_mstrWrBufPtr      = (volatile uint8 *) wrData;
        SPI_mstrControl       = (uint8) mode;

        slaveAddress = SPI_GET_I2C_8BIT_ADDRESS(slaveAddress);

        SPI_mstrStatus &= (uint16) ~SPI_I2C_MSTAT_WR_CMPLT;

        SPI_ClearMasterInterruptSource(SPI_INTR_MASTER_ALL);
        SPI_ClearTxInterruptSource(SPI_INTR_TX_UNDERFLOW);

        /* The TX and RX FIFO have to be EMPTY */

        /* Enable interrupt source to catch when address is sent */
        SPI_SetTxInterruptMode(SPI_INTR_TX_UNDERFLOW);

        /* Generate Start or ReStart */
        if(SPI_CHECK_I2C_MODE_RESTART(mode))
        {
            SPI_I2C_MASTER_GENERATE_RESTART;
            SPI_TX_FIFO_WR_REG = slaveAddress;
        }
        else
        {
            SPI_TX_FIFO_WR_REG = slaveAddress;
            SPI_I2C_MASTER_GENERATE_START;
        }
    }

    SPI_EnableInt();   /* Release lock */

    return(errStatus);
}


/*******************************************************************************
* Function Name: SPI_I2CMasterReadBuf
****************************************************************************//**
*
*  Automatically reads an entire buffer of data from a slave device.
*  Once the data transfer is initiated by this function, further data transfer
*  is handled by the included ISR.
*  Enables the I2C interrupt and clears SPI_I2C_MSTAT_RD_CMPLT 
*  status.
*
*  \param slaveAddr: 7-bit slave address.
*  \param xferData: Pointer to buffer of data to be sent.
*  \param cnt: Size of buffer to send.
*  \param mode: Transfer mode defines:
*  (1) Whether a start or restart condition is generated at the beginning 
*  of the transfer, and
*  (2) Whether the transfer is completed or halted before the stop 
*  condition is generated on the bus.Transfer mode, mode constants may 
*  be ORed together. See SPI_I2CMasterWriteBuf() 
*  function for constants.
*
* \return
*  Error status.See SPI_I2CMasterWriteBuf() 
*  function for constants.
*
* \globalvars
*  SPI_mstrStatus  - used to store current status of I2C Master.
*  SPI_state       - used to store current state of software FSM.
*  SPI_mstrControl - used to control master end of transaction with
*  or without the Stop generation.
*  SPI_mstrRdBufPtr - used to store pointer to master read buffer.
*  SPI_mstrRdBufIndex - used to current index within master read
*  buffer.
*  SPI_mstrRdBufSize - used to store master read buffer size.
*
*******************************************************************************/
uint32 SPI_I2CMasterReadBuf(uint32 slaveAddress, uint8 * rdData, uint32 cnt, uint32 mode)
{
    uint32 errStatus;

    errStatus = SPI_I2C_MSTR_NOT_READY;

    if(NULL != rdData)
    {
        /* Check FSM state and bus before generating Start/ReStart condition */
        if(SPI_CHECK_I2C_FSM_IDLE)
        {
            SPI_DisableInt();  /* Lock from interruption */

            /* Check bus state */
            errStatus = SPI_CHECK_I2C_STATUS(SPI_I2C_STATUS_BUS_BUSY) ?
                            SPI_I2C_MSTR_BUS_BUSY : SPI_I2C_MSTR_NO_ERROR;
        }
        else if(SPI_CHECK_I2C_FSM_HALT)
        {
            SPI_mstrStatus &= (uint16) ~SPI_I2C_MSTAT_XFER_HALT;
                              errStatus  =  SPI_I2C_MSTR_NO_ERROR;
        }
        else
        {
            /* Unexpected FSM state: exit */
        }
    }

    /* Check master ready to proceed */
    if(SPI_I2C_MSTR_NO_ERROR == errStatus) /* No error proceed */
    {
        #if (!SPI_CY_SCBIP_V0 && \
        SPI_I2C_MULTI_MASTER_SLAVE_CONST && SPI_I2C_WAKE_ENABLE_CONST)
            SPI_I2CMasterDisableEcAm();
        #endif /* (!SPI_CY_SCBIP_V0) */

        /* Set up read transaction */
        SPI_state = SPI_I2C_FSM_MSTR_RD_ADDR;
        SPI_mstrRdBufIndex = 0u;
        SPI_mstrRdBufSize  = cnt;
        SPI_mstrRdBufPtr   = (volatile uint8 *) rdData;
        SPI_mstrControl    = (uint8) mode;

        slaveAddress = (SPI_GET_I2C_8BIT_ADDRESS(slaveAddress) | SPI_I2C_READ_FLAG);

        SPI_mstrStatus &= (uint16) ~SPI_I2C_MSTAT_RD_CMPLT;

        SPI_ClearMasterInterruptSource(SPI_INTR_MASTER_ALL);

        /* TX and RX FIFO have to be EMPTY */

        /* Prepare reading */
        if(SPI_mstrRdBufSize < SPI_I2C_FIFO_SIZE)
        {
            /* Reading byte-by-byte */
            SPI_SetRxInterruptMode(SPI_INTR_RX_NOT_EMPTY);
        }
        else
        {
            /* Receive RX FIFO chunks */
            SPI_ENABLE_MASTER_AUTO_DATA_ACK;
            SPI_SetRxInterruptMode(SPI_INTR_RX_FULL);
        }

        /* Generate Start or ReStart */
        if(SPI_CHECK_I2C_MODE_RESTART(mode))
        {
            SPI_I2C_MASTER_GENERATE_RESTART;
            SPI_TX_FIFO_WR_REG = (slaveAddress);
        }
        else
        {
            SPI_TX_FIFO_WR_REG = (slaveAddress);
            SPI_I2C_MASTER_GENERATE_START;
        }
    }

    SPI_EnableInt();   /* Release lock */

    return(errStatus);
}


/*******************************************************************************
* Function Name: SPI_I2CMasterSendStart
****************************************************************************//**
*
*  Generates Start condition and sends slave address with read/write bit.
*  Disables the I2C interrupt.
*  This function is blocking and does not return until start condition and
*  address byte are sent and ACK/NACK response is received or errors occurred.
*
* \param slaveAddress: Right justified 7-bit Slave address (valid range 
*  8 to 120).
* \param bitRnW: Direction of the following transfer. 
*  It is defined by read/write bit within address byte.
*  - SPI_I2C_WRITE_XFER_MODE - Set write direction for the 
*    following transfer.
*  - SPI_I2C_READ_XFER_MODE - Set read direction for the 
*    following transfer.
*
* \return
*  Error status.
*   - SPI_I2C_MSTR_NO_ERROR - Function complete without error.
*   - SPI_I2C_MSTR_BUS_BUSY - Bus is busy. 
*     Nothing was sent on the bus. The attempt has to be retried.
*   - SPI_I2C_MSTR_NOT_READY - Master is not ready for to 
*     start transfer.
*     A master still has not completed previous transaction or a slave 
*     operation is in progress (in multi-master-slave configuration).
*     Nothing was sent on the bus. The attempt has to be retried.
*   - SPI_I2C_MSTR_ERR_LB_NAK - Error condition: Last byte was 
*     NAKed.
*   - SPI_I2C_MSTR_ERR_ARB_LOST - Error condition: Master lost 
*     arbitration.
*   - SPI_I2C_MSTR_ERR_BUS_ERR - Error condition: Master 
*     encountered a bus error. Bus error is misplaced start or stop detection.
*   - SPI_I2C_MSTR_ERR_ABORT_START - Error condition: The start 
*     condition generation was aborted due to beginning of Slave operation. 
*     This error condition is only applicable for Multi-Master-Slave mode.
*
* \globalvars
*  SPI_state - used to store current state of software FSM.
*
*******************************************************************************/
uint32 SPI_I2CMasterSendStart(uint32 slaveAddress, uint32 bitRnW)
{
    uint32  resetIp;
    uint32 errStatus;

    resetIp   = 0u;
    errStatus = SPI_I2C_MSTR_NOT_READY;

    /* Check FSM state before generating Start condition */
    if(SPI_CHECK_I2C_FSM_IDLE)
    {
        /* If bus is free, generate Start condition */
        if(SPI_CHECK_I2C_STATUS(SPI_I2C_STATUS_BUS_BUSY))
        {
            errStatus = SPI_I2C_MSTR_BUS_BUSY;
        }
        else
        {
            SPI_DisableInt();  /* Lock from interruption */

        #if (!SPI_CY_SCBIP_V0 && \
            SPI_I2C_MULTI_MASTER_SLAVE_CONST && SPI_I2C_WAKE_ENABLE_CONST)
            SPI_I2CMasterDisableEcAm();
        #endif /* (!SPI_CY_SCBIP_V0) */

            slaveAddress = SPI_GET_I2C_8BIT_ADDRESS(slaveAddress);

            if(0u == bitRnW) /* Write direction */
            {
                SPI_state = SPI_I2C_FSM_MSTR_WR_DATA;
            }
            else /* Read direction */
            {
                SPI_state = SPI_I2C_FSM_MSTR_RD_DATA;
                         slaveAddress |= SPI_I2C_READ_FLAG;
            }

            /* TX and RX FIFO have to be EMPTY */

            SPI_TX_FIFO_WR_REG = slaveAddress; /* Put address in TX FIFO */
            SPI_ClearMasterInterruptSource(SPI_INTR_MASTER_ALL);

            SPI_I2C_MASTER_GENERATE_START;


            while(!SPI_CHECK_INTR_MASTER(SPI_INTR_MASTER_I2C_ACK      |
                                                      SPI_INTR_MASTER_I2C_NACK     |
                                                      SPI_INTR_MASTER_I2C_ARB_LOST |
                                                      SPI_INTR_MASTER_I2C_BUS_ERROR))
            {
                /*
                * Write: wait until address has been transferred
                * Read : wait until address has been transferred, data byte is going to RX FIFO as well.
                */
            }

            /* Check the results of the address phase */
            if(SPI_CHECK_INTR_MASTER(SPI_INTR_MASTER_I2C_ACK))
            {
                errStatus = SPI_I2C_MSTR_NO_ERROR;
            }
            else if(SPI_CHECK_INTR_MASTER(SPI_INTR_MASTER_I2C_NACK))
            {
                errStatus = SPI_I2C_MSTR_ERR_LB_NAK;
            }
            else if(SPI_CHECK_INTR_MASTER(SPI_INTR_MASTER_I2C_ARB_LOST))
            {
                SPI_state = SPI_I2C_FSM_IDLE;
                             errStatus = SPI_I2C_MSTR_ERR_ARB_LOST;
                             resetIp   = SPI_I2C_RESET_ERROR;
            }
            else /* SPI_INTR_MASTER_I2C_BUS_ERROR set is else condition */
            {
                SPI_state = SPI_I2C_FSM_IDLE;
                             errStatus = SPI_I2C_MSTR_ERR_BUS_ERR;
                             resetIp   = SPI_I2C_RESET_ERROR;
            }

            SPI_ClearMasterInterruptSource(SPI_INTR_MASTER_I2C_ACK      |
                                                        SPI_INTR_MASTER_I2C_NACK     |
                                                        SPI_INTR_MASTER_I2C_ARB_LOST |
                                                        SPI_INTR_MASTER_I2C_BUS_ERROR);

            /* Reset block in case of: LOST_ARB or BUS_ERR */
            if(0u != resetIp)
            {
                SPI_SCB_SW_RESET;
            }
        }
    }

    return(errStatus);
}


/*******************************************************************************
* Function Name: SPI_I2CMasterSendRestart
****************************************************************************//**
*
*  Generates Restart condition and sends slave address with read/write bit.
*  This function is blocking and does not return until start condition and
*  address are sent and ACK/NACK response is received or errors occurred.
*
* \param slaveAddress: Right justified 7-bit Slave address (valid range 
*   8 to 120).
* \param bitRnW: Direction of the following transfer. It is defined by 
*  read/write bit within address byte. 
*  See SPI_I2CMasterSendStart() function for constants.
*
* \return
*  Error status. 
*  See SPI_I2CMasterSendStart() function for constants.
*
* \sideeffect
*  A valid Start or ReStart condition must be generated before calling 
*  this function. This function does nothing if Start or ReStart conditions 
*  failed before this function was called.
*  For read transaction, at least one byte has to be read before ReStart 
*  generation.
*
* \globalvars
*  SPI_state - used to store current state of software FSM.
*
*******************************************************************************/
uint32 SPI_I2CMasterSendRestart(uint32 slaveAddress, uint32 bitRnW)
{
    uint32 resetIp;
    uint32 errStatus;

    resetIp   = 0u;
    errStatus = SPI_I2C_MSTR_NOT_READY;

    /* Check FSM state before generating ReStart condition */
    if(SPI_CHECK_I2C_MASTER_ACTIVE)
    {
        slaveAddress = SPI_GET_I2C_8BIT_ADDRESS(slaveAddress);

        if(0u == bitRnW) /* Write direction */
        {
            SPI_state = SPI_I2C_FSM_MSTR_WR_DATA;
        }
        else  /* Read direction */
        {
            SPI_state  = SPI_I2C_FSM_MSTR_RD_DATA;
                      slaveAddress |= SPI_I2C_READ_FLAG;
        }

        /* TX and RX FIFO have to be EMPTY */

        /* Clean-up interrupt status */
        SPI_ClearMasterInterruptSource(SPI_INTR_MASTER_ALL);

        /* A proper ReStart sequence is: generate ReStart, then put an address byte in the TX FIFO.
        * Otherwise the master treats the address in the TX FIFO as a data byte if a previous transfer is write.
        * The write transfer continues instead of ReStart.
        */
        SPI_I2C_MASTER_GENERATE_RESTART;

        while(SPI_CHECK_I2C_MASTER_CMD(SPI_I2C_MASTER_CMD_M_START))
        {
            /* Wait until ReStart has been generated */
        }

        /* Put address into TX FIFO */
        SPI_TX_FIFO_WR_REG = slaveAddress;

        /* Wait for address to be transferred */
        while(!SPI_CHECK_INTR_MASTER(SPI_INTR_MASTER_I2C_ACK      |
                                                  SPI_INTR_MASTER_I2C_NACK     |
                                                  SPI_INTR_MASTER_I2C_ARB_LOST |
                                                  SPI_INTR_MASTER_I2C_BUS_ERROR))
        {
            /* Wait until address has been transferred */
        }

        /* Check results of address phase */
        if(SPI_CHECK_INTR_MASTER(SPI_INTR_MASTER_I2C_ACK))
        {
            errStatus = SPI_I2C_MSTR_NO_ERROR;
        }
        else if(SPI_CHECK_INTR_MASTER(SPI_INTR_MASTER_I2C_NACK))
        {
             errStatus = SPI_I2C_MSTR_ERR_LB_NAK;
        }
        else if(SPI_CHECK_INTR_MASTER(SPI_INTR_MASTER_I2C_ARB_LOST))
        {
            SPI_state = SPI_I2C_FSM_IDLE;
                         errStatus = SPI_I2C_MSTR_ERR_ARB_LOST;
                         resetIp   = SPI_I2C_RESET_ERROR;
        }
        else /* SPI_INTR_MASTER_I2C_BUS_ERROR set is else condition */
        {
            SPI_state = SPI_I2C_FSM_IDLE;
                         errStatus = SPI_I2C_MSTR_ERR_BUS_ERR;
                         resetIp   = SPI_I2C_RESET_ERROR;
        }

        SPI_ClearMasterInterruptSource(SPI_INTR_MASTER_I2C_ACK      |
                                                    SPI_INTR_MASTER_I2C_NACK     |
                                                    SPI_INTR_MASTER_I2C_ARB_LOST |
                                                    SPI_INTR_MASTER_I2C_BUS_ERROR);

        /* Reset block in case of: LOST_ARB or BUS_ERR */
        if(0u != resetIp)
        {
            SPI_SCB_SW_RESET;
        }
    }

    return(errStatus);
}


/*******************************************************************************
* Function Name: SPI_I2CMasterSendStop
****************************************************************************//**
*
*  Generates Stop condition on the bus. 
*  The NAK is generated before Stop in case of a read transaction.
*  At least one byte has to be read if a Start or ReStart condition with read 
*  direction was generated before.
*  This function is blocking and does not return until a Stop condition is 
*  generated or error occurred.
*
* \return
*  Error status.
*  See SPI_I2CMasterSendStart() function for constants.
*
* \sideeffect
*  A valid Start or ReStart condition must be generated before calling 
*  this function. This function does nothing if Start or ReStart conditions 
*  failed before this function was called.
*  For read transaction, at least one byte has to be read before ReStart 
*  generation.
*
* \globalvars
*  SPI_state - used to store current state of software FSM.
*
*******************************************************************************/
uint32 SPI_I2CMasterSendStop(void)
{
    uint32 resetIp;
    uint32 errStatus;

    resetIp   = 0u;
    errStatus = SPI_I2C_MSTR_NOT_READY;

    /* Check FSM state before generating Stop condition */
    if(SPI_CHECK_I2C_MASTER_ACTIVE)
    {
        /*
        * Write direction: generates Stop
        * Read  direction: generates NACK and Stop
        */
        SPI_I2C_MASTER_GENERATE_STOP;

        while(!SPI_CHECK_INTR_MASTER(SPI_INTR_MASTER_I2C_STOP     |
                                                  SPI_INTR_MASTER_I2C_ARB_LOST |
                                                  SPI_INTR_MASTER_I2C_BUS_ERROR))
        {
            /* Wait until Stop has been generated */
        }

        /* Check Stop generation */
        if(SPI_CHECK_INTR_MASTER(SPI_INTR_MASTER_I2C_STOP))
        {
            errStatus = SPI_I2C_MSTR_NO_ERROR;
        }
        else if(SPI_CHECK_INTR_MASTER(SPI_INTR_MASTER_I2C_ARB_LOST))
        {
            errStatus = SPI_I2C_MSTR_ERR_ARB_LOST;
            resetIp   = SPI_I2C_RESET_ERROR;
        }
        else /* SPI_INTR_MASTER_I2C_BUS_ERROR is set */
        {
            errStatus = SPI_I2C_MSTR_ERR_BUS_ERR;
            resetIp   = SPI_I2C_RESET_ERROR;
        }

        SPI_ClearMasterInterruptSource(SPI_INTR_MASTER_I2C_STOP     |
                                                    SPI_INTR_MASTER_I2C_ARB_LOST |
                                                    SPI_INTR_MASTER_I2C_BUS_ERROR);

        SPI_state = SPI_I2C_FSM_IDLE;

        /* Reset block in case of: LOST_ARB or BUS_ERR */
        if(0u != resetIp)
        {
            SPI_SCB_SW_RESET;
        }
    }

    return(errStatus);
}


/*******************************************************************************
* Function Name: SPI_I2CMasterWriteByte
****************************************************************************//**
*
*  Sends one byte to a slave.
*  This function is blocking and does not return until byte is transmitted
*  or error occurred.
*
* \param data: The data byte to send to the slave.
*
* \return
*  Error status.
*  See SPI_I2CMasterSendStart() function for constants.
*
* \sideeffect
*  A valid Start or ReStart condition must be generated before calling
*  this function. This function does nothing if Start or ReStart condition
*  failed before this function was called.
*
* \globalvars
*  SPI_state - used to store current state of software FSM.
*
*******************************************************************************/
uint32 SPI_I2CMasterWriteByte(uint32 theByte)
{
    uint32 resetIp;
    uint32 errStatus;

    resetIp   = 0u;
    errStatus = SPI_I2C_MSTR_NOT_READY;

    /* Check FSM state before write byte */
    if(SPI_CHECK_I2C_MASTER_ACTIVE)
    {
        SPI_TX_FIFO_WR_REG = theByte;

        while(!SPI_CHECK_INTR_MASTER(SPI_INTR_MASTER_I2C_ACK      |
                                                  SPI_INTR_MASTER_I2C_NACK     |
                                                  SPI_INTR_MASTER_I2C_ARB_LOST |
                                                  SPI_INTR_MASTER_I2C_BUS_ERROR))
        {
            /* Wait until byte has been transferred */
        }

        /* Check results after byte was sent */
        if(SPI_CHECK_INTR_MASTER(SPI_INTR_MASTER_I2C_ACK))
        {
            SPI_state = SPI_I2C_FSM_MSTR_HALT;
                         errStatus = SPI_I2C_MSTR_NO_ERROR;
        }
        else if(SPI_CHECK_INTR_MASTER(SPI_INTR_MASTER_I2C_NACK))
        {
            SPI_state = SPI_I2C_FSM_MSTR_HALT;
                         errStatus = SPI_I2C_MSTR_ERR_LB_NAK;
        }
        else if(SPI_CHECK_INTR_MASTER(SPI_INTR_MASTER_I2C_ARB_LOST))
        {
            SPI_state = SPI_I2C_FSM_IDLE;
                         errStatus = SPI_I2C_MSTR_ERR_ARB_LOST;
                         resetIp   = SPI_I2C_RESET_ERROR;
        }
        else /* SPI_INTR_MASTER_I2C_BUS_ERROR set is */
        {
            SPI_state = SPI_I2C_FSM_IDLE;
                         errStatus = SPI_I2C_MSTR_ERR_BUS_ERR;
                         resetIp   = SPI_I2C_RESET_ERROR;
        }

        SPI_ClearMasterInterruptSource(SPI_INTR_MASTER_I2C_ACK      |
                                                    SPI_INTR_MASTER_I2C_NACK     |
                                                    SPI_INTR_MASTER_I2C_ARB_LOST |
                                                    SPI_INTR_MASTER_I2C_BUS_ERROR);

        /* Reset block in case of: LOST_ARB or BUS_ERR */
        if(0u != resetIp)
        {
            SPI_SCB_SW_RESET;
        }
    }

    return(errStatus);
}


/*******************************************************************************
* Function Name: SPI_I2CMasterReadByte
****************************************************************************//**
*
*  Reads one byte from a slave and generates ACK or prepares to generate NAK. 
*  The NAK will be generated before Stop or ReStart condition by 
*  SPI_I2CMasterSendStop() or 
*  SPI_I2CMasterSendRestart() function appropriately.
*  This function is blocking. It does not return until a byte is received or 
*  an error occurs.
*
* \param ackNack: Response to received byte.
*  - SPI_I2C_ACK_DATA - Generates ACK. 
*     The master notifies slave that transfer continues.
*  - SPI_I2C_NAK_DATA - Prepares to generate NAK.
*     The master will notify slave that transfer is completed.
*
* \return
*  Byte read from the slave. In case of error the MSB of returned data 
*  is set to 1.
*
* \sideeffect
*  A valid Start or ReStart condition must be generated before calling
*  this function. This function does nothing if Start or ReStart condition
*  failed before this function was called.
*
* \globalvars
*  SPI_state - used to store current state of software FSM.
*
*******************************************************************************/
uint32 SPI_I2CMasterReadByte(uint32 ackNack)
{
    uint32 theByte;

    /* Return invalid byte in case BUS_ERR happen during receiving */
    theByte = SPI_I2C_INVALID_BYTE;

    /* Check FSM state before read byte */
    if(SPI_CHECK_I2C_MASTER_ACTIVE)
    {
        while((!SPI_CHECK_INTR_RX(SPI_INTR_RX_NOT_EMPTY)) &&
              (!SPI_CHECK_INTR_MASTER(SPI_INTR_MASTER_I2C_ARB_LOST |
                                                  SPI_INTR_MASTER_I2C_BUS_ERROR)))
        {
            /* Wait until byte has been received */
        }

        /* Check the results after the byte was sent */
        if(SPI_CHECK_INTR_RX(SPI_INTR_RX_NOT_EMPTY))
        {
            theByte = SPI_RX_FIFO_RD_REG;

            SPI_ClearRxInterruptSource(SPI_INTR_RX_NOT_EMPTY);

            if(0u == ackNack)
            {
                SPI_I2C_MASTER_GENERATE_ACK;
            }
            else
            {
                /* NACK is generated by Stop or ReStart command */
                SPI_state = SPI_I2C_FSM_MSTR_HALT;
            }
        }
        else
        {
            SPI_ClearMasterInterruptSource(SPI_INTR_MASTER_ALL);

            /* Reset block in case of: LOST_ARB or BUS_ERR */
            SPI_SCB_SW_RESET;
        }
    }

    return(theByte);
}


/*******************************************************************************
* Function Name: SPI_I2CMasterGetReadBufSize
****************************************************************************//**
*
*  Returns the number of bytes that has been transferred with an
*  SPI_I2CMasterReadBuf() function.
*
* \return
*  Byte count of transfer. If the transfer is not yet complete, it returns
*  the byte count transferred so far.
*
* \sideeffect
*  This function returns not valid value if 
*  SPI_I2C_MSTAT_ERR_ARB_LOST or
*  SPI_I2C_MSTAT_ERR_BUS_ERROR occurred while read transfer.
*
* \globalvars
*  SPI_mstrRdBufIndex - used to current index within master read
*  buffer.
*
*******************************************************************************/
uint32 SPI_I2CMasterGetReadBufSize(void)
{
    return(SPI_mstrRdBufIndex);
}


/*******************************************************************************
* Function Name: SPI_I2CMasterGetWriteBufSize
****************************************************************************//**
*
*  Returns the number of bytes that have been transferred with an
*  SCB_I2CMasterWriteBuf() function.
*
* \return
*   Byte count of transfer. If the transfer is not yet complete, it returns
*   zero unit transfer completion.
*
* \sideeffect
*   This function returns not valid value if 
*   SPI_I2C_MSTAT_ERR_ARB_LOST or 
*   SPI_I2C_MSTAT_ERR_BUS_ERROR occurred while read transfer.
*
* \globalvars
*  SPI_mstrWrBufIndex - used to current index within master write
*  buffer.
*
*******************************************************************************/
uint32 SPI_I2CMasterGetWriteBufSize(void)
{
    return(SPI_mstrWrBufIndex);
}


/*******************************************************************************
* Function Name: SPI_I2CMasterClearReadBuf
****************************************************************************//**
*
*  Resets the read buffer pointer back to the first byte in the buffer.
*
* \globalvars
*  SPI_mstrRdBufIndex - used to current index within master read
*   buffer.
*  SPI_mstrStatus - used to store current status of I2C Master.
*
*******************************************************************************/
void SPI_I2CMasterClearReadBuf(void)
{
    SPI_DisableInt();  /* Lock from interruption */

    SPI_mstrRdBufIndex = 0u;
    SPI_mstrStatus    &= (uint16) ~SPI_I2C_MSTAT_RD_CMPLT;

    SPI_EnableInt();   /* Release lock */
}


/*******************************************************************************
* Function Name: SPI_I2CMasterClearWriteBuf
****************************************************************************//**
*
*  Resets the write buffer pointer back to the first byte in the buffer.
*
* \globalvars
*  SPI_mstrRdBufIndex - used to current index within master read
*   buffer.
*  SPI_mstrStatus - used to store current status of I2C Master.
*
*******************************************************************************/
void SPI_I2CMasterClearWriteBuf(void)
{
    SPI_DisableInt();  /* Lock from interruption */

    SPI_mstrWrBufIndex = 0u;
    SPI_mstrStatus    &= (uint16) ~SPI_I2C_MSTAT_WR_CMPLT;

    SPI_EnableInt();   /* Release lock */
}


/*******************************************************************************
* Function Name: SPI_I2CMasterStatus
****************************************************************************//**
*
*  Returns the master's communication status.
*
* \return
*  Current status of I2C master. This status incorporates status constants. 
*  Each constant is a bit field value. The value returned may have multiple 
*  bits set to indicate the status of the read or write transfer.
*  - SPI_I2C_MSTAT_RD_CMPLT - Read transfer complete.
*    The error condition status bits must be checked to ensure that 
*    read transfer was completed successfully.
*  - SPI_I2C_MSTAT_WR_CMPLT - Write transfer complete.
*    The error condition status bits must be checked to ensure that write 
*    transfer was completed successfully.
*  - SPI_I2C_MSTAT_XFER_INP - Transfer in progress.
*  - SPI_I2C_MSTAT_XFER_HALT - Transfer has been halted. 
*    The I2C bus is waiting for ReStart or Stop condition generation.
*  - SPI_I2C_MSTAT_ERR_SHORT_XFER - Error condition: Write 
*    transfer completed before all bytes were transferred. The slave NAKed 
*    the byte which was expected to be ACKed.
*  - SPI_I2C_MSTAT_ERR_ADDR_NAK - Error condition: Slave did 
*    not acknowledge address.
*  - SPI_I2C_MSTAT_ERR_ARB_LOST - Error condition: Master lost 
*    arbitration during communications with slave.
*  - SPI_I2C_MSTAT_ERR_BUS_ERROR - Error condition: bus error 
*    occurred during master transfer due to misplaced Start or Stop 
*    condition on the bus.
*  - SPI_I2C_MSTAT_ERR_ABORT_XFER - Error condition: Slave was 
*    addressed by another master while master performed the start condition 
*    generation. As a result, master has automatically switched to slave 
*    mode and is responding. The master transaction has not taken place
*    This error condition only applicable for Multi-Master-Slave mode.
*  - SPI_I2C_MSTAT_ERR_XFER - Error condition: This is the 
*    ORed value of all error conditions provided above.
*
* \globalvars
*  SPI_mstrStatus - used to store current status of I2C Master.
*
*******************************************************************************/
uint32 SPI_I2CMasterStatus(void)
{
    uint32 status;

    SPI_DisableInt();  /* Lock from interruption */

    status = (uint32) SPI_mstrStatus;

    if (SPI_CHECK_I2C_MASTER_ACTIVE)
    {
        /* Add status of master pending transaction: MSTAT_XFER_INP */
        status |= (uint32) SPI_I2C_MSTAT_XFER_INP;
    }

    SPI_EnableInt();   /* Release lock */

    return(status);
}


/*******************************************************************************
* Function Name: SPI_I2CMasterClearStatus
****************************************************************************//**
*
*  Clears all status flags and returns the master status.
*
* \return
*  Current status of master. See the SPI_I2CMasterStatus() 
*  function for constants.
*
* \globalvars
*  SPI_mstrStatus - used to store current status of I2C Master.
*
*******************************************************************************/
uint32 SPI_I2CMasterClearStatus(void)
{
    uint32 status;

    SPI_DisableInt();  /* Lock from interruption */

    /* Read and clear master status */
    status = (uint32) SPI_mstrStatus;
    SPI_mstrStatus = SPI_I2C_MSTAT_CLEAR;

    SPI_EnableInt();   /* Release lock */

    return(status);
}


/*******************************************************************************
* Function Name: SPI_I2CReStartGeneration
****************************************************************************//**
*
*  Generates a ReStart condition:
*  - SCB IP V1 and later: Generates ReStart using the scb IP functionality
*    Sets the I2C_MASTER_CMD_M_START and I2C_MASTER_CMD_M_NACK (if the previous
*    transaction was read) bits in the SCB.I2C_MASTER_CMD register.
*    This combination forces the master to generate ReStart.
*
*  - SCB IP V0: Generates Restart using the GPIO and scb IP functionality.
*    After the master completes write or read, the SCL is stretched.
*    The master waits until SDA line is released by the slave. Then the GPIO
*    function is enabled and the scb IP disabled as it already does not drive
*    the bus. In case of the previous transfer was read, the NACK is generated
*    by the GPIO. The delay of tLOW is added to manage the hold time.
*    Set I2C_M_CMD.START and enable the scb IP. The ReStart generation
*    is started after the I2C function is enabled for the SCL.
*    Note1: the scb IP due re-enable generates Start but on the I2C bus it
*           appears as ReStart.
*    Note2: the I2C_M_CMD.START is queued if scb IP is disabled.
*    Note3: the I2C_STATUS_M_READ is cleared is address was NACKed before.
*
* \sideeffect
*  SCB IP V0: The NACK generation by the GPIO may cause a greater SCL period
*             than expected for the selected master data rate.
*
*******************************************************************************/
void SPI_I2CReStartGeneration(void)
{
#if(SPI_CY_SCBIP_V0)
    /* Generates Restart use GPIO and scb IP functionality. Ticket ID#143715,
    * ID#145238 and ID#173656.
    */
    uint32 status = SPI_I2C_STATUS_REG;

    while(SPI_WAIT_SDA_SET_HIGH)
    {
        /* Wait when slave release SDA line: SCL tHIGH is complete */
    }

    /* Prepare DR register to drive SCL line */
    SPI_SET_I2C_SCL_DR(SPI_I2C_SCL_LOW);

    /* Switch HSIOM to GPIO: SCL goes low */
    SPI_SET_I2C_SCL_HSIOM_SEL(SPI_HSIOM_GPIO_SEL);

    /* Disable SCB block */
    SPI_CTRL_REG &= (uint32) ~SPI_CTRL_ENABLED;

    if(0u != (status & SPI_I2C_STATUS_M_READ))
    {
        /* Generate NACK use GPIO functionality */
        SPI_SET_I2C_SCL_DR(SPI_I2C_SCL_LOW);
        CyDelayUs(SPI_I2C_TLOW_TIME); /* Count tLOW */

        SPI_SET_I2C_SCL_DR(SPI_I2C_SCL_HIGH);
        while(SPI_WAIT_SCL_SET_HIGH)
        {
            /* Wait until slave releases SCL in case if it stretches */
        }
        CyDelayUs(SPI_I2C_THIGH_TIME); /* Count tHIGH */
    }

    /* Count tLOW as hold time for write and read */
    SPI_SET_I2C_SCL_DR(SPI_I2C_SCL_LOW);
    CyDelayUs(SPI_I2C_TLOW_TIME); /* Count tLOW */

    /* Set command for Start generation: it will appear */
    SPI_I2C_MASTER_CMD_REG = SPI_I2C_MASTER_CMD_M_START;

    /* Enable SCB block */
    SPI_CTRL_REG |= (uint32) SPI_CTRL_ENABLED;

    /* Switch HSIOM to I2C: */
    SPI_SET_I2C_SCL_HSIOM_SEL(SPI_HSIOM_I2C_SEL);

    /* Revert SCL DR register */
    SPI_SET_I2C_SCL_DR(SPI_I2C_SCL_HIGH);
#else
    uint32 cmd;

    /* Generates ReStart use scb IP functionality */
    cmd  = SPI_I2C_MASTER_CMD_M_START;
    cmd |= SPI_CHECK_I2C_STATUS(SPI_I2C_STATUS_M_READ) ?
                (SPI_I2C_MASTER_CMD_M_NACK) : (0u);

    SPI_I2C_MASTER_CMD_REG = cmd;
#endif /* (SPI_CY_SCBIP_V1) */
}

#endif /* (SPI_I2C_MASTER_CONST) */


#if (!SPI_CY_SCBIP_V0 && \
    SPI_I2C_MULTI_MASTER_SLAVE_CONST && SPI_I2C_WAKE_ENABLE_CONST)
    /*******************************************************************************
    * Function Name: SPI_I2CMasterDisableEcAm
    ****************************************************************************//**
    *
    *  Disables externally clocked address match to enable master operation
    *  in active mode.
    *
    *******************************************************************************/
    static void SPI_I2CMasterDisableEcAm(void)
    {
        /* Disables externally clocked address match to enable master operation in active mode.
        * This applicable only for Multi-Master-Slave with wakeup enabled. Ticket ID#192742 */
        if (0u != (SPI_CTRL_REG & SPI_CTRL_EC_AM_MODE))
        {
            /* Enable external address match logic */
            SPI_Stop();
            SPI_CTRL_REG &= (uint32) ~SPI_CTRL_EC_AM_MODE;
            SPI_Enable();
        }
    }
#endif /* (!SPI_CY_SCBIP_V0) */


/* [] END OF FILE */
