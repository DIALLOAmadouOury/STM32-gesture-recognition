/**
 * @file    gesture.h
 * @brief   En-tête du module d'acquisition de données inertielles (Dataset).
 * Définit les paramètres d'échantillonnage et expose la fonction d'enregistrement.
 */

#ifndef INC_GESTURE_H_
#define INC_GESTURE_H_

#include "stm32l4xx_hal.h"
#include "lsm6dsl.h" // Nécessaire pour la lecture du capteur

// --- Paramètres d'acquisition ---
#define NB_ECHANTILLONS 100      // Nombre de points par geste (fenêtre temporelle)
#define DELAI_ECHANTILLON_MS 20  // Période de 20ms = Fréquence stricte de 50Hz

/**
 * @brief Surveille le bouton utilisateur (PC13) et déclenche une acquisition.
 * Les données brutes (X, Y, Z) sont formatées en CSV et envoyées via UART.
 * * @param hi2c Pointeur vers le handle I2C utilisé par le capteur LSM6DSL.
 */
void GESTURE_RecordDataset(I2C_HandleTypeDef *hi2c);

#endif /* INC_GESTURE_H_ */
