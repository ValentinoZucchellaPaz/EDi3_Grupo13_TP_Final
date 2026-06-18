import serial
import struct

# ==========================================
# CONFIG
# ==========================================

PORT = "COM13"
BAUDRATE = 115200

HEADER = b'\xAA\x55'

BUFFER_SIZE = 90
BYTES_PER_SAMPLE = 4
PAYLOAD_SIZE = BUFFER_SIZE * BYTES_PER_SAMPLE

# ==========================================
# UART
# ==========================================

try:
    ser = serial.Serial(
        PORT,
        BAUDRATE,
        timeout=1
    )

    print(f"Conectado a {PORT}")

except Exception as e:
    print(f"No se pudo abrir puerto serie: {e}")
    exit(1)

# ==========================================
# FUNCIONES
# ==========================================

def wait_for_header():

    state = 0

    while True:

        byte = ser.read(1)

        if len(byte) == 0:
            continue

        if state == 0:

            if byte[0] == 0xAA:
                state = 1

        elif state == 1:

            if byte[0] == 0x55:
                return

            if byte[0] != 0xAA:
                state = 0

# ==========================================
# LOOP
# ==========================================

while True:

    wait_for_header()

    payload = ser.read(PAYLOAD_SIZE)

    if len(payload) != PAYLOAD_SIZE:
        continue

    print("\n====================")
    print("PAQUETE RECIBIDO")
    print("====================")

    for i in range(BUFFER_SIZE):

        offset = i * 4

        angulo, distancia = struct.unpack(
            '<HH',
            payload[offset:offset + 4]
        )

        print(f"{{ {angulo}, {distancia} }}")