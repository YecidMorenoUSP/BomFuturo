import socket
import struct
import threading
import matplotlib.pyplot as plt
import matplotlib.animation as animation


# Función para recibir los datos del socket
def recibir_datos(sock):
    N = 0
    while True:
        data = sock.recv(12) # Se espera a recibir 4 bytes (un entero de 32 bits)
        entero = struct.unpack('<iff', data) # Se desempaqueta el entero
        # print(f'Recibido: {entero}')
        # valores.append(entero) # Se agrega el entero a la lista de valores recibidos
        if(entero[0] == 1234):
            valoresX.append(entero[1])
            valoresY.append(entero[2])
        # print("Size [%d]"%(len(valores)));


# Función para graficar los datos en tiempo real
def graficar_datos(valores,valoresX,valoresY):
    fig, ax = plt.subplots()
    # x = [i for i in range(len(valores))] # Eje x para la gráfica
    ax.plot(valoresX,valoresY)

    def actualizar_grafica(i):
        if(len(valoresX)<200):
            return
        
        ax.clear()
        # x = [i for i in range(len(valores[-200:]))] # Eje x para la gráfica
        # ax.plot(x, valores[-200:])
        ax.plot(valoresX[-200:],valoresY[-200:])

    ani = animation.FuncAnimation(fig, actualizar_grafica, interval=100)
    plt.show()

# Configuración del servidor 
HOST = '0.0.0.0'
PORT = 5000

# Crear el socket y vincularlo al puerto y dirección IP del servidor
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((HOST, PORT))

# Esperar por una conexión entrante
# sock.listen(1)
# print(f'Esperando conexiones en {HOST}:{PORT}')

# Aceptar la conexión entrante
# conn, addr = sock.accept()
# print(f'Conexión aceptada desde {addr[0]}:{addr[1]}')

# Crear una lista para almacenar los valores recibidos
valores = []
valoresX = []
valoresY = []

# Crear un hilo para recibir los datos del socket
hilo_datos = threading.Thread(target=recibir_datos, args=(sock,))
hilo_datos.start()

# Graficar los datos en tiempo real
graficar_datos(valores,valoresX,valoresY)
