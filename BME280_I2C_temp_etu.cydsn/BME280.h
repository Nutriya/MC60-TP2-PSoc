/* ========================================
 *
 * Fichier d'en-tête BME280.h
 * 
 * ========================================
*/
#include <stdint.h> 
// Adresses des registres contenant les données de Humidité, pression et température
#define ADR_CONFIG              0xF5       //Ajout du registre ADR_CONFIG pour corriger l'erreur de lecture rencontrée
#define ADR_RESET               0xE0
#define ADR_CTRL_HUM            0xF2
#define ADR_CTRL_MEAS           0xF4

#define ADR_PRESSION_MSB        0xF7
#define ADR_PRESSION_LSB        0xF8
#define ADR_PRESSION_XLSB       0xF9

#define ADR_TEMP_MSB            0xFA
#define ADR_TEMP_LSB            0xFB
#define ADR_TEMP_XLSB           0xFC

#define ADR_HUM_LSB             0xFD
#define ADR_HUM_MSB             0xFE

#define ADR_SLAVE               0x76    //SDO au GND
#define ID                      0xD0

#define dig_T1                  27504
#define dig_T2                  26435
#define dig_T3                  -1000

#define dig_P1                  36477
#define dig_P2                 -10685
#define dig_P3                   3024
#define dig_P4                   2855
#define dig_P5                    140
#define dig_P6                     -7
#define dig_P7                  15500
#define dig_P8                 -14600
#define dig_P9                   6000

int32_t t_fine;


// Prototypes des fonctions à utiliser
void    BME280_WriteReg( uint8_t , uint8_t  );
uint8_t BME280_ReadReg( uint8_t );
int32_t BME280_compensate_T_int32(int32_t);
uint32_t BME280_compensate_P_int64(int32_t adc_P);

/* FIN */
