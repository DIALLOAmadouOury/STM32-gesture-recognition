import customtkinter as ctk
import serial
import threading
import time
import re
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

# ==========================================
# CONFIGURATION
# ==========================================
# METS LE BON PORT COM ICI (ex: 'COM3' sur Windows, '/dev/ttyACM0' sur Mac/Linux)
PORT_COM = "COM4" 
BAUD_RATE = 115200

# Paramètres de l'interface graphique
ctk.set_appearance_mode("Dark")
ctk.set_default_color_theme("blue")

class DashboardIA(ctk.CTk):
    def __init__(self):
        super().__init__()
        self.title("Télémétrie TinyML - Détection de Gestes")
        self.geometry("800x600")

        # Variables pour les graphiques
        self.data_x, self.data_y, self.data_z = [], [], []

        self.build_ui()
        
        # Lancement du Thread de lecture USB (pour ne pas bloquer l'interface)
        self.serial_thread = threading.Thread(target=self.read_serial_data, daemon=True)
        self.serial_thread.start()

    def build_ui(self):
        # 1. ZONE DE STATUT
        self.status_label = ctk.CTkLabel(self, text="EN ATTENTE (Mode Veille)", font=("Arial", 24, "bold"), text_color="#aaaaaa")
        self.status_label.pack(pady=20)

        # 2. ZONE GRAPHIQUE (Matplotlib)
        self.fig = Figure(figsize=(8, 3), dpi=100, facecolor='#2b2b2b')
        self.ax = self.fig.add_subplot(111)
        self.ax.set_facecolor('#2b2b2b')
        self.ax.tick_params(colors='white')
        self.line_x, = self.ax.plot([], [], 'r-', label="X")
        self.line_y, = self.ax.plot([], [], 'g-', label="Y")
        self.line_z, = self.ax.plot([], [], 'b-', label="Z")
        self.ax.legend(loc="upper right")
        self.ax.set_ylim(-2.0, 2.0) # Plage de l'accéléromètre (+/- 2G)
        self.ax.set_xlim(0, 100)
        
        self.canvas = FigureCanvasTkAgg(self.fig, master=self)
        self.canvas.get_tk_widget().pack(pady=10, fill="x", padx=20)

        # 3. ZONE IA (Jauges de confiance)
        frame_ai = ctk.CTkFrame(self)
        frame_ai.pack(pady=20, fill="x", padx=40)

        ctk.CTkLabel(frame_ai, text="Score Geste 'Z' :", font=("Arial", 16)).grid(row=0, column=0, padx=10, pady=10)
        self.bar_z = ctk.CTkProgressBar(frame_ai, width=400, progress_color="#2ecc71")
        self.bar_z.grid(row=0, column=1, padx=10, pady=10)
        self.bar_z.set(0)
        self.lbl_score_z = ctk.CTkLabel(frame_ai, text="0 %", font=("Arial", 16, "bold"))
        self.lbl_score_z.grid(row=0, column=2, padx=10, pady=10)

        ctk.CTkLabel(frame_ai, text="Score Bruit :", font=("Arial", 16)).grid(row=1, column=0, padx=10, pady=10)
        self.bar_bruit = ctk.CTkProgressBar(frame_ai, width=400, progress_color="#e74c3c")
        self.bar_bruit.grid(row=1, column=1, padx=10, pady=10)
        self.bar_bruit.set(0)
        self.lbl_score_bruit = ctk.CTkLabel(frame_ai, text="0 %", font=("Arial", 16, "bold"))
        self.lbl_score_bruit.grid(row=1, column=2, padx=10, pady=10)

    def update_graph(self):
        self.line_x.set_data(range(len(self.data_x)), self.data_x)
        self.line_y.set_data(range(len(self.data_y)), self.data_y)
        self.line_z.set_data(range(len(self.data_z)), self.data_z)
        self.canvas.draw()

    def read_serial_data(self):
        try:
            ser = serial.Serial(PORT_COM, BAUD_RATE, timeout=1)
            print(f"✅ Connecté à la carte sur {PORT_COM}")
            
            while True:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if not line: continue

                # Détection d'état : Le bouton a été appuyé !
                if "ENREGISTREMENT" in line:
                    self.status_label.configure(text="🔴 ENREGISTREMENT (2s)...", text_color="#e74c3c")
                    self.data_x, self.data_y, self.data_z = [], [], [] # On vide le graphique
                    
                # Détection d'état : L'IA réfléchit
                elif "Analyse" in line:
                    self.status_label.configure(text="⚙️ INFÉRENCE IA...", text_color="#f39c12")
                    self.update_graph() # On dessine la courbe finale
                    
                # Si on reçoit des données X, Y, Z brutes (ex: "0.12,-0.45,1.02")
                elif len(line.split(',')) == 3:
                    try:
                        x, y, z = map(float, line.split(','))
                        self.data_x.append(x)
                        self.data_y.append(y)
                        self.data_z.append(z)
                        # Pour fluidifier, on pourrait updater le graph en direct ici, 
                        # mais pour la perf on le fait à la fin de l'enregistrement.
                    except: pass

                # Lecture du Score IA
                elif "Score Z :" in line:
                    # On extrait les chiffres avec une regex (ex: Score Z : 98.5 %)
                    scores = re.findall(r"[-+]?\d*\.\d+|\d+", line)
                    if len(scores) >= 2:
                        score_z = float(scores[0])
                        score_bruit = float(scores[1])
                        
                        # Mise à jour des jauges (0.0 à 1.0)
                        self.bar_z.set(score_z / 100.0)
                        self.lbl_score_z.configure(text=f"{score_z} %")
                        self.bar_bruit.set(score_bruit / 100.0)
                        self.lbl_score_bruit.configure(text=f"{score_bruit} %")

                        if score_z > 60.0:
                            self.status_label.configure(text="🚀 GESTE 'Z' RECONNU !", text_color="#2ecc71")
                        else:
                            self.status_label.configure(text="❌ MOUVEMENT INCONNU", text_color="#e74c3c")
                            
                        # Retour à l'état de veille après 3 secondes
                        time.sleep(3)
                        self.status_label.configure(text="EN ATTENTE (Mode Veille)", text_color="#aaaaaa")

        except Exception as e:
            print(f"❌ Erreur USB : {e}")

if __name__ == "__main__":
    app = DashboardIA()
    app.mainloop()
