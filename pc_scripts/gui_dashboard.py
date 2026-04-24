"""
================================================================================
PROJET : Gesture_IA - Dashboard Temps Réel (TinyML)
FICHIER : gui_dashboard.py
DESCRIPTION : Interface graphique plein écran (Tkinter) pour la visualisation
              des résultats d'inférence envoyés par la carte STM32 via UART.
AUTEUR : AIO
DATE : Avril 2026
================================================================================
"""

import serial
import serial.tools.list_ports
import tkinter as tk
from threading import Thread
import time
import sys

# --- CONFIGURATION MATÉRIELLE ---
# Note : Modifier PORT_COM selon l'attribution de votre système (ex: 'COM3' ou '/dev/ttyACM0')
PORT_COM = "COM4"  
BAUD_RATE = 115200

class GestureApp:
    """
    Classe principale gérant l'interface utilisateur et la communication série.
    Utilise le threading pour éviter le gel de l'interface pendant la lecture UART.
    """
    def __init__(self, root):
        self.root = root
        self.root.title("Détecteur de Gestes Edge AI - STM32")
        
        # --- CONFIGURATION DU MODE PLEIN ÉCRAN ---
        self.root.attributes('-fullscreen', True)
        self.root.configure(bg='#2c3e50') # Fond bleu nuit professionnel
        
        # Liaison de la touche Échap pour quitter l'application proprement
        self.root.bind("<Escape>", lambda e: self.on_closing())

        # --- ÉLÉMENTS GRAPHIQUES (UI) ---
        
        # Label de statut (indique l'état de la connexion ou de l'analyse)
        self.label_status = tk.Label(root, text="Initialisation du système...", font=("Helvetica", 18), 
                                     bg='#2c3e50', fg='#7f8c8d')
        self.label_status.pack(pady=(50, 0))

        # Affichage principal du geste détecté (Zone centrale)
        self.label_geste = tk.Label(root, text="EN ATTENTE", font=("Helvetica", 120, "bold"), 
                                    bg='#2c3e50', fg='#ecf0f1')
        self.label_geste.pack(expand=True)

        # Affichage du score de confiance renvoyé par le modèle TinyML
        self.label_score = tk.Label(root, text="Prêt pour acquisition", font=("Helvetica", 24), 
                                    bg='#2c3e50', fg='#bdc3c7')
        self.label_score.pack(pady=50)

        # Dictionnaire de couleurs pour une identification visuelle rapide des gestes
        self.colors = {
            "Z": "#3498db",      # Bleu
            "L": "#2ecc71",      # Vert
            "O": "#f1c40f",      # Jaune
            "BRUIT": "#e74c3c"   # Rouge
        }

        # --- GESTION DU FIL D'EXÉCUTION (THREADING) ---
        self.running = True
        # On lance la lecture série en arrière-plan (daemon=True pour s'arrêter avec l'appli)
        self.serial_thread = Thread(target=self.read_serial_loop, daemon=True)
        self.serial_thread.start()

    def read_serial_loop(self):
        """
        Boucle de lecture du port série. 
        Parse les messages envoyés par le firmware STM32.
        """
        ser = None
        try:
            # Ouverture de la communication série
            ser = serial.Serial(PORT_COM, BAUD_RATE, timeout=0.1)
            self.root.after(0, lambda: self.label_status.config(text=f"Connecté sur {PORT_COM}"))
            
            while self.running:
                if ser.in_waiting > 0:
                    # Lecture d'une ligne et décodage UTF-8
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if not line: continue

                    # LOGIQUE DE RÉCEPTION DES TRAMES
                    if line == "START":
                        # Événement : L'utilisateur a pressé le bouton, début de l'acquisition
                        self.root.after(0, self.ui_on_start)
                        
                    elif line.startswith("RES:"):
                        # Événement : L'inférence est terminée, on reçoit le résultat (RES:CLASSE:SCORE)
                        parts = line.split(":")
                        if len(parts) == 3:
                            self.update_ui_result(parts[1], parts[2])

                time.sleep(0.01) # Optimisation CPU
        except Exception as e:
            # Affichage de l'erreur sur l'UI en cas de déconnexion
            self.root.after(0, lambda: self.label_status.config(text=f"Erreur Matérielle : {e}"))
        finally:
            if ser and ser.is_open: 
                ser.close()
                print("[INFO] Port série libéré.")

    def ui_on_start(self):
        """
        Met à jour l'UI lors du déclenchement de l'acquisition matérielle.
        """
        self.label_geste.config(text="...", fg="#95a5a6") # Symbolise l'attente pendant le mouvement
        self.label_status.config(text="ACQUISITION ET ANALYSE EN COURS")
        self.label_score.config(text="Réalisation du mouvement...")

    def update_ui_result(self, geste, score):
        """
        Affiche le résultat final de l'IA.
        :param geste: Nom de la classe prédite (Z, L, O, etc.)
        :param score: Score de probabilité (0-100)
        """
        color = self.colors.get(geste, "#ecf0f1")
        self.label_status.config(text="INFÉRENCE TERMINÉE")
        self.label_geste.config(text=geste, fg=color)
        self.label_score.config(text=f"Confiance : {float(score):.1f} %")

    def on_closing(self):
        """
        Procédure de fermeture sécurisée de l'application.
        """
        print("[INFO] Fermeture de l'interface...")
        self.running = False
        self.root.destroy()
        sys.exit()

# --- POINT D'ENTRÉE DU SCRIPT ---
if __name__ == "__main__":
    root = tk.Tk()
    app = GestureApp(root)
    # Gestion de la fermeture via la croix de la fenêtre
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()
