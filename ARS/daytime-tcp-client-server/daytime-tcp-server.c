/*
 * Practica tema 5, Morales Garcia Victor
 * Servidor TCP
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>       // strncmp()
#include <netdb.h>        // getservbyname()
#include <errno.h>        // Para usar errno y perror()
#include <stdio.h>        // Para usar perror()
#include <stdlib.h>       // Para usar EXIT_FAILURE
#include <unistd.h>       // close, gethostname
#include<signal.h>        // gestion de senales (interrupcion, etc.)

#define SIZEBUFF 64       // tamano buffer

/*
 * Variables globales, para que puedan ser accedidas por signal_handler().
 */
int sock;                                     //  socket para aceptar conexiones
int conexion;                                 //  socket conectado al cliente
char buffer[SIZEBUFF];

/*
 * Funcion de gestion de senal de interrupcion.
 * Si la senal recibida es de interrupcion (SIGINT - Ctl-c),
 * cierra los socket de aceptacion de conexiones y los conectados a clientes.
 */
void signal_handler(int sig){
  int resultado;

  if (sig == SIGINT){

    /*
     * Cierra conexion tanto conexiones entrantes como salientes.
     * Si hay error, imprime mensaje y finaliza.
     */
    if (shutdown(conexion, SHUT_RDWR) < 0){
      perror("shutdown()");
      exit(EXIT_FAILURE);
    }

    /*
     * Recive los bytes pendientes antes de cerrar definitivamente la conexion.
     * (Buena practica para no perder datos).
     */
    do{
      resultado = recv(conexion, buffer, SIZEBUFF, 0);

      if (resultado == -1){
          perror("recv()");
          exit(EXIT_FAILURE);
      }
    } while (resultado > 0);

    /*
     * Se cierran definitivamente ambos sockets (el de conexion con cliente
     * y el de aceptacion de conexiones).
     */
    close(conexion);
    close(sock);
    exit(EXIT_SUCCESS);
  }
}

int main(int argc, char *argv[]){

  FILE *fich;
  int puerto;                                   //  puerto funcionamiento del servicio
  struct sockaddr_in servidor, cliente;         //  estructuras de direccionamiento servidor y cliente
  socklen_t longitudC = sizeof(cliente);        //  tamano de estructura datos del cliente
  socklen_t longitudS = sizeof(servidor);       //  tamano de estructura de datos del servidor
  struct servent *servicio;                     //  estructura que contiene puerto del servicio
  char hostname[SIZEBUFF];                      //  nombre del host
  char dosPuntos[] = ": ";


  /*
   * Con cantidad erronea de argumentos, mensaje de error, y termina.
   */
  if (argc > 3 || argc == 2) {
    fprintf(stderr, "Uso: %s [-p puerto-servidor]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  /*
   * Si se proporciona el puerto como argumento, que la bandera '-p'
   * sea correcta.
   */
  if (argc == 3 && strncmp(argv[1], "-p", 2) != 0) {
    fprintf(stderr, "Uso: %s [-p puerto-servidor]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  /*
   * Si usuario proporciona puerto, se almacena.
   * Si no, se busca puerto por defecto en el sistema para servicio DAYTIME.
   */
  if (argc == 3) {
    sscanf(argv[2],"%d",&puerto);
  }
  else {
    servicio = getservbyname("daytime", "tcp");

    // Si no encuentra el servicio (para buscar puerto), termina
    if (servicio == NULL) {
      fprintf(stderr, "No encontrado servicio DAYTIME para el protocolo UDP\n");
      exit(EXIT_FAILURE);
    }

    puerto = htons(servicio->s_port);
  }

  /*
   * Servidor funcionando indefinidamente
   */

  /*
   * Se crea el socket datagrama con el protocolo TCP.
   * Si hay error en la creacion del socket, se imprime
   * mensaje de error, y termina.
   */
  sock = socket(AF_INET, SOCK_STREAM, 0);

  if (sock<0){
    perror("socket()");
    exit(EXIT_FAILURE);
  }

  /*
   * Se establecen los valores del servidor. La direccion IP del servidor
   * se establece como INADDR_ANY, para que el servidor pueda recibir
   * mensajes por cualquiera de las conexiones fisicas de la maquina host.
   *
   * Se enlaza los datos del servidor al socket, para que los clientes
   * pueden enviar mensajes a este servidor.
   */
  servidor.sin_family      = AF_INET;       /* servidor is in Internet Domain */
  servidor.sin_port        = puerto;        /* Usa el puerto especificado     */
  servidor.sin_addr.s_addr = htonl(INADDR_ANY);    /* direccion del servidor   */

  /*
   * Se enlaza los datos del servidor al socket, para que los clientes
   * pueden enviar mensajes a este servidor.
   */
  if (bind(sock, (struct sockaddr *)&servidor, longitudS) < 0){
    perror("bind()");
    exit(EXIT_FAILURE);
  }

  /*
   * Bucle infinito. Termina con la senal de interrupcion (SIGINT = Ctl-c).
   */
  while(1){

    /* Deteccion de senal de interrupcion (funcion 'signal()')
     * Parametros: nombre senal, funcion de gestion de senal
     */
    if (signal(SIGINT, signal_handler) == SIG_ERR){
      perror("signal()");
    }

    /*
     * Modo escucha para recibir conexiones.
     * Se establece el valor de 'backlog'a 1 (tamano cola conexiones entrantes).
     */
    if (listen(sock, 1) != 0)
    {
        perror("listen()");
        exit(EXIT_FAILURE);
    }

    /*
     * Aceptacion de la conexion.
     */
    if ((conexion = accept(sock, (struct sockaddr *)&cliente, &longitudC)) == -1)
    {
        perror("accept()");
        exit(EXIT_FAILURE);
    }

    /*
     * Recive el mensaje en el socket de conexion.
     */
    if (recv(conexion, buffer, SIZEBUFF, 0) == -1)
    {
        perror("recv()");
        exit(EXIT_FAILURE);
    }

    /* Se obtiene la fecha y la hora, ejecutando el comando 'date', se almacena
     * su resultado en el fichero temporal '/tmp/tt.txt', y se almacena
     * en 'buffer'.
     */
    system("date > /tmp/tt.txt");
    fich = fopen("/tmp/tt.txt","r");
    if (fgets(buffer,SIZEBUFF,fich)==NULL) {
      printf("Error en system(), en fopen(), o en fgets()\n");
      exit(EXIT_FAILURE);
    }

    /*
     * Se obtiene el nombre del host.
     */
    gethostname(hostname, SIZEBUFF);

    /*
     * Se concatenan los string para formatear el mensaje de respuesta.
     */
    strcat(hostname, dosPuntos);
    strcat(hostname, buffer);
    strcpy(buffer, hostname);

    /*
     * Se envia mensaje al cliente.
     * Si hay error, se imprime mensaje de error y termina.
     */
    if (send(conexion, buffer, SIZEBUFF, 0) < 0) {
      perror("send()");
      exit(EXIT_FAILURE);
    }
  }
}
