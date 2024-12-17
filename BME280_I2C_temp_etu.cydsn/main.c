/* ===========================================
*
* Auteur : LOMBARD Ch.
* Version : 1.0
* Date : 2/11/2023
*
* Mesure de Température, pression 
* Capteur : BMP280
* Communication I2C
*
* ===========================================
*/
#include <project.h>
#include <stdint.h> 
#include <stdio.h> 
#include <math.h> 
#include "BME280.h"
 
    uint8_t pression[3]={0},temperature[3]={0};     // Tableaux température et pression
    char bufT1[20]={0},bufT2[20]={0},bufP1[20]={0},bufP2[20]={0};
    int32_t Temperature=0,Pression=0;
    int32_t Temperature_compense, Pression_compense;
 
CY_ISR(Timer_ISR)
{
        /************ Lecture de la température ************/ 
        temperature[0]=BME280_ReadReg( ADR_TEMP_MSB );        
        temperature[1]=BME280_ReadReg( ADR_TEMP_LSB );
        temperature[2]=BME280_ReadReg( ADR_TEMP_XLSB );
        /*********** mise en forme sur 20 bits *************/
        /* =====================================================================
        Temperature : variable qui doit contenir l'info. Temperature sur 20bits        
        code à compléter ici
        =======================================================================*/

        // Combiner les valeurs pour obtenir un nombre de 20 bits
        int32_t temperature_20 = ((int32_t)temperature[0] << 12) | ((int32_t)temperature[1] << 4) | ((int32_t)(temperature[2] >> 4));
 
        /*********** Compensation *************************/
        Temperature_compense= BME280_compensate_T_int32(temperature_20); 
        /************* lecture de la pression **************/
        pression[0]=BME280_ReadReg( ADR_PRESSION_MSB );
        pression[1]=BME280_ReadReg( ADR_PRESSION_LSB );
        pression[2]=BME280_ReadReg( ADR_PRESSION_XLSB );
        /*********** mise en forme sur 20 bits *************/
        /* =====================================================================
        Pression : variable qui doit contenir l'info. pression sur 20bits        
        code à compléter ici
        =======================================================================*/
         // Combiner les valeurs pour obtenir un nombre de 20 bits
        int32_t pression_20 = ((int32_t)pression[0] << 12) | ((int32_t)pression[1] << 4) | ((int32_t)(pression[2] >> 4));
 
        
        /*********** Compensation *************************/
        Pression_compense=BME280_compensate_P_int64(pression_20);
        /************ Calcul approché de l'altitude *******/
       /* conversion en chaine de caractères pour l'affichage sur terminal (teraterm)**/
        sprintf(bufT1, "Temp : %ld",Temperature_compense/100);
        sprintf(bufT2, "%ld",Temperature_compense%100);
        sprintf(bufP1, "Pression : %ld",Pression_compense/100);
        sprintf(bufP2, "%ld",Pression_compense%100);
        /**** Envoi sur l'UART *****/
        UART_UartPutString(bufT1);
        UART_UartPutChar('.'); 
        UART_UartPutString(bufT2); 
        UART_UartPutString(" degC"); 
        UART_UartPutString("\t");
        UART_UartPutString(bufP1);
        UART_UartPutChar('.'); 
        UART_UartPutString(bufP2); 
        UART_UartPutString(" hPa");      
        UART_UartPutString("\n\r");
}
 
int main()
{
    CyGlobalIntEnable;                              /* Enable global interrupts */

    ISR_StartEx(Timer_ISR);

    /* Démarrage des interface UART et I2C */
    UART_Start();
    I2C_Start();

    /* BMP280 : Initialisation*/
    BME280_WriteReg(ADR_CONFIG,0x69);    
    BME280_WriteReg(ADR_CTRL_HUM,0x00);  
    BME280_WriteReg(ADR_CTRL_MEAS,0x6F);  
    UART_UartPutString(" BME280 ready : \n\r");
    Timer_Start();
    for(;;)
    {    
    }   
}