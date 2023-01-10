#define _XOPEN_SOURCE 700
    #include <pthread.h> 
    #include <signal.h>
    #include <stdio.h>
    #include <time.h>
    #include <unistd.h>
    #include <stdlib.h>
    #include <string.h>

    pthread_mutex_t fichero, colaClientes, solicitudes;
    pthread_cond_t suficientesSolicitudesDomiciliarias, atencionDomiciliariaFinalizada;
    int nClientesApp, nClientesRed,nSolsDomiciliarias;
    struct cliente {
        int id, atendido, tipo, prioridad, solicitud;
    } clientes[20];
    pthread_t tecnico_1, tecnico_2, resprep_1, resprep_2, encargado, atencionDomiciliaria;
    FILE * logFile;
    

    int calculaAleatorios(int min, int max);
    void writeLogMessage(char *id, char *msg);
    void nuevoCliente(int sig);
    void terminarPrograma(int sig);
    void *accionesCliente(void *arg);
    void *accionesTecnico(void *arg);
    void *accionesEncargado(void * arg);
    void *accionesTecnicoDomiciliario(void * arg);
    

    int main(int argc, char *argv[]){	
        struct sigaction clienteApp, clienteRed, terminar; 
    //1. signal o sigaction SIGUSR1, cliente app.
        clienteApp.sa_flags=0;
        sigemptyset(&clienteApp.sa_mask);
        clienteApp.sa_handler=nuevoCliente;
        if (sigaction(SIGUSR1, &clienteApp, NULL)==-1) {
            perror("Llamada a sigaction cliente app.");
            exit(-1);
        }
        
    //2. signal o sigaction SIGUSR2, cliente red.
        clienteRed.sa_flags=0;
        sigemptyset(&clienteRed.sa_mask);
        clienteRed.sa_handler=nuevoCliente;
        if (sigaction(SIGUSR2, &clienteRed, NULL)==-1) {
            perror("Llamada a sigaction cliente red.");
            exit(-1);
        }

    //3. signal o sigaction SIGINT, terminar.        
        terminar.sa_flags=0;
        sigemptyset(&terminar.sa_mask);
        sigaddset(&terminar.sa_mask,SIGUSR1);
        sigaddset(&terminar.sa_mask,SIGUSR2);
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
        if (pthread_cond_init(&suficientesSolicitudesDomiciliarias, NULL)!=0) exit(-1);
        if (pthread_cond_init(&atencionDomiciliariaFinalizada, NULL)!=0) exit(-1);

        //b.f. Contador de clientes de cada tipo y variables relativas a la solicitud de atención domiciliaria.
        nClientesApp = 0, nClientesRed = 0, nSolsDomiciliarias = 0;

        //c. Lista de clientes id 0, Prioridad 0, atendido 0, solicitud 0.
        for(int i=0; i<20; i++){
            clientes[i].id= 0;
            clientes[i].prioridad= 0;
            clientes[i].atendido= 0;
            clientes[i].solicitud= 0;
            clientes[i].tipo= 0;
        }


        //e. Fichero de Log
        logFile = fopen("registroTiempos.log", "w");
        fclose(logFile);

        //5. Crear 6 hilos (técnicos, responsables de reparaciones, encargado y
        //atención domiciliaria).
        pthread_create(&tecnico_1, NULL, accionesTecnico, "tecnico_1");
        pthread_create(&tecnico_2, NULL, accionesTecnico, "tecnico_2");
        pthread_create(&resprep_1, NULL, accionesTecnico, "resprep_1");
        pthread_create(&resprep_2, NULL, accionesTecnico, "resprep_2");
        pthread_create(&encargado, NULL, accionesEncargado, "encargado"); 
        pthread_create(&atencionDomiciliaria, NULL, accionesTecnicoDomiciliario, "atencionDomiciliaria"); 

    //6. Esperar por señales de forma infinita.
        while(1){
        }
    }

    void nuevoCliente(int sig) {
        pthread_mutex_lock(&colaClientes);
        pthread_t nuevoCliente;
        int aniadido=0, posNuevoCliente=0; 
        
        //1. Comprobar si hay espacio en la lista de clientes.
        for(int i=0; i<20 && aniadido==0; i++){
        	//1a. Si se encuentra el hueco.
            if(clientes[i].id==0){
                aniadido=1;
                posNuevoCliente=i;
		    // I: se anade el cliente.
                if(sig==SIGUSR1){
                    // II y III: se aumenta el contador y se cambia el id.
                    clientes[i].id=++nClientesApp;
                    // IV: se cambia el flag de atendido a 0.
                    clientes[i].atendido=0;
                    // V: se asigna el tipo correspondiente al cliente.
                    clientes[i].tipo=1;
                    // VI: se pone el flag de solicitud a 0.
                    clientes[i].solicitud=0;
                    // VII: se calcula y se asigna la prioridad.
                    clientes[i].prioridad=calculaAleatorios(1,10);
                } else{
                    clientes[i].id=++nClientesRed;
                    clientes[i].atendido=0;
                    clientes[i].tipo=2;
                    clientes[i].solicitud=0;
                    clientes[i].prioridad=calculaAleatorios(1,10);
                }
		// VIII: se crea el hilo nuevo para el cliente.
                pthread_create(&nuevoCliente, NULL, accionesCliente, (void *)&posNuevoCliente);                
            }
            //1b. Si no se encuentra hueco se ignora la llamada.
        }

        pthread_mutex_unlock(&colaClientes);
    }

    void terminarPrograma(int sig) {
        pthread_mutex_lock(&fichero);
        writeLogMessage("ESTADO", "Se espera a que no haya clientes en el sistema y se cierra el programa");
        pthread_mutex_unlock(&fichero);

        int nClientesEnElSistema=0;
        do{
            nClientesEnElSistema=0;
            
            pthread_mutex_lock(&colaClientes);
            for(int i=0; i<20; i++){
                if(clientes[i].id!=0) nClientesEnElSistema++;
            }
            pthread_mutex_unlock(&colaClientes);

            pthread_mutex_lock(&solicitudes);
            if(nClientesEnElSistema==nSolsDomiciliarias && nSolsDomiciliarias<4){
                for(int i=0; i<nSolsDomiciliarias;i++) pthread_cond_signal(&atencionDomiciliariaFinalizada);
            }
            pthread_mutex_unlock(&solicitudes);

            sleep(2);

        } while(nClientesEnElSistema>0);
        

        pthread_mutex_lock(&fichero);
        writeLogMessage("ESTADO", "No hay clientes en el sistema, se cierra el programa");
        pthread_mutex_unlock(&fichero);

        if(pthread_mutex_destroy(&fichero)!=0) exit(-1);
        if(pthread_mutex_destroy(&colaClientes)!=0) exit(-1);
        if(pthread_mutex_destroy(&solicitudes)!=0) exit(-1);
        if(pthread_cond_destroy(&suficientesSolicitudesDomiciliarias)!=0) exit(-1);
        if(pthread_cond_destroy(&atencionDomiciliariaFinalizada)!=0) exit(-1);

        exit(0);
    }

    void * accionesCliente(void *arg){
        //1.  Guardar en el log la hora de entrada.
        //2.  Guardar en el log el tipo de cliente.
        char identificador[20]="";
        char nIdentificador[20]="";
        int pos = *(int*)arg;
        int tiempoEspera=0;
       
        if(clientes[pos].tipo == 2) strcpy(identificador,"clired_");
        else  strcpy(identificador,"cliapp_");

        sprintf(nIdentificador,"%d",clientes[pos].id);
        strcat(identificador, nIdentificador);
        pthread_mutex_lock(&fichero);
        writeLogMessage(identificador, "Nuevo cliente accede al sistema");
        pthread_mutex_unlock(&fichero);

        while(1){
            //  3. Comprueba si está siendo atendido.
            pthread_mutex_lock(&colaClientes);
            if(clientes[pos].atendido == 0){
                //a.  Si no lo está, calculamos el comportamiento del cliente (si se va por
                //    dificultad o si se cansa de esperar algo que solo ocurre cada 8
                //    segundos) y también el caso de que pierda la conexión a internet.
                
                //b.  Si se va se escribe en el log, se libera espacio en la cola de clientes
                //    y se da fin al hilo.
                switch(calculaAleatorios(1,10)){
                    //  Un 20 % de los clientes, se cansa de esperar y sale de la aplicaci´on cuando
                    //  han transcurrido 8 segundos.
                    case 10:
                    case 9:
                        if(tiempoEspera%8==0 && tiempoEspera>0){
                            pthread_mutex_lock(&fichero);
                            writeLogMessage(identificador,"Se cansa de esperar y se marcha");
                            pthread_mutex_unlock(&fichero);

                            clientes[pos].id=0;
                            clientes[pos].tipo=0;
                            clientes[pos].prioridad=0;
                            clientes[pos].solicitud=0;
                            clientes[pos].atendido=0;
                            pthread_mutex_unlock(&colaClientes);

                            pthread_exit(NULL);
                        }
                        break;
                    //  Un 10 % de los clientes encuentra difícil la aplicación y se va de inmediato.
                    case 8:
                        pthread_mutex_lock(&fichero);
                        writeLogMessage(identificador, "Encuentra dificil la aplicación y se marcha");
                        pthread_mutex_unlock(&fichero);

                        clientes[pos].id=0;
                        clientes[pos].tipo=0;
                        clientes[pos].prioridad=0;
                        clientes[pos].solicitud=0;
                        clientes[pos].atendido=0;
                        pthread_mutex_unlock(&colaClientes);

                        pthread_exit(NULL);
                        break;
                    case 7:
                    case 6:
                    case 5:
                    case 4:
                    case 3:
                    case 2:
                    //  Del 70 % restante un 5 % pierde la conexión
                    //  a internet y tambi´en abandona.
                    case 1:
                        if(calculaAleatorios(1,20) == 20){

                            pthread_mutex_lock(&fichero);
                            writeLogMessage(identificador, "Pierde la conexion y abandona");
                            pthread_mutex_unlock(&fichero);

                            clientes[pos].id=0;
                            clientes[pos].tipo=0;
                            clientes[pos].prioridad=0;
                            clientes[pos].solicitud=0;
                            clientes[pos].atendido=0;
                            pthread_mutex_unlock(&colaClientes);

                            pthread_exit(NULL);

                        }
                        break;
                }
                //  c. Sino debe dormir 2 segundos y vuelve a 3.
                tiempoEspera+=2;
                pthread_mutex_unlock(&colaClientes);
                sleep(2);
                

            //4.  Si está siendo atendido por el técnico correspondiente debemos esperar a que
            //    termine (puede comprobar cada 2 segundos).
            } else {
                while(clientes[pos].atendido == 1) {
                    pthread_mutex_unlock(&colaClientes);
                    sleep(2);
                    pthread_mutex_lock(&colaClientes);
                }
                pthread_mutex_unlock(&colaClientes);

                //5.  Si el cliente es de tipo red y quiere realizar una solicitud domiciliaria. 
                //    10% de probabilidad.
                if(clientes[pos].tipo == 2 && clientes[pos].atendido==2){
                    switch (calculaAleatorios(1,10))
                    {
                        case 1:
                        case 2:
                        case 3:
                        case 4:
                        case 5:
                        case 6:
                        case 7:
                            break;
                        case 8:
                        case 9:
                        case 10:
                            pthread_mutex_lock(&solicitudes);
                            //5a. Comprueba el numero de solicitudes pendientes.
                            while(nSolsDomiciliarias>=4){
                                pthread_mutex_unlock(&solicitudes);
                            //5c. Si no es menor que 4 duerme 3 segundos y vuelve a comprobar.
                                sleep(3);
                                pthread_mutex_lock(&solicitudes);
                            }
				
			    //5b. Si es menor que 4 se incrementa el contador de solicitudes.	
                            nSolsDomiciliarias++;
				
			    // I. Escribe en el log que espera para ser atendido.
                            pthread_mutex_lock(&fichero);
                            writeLogMessage(identificador, "Espera atencion domiciliaria");
                            pthread_mutex_unlock(&fichero);

			    // II. Cambia el valor de solicitud a 1.
                            pthread_mutex_lock(&colaClientes);
                            clientes[pos].solicitud=1;
                            pthread_mutex_unlock(&colaClientes);
				
			    // III. Si es el cuarto avisa al tecnico con un cond_signal.
                            if(nSolsDomiciliarias==4) pthread_cond_signal(&suficientesSolicitudesDomiciliarias);
                            
                            // IV. Se bloquea hasta que se haya finalizado la atencion.
                            pthread_cond_wait(&atencionDomiciliariaFinalizada,&solicitudes);   

			    // V. Comunica que su atencion ha finalizado.
                            pthread_mutex_lock(&fichero);
                            writeLogMessage(identificador, "Finaliza su atencion domiciliaria");
                            pthread_mutex_unlock(&fichero);

                            pthread_mutex_unlock(&solicitudes);

                            break;
                    }
                }

                //6.  Si no libera su posición en cola de clientes y se va.
                pthread_mutex_lock(&colaClientes);
                clientes[pos].id=0;
                clientes[pos].tipo=0;
                clientes[pos].prioridad=0;
                clientes[pos].solicitud=0;
                clientes[pos].atendido=0;
                pthread_mutex_unlock(&colaClientes);

                //7.  Escribe en el log
                pthread_mutex_lock(&fichero);
                writeLogMessage(identificador, "Se va despues de ser atendido");
                pthread_mutex_unlock(&fichero);

                //8.  Fin del hilo Cliente
                pthread_exit(NULL);
            }
        }
    }

    void * accionesTecnico(void* arg){
        char * identificadorTecnico = (char*) arg; 
        char identificadorCliente[20]="";
        char nIdentificadorCliente[20]="";
        char identificadorAtencion[40]="";
        struct cliente* clienteElegido;
        int n=0;
        int encontrado=0;
        int clientesDescanso = 0;

        //Codigo para tecnicos
        if(strcmp(identificadorTecnico, "tecnico_1")==0||strcmp(identificadorTecnico, "tecnico_2")==0) {
            while(1) {

                n=0;
                encontrado=0;

                pthread_mutex_lock(&colaClientes);

                //1. Se busca un cliente para atender de su tipo. Atendiendo a la prioridad
                //   y si no al que mas tiempo lleve esperando.
                for(int i=0; i<20 && encontrado==0; i++){
                    if(clientes[i].tipo==1 && clientes[i].atendido==0){
                        clienteElegido = &clientes[i];
                        encontrado = 1;
                        n = i;
                    }
                }

                if(encontrado==1){
                    //Se empieza a comparar desde la posicion del primero que se encontró
                    for(int i=n+1; i<20; i++) {
                        if(clientes[i].tipo == 1 && clientes[i].atendido==0) {
                            //Se comparan prioridades, si son iguales se coge al primero de la lista que llevará más tiempo esperando
                            if(clientes[i].prioridad > clienteElegido->prioridad || (clientes[i].prioridad == clienteElegido->prioridad && clientes[i].id < clienteElegido->id)) {
                                clienteElegido = &clientes[i];
                            }
                        }
                    }

                    strcpy(identificadorCliente,"cliapp_");
                    sprintf(nIdentificadorCliente,"%d",clienteElegido->id);
                    strcat(identificadorCliente, nIdentificadorCliente);
                    strcpy(identificadorAtencion, identificadorTecnico);
                    strcat(identificadorAtencion, "_");
                    strcat(identificadorAtencion,identificadorCliente);

		    //2. Se cambia el flag de atendido.
                    clienteElegido->atendido = 1;
                    pthread_mutex_unlock(&colaClientes);

                    //3. Calculamos el tipo de atencion y en funcion de esto el tiempo.
                    int tipoAtencion = calculaAleatorios(1,100);
                    int tiempoAtencion = 0;
                    char motivo[50]="";
                    char auxMotivo[50]="";

                    if(tipoAtencion <= 80) {
                        //80% bien identificados
                        tiempoAtencion = calculaAleatorios(1,4);
                        strcpy(auxMotivo, "Bien identificado.");
                    } else if (tipoAtencion > 80 && tipoAtencion <= 90) {
                        //10% mal identificados
                        tiempoAtencion = calculaAleatorios(2,6);
                        strcpy(auxMotivo, "Mal identificado.");
                    } else {
                        //10% confusion de compañia, abandonan sistema
                        tiempoAtencion = calculaAleatorios(1,2);
                        strcpy(auxMotivo, "Confusion de compañia. Abandona el sistema.");
                    }

		    //4. Guardamos en el log que comienza la atencion.
                    pthread_mutex_lock(&fichero);
                    writeLogMessage(identificadorAtencion,"Comienza la atención.");
                    pthread_mutex_unlock(&fichero);

		    //5. Dormimos el tiempo de atencion.
                    sleep(tiempoAtencion);

		    //6 y 7. Guardamos en el log que finaliza la atencion y el motivo.
                    strcpy(motivo,"Finaliza la atencion. ");
                    strcat(motivo,auxMotivo);
                    pthread_mutex_lock(&fichero);
                    writeLogMessage(identificadorAtencion,motivo);
                    pthread_mutex_unlock(&fichero);
                    
                    //8. Cambiamos el flag de atendido.
                    pthread_mutex_lock(&colaClientes);
                    if(tipoAtencion<=90){
                        clienteElegido->atendido = 2; //Clientes mal y bien identificados
                    } else{
                        clienteElegido->atendido = 3; //Clientes con confusion de compania
                    }
                    pthread_mutex_unlock(&colaClientes);

                    //9. Mira si le toca descansar y lo anota en el log.
                    clientesDescanso++;
                    if(clientesDescanso==5) {
                        pthread_mutex_lock(&fichero);
                        writeLogMessage(identificadorTecnico, "Comienza descanso");
                        pthread_mutex_unlock(&fichero);

                        sleep(5);
                        clientesDescanso = 0;

                        pthread_mutex_lock(&fichero);
                        writeLogMessage(identificadorTecnico, "Finaliza descanso");
                        pthread_mutex_unlock(&fichero);
                    }


                } else {
                    //1a. Si no hay clientes para atender espera 1 segundo y vuelve a 1.
                    pthread_mutex_unlock(&colaClientes);
                    sleep(1);
                }
                //10. Volvemos al paso 1 a buscar el siguiente.
            }

        //Igual que los tecnicos pero cambiando el tipo de cliente.
        } else {
            while(1) {

                n=0;
                encontrado=0;

                pthread_mutex_lock(&colaClientes);

                //1. Se busca un cliente para atender de su tipo. Atendiendo a la prioridad
                //   y si no al que mas tiempo lleve esperando.
                for(int i=0; i<20 && encontrado==0; i++){
                    if(clientes[i].tipo==2 && clientes[i].atendido==0){
                        clienteElegido = &clientes[i];
                        encontrado = 1;
                        n = i;
                    }
                }

                
                if(encontrado==1){
                    for(int i=n+1; i<20; i++) {
                        if(clientes[i].tipo == 2 && clientes[i].atendido==0) {
                            if(clientes[i].prioridad > clienteElegido->prioridad || (clientes[i].prioridad == clienteElegido->prioridad && clientes[i].id < clienteElegido->id)) {
                                clienteElegido = &clientes[i];
                            }
                        }
                    }

                    strcpy(identificadorCliente,"clired_");
                    sprintf(nIdentificadorCliente,"%d",clienteElegido->id);
                    strcat(identificadorCliente, nIdentificadorCliente);
                    strcpy(identificadorAtencion, identificadorTecnico);
                    strcat(identificadorAtencion, "_");
                    strcat(identificadorAtencion,identificadorCliente);

		    //2. Se cambia el flag de atendido.
                    clienteElegido->atendido = 1;
                    pthread_mutex_unlock(&colaClientes);

                    //3. Calculamos el tipo de atencion y en funcion de esto el tiempo.
                    int tipoAtencion = calculaAleatorios(1,100);
                    int tiempoAtencion = 0;
                    char motivo[50]="";
                    char auxMotivo[50]="";

                    if(tipoAtencion <= 80) {
                        //80% bien identificados
                        tiempoAtencion = calculaAleatorios(1,4);
                        strcpy(auxMotivo, "Bien identificado.");
                    } else if (tipoAtencion > 80 && tipoAtencion <= 90) {
                        //10% mal identificados
                        tiempoAtencion = calculaAleatorios(2,6);
                        strcpy(auxMotivo, "Mal identificado.");
                    } else {
                        //10% confusion de compañia, abandonan sistema
                        tiempoAtencion = calculaAleatorios(1,2);
                        strcpy(auxMotivo, "Confusion de compania. Abandona el sistema.");
                    }

		    //4. Guardamos en el log que comienza la atencion.
                    pthread_mutex_lock(&fichero);
                    writeLogMessage(identificadorAtencion,"Comienza la atencion.");
                    pthread_mutex_unlock(&fichero);

		    //5. Dormimos el tiempo de atencion.
                    sleep(tiempoAtencion);
                    
                    //6 y 7. Guardamos en el log que finaliza la atencion y el motivo.
                    strcpy(motivo,"Finaliza la atencion. ");
                    strcat(motivo,auxMotivo);
                    pthread_mutex_lock(&fichero);
                    writeLogMessage(identificadorAtencion,motivo);
                    pthread_mutex_unlock(&fichero);

		    //8. Cambiamos el flag de atendido.
                    pthread_mutex_lock(&colaClientes);
                    if(tipoAtencion<=90){
                        clienteElegido->atendido = 2; //Clientes mal y bien identificados
                    } else{
                        clienteElegido->atendido = 3; //Clientes con confusion de compania
                    }
                    pthread_mutex_unlock(&colaClientes);
                    
                    

                    //9. Mira si le toca descansar y lo anota en el log.
                    clientesDescanso++;
                    if(clientesDescanso==6) {
                        pthread_mutex_lock(&fichero);
                        writeLogMessage(identificadorTecnico, "Comienza descanso");
                        pthread_mutex_unlock(&fichero);

                        sleep(5);
                        clientesDescanso = 0;

                        pthread_mutex_lock(&fichero);
                        writeLogMessage(identificadorTecnico, "Finaliza descanso");
                        pthread_mutex_unlock(&fichero);
                    }


                } else {
                    //1a. Si no hay clientes para atender espera 1 segundo y vuelve a 1.
                    pthread_mutex_unlock(&colaClientes);
                    sleep(1);
                }
                //10. Volvemos al paso 1 a buscar el siguiente.
            }
        }
    }

    void * accionesEncargado(void * arg){
        char *identificadorTecnico = (char*) arg;
        char identificadorCliente[20]="";
        char nIdentificadorCliente[20]="";
        char identificadorAtencion[40]="";
        struct cliente* clienteElegido=NULL;
        

        while(1){
            
            pthread_mutex_lock(&colaClientes);

            //1. se busca un cliente de tipo red sin atender por el que empezar a comparar.
            //   Si existe, se elegirá al de mayor prioridad y si tienen la misma prioridad
            //   al de mayor tiempo de espera...
            for(int z= 0; z<20; z++){
                //Si se encuentra a un cliente disponible...
                if(clientes[z].tipo==2 && clientes[z].atendido==0){
                    //Si ya se selecciono un cliente
                    if(clienteElegido != NULL){
                        //Si el nuevo cliente tiene mayor prioridad
                        if(clienteElegido->prioridad < clientes[z].prioridad){
                            clienteElegido= &clientes[z];   
                        //Si el cliente a comprobar tiene la misma prioridad pero espera más, se selecciona:
                        } else if(clienteElegido->prioridad == clientes[z].prioridad && clienteElegido->id > clientes[z].id){ 
                            clienteElegido= &clientes[z];
                        }
                    //Si no se encontro un cliente aun
                    } else { clienteElegido= &clientes[z]; }
                }
            }

            // Si no se encuentra, se busca cliente de tipo app. Luego se elige en funcion
            // de la prioridad y si tienen la misma, en funcion del tiempo de espera.
            if(clienteElegido == NULL){

                 for(int z= 0; z<20; z++){
                    if(clientes[z].tipo==1 && clientes[z].atendido==0){
                        if(clienteElegido != NULL){
                            if(clienteElegido->prioridad < clientes[z].prioridad){
                                clienteElegido= &clientes[z]; 
                            } else if(clienteElegido->prioridad == clientes[z].prioridad && clienteElegido->id > clientes[z].id){ 
                                clienteElegido= &clientes[z];
                            }
                        } else { clienteElegido= &clientes[z]; }
                    }
                }
            }

            //1a. Si no se ha encontrado, duermo 3 segundos y regreso a 1.
            if(clienteElegido == NULL){ 
                pthread_mutex_unlock(&colaClientes);
                sleep(3);
                
            
            } else {

                if(clienteElegido->tipo == 2) strcpy(identificadorCliente,"clired_");
                else  strcpy(identificadorCliente,"cliapp_");

                sprintf(nIdentificadorCliente,"%d",clienteElegido->id);
                strcat(identificadorCliente, nIdentificadorCliente);
                strcpy(identificadorAtencion, identificadorTecnico);
                strcat(identificadorAtencion, "_");
                strcat(identificadorAtencion,identificadorCliente);

		//2.  Cambiamos el flag de atendido.
                clienteElegido->atendido= 1;
                pthread_mutex_unlock(&colaClientes);

                //3.  Calculamos el tipo de atención y en función de esto el tiempo de atención (el 80%, 10%, 10%).
                int tiempoAtencion= 0;
                char motivo[50]="";
                char auxMotivo[50]="";
                int tipoAtencion=calculaAleatorios(1,10);

                switch(tipoAtencion){
                    case 10:
                    case 9:
                    case 8:
                    case 7:
                    case 6:
                    case 5:
                    case 4:
                    // 80% bien identificados
                    case 3: 
                        tiempoAtencion= calculaAleatorios(1,4); 
                        strcpy(auxMotivo, "Bien identificado."); 
                        break;
                    // 10% mal identificados
                    case 2: 
                        tiempoAtencion= calculaAleatorios(2,6); 
                        strcpy(auxMotivo, "Mal identificado."); 
                        break;
                        //10% confusion de compañia, abandonan sistema
                    case 1: 
                        tiempoAtencion= calculaAleatorios(1,2); 
                        strcpy(auxMotivo, "Confusion de compañia. Abandona el sistema.");
                        break;
                }


                //4.  Guardamos en el log que comienza la atención
                pthread_mutex_lock(&fichero);
                writeLogMessage(identificadorAtencion,"Comienza la atención.");
                pthread_mutex_unlock(&fichero);

                //5.  Dormimos el tiempo de atención.
                sleep(tiempoAtencion);

                //6.  Guardamos en el log que finaliza la atención
                //7.  Guardamos en el log el motivo del fin de la atención.
                strcpy(motivo,"Finaliza la atencion. ");
                strcat(motivo,auxMotivo);
                pthread_mutex_lock(&fichero);
                writeLogMessage(identificadorAtencion,motivo);
                pthread_mutex_unlock(&fichero);

                //8.  Cambiamos el flag de atendido
                pthread_mutex_lock(&colaClientes);
                if(tipoAtencion!=1){
                    clienteElegido->atendido = 2;
                } else{
                    clienteElegido->atendido = 3; //Clientes con confusion de compania
                }
                pthread_mutex_unlock(&colaClientes);

                clienteElegido=NULL;
                
                //9.  Volvemos al paso 1 y buscamos el siguiente.
            }
        }
    }

    void * accionesTecnicoDomiciliario(void* arg){
        while(1){
            pthread_mutex_lock(&solicitudes);
            
            //1. Comprueba el número de solicitudes y se queda bloqueado (cond_wait)
	    //   mientras sea menor de 4.
            if(nSolsDomiciliarias<4) pthread_cond_wait(&suficientesSolicitudesDomiciliarias, &solicitudes);
		
	    //Se buscan los clientes para atender.
            for(int i=0; i<4; i++){
                struct cliente *clienteElegido;
                int encontrado=0;
                int n=0;
                
                pthread_mutex_lock(&colaClientes);
                for(int i=0; i<20 && encontrado==0; i++){
                    if(clientes[i].solicitud==1){
                        clienteElegido = &clientes[i];
                        encontrado = 1;
                        n = i;
                    }
                }

                for(int i=n+1; i<20; i++) {
                    if(clientes[i].solicitud==1) {
                        if(clientes[i].prioridad > clienteElegido->prioridad || (clientes[i].prioridad == clienteElegido->prioridad && clientes[i].id < clienteElegido->id)) {
                            clienteElegido = &clientes[i];
                        } 
                    }
                }
                pthread_mutex_unlock(&colaClientes);

                char identificador[20]="clired_";
                char nIdentificador[20]="";
                sprintf(nIdentificador,"%d",clienteElegido->id);
                strcat(identificador, nIdentificador);
                
                //2. Guardamos en el log que comienza la atencion.
                pthread_mutex_lock(&fichero);
                writeLogMessage(identificador, "Comienza la atención del técnico domiciliario");
                pthread_mutex_unlock(&fichero);
               
               	//3. Duerme un segundo para cada peticion.
                sleep(1);

		//4. Escribe que ha atendido uno.
                pthread_mutex_lock(&fichero);
                writeLogMessage(identificador, "Termina la atención del técnico domiciliario");
                pthread_mutex_unlock(&fichero);

		//5. Cambia el valor del flag solicitud a 0 en el que ha atendido.
                pthread_mutex_lock(&colaClientes);
                clienteElegido->solicitud=0;
                pthread_mutex_unlock(&colaClientes);
            }

	    //6. Cuando se han atendido a los 4 se pone el numero de solicitudes a 0.
            nSolsDomiciliarias=0;

	    //7. Guardamos en el log que se ha finalizado la atencion domiciliaria.
            pthread_mutex_lock(&fichero);
            writeLogMessage("Tecnico domiciliario", "Termina de atender las 4 solicitudes");
            pthread_mutex_unlock(&fichero);

	    //8. Se avisa a los que esperaban por la solicitud domiciliaria
	    //   que se ha finalizado, con un cond_signal.
            for (int i=0; i<4; i++){
                pthread_cond_signal(&atencionDomiciliariaFinalizada);
            }

            pthread_mutex_unlock(&solicitudes);
        }
        //9. Vuelve a 1.
    }

    void writeLogMessage(char *id, char *msg) {
        // Calculamos la hora actual
        time_t now = time(0);
        struct tm *tlocal = localtime(&now);
        char stnow[25];
        strftime(stnow, 25, "%d/%m/%y %H:%M:%S", tlocal);

        // Escribimos en el log
        logFile = fopen("registroTiempos.log", "a");
        fprintf(logFile, "[%s] %s: %s\n", stnow, id, msg);
        fclose(logFile);
    }

    int calculaAleatorios(int min, int max) {
        srand(time(NULL));
        return rand() % (max-min+1) + min;
    }









