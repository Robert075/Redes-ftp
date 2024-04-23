//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2º de grado de Ingeniería Informática
//                       
//              This class processes an FTP transaction.
// 
//****************************************************************************



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



ClientConnection::ClientConnection(int s) {
  int sock = (int)(s);

  char buffer[MAX_BUFF];

  control_socket = s;
  // Check the Linux man pages to know what fdopen does.
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


ClientConnection::~ClientConnection() {
 	fclose(fd);
	close(control_socket); 
  
}


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






void ClientConnection::stop() {
    close(data_socket);
    close(control_socket);
    parar = true;
  
}





    
#define COMMAND(cmd) strcmp(command, cmd)==0

// This method processes the requests.
// Here you should implement the actions related to the FTP commands.
// See the example for the USER command.
// If you think that you have to add other commands feel free to do so. You 
// are allowed to add auxiliary methods if necessary.

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
          PASVCommand();
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

void ClientConnection::RetrCommand() {
  fscanf(fd, "%s", arg);
  FILE *f = fopen(arg, "r");
  if (f == NULL) {
    fprintf(fd, "550 File not found\n");
    return;
  }
  fprintf(fd, "150 File status okay; about to open data connection.\n");
  while (!feof(f)) {
    char buffer[MAX_BUFF];
    size_t bytes_read = fread(buffer, 1, MAX_BUFF, f);
    if (bytes_read == 0) {
      break;
    }
    write(data_socket, buffer, bytes_read);
  }
  fprintf(fd, "226 Closing data connection. Requested file action successful.\n");
  close(data_socket);
  fclose(f);
}

void ClientConnection::StorCommand() {
  fscanf(fd, "%s", arg);
  FILE *f = fopen(arg, "w");
  if (f == NULL) {
    fprintf(fd, "550 File not found\n");
    return;
  }
  fprintf(fd, "150 File status okay; about to open data connection.\n");
  while (1) {
    char buffer[MAX_BUFF];
    size_t bytes_read = read(data_socket, buffer, MAX_BUFF);
    if (bytes_read == 0) {
      break;
    }
    fwrite(buffer, 1, bytes_read, f);
  }
  fprintf(fd, "226 Closing data connection. Requested file action successful.\n");
  close(data_socket);
  fclose(f);
}

void ClientConnection::PASVCommand() {
  struct sockaddr_in fsin;
  socklen_t slen = sizeof(fsin);
  int s = define_socket_TCP(0);
  getsockname(s, (sockaddr*)&fsin, &slen);
  int port = ntohs(fsin.sin_port);
  int p1 = port / 256;
  int p2 = port % 256;
  fprintf(fd, "227 Entering Passive Mode (127,0,0,1,%d,%d)\n", p1, p2);
  data_socket = accept(s, (struct sockaddr*)&fsin, &slen);
}

void ClientConnection::ListCommand() {
  DIR* d = opendir(".");
  fprintf(fd, "125 List started OK.\n");
  fflush(fd);
  while (1) {
    struct dirent* e = readdir(d);
    if (e == NULL) {
      break;
    }
    fprintf(fd, "%s\n", e->d_name);
  }
  fprintf(fd, "250 List completed succesfully.\n");
  fflush(fd);
}


//
//LIST (sin argumentos)
//DIR *d = opendir(" ");
//fprintf(code)
//while (e = Readdir(d)) {
// send(data_socket, e->name), strlen(e->name), 0);
// fflush(fd);
//}
//fprintf(code)
//
