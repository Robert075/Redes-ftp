//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2º de grado de Ingeniería Informática
//                       
//                    Header of the ClientConnection class
//
//        Roberto Giménez Fuentes  (alu0101540894@ull.edu.es)
//        Eric Rios Hamilton        (eric.rios.41@ull.edu.es)
// 
//****************************************************************************

#include <string>
#if !defined ClientConnection_H
#define ClientConnection_H

#include <pthread.h>

#include <cstdio>
#include <cstdint>

const int MAX_BUFF=1000;

class ClientConnection {
public:
    ClientConnection(int s);
    ~ClientConnection();
    
    void WaitForRequests();
    void stop();

    
private:  
  std::string GetHostIp();
  bool ok;  // This variable is a flag that avoids that the
     // server listens if initialization errors occured.
  void PortCommand();
  void RetrCommand();
  void StorCommand();
  void PASVCommand();
  void ListCommand();
  
  FILE *fd;	 // C file descriptor. We use it to buffer the
   // control connection of the socket and it allows to
   // manage it as a C file using fprintf, fscanf, etc.
  

  char command[MAX_BUFF];  // Buffer for saving the command.
  char arg[MAX_BUFF];      // Buffer for saving the arguments. 
  

  bool passive_mode_;
  int data_socket;         // Data socket descriptor;
  int control_socket;      // Control socket descriptor;
  bool parar;
  
   
};

#endif
