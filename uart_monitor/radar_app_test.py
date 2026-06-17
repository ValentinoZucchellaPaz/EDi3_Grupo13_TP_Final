import struct
import random
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.widgets import Button

BUFFER_SIZE = 25
PACKET_SIZE = BUFFER_SIZE * 4


class Radar180:

    def __init__(self):

        self.radar_data = np.full(181, np.nan)

        self.fig = plt.figure(figsize=(10, 10))

        self.ax = plt.subplot(projection="polar")

        self.ax.set_theta_zero_location("W")
        self.ax.set_theta_direction(1)
        self.ax.set_thetalim(0, np.pi)

        self.ax.set_ylim(0, 100)

        self.ax.set_facecolor("black")
        self.fig.patch.set_facecolor("black")

        self.ax.grid(color="green")
        self.ax.tick_params(colors="green")
        self.ax.spines["polar"].set_color("green")

        self.scatter = self.ax.scatter(
            [],
            [],
            c="red",
            s=40
        )

        self.auto_button_ax = plt.axes([0.75, 0.05, 0.10, 0.05])
        self.manual_button_ax = plt.axes([0.86, 0.05, 0.10, 0.05])

        self.btn_auto = Button(
            self.auto_button_ax,
            "AUTO"
        )

        self.btn_manual = Button(
            self.manual_button_ax,
            "MANUAL"
        )

        self.btn_auto.on_clicked(self.set_auto)
        self.btn_manual.on_clicked(self.set_manual)

        self.angulo_actual = 0
        self.direccion = 1

    # ==========================================
    # BOTONES
    # ==========================================

    def set_auto(self, event):

        print("Modo AUTO")

        # Cuando tengas la placa:
        # self.ser.write(b'A')

    def set_manual(self, event):

        print("Modo MANUAL")

        # Cuando tengas la placa:
        # self.ser.write(b'M')

    # ==========================================
    # SIMULADOR DEL LPC
    # ==========================================

    def generar_buffer_binario(self):

        datos = bytearray()

        for _ in range(BUFFER_SIZE):

            angulo = self.angulo_actual

            # Simulamos objetos solamente en algunas zonas
            if 20 < angulo < 50:
                distancia = random.randint(15, 80)

            elif 80 < angulo < 120:
                distancia = random.randint(20, 60)

            elif 140 < angulo < 170:
                distancia = random.randint(10, 90)

            else:
                distancia = random.randint(150, 450)

            datos += struct.pack(
                "<HH",
                angulo,
                distancia
            )

            self.angulo_actual += self.direccion

            if self.angulo_actual >= 180:
                self.angulo_actual = 180
                self.direccion = -1

            elif self.angulo_actual <= 0:
                self.angulo_actual = 0
                self.direccion = 1

        return bytes(datos)

    # ==========================================

    def read_packet(self):

        raw = self.generar_buffer_binario()

        if len(raw) != PACKET_SIZE:
            return

        for i in range(BUFFER_SIZE):

            offset = i * 4

            angulo, distancia = struct.unpack(
                "<HH",
                raw[offset:offset + 4]
            )

            if angulo > 180:
                continue

            if 0 < distancia <= 100:

                self.radar_data[angulo] = distancia

            else:

                self.radar_data[angulo] = np.nan

    # ==========================================

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

        if len(angles):

            puntos = np.column_stack(
                (angles, distances)
            )

            self.scatter.set_offsets(puntos)

        else:

            self.scatter.set_offsets(
                np.empty((0, 2))
            )

        self.ax.set_title(
            "RADAR 180°",
            color="lime",
            fontsize=16
        )

        return self.scatter,

    # ==========================================

    def run(self):

        self.ani = animation.FuncAnimation(
            self.fig,
            self.update,
            interval=50,
            blit=True,
            cache_frame_data=False
        )

        plt.show()


if __name__ == "__main__":

    radar = Radar180()
    radar.run()