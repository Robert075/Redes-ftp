//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2º de grado de Ingeniería Informática
//                       
//                    Header of the common functions
//
//        Roberto Giménez Fuentes  (alu0101540894@ull.edu.es)
//        Eric Rios Hamilton        (eric.rios.41@ull.edu.es)
// 
//****************************************************************************

#ifndef COMMON_H
#define COMMON_H

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

inline void errexit(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  exit(1);
}

#endif
