    #include <pthread.h> 
    #include <signal.h>
    #include <stdio.h>
    #include <time.h>
    #include <unistd.h>
    #include <stdlib.h>

    pthread_mutex_t fichero, colaClientes, solicitudes;
    pthread_cond_t suficientesSolicitudesDomiciliarias;
    int nClientesApp, nClientesRed,nSolsDomiciliarias, nClientesNoAtendidos;
    struct cliente {
        int id, atendido, tipo, prioridad, solicitud;
    } clientes[20];
    pthread_t tecnico_1, tecnico_2, resprep_1, resprep_2, encargado, atencionDomiciliaria;
    FILE * logFile;
    struct sigaction clienteApp, clienteRed, terminar; //DECLARACIÓN GLOBAL???


    int calculaAleatorios(int min, int max);
    void writeLogMessage(char *id, char *msg);
    void nuevoCliente(int sig);
    void terminarPrograma(int sig);
    void *accionesCliente(void *arg);
    void *accionesTecnico(void *arg);
    void *accionesEncargado(void * arg);
    void *accionesTecnicoDomiciliario(void * arg);
    

    int main(int argc, char *argv[]){	

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

        //b.f. Contador de clientes de cada tipo y variables relativas a la solicitud de atención domiciliaria.
        nClientesApp = 0, nClientesRed = 0, nSolsDomiciliarias = 0, nClientesNoAtendidos=0;

        //c. Lista de clientes id 0, Prioridad 0, atendido 0, solicitud 0.
        for(int i=0; i<20; i++){
            clientes[i].id= 0;
            clientes[i].prioridad= 0;
            clientes[i].atendido= 0;
            clientes[i].solicitud= 0;
            clientes[i].tipo=0;
        }


        //e. Fichero de Log
        
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
        for(int i=0; i<20; i++){
            if(clientes[i].id==0){
                nClientesNoAtendidos++;
                char identificador[10];
                char nIdentificador[3];
                int posNuevoCliente=i;
                if(sig==SIGUSR1){
                    clientes[i].id=++nClientesApp;
                    clientes[i].atendido=0;
                    clientes[i].tipo=0;
                    clientes[i].solicitud=0;
                    clientes[i].prioridad=calculaAleatorios(1,10);
                    strcat(identificador,"cliapp_");
                } else{
                    clientes[i].id=++nClientesRed;
                    clientes[i].atendido=0;
                    clientes[i].tipo=1;
                    clientes[i].solicitud=0;
                    clientes[i].prioridad=calculaAleatorios(1,10);
                    strcat(identificador,"clired_");
                }
    
                sprintf(nIdentificador,"%d",clientes[i].id);
                strcat(identificador, nIdentificador);
                pthread_mutex_lock(&fichero);
                writeLogMessage(identificador, "Nuevo cliente accede al sistema");
                pthread_mutex_unlock(&fichero);
                
                pthread_create(&nuevoCliente, NULL, accionesCliente, (void *)&posNuevoCliente);
                i=20;
            }
        }
        pthread_mutex_unlock(&colaClientes);
    }

    void terminarPrograma(int sig) {
        clienteApp.sa_handler=SIG_IGN;
        clienteRed.sa_handler=SIG_IGN;
        //SOLUCION PROVISIONAL PARA QUE NO PUEDAN LLEGAR NUEVOS CLIENTES
        if (sigaction(SIGUSR1, &clienteApp, NULL)==-1) {
            perror("Llamada a sigaction cliente app terminar.");
            exit(-1);
        }

        if (sigaction(SIGUSR2, &clienteRed, NULL)==-1) {
            perror("Llamada a sigaction cliente red terminar.");
            exit(-1);
        }

        pthread_mutex_lock(&colaClientes);
        while(nClientesNoAtendidos!=0){ //Hasta que todos los clientes no esten atentidos(por máquina o por recepcionista)
            pthread_mutex_unlock(&colaClientes);//el programa no se puede cerrar
            sleep(3);
        }
        pthread_mutex_lock(&fichero);
        writeLogMessage("ESTADO", "No quedan clientes por atender, se cierra el programa");
        pthread_mutex_unlock(&fichero);
        if(pthread_mutex_destroy(&fichero)!=0) exit(-1);
        if(pthread_mutex_destroy(&colaClientes)!=0) exit(-1);
        if(pthread_mutex_destroy(&solicitudes)!=0) exit(-1);
        if(pthread_cond_destroy(&suficientesSolicitudesDomiciliarias)!=0) exit(-1);
        exit(0);
    }

    void * accionesCliente(void *arg){
        //1.  Guardar en el log la hora de entrada.
        //2.  Guardar en el log el tipo de cliente.
        char[3] tipo;        
        switch(tipoCliente){
            case 0: tipo= "APP"
            case 1: tipo= "RED"
        }
        
        thread_mutex_lock(&fichero);
        writeLogMessage(clientes[arg], "Cliente de tipo " + tipo + " entra");
        pthread_mutex_unlock(&fichero);

        while(true){
            //  3. Comprueba si está siendo atendido.
            if(clientes[arg].atendido == 0){

                pthread_mutex_lock(&colaClientes);
                clientes[arg].atendido= 1;
                pthread_mutex_unlock(&colaClientes);

                //a.  Si no lo está, calculamos el comportamiento del cliente (si se va por
                //    dificultad o si se cansa de esperar algo que solo ocurre cada 8
                //    segundos) y también el caso de que pierda la conexión a internet.
                switch(calculaAleatorios(1,10)){
                    //  Un 20 % de los clientes, se cansa de esperar y sale de la aplicaci´on cuando
                    //  han transcurrido 8 segundos. 
                    case 10:
                    case 9: 
                        sleep(8);

                        thread_mutex_lock(&fichero);
                        writeLogMessage(clientes[arg], "Cliente de tipo " + tipo + " se cansa de esperar y se marcha");
                        pthread_mutex_unlock(&fichero);

                        pthread_mutex_lock(&colaClientes);
                        clientes[arg].id=0;
                        clientes[arg].tipo=-1;
                        pthread_mutex_unlock(&colaClientes);

                        pthread_exit(NULL);
                        break;
                    //  Un 10 % de los clientes encuentra difícil la aplicación y se va de inmediato.
                    case 8:

                        thread_mutex_lock(&fichero);
                        writeLogMessage(clientes[arg], "Cliente de tipo " + tipo + " encuentra muy dificil la aplicacion y se marcha");
                        pthread_mutex_unlock(&fichero);

                        pthread_mutex_lock(&colaClientes);
                        clientes[arg].id=0;
                        clientes[arg].tipo=-1;
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
                    case 1: if(calculaAleatorios(1,20) == 20){
                        
                        thread_mutex_lock(&fichero);
                        writeLogMessage(clientes[arg], "Cliente de tipo " + tipo + " ha perdido la conex y abandona");
                        pthread_mutex_unlock(&fichero);

                        pthread_mutex_lock(&colaClientes);
                        clientes[arg].id=0;
                        clientes[arg].tipo=-1;
                        pthread_mutex_unlock(&colaClientes);
                        
                        pthread_exit(NULL);
                    }
                }
                //  c. Sino debe dormir 2 segundos y vuelve a 3.

                pthread_mutex_lock(&colaClientes);
                clientes[arg].atendido= 0;
                pthread_mutex_unlock(&colaClientes);

                sleep(2);

            //4.  Si está siendo atendido por el técnico correspondiente debemos esperar a que
            //    termine (puede comprobar cada 2 segundos).
            } else {
                while(clientes[arg].atendido == 1) { 
                    sleep(2);
                }

                pthread_mutex_lock(&colaClientes);
                clientes[arg].atendido= 1;
                pthread_mutex_unlock(&colaClientes);

                //5.  Si el cliente es de tipo red y quiere realizar una solicitud domiciliaria
                if(tipo == "RED"){
                    while(true){
                        //a.  Comprueba el número de solicitudes pendientes
                        //b.  Si es menor de 4 incrementa el valor
                        if(nSolsDomiciliarias < 4){ 
                            thread_mutex_lock(&solicitudes); 
                            nSolsDomiciliarias++; 
                            pthread_mutex_unlock(&solicitudes);
                        
                            //i. Escribe en el log que espera para ser atendido
                            thread_mutex_lock(&fichero);
                            writeLogMessage(clientes[arg], "Cliente de tipo " + tipo + " espera para ser atendido por un técnico");
                            pthread_mutex_unlock(&fichero);

                            //ii. Cambia el valor de solicitud a 1
                            clientes[arg].solicitud= 1;

                            //iii. Si es el cuarto avisa al técnico con condition_signal
                            //COMPROBAR
                            if(nSolsDomiciliarias == 4){ pthread_cond_signal(&suficientesSolicitudesDomiciliarias); }

                            //iv. Se bloquea (condition_wait) hasta que la solicitud pase a ser 0 y
                            //por tanto ya se haya finalizado la atención
                            pthread_cond_wait(&suficientesSolicitudesDomiciliarias, &solicitudes);

                            //v. Comunica que su atención se ha finalizado
                            thread_mutex_lock(&fichero);
                            writeLogMessage(clientes[arg], "Cliente de tipo " + tipo + " ha terminado de ser atendido por un técnico");
                            pthread_mutex_unlock(&fichero);

                            break;
                        }
                    }
                }

                //6.  Sino libera su posición en cola de clientes y se va.
                pthread_mutex_lock(&colaClientes);
                clientes[arg].id=0;
                clientes[arg].tipo=-1;
                pthread_mutex_unlock(&colaClientes);

                //7.  Escribe en el log
                thread_mutex_lock(&fichero);
                writeLogMessage(clientes[arg], "Cliente de tipo " + tipo + " ha FINALIZADO");
                pthread_mutex_unlock(&fichero);

                //8.  Fin del hilo Cliente
                pthread_exit(NULL);
            }
        }
    }

    void * accionesTecnico(void* arg){

        char tipo[] = (char*) arg; //posible segmentation fault
        struct cliente* clienteElegido;
        int n;
        int encontrado;
        int clientesDescanso = 0;

        //se mira si son tecnicos
        if(strcmp(tipo, "tecnico_1")==0||strcmp(tipo, "tecnico_2")==0) {
            while(1) {

            n=0;
            encontrado=0;

            pthread_mutex_lock(&colaClientes);

            //se busca un cliente de tipo app sin atender por el que empezar a comparar
            for(int i=0; i<20 && encontrado==0; i++){
                if(clientes[i].tipo==0 && clientes[i].atendido==0){
                    clienteElegido = &clientes[i];
                    encontrado = 1;
                    n = i;
                }
            }

            //si hay algun cliente de tipo app y no atendido se empieza la comparación con el resto
            if(encontrado==1){
                //se empieza a comparar desde la posicion del primero que se encontró
                for(int i=n+1; i<20; i++) {
                    if(clientes[i].tipo == 0 && clientes[i].atendido==0) {
                        //se comparan prioridades, si son iguales se coge al primero de la lista que llevará más tiempo esperando??
                        if(clientes[i].prioridad > clienteElegido->prioridad) {
                            clienteElegido = &clientes[i];
                        }
                    }
                }

                clienteElegido->atendido = 1;
                pthread_mutex_unlock(&colaClientes);

                int tipoAtencion = calculaAleatorios(0,100);
                int tiempoAtencion = 0;
                char motivo[50];

                if(tipoAtencion <= 80) {
                    //80% bien identificados
                    tiempoAtencion = calculaAleatorios(1,4);
                    strcat(motivo, "Bien identificado.");
                } else if (tipoAtencion > 80 && tipoAtencion <= 90) {
                    //10% mal identificados
                    tiempoAtencion = calculaAleatorios(2,6);
                    strcat(motivo, "Mal identificado.");
                } else {
                    //10% confusion de compañia, abandonan sistema
                    tiempoAtencion = calculaAleatorios(1,2)
                    strcat(motivo, "Confusion de compañia. Abandona el sistema.");
                }

                pthread_mutex_lock(&fichero);
                writeLogMessage("Comienza la atención:\n");
                pthread_mutex_unlock(&fichero);

                sleep(tiempoAtencion);

                pthread_mutex_lock(&fichero);
                writeLogMessage("Finaliza la atención: ");
                writeLogMessage(motivo+"\n");
                pthread_mutex_unlock(&fichero);

                pthread_mutex_lock(&colaClientes);
                clienteElegido->atendido = 0;
                pthread_mutex_unlock(&colaClientes);
     
                //se aumenta contador parcial de clientes y se comprueba si se descansa
                clientesDescanso++;
                if(clientesDescanso==5) {
                    sleep(5);
                    clientesDescanso = 0;
                }


            } else {
                pthread_mutex_unlock(&colaClientes);
                sleep(1);
            }
            
        }

        //son responsables de reparaciones
        } else {
            while(1) {

                n=0;
                encontrado=0;

                pthread_mutex_lock(&colaClientes);

                //se busca un cliente de tipo red sin atender por el que empezar a comparar
                for(int i=0; i<20 && encontrado==0; i++){
                    if(clientes[i].tipo==1 && clientes[i].atendido==0){
                        clienteElegido = &clientes[i];
                        encontrado = 1;
                        n = i;
                    }
                }

                //si hay algun cliente de tipo red y no atendido se empieza la comparación con el resto
                if(encontrado==1){
                    //se empieza a comparar desde la posicion del primero que se encontró
                    for(int i=n+1; i<20; i++) {
                        if(clientes[i].tipo == 1 && clientes[i].atendido==0) {
                            //se comparan prioridades, si son iguales se coge al primero de la lista que llevará más tiempo esperando??
                            if(clientes[i].prioridad > clienteElegido->prioridad) {
                                clienteElegido = &clientes[i];
                            }
                        }
                    }

                    clienteElegido->atendido = 1;
                    pthread_mutex_unlock(&colaClientes);

                    //calculo del tiempo de atencion
                    int tipoAtencion = calculaAleatorios(0,100);
                    int tiempoAtencion = 0;
                    char motivo[50];

                    if(tipoAtencion <= 80) {
                        //80% bien identificados
                        tiempoAtencion = calculaAleatorios(1,4);
                        strcat(motivo, "Bien identificado.");
                    } else if (tipoAtencion > 80 && tipoAtencion <= 90) {
                        //10% mal identificados
                        tiempoAtencion = calculaAleatorios(2,6);
                        strcat(motivo, "Mal identificado.");
                    } else {
                        //10% confusion de compañia, abandonan sistema
                        tiempoAtencion = calculaAleatorios(1,2)
                        strcat(motivo, "Confusion de compania. Abandona el sistema.");
                    }

                    pthread_mutex_lock(&fichero);
                    writeLogMessage("Comienza la atencion: \n");
                    pthread_mutex_unlock(&fichero);

                    sleep(tiempoAtencion);

                    pthread_mutex_lock(&fichero);
                    writeLogMessage("Finaliza la atencion: ");
                    writeLogMessage(motivo+"\n");
                    pthread_mutex_unlock(&fichero);

                    pthread_mutex_lock(&colaClientes);
                    clienteElegido->atendido = 0;
                    pthread_mutex_unlock(&colaClientes);

                    //se aumenta contador parcial de clientes y se comprueba si se descansa
                    clientesDescanso++;
                    if(clientesDescanso==6) {
                        sleep(5);
                        clientesDescanso = 0;
                    }


                } else {
                    pthread_mutex_unlock(&colaClientes);
                    sleep(1);
                }
                
            }
        }
    }

    void * accionesEncargado(void * arg){
        
        struct cliente* clienteElegido=NULL;

        while(true){

            pthread_mutex_lock(&colaClientes);

            //1. se busca un cliente de tipo red sin atender por el que empezar a comparar
            //   si existe, se elegirá al de mayor prioridad y si tienen la misma prioridad
            //   al de mayor tiempo de espera...
            for(int z= 0; z<20; z++){
                //Si se encuentra a un cliente disponible...
                if(clientes[z].tipo==1 && clientes[z].atendido==0){
                    //Si ya se selecciono un cliente
                    if(clienteElegido != NULL){
                        //Si el nuevo cliente tiene mayor prioridad
                        if(clienteElegido.prioridad < clientes[z].prioridad){
                            clienteElegido= &clientes[z];   
                        //Si el cliente a comprobar tiene la misma prioridad pero espera más, se selecciona:
                        } else if(clienteElegido.prioridad == clientes[z].prioridad && clienteElegido.id > clientes[z].id){ 
                            clienteElegido= &clientes[z];
                        }
                    //Si no se encontro un cliente aun
                    } else { clienteElegido= &clientes[z]; }
                }
            }

            // si no se encuentra, se busca de tipo app, se busca de tipo red...
            if(clienteElegido == NULL){

                 for(int z= 0; z<20; z++){
                    if(clientes[z].tipo==0 && clientes[z].atendido==0){
                        if(clienteElegido != NULL){
                            if(clienteElegido.prioridad < clientes[z].prioridad){
                                clienteElegido= &clientes[z]; 
                            } else if(clienteElegido.prioridad == clientes[z].prioridad && clienteElegido.id > clientes[z].id){ 
                                clienteElegido= &clientes[z];
                            }
                        } else { clienteElegido= &clientes[z]; }
                    }
                }
            }

            //a. si no se han encontrado, duermo 3 segundos y regreso a 1.
            if(clienteElegido == NULL){ 
                pthread_mutex_unlock(&colaClientes);
                sleep(3);
                
            //2.  si lo he encontrado, cambiamos el flag de atendido
            } else {
                *clienteElegido->atendido= 1;
                pthread_mutex_unlock(&colaClientes);

                //3.  Calculamos el tipo de atención y en función de esto el tiempo de atención (el 80%, 10%, 10%).
                int tiempoAtencion= 0;
                char motivo[50];

                switch(calculaAleatorios(1,10)){
                    case 10:
                    case 9:
                    case 8:
                    case 7:
                    case 6:
                    case 5:
                    case 4:
                    // 80% bien identificados
                    case 3: tiempoAtencion= calculaAleatorios(1,4); strcat(motivo, "Bien identificado."); break;
                    // 10% mal identificados
                    case 2: tiempoAtencion= calculaAleatorios(2,6); strcat(motivo, "Mal identificado."); break;
                        //10% confusion de compañia, abandonan sistema
                    case 1; tiempoAtencion= calculaAleatorios(1,2); strcat(motivo, "Confusion de compañia. Abandona el sistema.");
                }

                //4.  Guardamos en el log que comienza la atención
                pthread_mutex_lock(&fichero);
                writeLogMessage("Comienza la atención:\n");
                pthread_mutex_unlock(&fichero);

                //5.  Dormimos el tiempo de atención.
                sleep(tiempoAtencion);

                //6.  Guardamos en el log que finaliza la atención
                pthread_mutex_lock(&fichero);
                writeLogMessage("Finaliza la atención:\n");

                //7.  Guardamos en el log el motivo del fin de la atención.
                writeLogMessage(motivo);
                pthread_mutex_unlock(&fichero);

                //8.  Cambiamos el flag de atendido
                    pthread_mutex_lock(&colaClientes);
                *clienteElegido->atendido
                pthread_mutex_unlock(&colaClientes);
                //9.  Volvemos al paso 1 y buscamos el siguiente.
            }
        }
    }

    void * accionesTecnicoDomiciliario(void* arg){
        while(1){
            pthread_mutex_lock(&solicitudes);
            if(nSolsDomiciliarias<4) pthread_cond_wait(&suficientesSolicitudesDomiciliarias, &solicitudes);
            
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
                    if(clientes[i].tipo == 0 && clientes[i].atendido==0) {
                        if(clientes[i].prioridad > clienteElegido->prioridad || (clientes[i].prioridad == clienteElegido->prioridad && clientes[i].id < clienteElegido->id)) {
                            clienteElegido = &clientes[i];
                        } 
                    }
                }
                pthread_mutex_unlock(&colaClientes);

                char identificador[10]="clired_";
                char nIdentificador[3];
                sprintf(nIdentificador,"%d",clienteElegido->id);
                strcat(identificador, nIdentificador);
                
                pthread_mutex_lock(&fichero);
                writeLogMessage(identificador, "Comienza la atención del técnico domiciliario");
                pthread_mutex_unlock(&fichero);
               
                sleep(1);

                pthread_mutex_lock(&fichero);
                writeLogMessage(identificador, "Termina la atención del técnico domiciliario");
                pthread_mutex_unlock(&fichero);

                pthread_mutex_lock(&colaClientes);
                clienteElegido->solicitud=0;
                pthread_mutex_unlock(&colaClientes);
            }

            nSolsDomiciliarias=0;
            pthread_mutex_lock(&fichero);
            writeLogMessage("Tecnico domiciliario", "Termina de atender las 4 solicitudes");
            pthread_mutex_unlock(&fichero);

            pthread_cond_signal(&suficientesSolicitudesDomiciliarias);
            pthread_mutex_unlock(&solicitudes);
        }
        

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









