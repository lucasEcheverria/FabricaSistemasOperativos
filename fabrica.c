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

pid_t pid_almacen, pid_fabrica, pid_ventas;

int unidades_producto = 0;

// Configuración escalable
#define NUM_HILOS_ENSAMBLAR 1
#define NUM_HILOS_PINTAR 1
// Variable compartida entre hilos
int total_ensamblados = 0;
int total_pintados = 0;

sem_t mutex_productos;
sem_t sem_ensamblado;

int tiempo_aleatorio(int min, int max) {
    return rand() % (max - min + 1) + min;
}

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
	printf("[Pintado] Comienzo de mi ejecución...\n");
	while (1) {


                printf("[Pintado]: Producto recibido. Pintando...\n");
                sleep(tiempo_aleatorio(2, 4));
		printf("[Pintado]: Producto pintado.\n");

        }
}


void* empaquetar(void* arg) {
	printf("[Empaquetado] Comienzo de mi ejecución...\n");
	while (1) {


                printf("[Empaquetado] Producto recibido: empaquetando...\n");
                sleep(tiempo_aleatorio(2, 5));
		printf("[Empaquetado] Producto empaquetado\n");


        }
}

int main(int argc, char* argv[]) {

	pid_almacen = fork();

	if (pid_almacen != 0) {
		pid_fabrica = fork();
		if (pid_fabrica != 0) {
			pid_ventas = fork();
			if(pid_ventas != 0) {
				/* Proceso padre */



			} else {
				/* Proceso Ventas */
				int num_orden = 0;
				while(1) {

					sleep(tiempo_aleatorio(10, 15));
					printf("[Ventas] Recibida compra desde cliente. Enviando orden nº %d al almacén...\n", num_orden);
					num_orden++;


				}
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
            
            printf("[Fábrica] Todos los hilos creados. Producción en marcha...\n");
            
            // Esperar a que terminen
            for(int i = 0; i < NUM_HILOS_ENSAMBLAR; i++) {
                pthread_join(hilos_ensamblar[i], NULL);
            }
            for(int i = 0; i < NUM_HILOS_PINTAR; i++) {
                pthread_join(hilos_pintar[i], NULL);
            }
            
            // Limpieza
            sem_destroy(&mutex_productos);
            sem_destroy(&sem_ensamblado);
            
            printf("[Fábrica] Finalizando ejecución\n");
		}
	} else {
		printf("[Almacén] Comienzo mi ejecución...\n");


	 	while(1) {
			char buff[50];


			printf("[Almacén] Recibida orden desde ventas\n");



			printf("[Almacén] Atendida orden nº %s. Unidades restantes: %d\n", buff , unidades_producto);
		}
	}

	exit(0);
}