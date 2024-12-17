/*******************************************************************************
* File Name: SCK.h  
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

#if !defined(CY_PINS_SCK_H) /* Pins SCK_H */
#define CY_PINS_SCK_H

#include "cytypes.h"
#include "cyfitter.h"
#include "SCK_aliases.h"


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
} SCK_BACKUP_STRUCT;

/** @} structures */


/***************************************
*        Function Prototypes             
***************************************/
/**
* \addtogroup group_general
* @{
*/
uint8   SCK_Read(void);
void    SCK_Write(uint8 value);
uint8   SCK_ReadDataReg(void);
#if defined(SCK__PC) || (CY_PSOC4_4200L) 
    void    SCK_SetDriveMode(uint8 mode);
#endif
void    SCK_SetInterruptMode(uint16 position, uint16 mode);
uint8   SCK_ClearInterrupt(void);
/** @} general */

/**
* \addtogroup group_power
* @{
*/
void SCK_Sleep(void); 
void SCK_Wakeup(void);
/** @} power */


/***************************************
*           API Constants        
***************************************/
#if defined(SCK__PC) || (CY_PSOC4_4200L) 
    /* Drive Modes */
    #define SCK_DRIVE_MODE_BITS        (3)
    #define SCK_DRIVE_MODE_IND_MASK    (0xFFFFFFFFu >> (32 - SCK_DRIVE_MODE_BITS))

    /**
    * \addtogroup group_constants
    * @{
    */
        /** \addtogroup driveMode Drive mode constants
         * \brief Constants to be passed as "mode" parameter in the SCK_SetDriveMode() function.
         *  @{
         */
        #define SCK_DM_ALG_HIZ         (0x00u) /**< \brief High Impedance Analog   */
        #define SCK_DM_DIG_HIZ         (0x01u) /**< \brief High Impedance Digital  */
        #define SCK_DM_RES_UP          (0x02u) /**< \brief Resistive Pull Up       */
        #define SCK_DM_RES_DWN         (0x03u) /**< \brief Resistive Pull Down     */
        #define SCK_DM_OD_LO           (0x04u) /**< \brief Open Drain, Drives Low  */
        #define SCK_DM_OD_HI           (0x05u) /**< \brief Open Drain, Drives High */
        #define SCK_DM_STRONG          (0x06u) /**< \brief Strong Drive            */
        #define SCK_DM_RES_UPDWN       (0x07u) /**< \brief Resistive Pull Up/Down  */
        /** @} driveMode */
    /** @} group_constants */
#endif

/* Digital Port Constants */
#define SCK_MASK               SCK__MASK
#define SCK_SHIFT              SCK__SHIFT
#define SCK_WIDTH              1u

/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in SCK_SetInterruptMode() function.
     *  @{
     */
        #define SCK_INTR_NONE      ((uint16)(0x0000u)) /**< \brief Disabled             */
        #define SCK_INTR_RISING    ((uint16)(0x5555u)) /**< \brief Rising edge trigger  */
        #define SCK_INTR_FALLING   ((uint16)(0xaaaau)) /**< \brief Falling edge trigger */
        #define SCK_INTR_BOTH      ((uint16)(0xffffu)) /**< \brief Both edge trigger    */
    /** @} intrMode */
/** @} group_constants */

/* SIO LPM definition */
#if defined(SCK__SIO)
    #define SCK_SIO_LPM_MASK       (0x03u)
#endif

/* USBIO definitions */
#if !defined(SCK__PC) && (CY_PSOC4_4200L)
    #define SCK_USBIO_ENABLE               ((uint32)0x80000000u)
    #define SCK_USBIO_DISABLE              ((uint32)(~SCK_USBIO_ENABLE))
    #define SCK_USBIO_SUSPEND_SHIFT        CYFLD_USBDEVv2_USB_SUSPEND__OFFSET
    #define SCK_USBIO_SUSPEND_DEL_SHIFT    CYFLD_USBDEVv2_USB_SUSPEND_DEL__OFFSET
    #define SCK_USBIO_ENTER_SLEEP          ((uint32)((1u << SCK_USBIO_SUSPEND_SHIFT) \
                                                        | (1u << SCK_USBIO_SUSPEND_DEL_SHIFT)))
    #define SCK_USBIO_EXIT_SLEEP_PH1       ((uint32)~((uint32)(1u << SCK_USBIO_SUSPEND_SHIFT)))
    #define SCK_USBIO_EXIT_SLEEP_PH2       ((uint32)~((uint32)(1u << SCK_USBIO_SUSPEND_DEL_SHIFT)))
    #define SCK_USBIO_CR1_OFF              ((uint32)0xfffffffeu)
#endif


/***************************************
*             Registers        
***************************************/
/* Main Port Registers */
#if defined(SCK__PC)
    /* Port Configuration */
    #define SCK_PC                 (* (reg32 *) SCK__PC)
#endif
/* Pin State */
#define SCK_PS                     (* (reg32 *) SCK__PS)
/* Data Register */
#define SCK_DR                     (* (reg32 *) SCK__DR)
/* Input Buffer Disable Override */
#define SCK_INP_DIS                (* (reg32 *) SCK__PC2)

/* Interrupt configuration Registers */
#define SCK_INTCFG                 (* (reg32 *) SCK__INTCFG)
#define SCK_INTSTAT                (* (reg32 *) SCK__INTSTAT)

/* "Interrupt cause" register for Combined Port Interrupt (AllPortInt) in GSRef component */
#if defined (CYREG_GPIO_INTR_CAUSE)
    #define SCK_INTR_CAUSE         (* (reg32 *) CYREG_GPIO_INTR_CAUSE)
#endif

/* SIO register */
#if defined(SCK__SIO)
    #define SCK_SIO_REG            (* (reg32 *) SCK__SIO)
#endif /* (SCK__SIO_CFG) */

/* USBIO registers */
#if !defined(SCK__PC) && (CY_PSOC4_4200L)
    #define SCK_USB_POWER_REG       (* (reg32 *) CYREG_USBDEVv2_USB_POWER_CTRL)
    #define SCK_CR1_REG             (* (reg32 *) CYREG_USBDEVv2_CR1)
    #define SCK_USBIO_CTRL_REG      (* (reg32 *) CYREG_USBDEVv2_USB_USBIO_CTRL)
#endif    
    
    
/***************************************
* The following code is DEPRECATED and 
* must not be used in new designs.
***************************************/
/**
* \addtogroup group_deprecated
* @{
*/
#define SCK_DRIVE_MODE_SHIFT       (0x00u)
#define SCK_DRIVE_MODE_MASK        (0x07u << SCK_DRIVE_MODE_SHIFT)
/** @} deprecated */

#endif /* End Pins SCK_H */


/* [] END OF FILE */
