/*
 * Practica tema 8, Morales Garcia Victor
 * Cliente ICMP Ping
 */

#include <sys/socket.h>     // socket()
#include <netinet/in.h>     // IPPROTO_ICMP
#include <arpa/inet.h>
#include <string.h>         // strncmp(), memcpy()
#include <netdb.h>          // getservbyname()
#include <errno.h>          // errno(), perror()
#include <stdio.h>          // perror()
#include <stdlib.h>         // EXIT_FAILURE
#include <unistd.h>         // close()
#include <sys/types.h>      // getpid()

#include "ip-icmp-ping.h"

// Constantes
#define IP4_HEADERLEN 20                   // Longitud cabecera IPv4
#define ICMP_HEADERLEN (8+REQ_DATASIZE)    // Longitud datagrama ICMP (sin payload)

/* Funcion que calcula el valor del campo checksum del datagrama ICMP
 *
 * Argumentos:
 * Puntero al comienzo del datagrama,
 * Longitud, en bloques de 16 bits, del datagrama.
 *
 * Devuelve el valor checksum, como complemento a 1 de 16 bits, de la suma
 * de todos los bytes del datagrama ICMP
 */
short int checksum(void *mem, int longitud)
{
    int numShorts = longitud / 2;       // numero de 'medias palabras'
                                        // (numero de bloques de 16 bits)
    unsigned short int *puntero = mem;  // puntero al inicio del datagrama
    unsigned int acumulador = 0;
    short int respuesta = 0;
    int i;

    /* Recorre en tramos de 16 bits el datagrama ICMP,
     * acumulando los resultados parciales en 'acumulador'
     */
    for (i = 0; i < numShorts - 1; i++ )
    {
      acumulador += (unsigned int) *puntero;
      puntero++;
    }

    /* Realiza las siguientes operaciones, en este orden:
     * Suma parte alta 16 bits a la parte baja de 16 bits
     * Suma el posible acarreo
     * Obtiene el complementario y se trunca a 16 bits
     */
    acumulador = (acumulador >> 16) + (acumulador & 0x0000ffff);
    acumulador += (acumulador >> 16);
    respuesta = ~acumulador;

    return respuesta;
}

/*
 * Imprime un mensaje en funcion del tipo y el codigo
 * de la cabecera ICMP.
 * Recibe el valor del tipo y el valor del codigo
 */
void printResponse(unsigned char t, unsigned char c) {
  char type[64], code[64];

  switch (t) {
    case 3:
      strcpy(type, "Destination Unreachable: ");
      switch (c) {
        case 0:
          strcpy(code, "Destination network unreachable");
          break;
        case 1:
          strcpy(code, "Destination host unreachable");
          break;
        case 2:
          strcpy(code, "Destination protocol unreachable");
          break;
        case 3:
          strcpy(code, "Destination port unreachable");
          break;
        case 4:
          strcpy(code, "Fragmentation required, and DF flag set");
          break;
        case 5:
          strcpy(code, "Source route failed");
          break;
        case 6:
          strcpy(code, "Destination network unknown");
          break;
        case 7:
          strcpy(code, "Destination host unknown");
          break;
        case 8:
          strcpy(code, "Source host isolated");
          break;
        case 9:
          strcpy(code, "Network administratively prohibited");
          break;
        case 10:
          strcpy(code, "Host administratively prohibited");
          break;
        case 11:
          strcpy(code, "Network unreachable for ToS");
          break;
        case 12:
          strcpy(code, "Host unreachable for ToS");
          break;
        case 13:
          strcpy(code, "Communication administratively prohibited");
          break;
        case 14:
          strcpy(code, "Host Precedence Violation");
          break;
        case 15:
          strcpy(code, "Precedence cutoff in effect");
          break;
      }
      break;

    case 5:
      strcpy(type, "Redirect Message: ");
      switch (c) {
        case 0:
          strcpy(code, "Redirect Datagram for the Network");
          break;
        case 1:
          strcpy(code, "Redirect Datagram for the Host");
          break;
        case 2:
          strcpy(code, "Redirect Datagram for the ToS & network");
          break;
        case 3:
          strcpy(code, "Redirect Datagram for the ToS & host");
          break;
      }
      break;

    case 8:
      strcpy(type, "Echo Request");
      strcpy(code, "");
      break;

    case 9:
      strcpy(type, "Router Advertisement");
      strcpy(code, "");
      break;

    case 10:
      strcpy(type, "Router Solicitation: ");
      strcpy(code, "Router discovery/selection/solicitation");
      break;

    case 11:
      strcpy(type, "Time Exceeded: ");
      switch (c) {
        case 0:
          strcpy(code, "TTL expired in transit");
          break;
        case 1:
          strcpy(code, "Fragment reassembly time exceeded");
          break;
      }
      break;

    case 12:
      strcpy(type, "Parameter Problem: Bad IP header: ");
      switch (c) {
        case 0:
          strcpy(code, "Pointer indicates the error");
          break;
        case 1:
          strcpy(code, "Missing a required option");
          break;
        case 2:
          strcpy(code, "Bad length");
          break;
      }
      break;

    case 13:
      strcpy(type, "Timestamp");
      strcpy(code, "");
      break;

    case 14:
      strcpy(type, "Timestamp Reply");
      strcpy(code, "");

    default:
      strcpy(type, "respuesta correcta");
      strcpy(code, "");
  }

  printf("Descripcion de la respuesta: %s %s (type: %u code: %u)\n", type, code, t, c);
}


int main(int argc, char *argv[])
{
  ICMPHeader icmpHeader;
  ECHORequest echoRequest;
  ECHOResponse echoResponse;

  int flag = 0;                       // flag verbose
  int sock;                           // socket
  struct sockaddr_in destino, origen; // estructura socket del servidor destino
  struct in_addr direccionIP;         // direccionIP transformada
  socklen_t longitud = sizeof(struct sockaddr_in);


  /*
   * Con cantidad erronea de argumentos, mensaje de error, y termina.
   */
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Uso: %s <direccionIP> [-v]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  /*
   * Si la direccion IP proporcionada como argumento argv[1] no es valida,
   * mensaje de error y termina.
   * 'inet_aton' guarda servidor IP en 'direccion', 1 si correcto, 0 si error.
   */
  if (inet_aton(argv[1], &direccionIP) == 0) {
    fprintf(stderr, "direccionIP IP invalida\n");
    exit(EXIT_FAILURE);
  }

  /*
   * Si se proporciona la opcion 'verbose', que la bandera '-v'
   * sea correcta.
   * Si es correcta, se activa la bandera flag para imprimir mensajes por
   * pantalla durante la ejecucion del programa
   */
  if (argc == 3) {
    if (strncmp(argv[2], "-v", 2) != 0) {
      fprintf(stderr, "Uso: %s <direccionIP> [-v]\n", argv[0]);
      exit(EXIT_FAILURE);
    }
    else flag = 1;
  }

  /*
   * Se crea el socket datagrama con el protocolo TCP.
   * Si hay error en la creacion del socket, se imprime
   * mensaje de error, y termina.
   */
  sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

  if (sock<0) {
    perror("socket()");
    exit(EXIT_FAILURE);
  }

  /*
   * Se establecen los valores del servidor.
   */
  destino.sin_family = AF_INET;
  destino.sin_port = 0;
  destino.sin_addr.s_addr = direccionIP.s_addr;

  /*
   * Se construye la cabecera de ICMP
   */
  if (flag) {
    printf("-> Generando cabecera ICMP\n");
  }

  icmpHeader.Type = 8;          // 8 para ping
  if (flag){
    printf("-> Type: %u\n", icmpHeader.Type);
  }

  icmpHeader.Code = 0;          // 0 para ping
  if (flag) {
    printf("-> Code = %u\n", icmpHeader.Code);
  }

  icmpHeader.Checksum = 0;      // Inicialmente a 0

  /*
   * Se prepara la peticion ping (estructura echoRequest)
   */

  echoRequest.ID = getpid();    // id del proceso que realiza el ping
  if (flag) {
    printf("-> Identifier (pid): %u\n", echoRequest.ID);
  }

  echoRequest.SeqNumber = 0;    // numero secuencia, 0 en este caso
  if (flag) {
    printf("-> Seq. number: %u\n", echoRequest.SeqNumber);
  }

  strcpy(echoRequest.payload, "test"); // inicializacion del campo payload
  if (flag) {
    printf("-> Cadena a enviar: %s\n", echoRequest.payload);
  }

  echoRequest.icmpHeader = icmpHeader;

  /*
   * Construida la peticion, se calcula el checksum del paquete de peticion
   */
   echoRequest.icmpHeader.Checksum = checksum(&echoRequest, ICMP_HEADERLEN);
   if (flag) {
     printf("-> Checksum: %hu\n", echoRequest.icmpHeader.Checksum);
   }

   /*
    * Longitud paquete ICMP
    */
   if (flag) {
    printf("-> Tamaño paquete ICMP: %d\n", ICMP_HEADERLEN);
   }

   /*
    * Se envia el datagrama al servidor destino.
    * Si hay error, se imprime mensaje de error y termina.
    */
   if (sendto(sock, &echoRequest, sizeof(echoRequest), 0, (struct sockaddr *)&destino, longitud) == -1) {
     perror("sendto()");
     exit(EXIT_FAILURE);
   }

   printf("Paquete ICMP enviado a %s\n", inet_ntoa(direccionIP));

   /*
    * Se recibe el datagrama desde el servidor destino.
    * Si hay error, se imprime mensaje de error y termina.
    */
   if (recvfrom(sock, &echoResponse, sizeof(echoResponse), 0, (struct sockaddr *)&origen, &longitud) == -1) {
     perror("recvfrom()");
     exit(EXIT_FAILURE);
   }


   printf("Respuesta recibida desde %s\n", inet_ntoa(direccionIP));
   close(sock);

   /*
    * verbose
    */
   if(flag) {
     printf("-> Tamaño de la respuesta: %d\n", ICMP_HEADERLEN+ICMP_HEADERLEN);
     printf("-> Cadena recibida: %s\n", echoResponse.payload);
     printf("-> Identificador (pid): %u\n", echoResponse.ID);
     printf("-> TTL: %u\n", echoResponse.ipHeader.TTL);
   }

   printResponse(echoResponse.icmpHeader.Type, echoResponse.icmpHeader.Code);

   return 1;
}
