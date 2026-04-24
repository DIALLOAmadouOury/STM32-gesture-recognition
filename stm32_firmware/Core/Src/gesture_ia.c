/**
 * @file    gesture_ia.c
 * @brief   Implémentation de l'inférence réseau de neurones pour la détection de gestes.
 * Utilise le runtime STM32 X-CUBE-AI.
 */

#include "gesture_ia.h"
#include "main.h"
#include "network.h"
#include "network_data.h"
#include <stdio.h>
#include <stdlib.h>

// --- VARIABLES X-CUBE-AI ---
static ai_handle network = AI_HANDLE_NULL;

// Buffers I/O du modèle
static ai_float ai_in_data[AI_NETWORK_IN_1_SIZE];
static ai_float ai_out_data[AI_NETWORK_OUT_1_SIZE];

// Buffer d'activation (Tensor Arena)
// Alignement mémoire sur 32 octets pour optimiser les accès du microcontrôleur
#if !defined(AI_ALIGNED)
#define AI_ALIGNED(x) __attribute__((aligned(x)))
#endif
AI_ALIGNED(32) static ai_u8 activations_buffer[AI_NETWORK_DATA_ACTIVATIONS_SIZE];

// Structures de liaison E/S
static ai_buffer ai_input[AI_NETWORK_IN_NUM];
static ai_buffer ai_output[AI_NETWORK_OUT_NUM];

// --- DÉPENDANCES MATÉRIELLES ---
extern I2C_HandleTypeDef hi2c2;
extern void LSM6DSL_ReadAccel(I2C_HandleTypeDef *hi2c, float *x_g, float *y_g, float *z_g);

// Dictionnaire des classes (Ordre strict issu de l'entraînement)
const char* GESTURE_LABELS[] = {"BRUIT", "L", "O", "Z"};

// --- INITIALISATION DU MODÈLE ---
void Gesture_Init(void)
{
    ai_error err;
    ai_network_params params;

    // 1. Instanciation du réseau
    err = ai_network_create(&network, AI_NETWORK_DATA_CONFIG);
    if (err.type != AI_ERROR_NONE) {
        printf("[ERREUR] Impossible d'instancier le modele IA.\r\n");
        return;
    }

    // 2. Récupération des paramètres de poids
    if (ai_network_data_params_get(&params) == false) {
        printf("[ERREUR] Parametres du modele introuvables.\r\n");
        return;
    }

    // 3. Allocation du buffer d'activation au runtime
    params.map_activations.buffer[0].data = AI_HANDLE_PTR(activations_buffer);

    // 4. Initialisation du réseau et liaison des buffers I/O
    if (ai_network_init(network, &params)) {
        ai_input[0] = ai_network_inputs_get(network, NULL)[0];
        ai_output[0] = ai_network_outputs_get(network, NULL)[0];
        ai_input[0].data = AI_HANDLE_PTR(ai_in_data);
        ai_output[0].data = AI_HANDLE_PTR(ai_out_data);

        printf("[INFO] Modele IA initialise avec succes. Pret pour l'inference.\r\n");
    } else {
        err = ai_network_get_error(network);
        printf("[ERREUR] Echec Init IA : Type %d, Code %d\r\n", err.type, err.code);
    }
}

// --- TÂCHE PRINCIPALE D'ACQUISITION ET D'INFÉRENCE ---
void Gesture_Task(void)
{
    // Déclencheur : Bouton utilisateur (PC13)
    if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET)
    {
        // Délai de synchronisation avant début du mouvement
        HAL_Delay(500);
        printf("START\r\n");

        // 1. ACQUISITION DU SIGNAL (Fenêtre de 2s à 50Hz)
        for (int i = 0; i < 100; i++) {
            uint32_t tick_start = HAL_GetTick(); // Capture du timestamp initial
            float x, y, z;

            // Lecture du capteur inertiel via I2C
            LSM6DSL_ReadAccel(&hi2c2, &x, &y, &z);

            // Remplissage du tenseur d'entrée
            // Normalisation linéaire (échelle de l'entraînement : / 2.0f)
            ai_in_data[i * 3 + 0] = (ai_float)(x / 2.0f);
            ai_in_data[i * 3 + 1] = (ai_float)(y / 2.0f);
            ai_in_data[i * 3 + 2] = (ai_float)(z / 2.0f);

            // Boucle bloquante pour garantir un ODR (Output Data Rate) strict de 50Hz (20ms)
            // Compense la variabilité du temps de lecture I2C
            while((HAL_GetTick() - tick_start) < 20);
        }

        printf("END_ACQ\r\n");

        // 2. EXÉCUTION DE L'INFÉRENCE
        if (ai_network_run(network, &ai_input[0], &ai_output[0]) > 0)
        {
            float max_score = 0;
            int best_id = 0;

            // Analyse de la couche Softmax pour extraire la prédiction dominante
            for (int j = 0; j < 4; j++) {
                if (ai_out_data[j] > max_score) {
                    max_score = ai_out_data[j];
                    best_id = j;
                }
            }

            // 3. TRANSMISSION DU RÉSULTAT
            printf("RES:%s:%.2f\r\n", GESTURE_LABELS[best_id], max_score * 100.0f);
        }

        // Anti-rebond et temporisation avant la prochaine capture
        HAL_Delay(1000);
    }
}
