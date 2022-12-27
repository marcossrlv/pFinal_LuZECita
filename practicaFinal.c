#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

pthread_t tec1, tec2, resp1, resp2, enc, aten;
pthread_mutex_t solicitudes, fichero, colaClientes, solicitudes,solsDomicilarias;
pthread_cond_t suficientesSolicitudes;
int nSolsDomiciliarias, nClientesApp, nClientesRed;
struct cliente clientes[20];
FILE logfile;

struct cliente {
    int id, prioridad, atendido, solicitud;
};


void terminarPrograma(int sig);
void nuevoCliente(int sig);

int main(int argc, char const *argv[]){	

	// Seed para los numeros aleatorios
	srand(time(NULL));

    struct sigaction clienteRed, clienteApp, terminar;

//1. signal o sigaction SIGUSR1, cliente app.
    clienteApp.sa_flags=0;
    sigemptyset(&clienteApp.sa_mask);
    clienteApp.sa_handler=nuevoCliente;
    if (sigaction(SIGUSR2, &clienteApp, NULL)==-1) {
        perror("Llamada a sigaction cliente app.");
        exit(-1);
    }
    
//2. signal o sigaction SIGUSR2, cliente red.
    clienteRed.sa_flags=0;
    sigemptyset(&clienteRed.sa_mask);
    clienteRed.sa_handler=nuevoCliente;
    if (sigaction(SIGUSR1, &clienteRed, NULL)==-1) {
        perror("Llamada a sigaction cliente red.");
        exit(-1);
    }

//3. signal o sigaction SIGINT, terminar.        
    terminar.sa_flags=0;
    sigemptyset(&terminar.sa_mask);
    terminar.sa_handler=terminarPrograma;
    if (sigaction(SIGINT, &terminar, NULL)==-1) {
        perror("Llamada a sigaction terminar.");
        exit(-1);
    }

//4. Inicializar recursos (¡Ojo!, Inicializar!=Declarar).

    //a.g. Semáforos y variables condición
    if (pthread_mutex_init(&fichero, NULL)!=0) exit(-1); 
    if (pthread_mutex_init(&colaClientes, NULL)!=0) exit(-1); 
    if (pthread_mutex_init(&solicitudes, NULL)!=0) exit(-1);  
    if (pthread_cond_init(&suficientesSolicitudes, NULL)!=0) exit(-1);

    //b.f. Contador de clientes de cada tipo y variables relativas a la solicitud de atención domiciliaria.
    nClientesApp = 0;
    nClientesRed = 0;
    nSolsDomiciliarias = 0;

    //c. Lista de clientes id 0, Prioridad 0, atendido 0, solicitud 0.
    for(int i=0; i<20; i++){
        clientes[i].atendido= 0;
        clientes[i].id= 0;
        clientes[i].prioridad= 0;
        clientes[i].solicitud= 0;
    }

    //d. Lista de técnicos (si se incluye).

    //e. Fichero de Log
    
    //5. Crear 6 hilos (técnicos, responsables de reparaciones, encargado y
    //atención domiciliaria).
    pthread_create(&tec1, NULL, AccionesTécnico, "App");
    pthread_create(&tec2, NULL, AccionesTécnico, "App");
    pthread_create(&resp1, NULL, AccionesTécnico, "Red");
    pthread_create(&resp2, NULL, AccionesTécnico, "Red");
    pthread_create(&enc, NULL, AccionesEncargado, NULL); 
    pthread_create(&aten, NULL, AccionesTecnicoDomiciliario, NULL); 

//6. Esperar por señales de forma infinita.
    while(true){

    }
}

void writeLogMessage(char *id, char *msg) {
    // Calculamos la hora actual
    time_t now = time(0);
    struct tm *tlocal = localtime(&now);
    char stnow[25];
    strftime(stnow, 25, "%d/%m/%y %H:%M:%S", tlocal);

    // Escribimos en el log
    logFile = fopen(logFileName, "a");
    fprintf(logFile, "[%s] %s: %s\n", stnow, id, msg);
    fclose(logFile);
}

void nuevoCliente(int sig){

}

void AccionesCliente(){

}

void AccionesTécnico(){

}

void AccionesEncargado(){
    
}

void AccionesTecnicoDomiciliario(){

}









