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
  // std::string msg = "227 Entering Passive Mode (" + this->GetHostIp() + ",";
  // this->data_socket = define_socket_TCP(2322); // port 2322 for data conections
  // uint8_t p1 = 9; // 8 bits más significativos de 2322 -> 00001001
  // uint8_t p2 = 18; // 8 bits menos significativos de 2322 -> 00010010
  // msg += std::to_string(p1) + "," + std::to_string(p2) + ")\n";
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


/**
 *
 * LIST (sin argumentos)
 * DIR *d = opendir(" ");
 * fprintf(code)
 * while (e = Readdir(d)) {
 *  send(data_socket, e->name), strlen(e->name), 0);
 *  fflush(fd);
 * }
 * fprintf(code)
 * 
 * PASSV
 * struct sockaddr_in fsin;
 * int s = define_socket_TCP(0);
 * getsockname(s, (sockaddr*)&fsin, sizeof(fsin));
 * port = ntohs(fsin.sin_port);
 * int p1 = port / 256;
 * int p2 = port % 256;
 * fprintf(fd, "227 Entering Passive Mode (127,0,0,1,%d,%d)\n", p1, p2);
 * 
 * En un lugar de La Mancha, de cuyo nombre no quiero acordarme, no ha mucho tiempo que vivía un hidalgo de los de lanza en astillero, adarga antigua, rocín flaco y galgo corrredor. 
 * Una olla de algo más vaca que carnero, salpicón las más noches, duelos y quebrantos los sábados, lantejas los viernes, algún palomino de añadidura los domingos, consumían las tres partes de su hacienda.
 * El resto della concluían sayo de velarte, calzas de velludo para las fiestas con sus pantuflos de lo mismo, los días de entre semana se honraba con su vellorí de lo más fino.
 * Tenía en su casa una ama que pasaba de los cuarenta, y una sobrina que no llegaba a los veinte, y un mozo de campo y plaza, que así ensillaba el rocín como tomaba la podadera.
 * Frisaba la edad de nuestro hidalgo con los cincuenta años, era de complexión recia, seco de carnes, enjuto de rostro, gran madrugador y amigo de la caza. Quieren decir que tenía el sobrenombre de Quijada, o Quesada, que en esto hay alguna diferencia en los autores que deste caso escriben; aunque, por conjeturas verosímiles, se deja entender que se llamaba Quejana.
 * Pero esto importa poco a nuestro cuento: basta que en la narración dél no se salga un punto de la verdad.
 * Es, pues, de saber que este sobredicho hidalgo, los ratos que estaba ocioso, que eran los más del año, se daba a leer libros de caballerías con tanta afición y gusto, que olvidó casi de todo punto el ejercicio de la caza, y aun la administración de su hacienda; y llegó a tanto su curiosidad y desatino en esto, que vendió muchas hanegas de tierra de sembradura para comprar libros de caballerías en que leer, y así llevó a su casa todos cuantos pudo haber dellos;
 * y de todos, ningunos le parecían tan bien como los que compuso el famoso Feliciano de Silva, porque la claridad de su prosa y aquellas entricadas razones suyas le parecían de perlas, y más cuando llegaba a leer aquellos requiebros y cartas de desafío, donde en muchas partes hallaba escrito: la razón de la sinrazón que a mi razón se hace, de tal manera mi razón enflaquece, que con razón me quejo de la vuestra fermosura.
 * Y también cuando leía: los altos cielos que de vuestra divinidad divinamente con las estrellas os fortifican, y os hacen merecedora del merecimiento que merece la vuestra grandeza.
 * Con estas razones perdía el pobre caballero el juicio, y desvelábase por entenderlas, y desentrañarles el sentido, que no se lo sacara ni las entendiera el mesmo Aristóteles, si resucitara para sólo ello.
 * No estaba muy bien con las heridas que don Belianis daba y recebía, porque se imaginaba que por grandes maestros que le hubiesen curado, no dejaría de tener el rostro y todo el cuerpo lleno de cicatrices y señales; pero con todo alababa en su autor aquel acabar su libro con la promesa de aquella inacabable aventura, y muchas veces le vino deseo de tomar la pluma y dalle fin al pie de la letra, como allí se promete; y sin duda alguna lo hiciera, y aun saliera con ello, si otros mayores y continuos pensamientos no se lo estorbaran.
 * 
 * Call me Ishmael. Some years ago—never mind how long precisely—having little or no money in my purse, and nothing particular to interest me on shore, I thought
 * I would sail about a little and see the watery part of the world. It is a way I have of driving off the spleen and regulating the circulation. Whenever I find myself growing grim about the mouth; whenever it is a damp, drizzly November in my soul; whenever I find myself involuntarily pausing before coffin warehouses, and bringing up the rear of every funeral I meet; and especially whenever my hypos get such an upper hand of me, that it requires a strong moral principle to prevent me from deliberately stepping into the street, and methodically knocking people’s hats off—then, I account it high time to get to sea as soon as I can. This is my substitute for pistol and ball. With a philosophical flourish Cato throws himself upon his sword; I quietly take to the ship. There is nothing surprising in this. If they but knew it
 * almost all. 
 */