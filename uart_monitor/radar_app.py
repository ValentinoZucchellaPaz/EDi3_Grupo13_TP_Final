import serial
import struct
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.widgets import Button
from collections import deque
import time

# ==========================================================
# CONFIG
# ==========================================================

PORT = "COM13"
BAUDRATE = 115200

HEADER = b'\xAA\x55'

BUFFER_SIZE = 90
BYTES_PER_SAMPLE = 4
PACKET_SIZE = BUFFER_SIZE * BYTES_PER_SAMPLE

MAX_DISTANCE = 450
MIN_DISTANCE = 5        # Para log: evitar log(0)
POINT_LIFETIME = 2.0    # segundos hasta borrar un punto automáticamente

# ==========================================================
# RADAR
# ==========================================================

class Radar180:

    def __init__(self):

        # --------------------------------------
        # UART
        # --------------------------------------
        try:
            self.ser = serial.Serial(PORT, BAUDRATE, timeout=0.1)
            self.connected = True
            print(f"Conectado a {PORT}")
        except:
            self.connected = False
            self.ser = None
            print("No se pudo abrir puerto serie")

        # --------------------------------------
        # Estado interno
        # --------------------------------------
        self.radar_data = np.full(181, np.nan)
        # Timestamp de cuándo se escribió cada ángulo (para expiración)
        self.radar_timestamps = np.full(181, -np.inf)

        # Paquete recibido completo, pendiente de animar
        # Cada elemento: (angulo, distancia)
        self.pending_samples = []
        self.anim_index = 0

        self.current_angle = 0
        self.mode = "AUTO"

        # Comando pendiente de enviar por UART (lo despacha update())
        self.pending_command = None

        # Estadísticas
        self.packets_ok = 0
        self.packets_err = 0
        self.last_packet_time = None

        # --------------------------------------
        # Figura
        # --------------------------------------
        self.fig = plt.figure(figsize=(11, 10), facecolor='black')

        self.ax = plt.subplot(projection='polar')
        self.ax.set_facecolor('black')

        self.ax.set_theta_zero_location('W')
        self.ax.set_theta_direction(1)
        self.ax.set_thetalim(0, np.pi)

        # ---- Escala logarítmica ----
        # Transformamos distancia a escala log para el eje r.
        # Usamos log10; el eje muestra los ticks en distancia real.
        self._log_ticks = [5, 10, 20, 50, 100, 200, 450]
        self._r_max_log = np.log10(MAX_DISTANCE)
        self._r_min_log = np.log10(MIN_DISTANCE)

        self.ax.set_ylim(self._r_min_log, self._r_max_log)
        self.ax.set_yticks([np.log10(d) for d in self._log_ticks])
        self.ax.set_yticklabels([f"{d}" for d in self._log_ticks], color='green', fontsize=8)

        self.ax.grid(color='green', alpha=0.4)
        self.ax.tick_params(colors='green')
        self.ax.spines['polar'].set_color('green')

        # ---- Círculos de referencia en distancias reales ----
        theta = np.linspace(0, np.pi, 200)
        for d in self._log_ticks:
            r = np.full_like(theta, np.log10(d))
            self.ax.plot(theta, r, color='green', linewidth=0.4, alpha=0.5, linestyle='--')

        # --------------------------------------
        # Puntos detectados
        # --------------------------------------
        self.scatter = self.ax.scatter([], [], c='red', s=60, zorder=5)

        # --------------------------------------
        # Línea de barrido
        # --------------------------------------
        self.sweep_line, = self.ax.plot([], [], linewidth=2, color='lime', zorder=4)

        # Estela de la línea (efecto "phosphor")
        self.trail_lines = []
        for i in range(5):
            alpha = 0.15 * (i + 1)
            line, = self.ax.plot([], [], linewidth=1.5, color='lime', alpha=alpha, zorder=3)
            self.trail_lines.append(line)
        self.trail_angles = deque(maxlen=5)

        # --------------------------------------
        # Info panel
        # --------------------------------------
        self.info_text = self.fig.text(
            0.02, 0.95, "",
            color="lime", fontsize=10,
            fontfamily='monospace',
            verticalalignment='top'
        )

        # --------------------------------------
        # Botones
        # --------------------------------------
        ax_auto = plt.axes([0.75, 0.02, 0.10, 0.05])
        self.btn_auto = Button(ax_auto, "AUTO")
        self.btn_auto.on_clicked(self.set_auto)

        ax_manual = plt.axes([0.86, 0.02, 0.10, 0.05])
        self.btn_manual = Button(ax_manual, "MANUAL")
        self.btn_manual.on_clicked(self.set_manual)

    # ======================================================
    # Helpers: conversión log
    # ======================================================

    def _to_log(self, distancia):
        """Convierte distancia real a coordenada logarítmica del eje."""
        d = np.clip(distancia, MIN_DISTANCE, MAX_DISTANCE)
        return np.log10(d)

    # ======================================================
    # UART
    # ======================================================

    def wait_for_header(self):
        state = 0
        while True:
            byte = self.ser.read(1)
            if len(byte) == 0:
                return False
            if state == 0:
                if byte[0] == 0xAA:
                    state = 1
            elif state == 1:
                if byte[0] == 0x55:
                    return True
                if byte[0] != 0xAA:
                    state = 0

    def read_packet(self):
        """
        Intenta leer un paquete completo del puerto serie.
        Si lo consigue, carga pending_samples y resetea anim_index.
        No bloquea: si no hay header disponible rápido, retorna.
        """
        if not self.connected:
            return

        if not self.wait_for_header():
            return

        data = self.ser.read(PACKET_SIZE)

        if len(data) != PACKET_SIZE:
            self.packets_err += 1
            return

        samples = []
        for i in range(BUFFER_SIZE):
            offset = i * 4
            angulo, distancia = struct.unpack('<HH', data[offset:offset + 4])
            if angulo <= 180:
                samples.append((angulo, distancia))

        if samples:
            self.pending_samples = samples
            self.anim_index = 0
            self.packets_ok += 1
            self.last_packet_time = time.time()

    # ======================================================
    # Botones
    # ======================================================

    def set_auto(self, event):
        self.mode = "AUTO"
        self.pending_command = b'A'
        print("Modo AUTO")

    def set_manual(self, event):
        self.mode = "MANUAL"
        self.pending_command = b'M'
        print("Modo MANUAL")

    # ======================================================
    # Animación
    # ======================================================

    def update(self, frame):

        # --- 0. Despachar comando pendiente (prioridad máxima) ---
        if self.pending_command is not None and self.connected:
            try:
                self.ser.write(self.pending_command)
            except:
                pass
            self.pending_command = None

        # --- 1. Si terminamos de animar el paquete anterior, leer el siguiente ---
        if self.anim_index >= len(self.pending_samples):
            self.read_packet()

        # --- 2. Avanzar la animación: dibujar el próximo punto del paquete ---
        if self.anim_index < len(self.pending_samples):

            angulo, distancia = self.pending_samples[self.anim_index]
            self.anim_index += 1
            self.current_angle = angulo
            now = time.time()

            if 0 < distancia <= MAX_DISTANCE:
                self.radar_data[angulo] = distancia
                self.radar_timestamps[angulo] = now
            else:
                self.radar_data[angulo] = np.nan
                self.radar_timestamps[angulo] = -np.inf

        # --- 2b. Expirar puntos más viejos que POINT_LIFETIME ---
        now = time.time()
        expired = (now - self.radar_timestamps) > POINT_LIFETIME
        self.radar_data[expired] = np.nan
        self.radar_timestamps[expired] = -np.inf

        # --- 3. Actualizar scatter con todos los datos acumulados ---
        angles_rad = []
        distances_log = []

        for ang in range(181):
            dist = self.radar_data[ang]
            if not np.isnan(dist):
                angles_rad.append(np.radians(ang))
                distances_log.append(self._to_log(dist))

        if angles_rad:
            self.scatter.set_offsets(np.column_stack((angles_rad, distances_log)))
        else:
            self.scatter.set_offsets(np.empty((0, 2)))

        # --- 4. Línea de barrido + estela ---
        sweep_rad = np.radians(self.current_angle)

        self.sweep_line.set_data(
            [sweep_rad, sweep_rad],
            [self._r_min_log, self._r_max_log]
        )

        self.trail_angles.append(sweep_rad)
        for i, line in enumerate(self.trail_lines):
            if i < len(self.trail_angles):
                a = list(self.trail_angles)[-(i + 1)]
                line.set_data([a, a], [self._r_min_log, self._r_max_log])
            else:
                line.set_data([], [])

        # --- 5. Info ---
        objetos = int(np.count_nonzero(~np.isnan(self.radar_data)))

        elapsed = ""
        if self.last_packet_time:
            dt = time.time() - self.last_packet_time
            elapsed = f"{dt*1000:.0f} ms"

        self.info_text.set_text(
            f"Estado:   {'CONECTADO' if self.connected else 'SIN UART'}\n"
            f"Modo:     {self.mode}\n"
            f"Ángulo:   {self.current_angle}°\n"
            f"Objetos:  {objetos}\n"
            f"Pkts OK:  {self.packets_ok}  |  ERR: {self.packets_err}\n"
            f"Último:   {elapsed}"
        )

        self.ax.set_title("RADAR 180°", color='lime', fontsize=16, pad=15)

        return (self.scatter, self.sweep_line, self.info_text, *self.trail_lines)

    # ======================================================

    def run(self):
        self._anim = animation.FuncAnimation(
            self.fig,
            self.update,
            interval=30,       # ~33 fps
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