/*
 * Practica tema 5, VMMG00
 * Cliente UDP
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>       // strncmp()
#include <netdb.h>        // getservbyname()
#include <errno.h>        // Para usar errno y perror()
#include <stdio.h>        // Para usar perror()
#include <stdlib.h>       // Para usar EXIT_FAILURE
#include <unistd.h>       // close()

#define SIZEBUFF 64       // tamano buffer

int main(int argc, char *argv[])
{

  int sock;                                         // socket
  int puerto;                                       // puerto funcionamiento del servicio
  struct sockaddr_in destino, origen;
  socklen_t longitud = sizeof(struct sockaddr_in);
  struct in_addr direccion;                         // direccionIP transformada
  struct servent *servicio;                         // estructura que contiene puerto del servicio
  char buffer[SIZEBUFF]="test";                     // buffer de datos que se van a transmitir

  /*
   * Con cantidad erronea de argumentos, mensaje de error, y termina.
   */
  if (argc < 2 || argc == 3 || argc > 4) {
    fprintf(stderr, "Uso: %s <direccionIP> [-p puerto-servidor]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  /*
   * Si se proporciona el puerto como argumento, que la bandera '-p'
   * sea correcta.
   */
  if (argc == 4 && strncmp(argv[2], "-p", 2) != 0) {
    fprintf(stderr, "Uso: %s <direccionIP> [-p puerto-servidor]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  /*
   * Si la direccion IP proporcionada como argumento argv[1] no es valida,
   * mensaje de error y termina.
   * 'inet_aton' guarda servidor IP en 'direccion', 1 si correcto, 0 si error.
   */
  if (inet_aton(argv[1], &direccion) == 0) {
    fprintf(stderr, "Direccion IP invalida\n");
    exit(EXIT_FAILURE);
  }

  /*
   * Si usuario proporciona puerto, se almacena.
   * Si no, se busca puerto por defecto en el sistema para servicio DAYTIME.
   */
  if (argc == 4) {
    sscanf(argv[3],"%d",&puerto);
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
   * Se establecen los valores del servidor.
   */
  destino.sin_family = AF_INET;
  destino.sin_port = puerto;
  destino.sin_addr.s_addr = direccion.s_addr;

  /*
   * Se envia el datagrama al servidor destino.
   * Si hay error, se imprime mensaje de error y termina.
   */
  if (sendto(sock, buffer, SIZEBUFF, 0, (struct sockaddr *)&destino, longitud) == -1) {
    perror("sendto()");
    exit(EXIT_FAILURE);
  }

  /*
   * Se recibe el datagrama desde el servidor destino.
   * Si hay error, se imprime mensaje de error y termina.
   */
  if (recvfrom(sock, buffer, SIZEBUFF, 0, (struct sockaddr *)&origen, &longitud) == -1) {
    perror("recvfrom()");
    exit(EXIT_FAILURE);
  }

  /*
   * Se libera el socket.
   */
  close(sock);

  /*
   * Se imprime mensaje recibido desde el servidor.
   */
  printf("%s\n", buffer);

  return 1;
}
