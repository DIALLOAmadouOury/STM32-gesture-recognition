# Reconnaissance de gestes sur STM32 — TinyML

Reconnaissance de gestes dynamiques **L**, **O** et **Z**, avec une quatrième classe **BRUIT**, à partir des mesures d'accélération d'une carte STM32. L'acquisition, le prétraitement et l'inférence sont réalisés sur le microcontrôleur ; le PC sert uniquement à afficher les résultats par liaison série.

**Technologies :** C, STM32L475, HAL / I²C / UART, LSM6DSL, TensorFlow/Keras, TensorFlow Lite, STM32 X-CUBE-AI et Python.

[Voir la démonstration vidéo](assets/video_demo.mp4) · [Consulter le notebook d'entraînement](model_training/Copie%20de%20gesture_ML.ipynb) · [Voir l'interface Python](pc_scripts/gui_dashboard.py)

## Fonctionnement

```text
Bouton utilisateur (PC13)
          |
          v
Accéléromètre LSM6DSL -- I²C2 --> Acquisition de 100 mesures (X, Y, Z)
                                      |
                                      v
                              Normalisation / 2,0
                                      |
                                      v
                           Réseau de neurones embarqué
                             300 → 16 → 8 → 4
                                      |
                                      v
                          Classe prédite + score
                                      |
                                      v
                            UART1 (115 200 bauds)
                                      |
                                      v
                        Interface Python / Tkinter
```

Une pression sur le bouton utilisateur déclenche une courte attente, puis une acquisition d'environ **2 secondes**. Le firmware espace les lectures par une boucle basée sur `HAL_GetTick()`, avec une cible de **20 ms par mesure (50 Hz)**. Il normalise les valeurs `ax`, `ay` et `az` en les divisant par `2.0`, remplit les 300 entrées du réseau et exécute l'inférence avec X-CUBE-AI.

Le capteur LSM6DSL est configuré pour une pleine échelle de **±2 g** et un débit interne de **104 Hz**. La cadence de lecture applicative ciblée est différente de ce réglage du capteur. La régularité temporelle effective n'est pas mesurée dans les sources publiées.

Le résultat est transmis sur UART au format `RES:CLASSE:SCORE`, par exemple `RES:Z:92.50`. Les classes sont ordonnées ainsi : `BRUIT`, `L`, `O`, `Z`. Le score est celui de la classe prédite, et non une précision globale de classification.

## Architecture logicielle

| Élément | Rôle | Code |
| --- | --- | --- |
| STM32L475 / HAL | Initialisation I²C2, UART1 et bouton utilisateur | [`main.c`](stm32_firmware/Core/Src/main.c) |
| Pilote LSM6DSL | Identification du capteur, configuration et conversion des accélérations en g | [`lsm6dsl.c`](stm32_firmware/Core/Src/lsm6dsl.c) |
| Acquisition et inférence | Échantillonnage, normalisation, exécution du réseau et envoi du résultat | [`gesture_ia.c`](stm32_firmware/Core/Src/gesture_ia.c) |
| Acquisition de données | Module distinct pour enregistrer une fenêtre de données par UART | [`gesture.c`](stm32_firmware/Core/Src/gesture.c) |
| Réseau embarqué | Fichiers générés par STM32 X-CUBE-AI | [`X-CUBE-AI/App/`](stm32_firmware/X-CUBE-AI/App/) |
| Supervision sur PC | Lecture série et affichage de la classe et du score | [`gui_dashboard.py`](pc_scripts/gui_dashboard.py) |

Le firmware utilise une **boucle principale déclenchée par scrutation du bouton**, et non FreeRTOS. Le module `gesture.c` constitue un outil d'acquisition distinct : la boucle principale appelle `Gesture_Task()`.

## Modèle et entraînement

Le notebook [`model_training/Copie de gesture_ML.ipynb`](model_training/Copie%20de%20gesture_ML.ipynb) documente l'entraînement du classifieur :

- **4 classes :** BRUIT, L, O et Z ;
- **120 mouvements** chargés lors de l'exécution enregistrée dans le notebook, soit **30 par classe** ;
- **100 échantillons × 3 axes** par mouvement, normalisés par `2.0` ;
- **MLP 300 → 16 → 8 → 4** avec ReLU, dropout pendant l'entraînement et sortie softmax ;
- export dans [`gesture_model_quantized.tflite`](model_training/gesture_model_quantized.tflite).

Le notebook applique une optimisation TensorFlow Lite (`tf.lite.Optimize.DEFAULT`) et produit un fichier TFLite d'environ **8,6 Kio**. Cependant, les fichiers C actuellement publiés par X-CUBE-AI déclarent des entrées/sorties **Float32** et utilisent des poids en **Float32** : la présence d'un TFLite optimisé ne démontre pas que l'inférence embarquée utilise exclusivement des opérations INT8.

### Limites de l'évaluation

L'exécution enregistrée dans le notebook affiche **95,83 % de précision de validation**. Ce résultat est **indicatif** : l'augmentation des données a été effectuée **avant** le découpage entraînement/validation. Des variantes du même mouvement d'origine peuvent donc figurer dans les deux ensembles et rendre l'estimation trop optimiste.

Pour obtenir une mesure fiable, il faudra séparer les mouvements originaux avant toute augmentation et évaluer le modèle sur des acquisitions indépendantes (autres sessions, variations de vitesse, d'orientation ou d'utilisateur).

Le notebook attend un fichier `data.zip` contenant les dossiers des quatre classes. **Cette archive de données brutes n'est pas incluse dans le dépôt** : l'entraînement n'est donc pas entièrement reproductible avec les seuls fichiers publiés.

## Démonstration

[**Voir la vidéo d'exécution sur STM32 et PC**](assets/video_demo.mp4)

Le script [`pc_scripts/gui_dashboard.py`](pc_scripts/gui_dashboard.py) utilise **Tkinter** et **PySerial** pour afficher le geste prédit, son score et l'état d'acquisition. Il reconnaît les messages `START` et `RES:CLASSE:SCORE` envoyés par le firmware publié.

Pour installer la dépendance Python et lancer l'interface depuis la racine du dépôt :

```bash
python -m pip install pyserial
python pc_scripts/gui_dashboard.py
```

Adapter la constante `PORT_COM = "COM4"` dans le script au port série réellement attribué à la carte. Le débit UART configuré dans le firmware est **115 200 bauds**.

Le second script [`pc_scripts/dashboard_ia.py`](pc_scripts/dashboard_ia.py) correspond à une autre version de l'interface, avec affichage des axes. Il attend des messages série différents de ceux produits par `Gesture_Task()` et n'est **pas directement compatible** avec le protocole du firmware présenté ici.

## État du dépôt

Les sources C de l'application, le pilote du capteur, le notebook, le modèle TFLite, les fichiers générés du réseau et la vidéo sont disponibles. **Ce dépôt n'est toutefois pas un projet STM32CubeIDE autonome prêt à compiler** : il manque notamment la configuration complète CubeIDE/CubeMX, les dépendances HAL et certains en-têtes générés par X-CUBE-AI. L'intégration dans un projet STM32CubeIDE approprié reste nécessaire pour reconstruire le firmware.

Les valeurs de mémoire totale du firmware et de durée d'inférence sur cible ne sont pas annoncées ici, car aucun rapport de compilation complet ni mesure instrumentée n'est fourni dans le dépôt.

### Améliorations envisagées

- Publier un projet STM32CubeIDE complet avec les versions des dépendances.
- Ajouter l'archive de données ou une procédure publique de collecte reproductible.
- Refaire l'évaluation avec une séparation des gestes avant augmentation et un jeu de test indépendant.
- Mesurer les temps d'inférence, l'utilisation Flash/RAM et la cadence réelle d'acquisition sur carte.
- Harmoniser les protocoles UART utilisés par les interfaces Python.

---

**Auteur :** Amadou Oury Diallo — projet personnel de systèmes embarqués et TinyML.
