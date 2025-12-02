#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <mqueue.h>
#include <string.h>
#include <fcntl.h>      
#include <sys/stat.h> 
#define MAX_PRODUCTOS 20
#define MAX_PEDIDOS   10




pid_t pid_almacen, pid_fabrica, pid_ventas;

int unidades_producto = 0;


// Configuración escalable
#define NUM_HILOS_ENSAMBLAR 1
#define NUM_HILOS_PINTAR 1
// Variable compartida entre hilos
int total_ensamblados = 0;
int total_pintados = 0;

sem_t mutex_productos;
sem_t mutex_pintados;
sem_t sem_ensamblado;
sem_t sem_pintado;

int tiempo_aleatorio(int min, int max) {
    return rand() % (max - min + 1) + min;
}
#define COLA "/cola_ventas"



int pedido;

int pedidos_atendidos = 0;

int stock = 0;
volatile sig_atomic_t nuevo;

mqd_t cola;




void* ensamblar(void* args) {
    srand(time(NULL) ^ pthread_self());
    
    printf("[Ensamblaje] Comienzo de mi ejecución...\n");
    
    while(1) {
        printf("[Ensamblaje] Ensamblando producto...\n");
        sleep(tiempo_aleatorio(3, 8));
        
        //SECCIÓN CRÍTICA 
        sem_wait(&mutex_productos);
        
        total_ensamblados++;
        int num_producto = total_ensamblados;
        
        sem_post(&mutex_productos);
        
        printf("[Ensamblaje] Producto #%d ensamblado.\n", num_producto);
        
        //Señalizar hilo de pintado
        sem_post(&sem_ensamblado);
    }
    
    return NULL;
}

void* pintar(void* arg) {
    srand(time(NULL) ^ pthread_self());

    printf("[Pintado] Comienzo de mi ejecución...\n");
    while (1)
    {
        sem_wait(&sem_ensamblado);
        printf("[Pintado]: Producto recibido. Pintando...\n");
        sleep(tiempo_aleatorio(2, 4));

        // SECCIÓN CRÍTICA
        sem_wait(&mutex_pintados);
        total_pintados++;
        int num_producto = total_pintados;

        sem_post(&mutex_pintados);

        printf("[Pintado]: Producto #%d pintado.\n", num_producto);

        // Señalizar hilo de empaquetado
        sem_post(&sem_pintado);
    }

	return NULL;
}




void* empaquetar(void* arg) {
    srand(time(NULL) ^ pthread_self());
    
    printf("[Empaquetado] Comienzo de mi ejecución...\n");

    while (1) {

        // Espera a que el pintado termine un producto
        sem_wait(&sem_pintado);

        printf("[Empaquetado] Producto recibido: empaquetando...\n");
        sleep(tiempo_aleatorio(2, 5));

        printf("[Empaquetado] Producto empaquetado\n");

        // Aquí después vas a agregar señal al almacén
        kill(pid_almacen, SIGUSR1);
    }

    return NULL;
}
void recibe_producto(int sig) {
    nuevo = 1;
}


int main(int argc, char* argv[]) {

	pid_almacen = fork();

	if (pid_almacen != 0) {
		pid_fabrica = fork();
		if (pid_fabrica != 0) {
			pid_ventas = fork();
			if(pid_ventas != 0) {
				/* Proceso padre */


            //-------separador----

			} else {
                //PROCESO VENTAS
                printf("[Ventas] Comienzo mi ejecución...\n");

                mqd_t cola_ventas;
                int num_orden = 0;

                cola_ventas = mq_open(COLA, O_WRONLY); //Se abre la cola creada por el almacén en modo escritura

                while (1) {

                    int unidades;
                    sleep(tiempo_aleatorio(10, 15)); //Cada x tiempo llega una compra de un cliente
                    unidades = tiempo_aleatorio(1, 3); //Número de unidades que pide el cliente (por ejemplo, de 1 a 3)

                    printf("[Ventas] Recibida compra. Orden %d: %d unidades.\n", num_orden, unidades);

                    mq_send(cola_ventas, (char*) &unidades, sizeof(int), 0); //Se envía el número de unidades al almacén por la cola de mensajes

                    printf("[Ventas] Orden número %d enviada al almacén\n", num_orden);

                    num_orden = num_orden + 1;

                }

                //------separador----
			
            }


		} else {
			//PROCESO FÁBRICA 
            printf("[Fábrica] Comienzo mi ejecución...\n");

            // Inicializar semáforos
            if(sem_init(&mutex_productos, 0, 1) != 0) {
                perror("[Fábrica] Error al inicializar mutex_productos");
                exit(EXIT_FAILURE);
            }
            
            if(sem_init(&sem_ensamblado, 0, 0) != 0) {
                perror("[Fábrica] Error al inicializar sem_ensamblado");
                exit(EXIT_FAILURE);
            }

            if(sem_init(&sem_pintado, 0, 0) != 0) {
                perror("[Fábrica] Error al inicializar sem_pintado");
                exit(EXIT_FAILURE);
            }
            if (sem_init(&mutex_pintados, 0, 1) != 0) {
             perror("[Fábrica] Error al inicializar mutex_pintados");
            exit(EXIT_FAILURE);
            }

            
            printf("[Fábrica] Semáforos inicializados correctamente\n");
            
            // Crear hilos de ensamblaje
            pthread_t hilos_ensamblar[NUM_HILOS_ENSAMBLAR];
            printf("[Fábrica] Creando %d hilo(s) de ensamblaje...\n", NUM_HILOS_ENSAMBLAR);
            for(int i = 0; i < NUM_HILOS_ENSAMBLAR; i++) {
                if(pthread_create(&hilos_ensamblar[i], NULL, ensamblar, NULL) != 0) {
                    perror("[Fábrica] Error al crear hilo de ensamblaje");
                    exit(EXIT_FAILURE);
                }
            }
            // Crear hilos de pintado
            pthread_t hilos_pintar[NUM_HILOS_PINTAR];
            printf("[Fábrica] Creando %d hilo(s) de pintado...\n", NUM_HILOS_PINTAR);
            for(int i = 0; i < NUM_HILOS_PINTAR; i++) {
                if(pthread_create(&hilos_pintar[i], NULL, pintar, NULL) != 0) {
                    perror("[Fábrica] Error al crear hilo de pintado");
                    exit(EXIT_FAILURE);
                }
            }

            pthread_t hilo_empaquetado;
            printf("[Fábrica] Creando hilo de empaquetado...\n");
            if(pthread_create(&hilo_empaquetado, NULL, empaquetar, NULL) != 0) {
                perror("[Fábrica] Error al crear hilo de empaquetado");
                exit(EXIT_FAILURE);
            }

            
            printf("[Fábrica] Todos los hilos creados. Producción en marcha...\n");
            
            // Esperar a que terminen
            for(int i = 0; i < NUM_HILOS_ENSAMBLAR; i++) {
                pthread_join(hilos_ensamblar[i], NULL);
            }
            for(int i = 0; i < NUM_HILOS_PINTAR; i++) {
                pthread_join(hilos_pintar[i], NULL);
            }

            pthread_join(hilo_empaquetado, NULL);

            
            // Limpieza
            sem_destroy(&mutex_productos);
            sem_destroy(&sem_ensamblado);
            sem_destroy(&mutex_pintados);
            sem_destroy(&sem_pintado);

            
            printf("[Fábrica] Finalizando ejecución\n");
		}
} else {

    printf("[ALMACÉN] Comienzo mi ejecución...\n");

   
    struct mq_attr attr;
    attr.mq_flags   = 0;
    attr.mq_maxmsg  = 10;
    attr.mq_msgsize = sizeof(int);
    attr.mq_curmsgs = 0;

    
    cola = mq_open(COLA, O_CREAT | O_RDONLY, 0644, &attr);
    if (cola == (mqd_t)-1) {
        perror("[ALMACÉN] Error al abrir la cola");
        exit(1);
    }

    // Señales
    signal(SIGUSR1, recibe_producto);


    printf("[ALMACÉN] Esperando productos y pedidos...\n");

    
    while (stock < MAX_PRODUCTOS && pedidos_atendidos < MAX_PEDIDOS) {

        // Llega producto desde fábrica
        if (nuevo) {
            stock++;
            nuevo = 0;
            printf("[ALMACÉN] Producto recibido. Stock = %d\n", stock);
        }

        // Llega pedido desde ventas
        int pedido;
        if (mq_receive(cola, (char*)&pedido, sizeof(int), NULL) > 0) {

            printf("[ALMACÉN] Pedido recibido: %d\n", pedido);

            if (stock >= pedido) {
                stock -= pedido;
                pedidos_atendidos++;
                printf("[ALMACÉN] Pedido atendido. Stock = %d | Pedidos = %d\n",
                        stock, pedidos_atendidos);
            } else {
                printf("[ALMACÉN] No hay stock suficiente\n");
            }
        }

        sleep(1);
    }

    
    printf("[ALMACÉN] Fin automático. Cerrando recursos...\n");
    mq_close(cola);
    mq_unlink(COLA);
    exit(0);
}




	exit(0);
}

//prueba primer commit
