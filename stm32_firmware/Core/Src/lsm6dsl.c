/**
 * @file    lsm6dsl.c
 * @brief   Pilote matériel pour la centrale inertielle (IMU) LSM6DSL via I2C.
 * Gère l'initialisation des registres et l'extraction/conversion des données brutes.
 */

#include "lsm6dsl.h"

// Sensibilité de l'accéléromètre pour la pleine échelle ±2g (issue de la datasheet)
// Valeur : 0.061 mg/LSB
#define LSM6DSL_SENSITIVITY_2G (0.061f / 1000.0f)

/**
 * @brief Initialise le capteur LSM6DSL sur le bus I2C.
 * Vérifie l'identifiant du composant (Device ID) et configure le registre
 * de contrôle pour un ODR (Output Data Rate) de 104 Hz à ±2g.
 * @param hi2c Pointeur vers le handle I2C utilisé.
 * @retval 1 si succès, 0 si erreur de communication ou capteur non reconnu.
 */
uint8_t LSM6DSL_Init(I2C_HandleTypeDef *hi2c) {
    uint8_t valeur_lue = 0;

    // 1. Vérification de l'identité du composant (WHO_AM_I)
    // Le registre doit retourner 0x6A selon la datasheet du LSM6DSL
    HAL_I2C_Mem_Read(hi2c, LSM6DSL_ADDR, REG_WHO_AM_I, I2C_MEMADD_SIZE_8BIT, &valeur_lue, 1, 100);
    if (valeur_lue != 0x6A) {
        return 0; // Erreur : capteur introuvable ou mauvais composant
    }

    // 2. Configuration de l'accéléromètre (Registre CTRL1_XL)
    // Valeur 0x40 -> ODR = 104 Hz, Pleine échelle = ±2g, Filtre anti-repliement par défaut
    uint8_t config_accel = 0x40;
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(hi2c, LSM6DSL_ADDR, REG_CTRL1_XL, I2C_MEMADD_SIZE_8BIT, &config_accel, 1, 100);

    return (status == HAL_OK) ? 1 : 0;
}

/**
 * @brief Lit les données brutes d'accélération et les convertit en unité 'g'.
 * @param hi2c Pointeur vers le handle I2C.
 * @param x_g Pointeur pour stocker l'accélération convertie sur l'axe X.
 * @param y_g Pointeur pour stocker l'accélération convertie sur l'axe Y.
 * @param z_g Pointeur pour stocker l'accélération convertie sur l'axe Z.
 */
void LSM6DSL_ReadAccel(I2C_HandleTypeDef *hi2c, float *x_g, float *y_g, float *z_g) {
    uint8_t data[6];
    int16_t x_raw, y_raw, z_raw;

    // Lecture I2C en rafale (burst read) des 6 registres de sortie consécutifs
    HAL_I2C_Mem_Read(hi2c, LSM6DSL_ADDR, REG_OUTX_L_XL, I2C_MEMADD_SIZE_8BIT, data, 6, 100);

    // Concaténation des octets LSB et MSB (Complément à deux, 16 bits)
    x_raw = (int16_t)((data[1] << 8) | data[0]);
    y_raw = (int16_t)((data[3] << 8) | data[2]);
    z_raw = (int16_t)((data[5] << 8) | data[4]);

    // Conversion des valeurs brutes en flottants (g) en appliquant la sensibilité matérielle
    *x_g = (float)x_raw * LSM6DSL_SENSITIVITY_2G;
    *y_g = (float)y_raw * LSM6DSL_SENSITIVITY_2G;
    *z_g = (float)z_raw * LSM6DSL_SENSITIVITY_2G;
}
