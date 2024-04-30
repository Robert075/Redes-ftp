//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2º de grado de Ingeniería Informática
//                       
//                     This class processes an FTP transaction.
//
//        Roberto Giménez Fuentes  (alu0101540894@ull.edu.es)
//        Eric Rios Hamilton        (eric.rios.41@ull.edu.es)
// 
//****************************************************************************




#include <algorithm>
#include <cstdint>
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cerrno>
#include <netdb.h>

#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h> 
#include <iostream>
#include <dirent.h>

#include "FTPServer.h"
#include "common.h"

#include "ClientConnection.h"


/**
  * @brief Constructor of the class
  *
  * @param s the socket descriptor for the connection
  */
ClientConnection::ClientConnection(int s) {
  int sock = (int)(s);

  char buffer[MAX_BUFF];

  control_socket = s;
  fd = fdopen(s, "a+");
  if (fd == NULL){
    std::cout << "Connection closed" << std::endl;

    fclose(fd);
    close(control_socket);
    ok = false;
    return ;
  }

  ok = true;
  this->passive_mode_ = false; 
  data_socket = -1;
  parar = false;  
};

/**
  * @brief Destructor of the class
  *
  */
ClientConnection::~ClientConnection() {
 	fclose(fd);
	close(control_socket); 
}

/**
  * @brief Define a TCP socket
  *
  * @param port the port to bind the socket
  * @return the socket descriptor of the new socket
  */
int connect_TCP(uint32_t address,  uint16_t  port) {
  struct sockaddr_in sin;
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port); 
  sin.sin_addr.s_addr = htonl(address);

  int data_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (data_socket < 0) {
    std::string msg_error = "Error al crear socket de datos " + std::string(strerror(errno));
    throw std::logic_error(msg_error);
  }
  std::cout << "port -> " << port << "\n";
  std::cout << "address -> " << address << "\n";
  if (connect(data_socket, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
    std::string msg_error = "Error al conectar " + std::string(strerror(errno));
    throw std::logic_error(msg_error);
  }
  return data_socket; // You must return the socket descriptor.
}

/**
  * @brief Stop the client connection
  *
  */
void ClientConnection::stop() {
    close(data_socket);
    close(control_socket);
    parar = true;
  
}

    
#define COMMAND(cmd) strcmp(command, cmd)==0

/**
  * @brief Wait for requests from the client
  *
  */
void ClientConnection::WaitForRequests() {
    if (!ok) {
      return;
    }
    
    fprintf(fd, "220 Service ready\n");
  
    while(!parar) {

      fscanf(fd, "%s", command);
      if (COMMAND("USER")) {
        fscanf(fd, "%s", arg); // Cualquier usuario es válido
        fprintf(fd, "331 User name ok, need password\n");
      }
      else if (COMMAND("PWD")) {
        
      }
      else if (COMMAND("PASS")) {
        std::cout << "CONTRASEÑA INTRODUCIDA\n";
        fscanf(fd, "%s", arg);
        if(strcmp(arg,"1234") == 0){ // La contraseña por defecto es 1234
          fprintf(fd, "230 User logged in\n");
        }
        else{
          fprintf(fd, "530 Not logged in.\n");
          parar = true;
        }
      }
      else if (COMMAND("PORT")) {
        try {
          std::cout << "PORT COMMAND\n";
          this->PortCommand();
          fprintf(this->fd, "200 OK\n");
          fflush(this->fd);
        } catch (std::logic_error& exception) {
          std::cerr << exception.what() << "\n";
          fprintf(this->fd, "425 Can't open data connection\n");
          fflush(this->fd);
        } 
      }
      else if (COMMAND("PASV")) {
        try {
          this->PASVCommand();
          std::cout << "MESSAGE PRINT\n";
        } catch (std::logic_error& exception) {
          std::cerr << exception.what() << "\n";
          fprintf(this->fd, "425 Can't open data connection\n");
        }
      } else if (COMMAND("SIZE")) { 
          std::cout << "SIZE COMMAND" << "\n";
          fscanf(fd, "%s", arg);
          fprintf(fd, "502 Command not implemented.\n");
          fflush(this->fd);
      } else if (COMMAND("EPRT")) {
          std::cout << "EPRT COMMAND" << "\n";
          fscanf(fd, "%s", arg);
          fprintf(fd, "502 Command not implemented.\n");
          fflush(this->fd);
      } else if (COMMAND("STOR") ) {
        try {
          this->StorCommand();
        } catch (std::logic_error& exception) {
          std::cerr << exception.what() << "\n";
          fprintf(this->fd, "425 Can't open data connection\n");
        }
      } else if (COMMAND("RETR")) {
        try {
          this->RetrCommand();
        } catch (std::logic_error& exception) {
          std::cerr << exception.what() << "\n";
          fprintf(this->fd, "425 Can't open data connection\n");
        }
      } else if (COMMAND("LIST")) {
        try {
          this->ListCommand();
        } catch (std::logic_error& exception) {
          std::cerr << exception.what() << "\n";
          fprintf(this->fd, "425 Can't open data connection\n");
        }
      } else if (COMMAND("SYST")) {
           fprintf(fd, "215 UNIX Type: L8.\n");   
      } else if (COMMAND("TYPE")) {
        fscanf(fd, "%s", arg);
        fprintf(fd, "200 OK\n");   
      } else if (COMMAND("QUIT")) {
        std::cout << "SALIENDO\n";
        fprintf(fd, "221 Service closing control connection. Logged out if appropriate.\n");
        close(data_socket);	
        parar=true;
        break;
      } else  {
	    fprintf(fd, "502 Command not implemented.\n"); fflush(fd);
	    printf("Comando : %s %s\n", command, arg);
	    printf("Error interno del servidor\n");
      } 
    }
    fclose(fd);
    return;
};

/**
  * @brief Establish a connection with the client
  *
  */
void ClientConnection::PortCommand() {
  int a1, a2, a3, a4; // Dividimos por trozos la dir. ip
  int p1, p2; // Dividimos por trozos el puerto
  std::cout << "PRE READ PORT INFO\n";
  fscanf(this->fd, "%d,%d,%d,%d,%d,%d", &a1, &a2, &a3, &a4, &p1, &p2);
  std::cout << "address: " << a1 << "." << a2 << "." << a3 << "." << a4 << std::endl;
  uint32_t address = (a1 << 24) | (a2 << 16) | (a3 << 8) | a4;
  uint16_t port = (p1 << 8) | p2;
  std::cout << "PRE CONNECT_TCP\n";
  std::cout << "Address ->" << address << " Port -> " << port << "\n";
  this->data_socket = connect_TCP(address, port);
  std::cout << "holasd\n";
  return;
}

/**
  * @brief Get the IP of the host
  *
  * @return std::string
  */
std::string ClientConnection::GetHostIp() {
  struct hostent* he;
  char ownIP[16];
  gethostname(ownIP, 16);
  he = gethostbyname(ownIP);
  std::string str = inet_ntoa(*(struct in_addr*)he->h_addr_list[0]);
  for (char& c: str) {
    if (c == '.') {
      c = ',';
    }
  }
  return str;

}

/**
  * @brief Retrieve a file from the server
  *
  */
void ClientConnection::RetrCommand() {
  fscanf(fd, "%s", arg);
  FILE *f = fopen(arg, "r"); // Abre el archivo 'arg'. Si no existe delvuelve nullptr
  if (f == NULL) { // El no existe
    fprintf(fd, "550 File not found\n");
    fflush(this->fd);
    return;
  }
  fprintf(fd, "150 File status okay; about to open data connection.\n"); // Notifica al cliente que se van a empezar a enviar datos
  fflush(this->fd);
  while (1) {
    char buffer[MAX_BUFF];
    size_t bytes_read = fread(buffer, 1, MAX_BUFF, f); // Lee los datos del archivo y los guarda en el buffer
    if (bytes_read == 0) { // Si se leyeron 0 bytes, no queda nada por leer en el archivo
      // No utilizamos (bytes_read < MAX_BUFF) porque al hacer pruebas, en ocasiones se envían paquetes con un tamaño menor
      // pero esto no significa que no se vayan a enviar más datos
      break;
    }
    write(data_socket, buffer, bytes_read); // Escribe los datos en el socket, es decir, los envía al cliente
  }
  fprintf(fd, "226 Closing data connection. Requested file action successful.\n"); // Notifica al cliente de que se han recibido todos los datos
  fflush(this->fd);
  close(data_socket); // Cierra el socket de datos
  fclose(f);
}

/**
  * @brief Store a file in the server
  *
  */
void ClientConnection::StorCommand() {
  fscanf(fd, "%s", arg);
  FILE *f = fopen(arg, "w"); // Abre el archivo con nombre 'arg'. Si no existe, lo crea
  if (f == NULL) { // No se pudo crear el archivo
    fprintf(fd, "550 File not found\n");
    return;
  }
  fprintf(fd, "150 File status okay; about to open data connection.\n"); // Notifica al cliente que se van a empezar a enviar datos
  fflush(this->fd);
  while (1) {
    char buffer[MAX_BUFF];
    size_t bytes_read = read(data_socket, buffer, MAX_BUFF); // Lee los datos que se enviaron por el socket y los guarda en el buffer
    if (bytes_read == 0) {
      // Si se recibieron 0 bytes, se enteinde que n
      break;
    }
    fwrite(buffer, 1, bytes_read, f);
  }
  fprintf(fd, "226 Closing data connection. Requested file action successful.\n");
  fflush(this->fd);
  close(data_socket); // Cierra el socket de datos
  fclose(f);
}

/**
  * @brief Turn on passive mode
  *
  */
void ClientConnection::PASVCommand() {
  struct sockaddr_in fsin;
  socklen_t slen = sizeof(fsin);
  int s = define_socket_TCP(0); // Se define un socket en un puerto aleatorio libre en el sistema
  getsockname(s, (sockaddr*)&fsin, &slen);
  int port = ntohs(fsin.sin_port); // obtenemos el puerto
  int p1 = port / 256;
  int p2 = port % 256;
  fprintf(fd, "227 Entering Passive Mode (127,0,0,1,%d,%d)\n", p1, p2); // enviamos ip y puerto en el formato adecuado
  fflush(this->fd);
  data_socket = accept(s, (struct sockaddr*)&fsin, &slen); // Esperamos la respuesta del cliente para realizar la conexión TCP
  return;
}

/**
  * @brief List the files in the directory
  *
  */
void ClientConnection::ListCommand() {
  DIR* d = opendir(".");
  fprintf(fd, "125 List started OK.\n");
  fflush(fd);
  struct dirent* entry;
  FILE* data_file = fdopen(this->data_socket, "wb");
  while ((entry = readdir(d)) != NULL) { // Mientras obtengamos un nombre de archivo/directorio
    std::string dir_name(entry->d_name);
    if (!(dir_name == "." || dir_name == "..")) {
      dir_name += "\x0d\x0a";
      fwrite(dir_name.c_str(), 1, dir_name.length(), data_file);      
    }
  }
  fflush(data_file);
  close(data_socket);
  closedir(d);
  fprintf(fd, "250 List completed succesfully.\n");
  fflush(fd);
  return;
}