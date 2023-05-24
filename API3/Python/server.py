import socket
import struct
import threading
import matplotlib.pyplot as plt
import matplotlib.animation as animation

# Función para recibir los datos del socket
def recibir_datos(sock):
    while True:
        data = sock.recv(4) # Se espera a recibir 4 bytes (un entero de 32 bits)
        entero = struct.unpack('<f', data)[0] # Se desempaqueta el entero
        # print(f'Recibido: {entero}')
        valores.append(entero) # Se agrega el entero a la lista de valores recibidos
        print("Size [%d]"%(len(valores)));

# Función para graficar los datos en tiempo real
def graficar_datos(valores):
    fig, ax = plt.subplots()
    x = [i for i in range(len(valores))] # Eje x para la gráfica
    ax.plot(x, valores)

    def actualizar_grafica(i):
        if(len(valores)<200):
            return
        
        ax.clear()
        x = [i for i in range(len(valores[-200:]))] # Eje x para la gráfica
        ax.plot(x, valores[-200:])

    ani = animation.FuncAnimation(fig, actualizar_grafica, interval=100)
    plt.show()

# Configuración del servidor
HOST = '0.0.0.0'
PORT = 5000

# Crear el socket y vincularlo al puerto y dirección IP del servidor
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.bind((HOST, PORT))

# Esperar por una conexión entrante
sock.listen(1)
print(f'Esperando conexiones en {HOST}:{PORT}')

# Aceptar la conexión entrante
conn, addr = sock.accept()
print(f'Conexión aceptada desde {addr[0]}:{addr[1]}')

# Crear una lista para almacenar los valores recibidos
valores = []

# Crear un hilo para recibir los datos del socket
hilo_datos = threading.Thread(target=recibir_datos, args=(conn,))
hilo_datos.start()

# Graficar los datos en tiempo real
graficar_datos(valores)
