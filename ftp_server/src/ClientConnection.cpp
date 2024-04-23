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
  sin.sin_port = port; // DUDA. CREO QUE YA ESTÁ CONVERTIDO 
  sin.sin_addr.s_addr = address; // DUDA. CREO QUE YA ESTA CONVERTIDO. SI FALLA USAR htons();

  int data_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (data_socket < 0) {
    std::string msg_error = "Error al crear socket de datos " + std::string(strerror(errno));
    throw std::logic_error(msg_error);
  }

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
          fprintf(this->fd, "200 OK.");
          fflush(this->fd);
        } catch (std::logic_error& exception) {
          std::cerr << exception.what() << "\n";
          fprintf(this->fd, "425 Can't open data connection\n");
          fflush(this->fd);
        } 
      }
      else if (COMMAND("PASV")) {
        try {
          std::cout << "ENTERING PASV\n";
          std::string msg = this->PASVCommand();
          std::cout << "MESSAGE -> " << msg << "\n";
          fprintf(this->fd, "%s\n", msg.c_str());
          fflush(this->fd);
          std::cout << "MESSAGE PRINT\n";
        } catch (std::logic_error& exception) {
          std::cerr << exception.what() << "\n";
          fprintf(this->fd, "425 Can't open data connection\n");
        }
      } else if (COMMAND("STOR") ) {
        // To be implemented by students
      } else if (COMMAND("RETR")) {
       // To be implemented by students
      } else if (COMMAND("LIST")) {
       // To be implemented by students	
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

std::string ClientConnection::PASVCommand() {
	this->passive_mode_ = true;
  // std::string msg = "227 Entering Passive Mode (" + this->GetHostIp() + ",";
  this->data_socket = define_socket_TCP(2322); // port 2322 for data conections
  // uint8_t p1 = 9; // 8 bits más significativos de 2322 -> 00001001
  // uint8_t p2 = 18; // 8 bits menos significativos de 2322 -> 00010010
  // msg += std::to_string(p1) + "," + std::to_string(p2) + ")\n";
  
  return "227 Entering Passive Mode (127,0,0,1,9,18)";
}
