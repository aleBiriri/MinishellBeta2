#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include "parser.h"
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

int igualComandos(tline* l, char * comando,int tope);
void cd(char * directorio);
int comprobacionMandatos(tline* l);
void manejador(int sig);
int main(void) {
	char buf[1024];
	tline * line;
	int i,j, pid;
	int* hijos;

	/* EL SIGUIENTE SIGNAL SOLAMENTE ES DE PRUEBA*/
	signal(SIGCHLD,manejador);

	struct sigaction gestionINT;
	gestionINT.sa_handler = SIG_IGN;
	gestionINT.sa_flags = SA_ONESHOT;
	sigaction ( SIGINT, &gestionINT, 0);

	struct sigaction gestionQUIT;
        gestionQUIT.sa_handler = SIG_IGN;
        gestionQUIT.sa_flags = SA_ONESHOT;
        sigaction ( SIGQUIT, &gestionQUIT, 0);

	printf("msh> ");
	while (fgets(buf, 1024, stdin) != NULL) {
		line = tokenize(buf);
		if (line==NULL) {
			continue;
			}
		/*COMPROBACION DE LOS MANDATOS*/
	if(line->ncommands > 0){
		if(igualComandos(line,"cd",2))
			cd(line->commands[0].argv[1]);
		else if(igualComandos(line,"jobs",4))
			printf("Es un jobs\n");
		else if(igualComandos(line,"fg",2))
			printf("Es un fg\n");
		else if((comprobacionMandatos(line) != 0)){
			if(line->ncommands == 1){ // si solo hay un comando
				if(line->commands[0].filename != NULL){//INICIO Comprobacion Existe
					pid = fork();

					if(pid == 0){ // INICIO codigo Hijo
						if(line->redirect_input != NULL){
							dup2(open(line->redirect_input,O_RDONLY),0);
						}
						if(line->redirect_output != NULL){
							dup2(open(line->redirect_output,O_WRONLY|O_TRUNC),1);
						}
						if (line->redirect_error != NULL) {
                       	 				dup2(open(line->redirect_output,O_WRONLY|O_TRUNC),2);
                				}

						/* SI NO HAY BACKGROUND ENTONCES HAGO LA ACCION POR DEFECTO*/
						if(line->background == 0){
							gestionINT.sa_handler = SIG_DFL;
        						gestionINT.sa_flags = SA_ONESHOT;
        						sigaction ( SIGINT, &gestionINT, 0);

							gestionQUIT.sa_handler = SIG_DFL;
     							gestionQUIT.sa_flags = SA_ONESHOT;
       	 						sigaction ( SIGQUIT, &gestionQUIT, 0);

						}
						execvp(line->commands[0].filename,line->commands[0].argv);
					}//FIN codigo Hijo
					else{
						if(line->background == 0){
							int status;
							waitpid(pid,&status,0);
						}
					}

				}//FIN Comprobacion Existe
			}// fin si solo hay un comando
			else{

				int salidaPipe;
				 fprintf(stderr,"Hola, hay mas de un mandato.\n");
				/*RESERVAMOS MEMORIA PARA GUARDAR LOS PIDS*/
				hijos = malloc(line->ncommands*sizeof(int));
				for(i = 0; i < line->ncommands; i++)
					hijos[i] = -1;

				/*DECLARAMOS E INICIALIZAMOS EL PIPE*/
				int p[2];
				pipe(p);

				/*CREAMOS EL PRIMER HIJO*/
				hijos[0] = fork();
        			if(hijos[0] == 0){
                			close(p[0]);
                			dup2(p[1],1);
					fprintf(stderr,"Hola, soy el hijo 0.\n");
					if(line->redirect_input != NULL){
                                     		dup2(open(line->redirect_input,O_RDONLY),0);
                              		}
					if(line->background == 0){
                                          	gestionINT.sa_handler = SIG_DFL;
                                             	gestionINT.sa_flags = SA_ONESHOT;
                                             	sigaction ( SIGINT, &gestionINT, 0);

                                            	gestionQUIT.sa_handler = SIG_DFL;
                                          	gestionQUIT.sa_flags = SA_ONESHOT;
                                   		sigaction ( SIGQUIT, &gestionQUIT, 0);

                                      	}

                			execvp(line->commands[0].filename,line->commands[0].argv);
                			exit(1);
        			}

				/* CREACION DE LOS HIJOS*/
				int i;
				for(i = 1; i < line->ncommands; i++){//INICIO FOR
					if(i != line->ncommands -1){
						close(p[1]);
                        			salidaPipe = dup(p[0]);
                        			pipe(p);
                        			hijos[i] = fork();
                                		if(hijos[i] == 0){
                                        		close(p[0]);
                                        		dup2(p[1],1);
                                        		dup2(salidaPipe,0);

							if(line->background == 0){
                                                		gestionINT.sa_handler = SIG_DFL;
                                                		gestionINT.sa_flags = SA_ONESHOT;
                                                		sigaction ( SIGINT, &gestionINT, 0);

                                                		gestionQUIT.sa_handler = SIG_DFL;
                                                		gestionQUIT.sa_flags = SA_ONESHOT;
                                                		sigaction ( SIGQUIT, &gestionQUIT, 0);

                                        		}

                                        		execvp(line->commands[i].filename,line->commands[i].argv);
                                        		exit(1);
                                		}//FIN hijos[i]== 0
                			}//FIN  i != NUMEROMANDATOS -1
                			else{
						hijos[i] = fork();
                        			if(hijos[i] == 0){
                                        		close(p[1]);
                                        		dup2(p[0],0);
							if(line->redirect_output != NULL){
        	                                        	dup2(open(line->redirect_output,O_WRONLY|O_TRUNC),1);
                	                        	}
							if (line->redirect_error != NULL) {
                                                        	dup2(open(line->redirect_output,O_WRONLY|O_TRUNC),2);
                                                	}
							if(line->background == 0){
                                               	 		gestionINT.sa_handler = SIG_DFL;
                                                		gestionINT.sa_flags = SA_ONESHOT;
                                                		sigaction ( SIGINT, &gestionINT, 0);

                                                		gestionQUIT.sa_handler = SIG_DFL;
                                                		gestionQUIT.sa_flags = SA_ONESHOT;
                                                		sigaction ( SIGQUIT, &gestionQUIT, 0);
                                        		}
                                        		execvp(line->commands[i].filename,line->commands[i].argv);
                                        		exit(1);
                       	 			}
                        			else{
							fprintf(stderr,"Hola soy el padre.\n");
                                			close(p[1]);
                                			close(p[0]);
                                			int j;
                                			int status2;
                                			for(j = 0;j < line->ncommands;j++){
                                        			waitpid(hijos[j],&status2,0);
                                			}

                        			}
                			}

				}//FIN FOR
			}//FIN SI HAY MAS DE UN MANDATO









		/*
		if (line->redirect_input != NULL) {
			printf("redirección de entrada: %s\n", line->redirect_input);
		}
		if (line->redirect_output != NULL) {
			printf("redirección de salida: %s\n", line->redirect_output);
		}
		if (line->redirect_error != NULL) {
			printf("redirección de error: %s\n", line->redirect_error);
		}
		if (line->background) {
			printf("comando a ejecutarse en background\n");
		}
		for (i=0; i<line->ncommands; i++) {
			printf("orden %d (%s):\n", i, line->commands[i].filename);
			for (j=0; j<line->commands[i].argc; j++) {
				printf("  argumento %d: %s\n", j, line->commands[i].argv[j]);
			}
		}*/
		}
		}
		printf("msh> ");

	}//FIN WHILE
	return 0;
}


int comprobacionMandatos(tline* l){
	int n,bandera = 1;
     	for(n = 0; n < l->ncommands;n++){
           	if(l->commands[n].filename == NULL){
                 	printf("%s : No se encuentra el mandato.\n",l->commands[n].argv[0]);
			bandera = 0;
         	}
	}
	return bandera;
}

void manejador(int sig){
	/* SI SOY UN HIJO Y NO ESTOY EN BACKGROUND HAGO UN EXIT*/
//	fprintf(stderr,"HOLA ME LLEGO LA SEÑAL, UN HIJO A TERMINADO.\n");
}

int igualComandos(tline* l,char * cadena,int tope){

	int i,bandera = 0;
	if(strncmp(l->commands[0].argv[0],cadena, tope)== 0){// si son del mismo tamaño compruebo que las letras son las mismas
		bandera = 1;
		for(i = 0; i < tope; i++){
			if(l->commands[0].argv[0][i] != cadena[i])
				bandera = 0;
		}
	}
	return bandera;
}

void cd(char * directorio){
	char directorioActual[FILENAME_MAX];
	if(directorio == NULL)
		directorio = getenv("HOME");
	if(chdir(directorio) != 0)
      		printf("Error: %s\n",strerror(errno));
	else{
		getcwd(directorioActual,FILENAME_MAX);
		printf("%s\n",directorioActual);
	}
}
