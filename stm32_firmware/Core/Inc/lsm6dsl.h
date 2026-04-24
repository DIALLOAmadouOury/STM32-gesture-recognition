/**
 * @file    lsm6dsl.h
 * @brief   En-tête du pilote matériel pour le capteur inertiel LSM6DSL (IMU).
 * Définit les adresses I2C, les registres et expose les fonctions de contrôle.
 */

#ifndef INC_LSM6DSL_H_
#define INC_LSM6DSL_H_

#include "stm32l4xx_hal.h"

// --- Adresses matérielles et registres I2C ---
#define LSM6DSL_ADDR         0xD4  // Adresse I2C du composant
#define REG_WHO_AM_I         0x0F  // Registre d'identification (Valeur attendue : 0x6A)
#define REG_CTRL1_XL         0x10  // Registre de contrôle de l'accéléromètre
#define REG_OUTX_L_XL        0x28  // Premier registre de données (Axe X, LSB)

// --- Prototypes des fonctions publiques ---

/**
 * @brief Initialise le capteur LSM6DSL sur le bus I2C spécifié.
 * @param hi2c Pointeur vers l'instance du bus I2C connecté au capteur.
 * @retval 1 si succès, 0 si erreur de communication ou composant non reconnu.
 */
uint8_t LSM6DSL_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief Lit l'accélération sur les 3 axes (rafale I2C) et applique la conversion.
 * @param hi2c Pointeur vers l'instance du bus I2C.
 * @param x_g Pointeur pour récupérer l'accélération sur l'axe X (en unité g).
 * @param y_g Pointeur pour récupérer l'accélération sur l'axe Y (en unité g).
 * @param z_g Pointeur pour récupérer l'accélération sur l'axe Z (en unité g).
 */
void LSM6DSL_ReadAccel(I2C_HandleTypeDef *hi2c, float *x_g, float *y_g, float *z_g);

#endif /* INC_LSM6DSL_H_ */
