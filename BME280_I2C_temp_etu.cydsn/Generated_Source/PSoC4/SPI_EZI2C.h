/***************************************************************************//**
* \file SPI_EZI2C.h
* \version 3.20
*
* \brief
*  This file provides constants and parameter values for the SCB Component in
*  the EZI2C mode.
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

#if !defined(CY_SCB_EZI2C_SPI_H)
#define CY_SCB_EZI2C_SPI_H

#include "SPI.h"


/***************************************
*   Initial Parameter Constants
****************************************/

#define SPI_EZI2C_CLOCK_STRETCHING         (1u)
#define SPI_EZI2C_MEDIAN_FILTER_ENABLE     (1u)
#define SPI_EZI2C_NUMBER_OF_ADDRESSES      (0u)
#define SPI_EZI2C_PRIMARY_SLAVE_ADDRESS    (8u)
#define SPI_EZI2C_SECONDARY_SLAVE_ADDRESS  (9u)
#define SPI_EZI2C_SUB_ADDRESS_SIZE         (0u)
#define SPI_EZI2C_WAKE_ENABLE              (0u)
#define SPI_EZI2C_DATA_RATE                (100u)
#define SPI_EZI2C_SLAVE_ADDRESS_MASK       (254u)
#define SPI_EZI2C_BYTE_MODE_ENABLE         (0u)


/***************************************
*  Conditional Compilation Parameters
****************************************/

#if(SPI_SCB_MODE_UNCONFIG_CONST_CFG)

    #define SPI_SUB_ADDRESS_SIZE16             (0u != SPI_subAddrSize)
    #define SPI_SECONDARY_ADDRESS_ENABLE       (0u != SPI_numberOfAddr)

    #define SPI_EZI2C_EC_AM_ENABLE         (0u != (SPI_CTRL_REG & \
                                                                SPI_CTRL_EC_AM_MODE))
    #define SPI_EZI2C_SCL_STRETCH_ENABLE   (0u != (SPI_GetSlaveInterruptMode() & \
                                                                SPI_INTR_SLAVE_I2C_ADDR_MATCH))
    #define SPI_EZI2C_SCL_STRETCH_DISABLE       (!SPI_EZI2C_SCL_STRETCH_ENABLE)

    #define SPI_SECONDARY_ADDRESS_ENABLE_CONST  (1u)
    #define SPI_SUB_ADDRESS_SIZE16_CONST        (1u)
    #define SPI_EZI2C_SCL_STRETCH_ENABLE_CONST  (1u)
    #define SPI_EZI2C_SCL_STRETCH_DISABLE_CONST (1u)
    #define SPI_EZI2C_WAKE_ENABLE_CONST         (1u)

    #if (SPI_CY_SCBIP_V0 || SPI_CY_SCBIP_V1)
        #define SPI_EZI2C_FIFO_SIZE    (SPI_FIFO_SIZE)
    #else
        #define SPI_EZI2C_FIFO_SIZE    (SPI_GET_FIFO_SIZE(SPI_CTRL_REG & \
                                                                                    SPI_CTRL_BYTE_MODE))
    #endif /* (SPI_CY_SCBIP_V0 || SPI_CY_SCBIP_V1) */

#else

    #define SPI_SUB_ADDRESS_SIZE16         (0u != SPI_EZI2C_SUB_ADDRESS_SIZE)
    #define SPI_SUB_ADDRESS_SIZE16_CONST   (SPI_SUB_ADDRESS_SIZE16)

    #define SPI_SECONDARY_ADDRESS_ENABLE        (0u != SPI_EZI2C_NUMBER_OF_ADDRESSES)
    #define SPI_SECONDARY_ADDRESS_ENABLE_CONST  (SPI_SECONDARY_ADDRESS_ENABLE)

    #define SPI_EZI2C_SCL_STRETCH_ENABLE        (0u != SPI_EZI2C_CLOCK_STRETCHING)
    #define SPI_EZI2C_SCL_STRETCH_DISABLE       (!SPI_EZI2C_SCL_STRETCH_ENABLE)
    #define SPI_EZI2C_SCL_STRETCH_ENABLE_CONST  (SPI_EZI2C_SCL_STRETCH_ENABLE)
    #define SPI_EZI2C_SCL_STRETCH_DISABLE_CONST (SPI_EZI2C_SCL_STRETCH_DISABLE)

    #define SPI_EZI2C_WAKE_ENABLE_CONST         (0u != SPI_EZI2C_WAKE_ENABLE)
    #define SPI_EZI2C_EC_AM_ENABLE              (0u != SPI_EZI2C_WAKE_ENABLE)

    #define SPI_EZI2C_FIFO_SIZE SPI_GET_FIFO_SIZE(SPI_EZI2C_BYTE_MODE_ENABLE)

#endif /* (SPI_SCB_MODE_UNCONFIG_CONST_CFG) */


/***************************************
*       Type Definitions
***************************************/
/**
* \addtogroup group_structures
* @{
*/
typedef struct
{
    /** When enabled the SCL is stretched as required for proper operation: 
     *  0 – disable, 1 – enable.
    */
    uint32 enableClockStretch;
    
    /** This field is left for compatibility and its value is ignored. 
     *  Median filter is disabled for EZI2C mode.
    */
    uint32 enableMedianFilter;
    
    /** Number of supported addresses: 
     *  - SPI_EZI2C_ONE_ADDRESS
     *  - SPI_EZI2C_TWO_ADDRESSES
    */
    uint32 numberOfAddresses;
    
    /** Primary 7-bit slave address.
    */
    uint32 primarySlaveAddr;
    
    /** Secondary 7-bit slave address.
    */
    uint32 secondarySlaveAddr;
    
    /** Size of sub-address.
     *  - SPI_EZI2C_SUB_ADDR8_BITS 
     *  - SPI_EZI2C_SUB_ADDR16_BITS
    */
    uint32 subAddrSize;
    
    /** When enabled the TX and RX FIFO depth is doubled and equal to 
     *  16 bytes: 0 – disable, 1 – enable.
    */
    uint32 enableWake;
    
    /** When enabled the TX and RX FIFO depth is 16 bytes: 0 – disable, 
     *  1 – enable.
     * 
     *  Ignored for all devices other than PSoC 4100 BLE / PSoC 4200 BLE / 
     *  PSoC 4100M / PSoC 4200M / PSoC 4200L / PSoC 4000S / PSoC 4100S / 
     *  PSoC Analog Coprocessor.
    */
    uint8  enableByteMode;
} SPI_EZI2C_INIT_STRUCT;
/** @} structures */

/***************************************
*        Function Prototypes
***************************************/

/**
* \addtogroup group_ezi2c
* @{
*/

#if(SPI_SCB_MODE_UNCONFIG_CONST_CFG)
    void SPI_EzI2CInit(const SPI_EZI2C_INIT_STRUCT *config);
#endif /* (SPI_SCB_MODE_UNCONFIG_CONST_CFG) */

uint32 SPI_EzI2CGetActivity(void);

void   SPI_EzI2CSetAddress1(uint32 address);
uint32 SPI_EzI2CGetAddress1(void);
void   SPI_EzI2CSetBuffer1(uint32 bufSize, uint32 rwBoundary, volatile uint8 * buffer);
void   SPI_EzI2CSetReadBoundaryBuffer1(uint32 rwBoundary);

#if(SPI_SECONDARY_ADDRESS_ENABLE_CONST)
    void   SPI_EzI2CSetAddress2(uint32 address);
    uint32 SPI_EzI2CGetAddress2(void);
    void   SPI_EzI2CSetBuffer2(uint32 bufSize, uint32 rwBoundary, volatile uint8 * buffer);
    void   SPI_EzI2CSetReadBoundaryBuffer2(uint32 rwBoundary);
#endif /* (SPI_SECONDARY_ADDRESS_ENABLE_CONST) */
/** @} ezi2c */

#if(SPI_EZI2C_SCL_STRETCH_ENABLE_CONST)
    CY_ISR_PROTO(SPI_EZI2C_STRETCH_ISR);
#endif /* (SPI_EZI2C_SCL_STRETCH_ENABLE_CONST) */

#if(SPI_EZI2C_SCL_STRETCH_DISABLE_CONST)
    CY_ISR_PROTO(SPI_EZI2C_NO_STRETCH_ISR);
#endif /* (SPI_EZI2C_SCL_STRETCH_DISABLE) */


/***************************************
*            API Constants
***************************************/

/* Configuration structure constants */
#define SPI_EZI2C_ONE_ADDRESS      (0u)
#define SPI_EZI2C_TWO_ADDRESSES    (1u)

#define SPI_EZI2C_PRIMARY_ADDRESS  (0u)
#define SPI_EZI2C_BOTH_ADDRESSES   (1u)

#define SPI_EZI2C_SUB_ADDR8_BITS   (0u)
#define SPI_EZI2C_SUB_ADDR16_BITS  (1u)

/* SPI_EzI2CGetActivity() return constants */
#define SPI_EZI2C_STATUS_SECOND_OFFSET (1u)
#define SPI_EZI2C_STATUS_READ1     ((uint32) (SPI_INTR_SLAVE_I2C_NACK))        /* Bit [1]   */
#define SPI_EZI2C_STATUS_WRITE1    ((uint32) (SPI_INTR_SLAVE_I2C_WRITE_STOP))  /* Bit [3]   */
#define SPI_EZI2C_STATUS_READ2     ((uint32) (SPI_INTR_SLAVE_I2C_NACK >> \
                                                           SPI_EZI2C_STATUS_SECOND_OFFSET)) /* Bit [0]   */
#define SPI_EZI2C_STATUS_WRITE2    ((uint32) (SPI_INTR_SLAVE_I2C_WRITE_STOP >> \
                                                           SPI_EZI2C_STATUS_SECOND_OFFSET)) /* Bit [2]   */
#define SPI_EZI2C_STATUS_ERR       ((uint32) (0x10u))                                       /* Bit [4]   */
#define SPI_EZI2C_STATUS_BUSY      ((uint32) (0x20u))                                       /* Bit [5]   */
#define SPI_EZI2C_CLEAR_STATUS     ((uint32) (0x1Fu))                                       /* Bits [0-4]*/
#define SPI_EZI2C_CMPLT_INTR_MASK  ((uint32) (SPI_INTR_SLAVE_I2C_NACK | \
                                                           SPI_INTR_SLAVE_I2C_WRITE_STOP))


/***************************************
*     Vars with External Linkage
***************************************/

#if(SPI_SCB_MODE_UNCONFIG_CONST_CFG)
    extern const SPI_EZI2C_INIT_STRUCT SPI_configEzI2C;
#endif /* (SPI_SCB_MODE_UNCONFIG_CONST_CFG) */


/***************************************
*           FSM states
***************************************/

/* Returns to master when there is no data to transmit */
#define SPI_EZI2C_OVFL_RETURN  (0xFFu)

/* States of EZI2C Slave FSM */
#define SPI_EZI2C_FSM_OFFSET_HI8 ((uint8) (0x02u)) /* Idle state for 1 addr: waits for high byte offset */
#define SPI_EZI2C_FSM_OFFSET_LO8 ((uint8) (0x00u)) /* Idle state for 2 addr: waits for low byte offset  */
#define SPI_EZI2C_FSM_BYTE_WRITE ((uint8) (0x01u)) /* Data byte write state: byte by byte mode          */
#define SPI_EZI2C_FSM_WAIT_STOP  ((uint8) (0x03u)) /* Discards written bytes as they do not match write
                                                                   criteria */
#define SPI_EZI2C_FSM_WRITE_MASK ((uint8) (0x01u)) /* Incorporates write states: EZI2C_FSM_BYTE_WRITE and
                                                                   EZI2C_FSM_WAIT_STOP  */

#define SPI_EZI2C_FSM_IDLE     ((SPI_SUB_ADDRESS_SIZE16) ?      \
                                                (SPI_EZI2C_FSM_OFFSET_HI8) : \
                                                (SPI_EZI2C_FSM_OFFSET_LO8))

#define SPI_EZI2C_CONTINUE_TRANSFER    (1u)
#define SPI_EZI2C_COMPLETE_TRANSFER    (0u)

#define SPI_EZI2C_NACK_RECEIVED_ADDRESS  (0u)
#define SPI_EZI2C_ACK_RECEIVED_ADDRESS   (1u)

#define SPI_EZI2C_ACTIVE_ADDRESS1  (0u)
#define SPI_EZI2C_ACTIVE_ADDRESS2  (1u)


/***************************************
*       Macro Definitions
***************************************/

/* Access to global variables */
#if(SPI_SECONDARY_ADDRESS_ENABLE_CONST)

    #define SPI_EZI2C_UPDATE_LOC_STATUS(activeAddress, locStatus) \
            do{ \
                (locStatus) >>= (activeAddress); \
            }while(0)

    #define SPI_EZI2C_GET_INDEX(activeAddress) \
            ((SPI_EZI2C_ACTIVE_ADDRESS1 == (activeAddress)) ? \
                ((uint32) SPI_indexBuf1) :                    \
                ((uint32) SPI_indexBuf2))

    #define SPI_EZI2C_GET_OFFSET(activeAddress) \
            ((SPI_EZI2C_ACTIVE_ADDRESS1 == (activeAddress)) ? \
                ((uint32) SPI_offsetBuf1) :                   \
                ((uint32) SPI_offsetBuf2))

    #define SPI_EZI2C_SET_INDEX(activeAddress, locIndex) \
            do{ \
                if(SPI_EZI2C_ACTIVE_ADDRESS1 == (activeAddress)) \
                {    \
                    SPI_indexBuf1 = (uint16) (locIndex); \
                }    \
                else \
                {    \
                    SPI_indexBuf2 = (uint16) (locIndex); \
                }    \
            }while(0)

    #define SPI_EZI2C_SET_OFFSET(activeAddress, locOffset) \
            do{ \
                if(SPI_EZI2C_ACTIVE_ADDRESS1 == (activeAddress)) \
                {    \
                    SPI_offsetBuf1 = (uint16) (locOffset); \
                }    \
                else \
                {    \
                    SPI_offsetBuf2 = (uint16) (locOffset); \
                }    \
            }while(0)
#else
    #define SPI_EZI2C_UPDATE_LOC_STATUS(activeAddress, locStatus)  do{ /* Empty*/ }while(0)

    #define SPI_EZI2C_GET_INDEX(activeAddress)  ((uint32) (SPI_indexBuf1))

    #define SPI_EZI2C_GET_OFFSET(activeAddress) ((uint32) (SPI_offsetBuf1))

    #define SPI_EZI2C_SET_INDEX(activeAddress, locIndex) \
            do{ \
                SPI_indexBuf1 = (uint16) (locIndex); \
            }while(0)

    #define SPI_EZI2C_SET_OFFSET(activeAddress, locOffset) \
            do{ \
                SPI_offsetBuf1 = (uint16) (locOffset); \
            }while(0)

#endif  /* (SPI_SUB_ADDRESS_SIZE16_CONST) */


/* This macro only applicable for EZI2C slave without clock stretching.
* It should not be used for other pusposes. */
#define SPI_EZI2C_TX_FIFO_CTRL_SET   (SPI_EZI2C_TX_FIFO_CTRL | \
                                                   SPI_TX_FIFO_CTRL_CLEAR)
#define SPI_EZI2C_TX_FIFO_CTRL_CLEAR (SPI_EZI2C_TX_FIFO_CTRL)
#define SPI_FAST_CLEAR_TX_FIFO \
                            do{             \
                                SPI_TX_FIFO_CTRL_REG = SPI_EZI2C_TX_FIFO_CTRL_SET;   \
                                SPI_TX_FIFO_CTRL_REG = SPI_EZI2C_TX_FIFO_CTRL_CLEAR; \
                            }while(0)


/***************************************
*      Common Register Settings
***************************************/

#define SPI_CTRL_EZI2C     (SPI_CTRL_MODE_I2C)

#define SPI_EZI2C_CTRL     (SPI_I2C_CTRL_S_GENERAL_IGNORE | \
                                         SPI_I2C_CTRL_SLAVE_MODE)

#define SPI_EZI2C_CTRL_AUTO    (SPI_I2C_CTRL_S_READY_ADDR_ACK      | \
                                             SPI_I2C_CTRL_S_READY_DATA_ACK      | \
                                             SPI_I2C_CTRL_S_NOT_READY_ADDR_NACK | \
                                             SPI_I2C_CTRL_S_NOT_READY_DATA_NACK)

#define SPI_EZI2C_RX_CTRL  ((SPI_FIFO_SIZE - 1u)   | \
                                          SPI_RX_CTRL_MSB_FIRST | \
                                          SPI_RX_CTRL_ENABLED)

#define SPI_EZI2C_TX_FIFO_CTRL (2u)
#define SPI_TX_LOAD_SIZE       (2u)

#define SPI_EZI2C_TX_CTRL  ((SPI_ONE_BYTE_WIDTH - 1u) | \
                                          SPI_TX_CTRL_MSB_FIRST    | \
                                          SPI_TX_CTRL_ENABLED)

#define SPI_EZI2C_INTR_SLAVE_MASK  (SPI_INTR_SLAVE_I2C_BUS_ERROR | \
                                                 SPI_INTR_SLAVE_I2C_ARB_LOST  | \
                                                 SPI_INTR_SLAVE_I2C_STOP)

#define SPI_INTR_SLAVE_COMPLETE    (SPI_INTR_SLAVE_I2C_STOP | \
                                                 SPI_INTR_SLAVE_I2C_NACK | \
                                                 SPI_INTR_SLAVE_I2C_WRITE_STOP)


/***************************************
*    Initialization Register Settings
***************************************/

#if(SPI_SCB_MODE_EZI2C_CONST_CFG)

    #define SPI_EZI2C_DEFAULT_CTRL \
                                (SPI_GET_CTRL_BYTE_MODE  (SPI_EZI2C_BYTE_MODE_ENABLE)    | \
                                 SPI_GET_CTRL_ADDR_ACCEPT(SPI_EZI2C_NUMBER_OF_ADDRESSES) | \
                                 SPI_GET_CTRL_EC_AM_MODE (SPI_EZI2C_WAKE_ENABLE))

    #if(SPI_EZI2C_SCL_STRETCH_ENABLE_CONST)
        #define SPI_EZI2C_DEFAULT_I2C_CTRL (SPI_EZI2C_CTRL)
    #else
        #define SPI_EZI2C_DEFAULT_I2C_CTRL (SPI_EZI2C_CTRL_AUTO | SPI_EZI2C_CTRL)
    #endif /* (SPI_EZI2C_SCL_STRETCH_ENABLE_CONST) */


    #define SPI_EZI2C_DEFAULT_RX_MATCH \
                                (SPI_GET_I2C_8BIT_ADDRESS(SPI_EZI2C_PRIMARY_SLAVE_ADDRESS) | \
                                 SPI_GET_RX_MATCH_MASK   (SPI_EZI2C_SLAVE_ADDRESS_MASK))

    #define SPI_EZI2C_DEFAULT_RX_CTRL  (SPI_EZI2C_RX_CTRL)
    #define SPI_EZI2C_DEFAULT_TX_CTRL  (SPI_EZI2C_TX_CTRL)

    #define SPI_EZI2C_DEFAULT_RX_FIFO_CTRL (0u)
    #if(SPI_EZI2C_SCL_STRETCH_ENABLE_CONST)
        #define SPI_EZI2C_DEFAULT_TX_FIFO_CTRL (0u)
    #else
        #define SPI_EZI2C_DEFAULT_TX_FIFO_CTRL (2u)
    #endif /* (SPI_EZI2C_SCL_STRETCH_ENABLE_CONST) */

    /* Interrupt sources */
    #define SPI_EZI2C_DEFAULT_INTR_I2C_EC_MASK (SPI_NO_INTR_SOURCES)
    #define SPI_EZI2C_DEFAULT_INTR_SPI_EC_MASK (SPI_NO_INTR_SOURCES)
    #define SPI_EZI2C_DEFAULT_INTR_MASTER_MASK (SPI_NO_INTR_SOURCES)
    #define SPI_EZI2C_DEFAULT_INTR_TX_MASK     (SPI_NO_INTR_SOURCES)

    #if(SPI_EZI2C_SCL_STRETCH_ENABLE_CONST)
        #define SPI_EZI2C_DEFAULT_INTR_RX_MASK     (SPI_NO_INTR_SOURCES)
        #define SPI_EZI2C_DEFAULT_INTR_SLAVE_MASK  (SPI_INTR_SLAVE_I2C_ADDR_MATCH | \
                                                                 SPI_EZI2C_INTR_SLAVE_MASK)
    #else
        #define SPI_EZI2C_DEFAULT_INTR_RX_MASK     (SPI_INTR_RX_NOT_EMPTY)
        #define SPI_EZI2C_DEFAULT_INTR_SLAVE_MASK  (SPI_INTR_SLAVE_I2C_START | \
                                                                 SPI_EZI2C_INTR_SLAVE_MASK)
    #endif /* (SPI_EZI2C_SCL_STRETCH_ENABLE_CONST) */

#endif /* (SPI_SCB_MODE_EZI2C_CONST_CFG) */

#endif /* (CY_SCB_EZI2C_SPI_H) */


/* [] END OF FILE */
