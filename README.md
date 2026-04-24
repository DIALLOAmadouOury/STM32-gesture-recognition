# 🚀 Gesture_IA – Reconnaissance de gestes sur STM32 (TinyML)

Ce projet implémente une solution complète de **TinyML** (IA embarquée) pour la détection et la classification de gestes dynamiques (lettres **Z**, **L**, **O**) et la distinction du **BRUIT**. Le modèle s'exécute en temps réel sur un microcontrôleur STM32 sans aucune dépendance externe (Cloud ou PC).

---

## 💡 Principe de fonctionnement

Le système capture le mouvement via un accéléromètre, traite la fenêtre temporelle, puis prédit le geste à l'aide d'un réseau de neurones compressé.

**Chaîne de traitement :**
`Accéléromètre (LSM6DSL)` ➡️ `Prétraitement (Normalisation)` ➡️ `Inférence (MLP INT8)` ➡️ `UART` ➡️ `Dashboard Python`

---

## 🛠️ Stack technique

- **Microcontrôleur :** STM32 B-L475E-IOT01A (Cortex-M4 @80MHz)
- **Capteur :** LSM6DSL (accéléromètre 3 axes, liaison I2C)
- **Machine Learning :** TensorFlow Lite / Keras (Entraînement sous Jupyter)
- **Déploiement :** X-CUBE-AI (Conversion et optimisation INT8)
- **Interface :** Python (PySerial & Tkinter)

---

## 📊 Modèle & Dataset

- **Architecture :** MLP (Multi-Layer Perceptron) optimisé : 300 entrées ➡️ 16 neurones ➡️ 8 neurones ➡️ 4 sorties.
- **Données :** Dataset propriétaire de **120 enregistrements** (30 par classe).
- **Acquisition :** Fenêtre de 2 secondes à 50 Hz (100 échantillons par axe).
- **Performance :** Précision de validation de **95,83 %**.

---

## ⚡ Contraintes & Optimisations Edge AI

Grâce à la quantification post-entraînement (INT8), le modèle présente une empreinte extrêmement réduite :

- **Flash (Stockage) :** 30,08 Ko (Incluant le moteur de calcul X-CUBE-AI).
- **RAM (Calcul) :** 3,31 Ko.
- **Vitesse :** Temps d'inférence **< 2 ms** sur la cible STM32L4.

---

## 📂 Structure du projet

- `stm32_firmware/` : Code source C (Core, Pilotes I2C, Logique d'acquisition, Inférence).
- `model_training/` : Notebook Jupyter (`.ipynb`), Dataset et modèle quantifié (`.tflite`).
- `pc_scripts/` : Logiciels utilitaires (Datalogger pour création de dataset et GUI Dashboard).

---

## 🚀 Utilisation rapide

1. **Firmware :** Flasher le code situé dans `stm32_firmware/` via STM32CubeIDE.
2. **Dashboard :** Lancer l'interface graphique sur votre PC :
   ```bash
   pip install pyserial
   python pc_scripts/gui_dashboard.py
