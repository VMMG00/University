/*
 * Practica tema 5, Morales Garcia Victor
 * Servidor UDP
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

#define SIZEBUFF 64       // tamano buffer

int main(int argc, char *argv[]){

  FILE *fich;
  int sock;                                     //  socket
  int puerto;                                   //  puerto funcionamiento del servicio
  struct sockaddr_in servidor, cliente;         //  estructuras de direccionamiento
  socklen_t longitudC = sizeof(cliente);
  socklen_t longitudS = sizeof(servidor);
  struct servent *servicio;                     //  estructura que contiene puerto del servicio
  char buffer[SIZEBUFF], hostname[SIZEBUFF];
  char dosPuntos[] = ": ";

  /*
   * Servidor funcionando indefinidamente
   */
  while(1){

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
      servicio = getservbyname("daytime", "udp");

      // Si no encuentra el servicio (para buscar puerto), termina
      if (servicio == NULL) {
        fprintf(stderr, "No encontrado servicio DAYTIME para el protocolo UDP\n");
        exit(EXIT_FAILURE);
      }

      puerto = htons(servicio->s_port);
    }

    /*
     * Se crea el socket datagrama con el protocolo UDP.
     * Si hay error en la creacion del socket, se imprime
     * mensaje de error, y termina.
     */
    sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock<0) {
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

    if (bind(sock, (struct sockaddr *)&servidor, sizeof(servidor)) < 0){
      perror("bind()");
      exit(EXIT_FAILURE);
    }

    /*
     * Se recibe un mensaje en el socket 'sock' en 'buffer' de tamano maximo
     * de SIZEBUFF desde un cliente. Al ser por protocolo UDP, los ultimos datos
     * parametros no son nulos, por lo que el nombre del client se almacena en
     * la estructura de datos 'cliente', y el tamano de la direccion del cliente
     * se almacena en 'longitudC'.
     *
     * Si hay un error en la recepcion, se imprime mensaje de error y termina.
     */
    if(recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &cliente, &longitudC) <0){
       perror("recvfrom()");
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

    //printf("La fecha y la hora son %s\n",buffer);

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
     * Se envia el datagrama al cliente.
     * Si hay error, se imprime mensaje de error y termina.
     */
    if (sendto(sock, buffer, SIZEBUFF, 0, (struct sockaddr *)&cliente, longitudS) == -1) {
      perror("sendto()");
      exit(EXIT_FAILURE);
    }

    /*
     * Se libera el socket.
     */
    close(sock);
  }
}
