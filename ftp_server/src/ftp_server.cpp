//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2º de grado de Ingeniería Informática
//                       
//                    Main file of the FTP server
//
//        Roberto Giménez Fuentes  (alu0101540894@ull.edu.es)
//        Eric Rios Hamilton        (eric.rios.41@ull.edu.es)
// 
//****************************************************************************

#include <iostream>
#include <signal.h>

#include "FTPServer.h"

FTPServer *server;

extern "C" void sighandler(int signal, siginfo_t *info, void *ptr) {
  std::cout << "Dispara sigaction" << std::endl;  
  server->stop();
  exit(-1);
}


void exit_handler() {
    server->stop();
}


int main(int argc, char **argv) {
  struct sigaction action;
  action.sa_sigaction = sighandler;
  action.sa_flags = SA_SIGINFO;
  sigaction(SIGINT, &action , NULL);
  server = new FTPServer(2121); // Gracias a usar este puerto, no es necesario ejecutar como sudo
  atexit(exit_handler);
  server->run();

}
