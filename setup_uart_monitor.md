### Instalar Python

Ir a (pagina de python)[https://www.python.org/downloads/] e instalar version recomendada.
Durante la instalación en Windows es importante marcar:

```bash
✅ Add Python to PATH
```

antes de hacer clic en "Install Now".

Revisar instalacion corriendo en consola:

```bash
python --version
```

### Instalar Dependencias de Proyecto

Una vez instalado python en la maquina, en la consola ejecutar:

```bash
cd uart_monitor

# Hacer entorno virtual para no instalar dependencias globalmente
python -m venv .venv # crea entorno, si falla cambiar python por py o python3
source .venv/bin/activate # activa para linux
.venv\Scripts\activate.bat # activa en CMD de Windows
.venv\Scripts\Activate.ps1 # activa en Powershell de Windows

# Instalar dependencias
pip install -r requirements.txt
```

### Correr script

```bash
python radar_app.py
```
