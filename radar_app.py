import serial
import struct
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation

from matplotlib.widgets import Button

# ==========================================================
# CONFIG
# ==========================================================

PORT = "COM13"
BAUDRATE = 115200

BUFFER_SIZE = 25
BYTES_PER_SAMPLE = 4
PACKET_SIZE = BUFFER_SIZE * BYTES_PER_SAMPLE

# ==========================================================
# RADAR
# ==========================================================

class Radar180:

    def __init__(self):

        # --------------------------------------
        # UART
        # --------------------------------------

        try:

            self.ser = serial.Serial(
                PORT,
                BAUDRATE,
                timeout=0.05
            )

            self.connected = True

            print(f"Conectado a {PORT}")

        except:

            self.connected = False
            self.ser = None

            print("No se pudo abrir puerto serie")

        # --------------------------------------
        # Datos radar
        # --------------------------------------

        self.radar_data = np.full(181, np.nan)

        self.current_angle = 0

        self.mode = "AUTO"

        # --------------------------------------
        # Figura
        # --------------------------------------

        self.fig = plt.figure(
            figsize=(11,10),
            facecolor='black'
        )

        self.ax = plt.subplot(
            projection='polar'
        )

        self.ax.set_facecolor('black')

        self.ax.set_theta_zero_location('W')
        self.ax.set_theta_direction(1)
        self.ax.set_thetalim(0, np.pi)

        self.ax.set_ylim(0,100)

        self.ax.grid(color='green')

        self.ax.tick_params(colors='green')

        self.ax.spines['polar'].set_color('green')

        # --------------------------------------
        # Puntos detectados
        # --------------------------------------

        self.scatter = self.ax.scatter(
            [],
            [],
            c='red',
            s=60
        )

        # --------------------------------------
        # Línea barrido
        # --------------------------------------

        self.sweep_line, = self.ax.plot(
            [],
            [],
            linewidth=2,
            color='lime'
        )

        # --------------------------------------
        # Información
        # --------------------------------------

        self.info_text = self.fig.text(
            0.02,
            0.95,
            "",
            color="lime",
            fontsize=11
        )

        # --------------------------------------
        # Botón AUTO
        # --------------------------------------

        ax_auto = plt.axes(
            [0.75, 0.02, 0.10, 0.05]
        )

        self.btn_auto = Button(
            ax_auto,
            "AUTO"
        )

        self.btn_auto.on_clicked(
            self.set_auto
        )

        # --------------------------------------
        # Botón MANUAL
        # --------------------------------------

        ax_manual = plt.axes(
            [0.86, 0.02, 0.10, 0.05]
        )

        self.btn_manual = Button(
            ax_manual,
            "MANUAL"
        )

        self.btn_manual.on_clicked(
            self.set_manual
        )

    # ======================================================

    def set_auto(self, event):

        self.mode = "AUTO"

        if self.connected:

            try:
                self.ser.write(b'A')
            except:
                pass

        print("Modo AUTO")

    # ======================================================

    def set_manual(self, event):

        self.mode = "MANUAL"

        if self.connected:

            try:
                self.ser.write(b'M')
            except:
                pass

        print("Modo MANUAL")

    # ======================================================

    def read_packet(self):

        if not self.connected:
            return

        data = self.ser.read(
            PACKET_SIZE
        )

        if len(data) != PACKET_SIZE:
            return

        last_angle = self.current_angle

        for i in range(BUFFER_SIZE):

            offset = i * 4

            angulo, distancia = struct.unpack(
                '<HH',
                data[offset:offset+4]
            )

            if angulo > 180:
                continue

            last_angle = angulo

            if 0 < distancia <= 100:

                self.radar_data[angulo] = distancia

            else:

                self.radar_data[angulo] = np.nan

        self.current_angle = last_angle

    # ======================================================

    def update(self, frame):

        self.read_packet()

        angles = []
        distances = []

        for angulo in range(181):

            distancia = self.radar_data[angulo]

            if not np.isnan(distancia):

                angles.append(
                    np.radians(angulo)
                )

                distances.append(
                    distancia
                )

        # ----------------------------------
        # Objetos
        # ----------------------------------

        if len(angles):

            points = np.column_stack(
                (angles, distances)
            )

            self.scatter.set_offsets(
                points
            )

        else:

            self.scatter.set_offsets(
                np.empty((0,2))
            )

        # ----------------------------------
        # Línea de barrido
        # ----------------------------------

        sweep_angle = np.radians(
            self.current_angle
        )

        self.sweep_line.set_data(
            [sweep_angle, sweep_angle],
            [0, 100]
        )

        # ----------------------------------
        # Info
        # ----------------------------------

        objetos = np.count_nonzero(
            ~np.isnan(self.radar_data)
        )

        estado = (
            "CONECTADO"
            if self.connected
            else
            "SIN UART"
        )

        self.info_text.set_text(
            f"Estado: {estado}\n"
            f"Modo: {self.mode}\n"
            f"Angulo Servo: {self.current_angle}°\n"
            f"Objetos Detectados: {objetos}"
        )

        self.ax.set_title(
            "RADAR 180°",
            color='lime',
            fontsize=16
        )

        return (
            self.scatter,
            self.sweep_line
        )

    # ======================================================

    def run(self):

        animation.FuncAnimation(
            self.fig,
            self.update,
            interval=50,
            blit=False,
            cache_frame_data=False
        )

        plt.show()

# ==========================================================
# MAIN
# ==========================================================

if __name__ == "__main__":

    radar = Radar180()

    radar.run()