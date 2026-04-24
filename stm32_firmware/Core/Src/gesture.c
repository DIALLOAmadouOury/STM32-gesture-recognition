/**
 * @file    gesture_dataset.c
 * @brief   Module d'acquisition de données inertielles pour la création du dataset.
 * Capture une fenêtre temporelle stricte à 50Hz via le capteur LSM6DSL.
 */

#include "gesture.h"
#include "main.h"
#include <stdio.h>

// Remplace ceci par le header exact de ton capteur si nécessaire
extern void LSM6DSL_ReadAccel(I2C_HandleTypeDef *hi2c, float *x_g, float *y_g, float *z_g);

/**
 * @brief Surveille le bouton utilisateur et déclenche une acquisition de 2 secondes.
 * Les données (X, Y, Z) sont transmises via UART au format CSV.
 * @param hi2c Pointeur vers le handle I2C utilisé par le capteur.
 */
void GESTURE_RecordDataset(I2C_HandleTypeDef *hi2c)
{
    // Déclencheur : Bouton utilisateur (PC13)
    if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET)
    {
        float x_g, y_g, z_g;
        uint32_t tick_start;

        // Délai de synchronisation pour stabiliser la main avant le mouvement
        HAL_Delay(500);

        // Signal de début d'acquisition pour le script Python (Datalogger)
        printf("START\r\n");

        // --- ACQUISITION STRICTE À 50 HZ ---
        // Fenêtre de 2 secondes = 100 échantillons
        for (int i = 0; i < 100; i++) {

            // Capture du timestamp initial pour garantir l'ODR (Output Data Rate)
            tick_start = HAL_GetTick();

            // Lecture du capteur inertiel
            LSM6DSL_ReadAccel(hi2c, &x_g, &y_g, &z_g);

            // Transmission UART au format CSV
            printf("%.3f,%.3f,%.3f\r\n", x_g, y_g, z_g);

            // Boucle bloquante compensant le temps d'exécution de la lecture I2C et du printf.
            // Garantit un espacement exact de 20 ms (50 Hz) entre chaque échantillon.
            while ((HAL_GetTick() - tick_start) < 20);
        }

        // Signal de fin d'acquisition
        printf("END\r\n");

        // Anti-rebond logiciel pour éviter les déclenchements multiples
        HAL_Delay(1000);
    }
}
