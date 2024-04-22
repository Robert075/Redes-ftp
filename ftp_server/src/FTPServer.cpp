//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2º de grado de Ingeniería Informática
//                       
//                        Main class of the FTP server
// 
//****************************************************************************

#include <cerrno>
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <netdb.h>

#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

 #include <unistd.h>


#include <pthread.h>


#include "common.h"
#include "FTPServer.h"
#include "ClientConnection.h"


int define_socket_TCP(int port) {
  // Include the code for defining the socket.
  struct sockaddr_in sin;
  int socket_fd;
  socket_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (socket_fd < 0) {
    std::string err_msg = "Error al crear el socket " + std::string(strerror(errno));
    throw std::logic_error(err_msg);
  }

  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons(port);
  
  if (bind(socket_fd, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
    std::string err_msg = "Error al hacer bind con el puerto suministrado " + std::string(strerror(errno) + std::to_string(port));
    throw std::logic_error(err_msg);
  }

  if (listen(socket_fd, 10) < 0) { // Como maximo 10 solicitudes al mismo tiempo
    std::string err_msg = "Error al hacer listen " + std::string(strerror(errno));
    throw std::logic_error(err_msg);
  }
  
  return socket_fd;
}





// This function is executed when the thread is executed.
void* run_client_connection(void *c) {
    ClientConnection *connection = (ClientConnection *)c;
    connection->WaitForRequests();
  
    return NULL;
}



FTPServer::FTPServer(int port) {
    this->port = port;
  
}


// Parada del servidor.
void FTPServer::stop() {
    close(msock);
    shutdown(msock, SHUT_RDWR);

}


// Starting of the server
void FTPServer::run() {

  struct sockaddr_in fsin;
  int ssock;
  socklen_t alen = sizeof(fsin);
  try {
    msock = define_socket_TCP(port);  // This function must be implemented by you.
  } catch (std::logic_error& exception) {
    std::cerr << exception.what() << "\n";
  }
    while (1) {
    pthread_t thread;
    ssock = accept(msock, (struct sockaddr *)&fsin, &alen);
    if(ssock < 0) {
      errexit("Fallo en el accept: %s\n", strerror(errno));
    }
    ClientConnection *connection = new ClientConnection(ssock);
    
    // Here a thread is created in order to process multiple
    // requests simultaneously
    pthread_create(&thread, NULL, run_client_connection, (void*)connection);
       
  }

}
