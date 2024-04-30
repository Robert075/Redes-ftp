//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2º de grado de Ingeniería Informática
//                       
//                    Header of the FTPServer class
//
//        Roberto Giménez Fuentes  (alu0101540894@ull.edu.es)
//        Eric Rios Hamilton        (eric.rios.41@ull.edu.es)
// 
//****************************************************************************

#if !defined FTPServer_H
#define FTPServer_H

#include <list>


#include "ClientConnection.h"

int define_socket_TCP(int port); 

class FTPServer {
public:
  FTPServer(int port = 21);
  void run();
  void stop();

private:
  int port;
  int msock;
  std::list<ClientConnection*> connection_list;
};

#endif
