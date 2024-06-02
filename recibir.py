#!/bin/env python

import numpy as np
import matplotlib.pyplot as plt
import serial

puerto = serial.Serial('/dev/ttyACM0', 9600)

# Generar la señal (por ejemplo, una señal sinusoidal)
fs = 1000  # Frecuencia de muestreo en Hz
f = 10     # Frecuencia de la señal en Hz
t = np.arange(0, 1, 1/fs)  # Vector de tiempo de 1 segundo de duración
signal = np.sin(2 * np.pi * f * t)  # Señal sinusoidal de 10 Hz

res = np.zeros(signal.shape[0])

# Graficar la señal antes de enviarla
plt.figure()
plt.plot(t, signal)
plt.title('Señal antes de enviarla')
plt.xlabel('Tiempo (s)')
plt.ylabel('Amplitud')
plt.grid(True)
plt.show()

i = 0
for sample in signal:
    sample_int = int((sample + 1) * 127.5)
    puerto.write(bytes([sample_int]))
    i += 1
    res[i] = puerto.read(1)

plt.figure()
plt.plot(t, res)
plt.title('Señal despues de enviarla')
plt.xlabel('Tiempo (s)')
plt.ylabel('Amplitud')
plt.grid(True)
plt.show()

#try:
    #while True:
        ## Esperar a recibir datos
        #dato_recibido = puerto.read(1)
#
        #if dato_recibido:
            ## Procesar los datos recibidos
            #puerto.write(dato_recibido)
#
#except KeyboardInterrupt:
    ## Cerrar el puerto serie al salir del bucle
    #puerto_serie.close()
