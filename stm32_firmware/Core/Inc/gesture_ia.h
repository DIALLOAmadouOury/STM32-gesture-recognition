/**
 * @file    gesture_ia.h
 * @brief   En-tête du module d'inférence de reconnaissance de gestes.
 * Expose les fonctions d'initialisation et la tâche principale.
 */

#ifndef INC_GESTURE_IA_H_
#define INC_GESTURE_IA_H_

/**
 * @brief Initialise le runtime X-CUBE-AI, alloue les buffers mémoire
 * et charge les poids du modèle quantifié en INT8.
 */
void Gesture_Init(void);

/**
 * @brief Tâche bloquante gérant l'acquisition I2C (50Hz),
 * l'exécution de l'inférence et la transmission UART du résultat.
 * Déclenchée par une interruption ou un polling sur le bouton utilisateur.
 */
void Gesture_Task(void);

#endif /* INC_GESTURE_IA_H_ */
