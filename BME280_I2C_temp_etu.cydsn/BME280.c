#include "BME280.h"
#include "project.h"

/* Fonction    : BME280_WriteReg
** But         : Ecrire une commande dans un registre
** Entrée      : WriteAddr (adresse du registre), WriteData (donnée à écrire)
** Sortie      : None
** Usage       : BME280_WriteReg(WriteAddr, WriteData);
*/
void BME280_WriteReg(uint8 WriteAddr, uint8 WriteData) {
    uint8 writeBuf[2] = {0};
    writeBuf[0] = WriteAddr;  // Adresse du registre
    writeBuf[1] = WriteData;  // Donnée à écrire dans le registre
    I2C_I2CMasterWriteBuf(ADR_SLAVE, (uint8 *)&writeBuf, sizeof(writeBuf), I2C_I2C_MODE_COMPLETE_XFER);
    while ((I2C_I2CMasterStatus() & I2C_I2C_MSTAT_WR_CMPLT) == 0) {}  // Attendre la fin de l'écriture
    I2C_I2CMasterClearStatus();  // Effacer le statut de l'I2C
}

/* Fonction  : BME280_ReadReg
** But       : Lire les informations d'un registre
** Entrée    : ReadAddr (adresse du registre)
** Sortie    : ReadData (donnée lue)
** Usage     : ReadData = BME280_ReadReg(ReadAddr);
*/
uint8_t BME280_ReadReg(uint8_t ReadAddr) {
    uint8 ReadData;
    /* =======================================  
    Code à compléter ici
    =========================================*/
    
    uint8 readBuf[2] = {0};
    readBuf[0] = ReadAddr;  // Adresse du registre à lire
    readBuf[1] = ReadData;  // Variable pour stocker la donnée lue
    
    uint8_t valeur;
    I2C_I2CMasterSendStart(ADR_SLAVE, I2C_I2C_WRITE_XFER_MODE);  // Démarrer la communication I2C en mode écriture
    I2C_I2CMasterWriteByte(ReadAddr);  // Envoyer l'adresse du registre à lire
    I2C_I2CMasterSendRestart(ADR_SLAVE, I2C_I2C_READ_XFER_MODE);  // Redémarrer la communication en mode lecture
    valeur = I2C_I2CMasterReadByte(I2C_I2C_NAK_DATA);  // Lire la donnée du registre

    I2C_I2CMasterSendStop();  // Arrêter la communication I2C
    return valeur;  // Retourner la valeur lue
}

/* =========================================================
 ** Routines établies à partir de la datasheet du composant
* ==========================================================
*/

// Retourne la température en degrés Celsius, résolution de 0.01 °C. La valeur de sortie de "5123" correspond à 51.23 °C.
// t_fine transporte la valeur de température fine comme valeur globale
int32_t BME280_compensate_T_int32(int32_t adc_T) {
    int32_t var1, var2, T;
    var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T;  // Retourner la température compensée
}

// Retourne la pression en Pa comme entier non signé 32 bits au format Q24.8 (24 bits entiers et 8 bits fractionnaires).
// La valeur de sortie de "24674867" représente 24674867/256 = 96386.2 Pa = 963.862 hPa
uint32_t BME280_compensate_P_int64(int32_t adc_P) {
    int64_t var1, var2, p;
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)dig_P6;
    var2 = var2 + ((var1 * (int64_t)dig_P5) << 17);
    var2 = var2 + (((int64_t)dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)dig_P3) >> 8) + ((var1 * (int64_t)dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)dig_P1) >> 33;
    if (var1 == 0) return 0;  // Éviter la division par zéro
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)dig_P7) << 4);
    p = p >> 8;
    return (uint32_t)p;  // Retourner la pression compensée
}

// Retourne la pression en Pa comme double. La valeur de sortie de "96386.2" correspond à 96386.2 Pa = 963.862 hPa
double BME280_compensate_P_double(int32_t adc_P) {
    double var1, var2, p;
    var1 = ((double)t_fine / 2.0) - 64000.0;
    var2 = var1 * var1 * ((double)dig_P6) / 32768.0;
    var2 = var2 + var1 * ((double)dig_P5) * 2.0;
    var1 = (((double)dig_P3) * var1 * var1 / 524288.0 + ((double)dig_P2) * var1) / 524288.0;
    var1 = (1.0 + var1 / 32768.0) * ((double)dig_P1);
    p = 1048576.0 - (double)adc_P;
    p = (p - (var2 / 4096.0)) * 6250.0 / var1;
    var1 = ((double)dig_P9) * p * p / 2147483648.0;
    var2 = p * ((double)dig_P8) / 32768.0;
    p = p + (var1 + var2 + ((double)dig_P7)) / 16.0;
    return p;  // Retourner la pression compensée
}

/* [] END OF FILE */