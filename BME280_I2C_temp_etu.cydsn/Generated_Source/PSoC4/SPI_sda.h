/*******************************************************************************
* File Name: SPI_sda.h  
* Version 2.20
*
* Description:
*  This file contains Pin function prototypes and register defines
*
********************************************************************************
* Copyright 2008-2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#if !defined(CY_PINS_SPI_sda_H) /* Pins SPI_sda_H */
#define CY_PINS_SPI_sda_H

#include "cytypes.h"
#include "cyfitter.h"
#include "SPI_sda_aliases.h"


/***************************************
*     Data Struct Definitions
***************************************/

/**
* \addtogroup group_structures
* @{
*/
    
/* Structure for sleep mode support */
typedef struct
{
    uint32 pcState; /**< State of the port control register */
    uint32 sioState; /**< State of the SIO configuration */
    uint32 usbState; /**< State of the USBIO regulator */
} SPI_sda_BACKUP_STRUCT;

/** @} structures */


/***************************************
*        Function Prototypes             
***************************************/
/**
* \addtogroup group_general
* @{
*/
uint8   SPI_sda_Read(void);
void    SPI_sda_Write(uint8 value);
uint8   SPI_sda_ReadDataReg(void);
#if defined(SPI_sda__PC) || (CY_PSOC4_4200L) 
    void    SPI_sda_SetDriveMode(uint8 mode);
#endif
void    SPI_sda_SetInterruptMode(uint16 position, uint16 mode);
uint8   SPI_sda_ClearInterrupt(void);
/** @} general */

/**
* \addtogroup group_power
* @{
*/
void SPI_sda_Sleep(void); 
void SPI_sda_Wakeup(void);
/** @} power */


/***************************************
*           API Constants        
***************************************/
#if defined(SPI_sda__PC) || (CY_PSOC4_4200L) 
    /* Drive Modes */
    #define SPI_sda_DRIVE_MODE_BITS        (3)
    #define SPI_sda_DRIVE_MODE_IND_MASK    (0xFFFFFFFFu >> (32 - SPI_sda_DRIVE_MODE_BITS))

    /**
    * \addtogroup group_constants
    * @{
    */
        /** \addtogroup driveMode Drive mode constants
         * \brief Constants to be passed as "mode" parameter in the SPI_sda_SetDriveMode() function.
         *  @{
         */
        #define SPI_sda_DM_ALG_HIZ         (0x00u) /**< \brief High Impedance Analog   */
        #define SPI_sda_DM_DIG_HIZ         (0x01u) /**< \brief High Impedance Digital  */
        #define SPI_sda_DM_RES_UP          (0x02u) /**< \brief Resistive Pull Up       */
        #define SPI_sda_DM_RES_DWN         (0x03u) /**< \brief Resistive Pull Down     */
        #define SPI_sda_DM_OD_LO           (0x04u) /**< \brief Open Drain, Drives Low  */
        #define SPI_sda_DM_OD_HI           (0x05u) /**< \brief Open Drain, Drives High */
        #define SPI_sda_DM_STRONG          (0x06u) /**< \brief Strong Drive            */
        #define SPI_sda_DM_RES_UPDWN       (0x07u) /**< \brief Resistive Pull Up/Down  */
        /** @} driveMode */
    /** @} group_constants */
#endif

/* Digital Port Constants */
#define SPI_sda_MASK               SPI_sda__MASK
#define SPI_sda_SHIFT              SPI_sda__SHIFT
#define SPI_sda_WIDTH              1u

/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in SPI_sda_SetInterruptMode() function.
     *  @{
     */
        #define SPI_sda_INTR_NONE      ((uint16)(0x0000u)) /**< \brief Disabled             */
        #define SPI_sda_INTR_RISING    ((uint16)(0x5555u)) /**< \brief Rising edge trigger  */
        #define SPI_sda_INTR_FALLING   ((uint16)(0xaaaau)) /**< \brief Falling edge trigger */
        #define SPI_sda_INTR_BOTH      ((uint16)(0xffffu)) /**< \brief Both edge trigger    */
    /** @} intrMode */
/** @} group_constants */

/* SIO LPM definition */
#if defined(SPI_sda__SIO)
    #define SPI_sda_SIO_LPM_MASK       (0x03u)
#endif

/* USBIO definitions */
#if !defined(SPI_sda__PC) && (CY_PSOC4_4200L)
    #define SPI_sda_USBIO_ENABLE               ((uint32)0x80000000u)
    #define SPI_sda_USBIO_DISABLE              ((uint32)(~SPI_sda_USBIO_ENABLE))
    #define SPI_sda_USBIO_SUSPEND_SHIFT        CYFLD_USBDEVv2_USB_SUSPEND__OFFSET
    #define SPI_sda_USBIO_SUSPEND_DEL_SHIFT    CYFLD_USBDEVv2_USB_SUSPEND_DEL__OFFSET
    #define SPI_sda_USBIO_ENTER_SLEEP          ((uint32)((1u << SPI_sda_USBIO_SUSPEND_SHIFT) \
                                                        | (1u << SPI_sda_USBIO_SUSPEND_DEL_SHIFT)))
    #define SPI_sda_USBIO_EXIT_SLEEP_PH1       ((uint32)~((uint32)(1u << SPI_sda_USBIO_SUSPEND_SHIFT)))
    #define SPI_sda_USBIO_EXIT_SLEEP_PH2       ((uint32)~((uint32)(1u << SPI_sda_USBIO_SUSPEND_DEL_SHIFT)))
    #define SPI_sda_USBIO_CR1_OFF              ((uint32)0xfffffffeu)
#endif


/***************************************
*             Registers        
***************************************/
/* Main Port Registers */
#if defined(SPI_sda__PC)
    /* Port Configuration */
    #define SPI_sda_PC                 (* (reg32 *) SPI_sda__PC)
#endif
/* Pin State */
#define SPI_sda_PS                     (* (reg32 *) SPI_sda__PS)
/* Data Register */
#define SPI_sda_DR                     (* (reg32 *) SPI_sda__DR)
/* Input Buffer Disable Override */
#define SPI_sda_INP_DIS                (* (reg32 *) SPI_sda__PC2)

/* Interrupt configuration Registers */
#define SPI_sda_INTCFG                 (* (reg32 *) SPI_sda__INTCFG)
#define SPI_sda_INTSTAT                (* (reg32 *) SPI_sda__INTSTAT)

/* "Interrupt cause" register for Combined Port Interrupt (AllPortInt) in GSRef component */
#if defined (CYREG_GPIO_INTR_CAUSE)
    #define SPI_sda_INTR_CAUSE         (* (reg32 *) CYREG_GPIO_INTR_CAUSE)
#endif

/* SIO register */
#if defined(SPI_sda__SIO)
    #define SPI_sda_SIO_REG            (* (reg32 *) SPI_sda__SIO)
#endif /* (SPI_sda__SIO_CFG) */

/* USBIO registers */
#if !defined(SPI_sda__PC) && (CY_PSOC4_4200L)
    #define SPI_sda_USB_POWER_REG       (* (reg32 *) CYREG_USBDEVv2_USB_POWER_CTRL)
    #define SPI_sda_CR1_REG             (* (reg32 *) CYREG_USBDEVv2_CR1)
    #define SPI_sda_USBIO_CTRL_REG      (* (reg32 *) CYREG_USBDEVv2_USB_USBIO_CTRL)
#endif    
    
    
/***************************************
* The following code is DEPRECATED and 
* must not be used in new designs.
***************************************/
/**
* \addtogroup group_deprecated
* @{
*/
#define SPI_sda_DRIVE_MODE_SHIFT       (0x00u)
#define SPI_sda_DRIVE_MODE_MASK        (0x07u << SPI_sda_DRIVE_MODE_SHIFT)
/** @} deprecated */

#endif /* End Pins SPI_sda_H */


/* [] END OF FILE */
