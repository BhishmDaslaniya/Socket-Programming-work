#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <bits/stdc++.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

int main(int argc, char** argv){
  
  if(argc!=4){
    cerr<<"use 'SimpleEmailClientPhase1 <serverIPAddr:port> <user_name> <passwd> ' \n  serverIPAddr:port : info about server ip addr with portNum \n   user_name : user name of client \n   passwd : password of client "<<endl;
    exit(1);
  }

  string server_ip_addr_port(argv[1]), username(argv[2]), passwd(argv[3]); 
  int server_port=stoi(server_ip_addr_port.substr(server_ip_addr_port.find(':')+1,server_ip_addr_port.length()));
  const char* server_ip_addr=server_ip_addr_port.substr(0,server_ip_addr_port.find(':')).c_str();
  
  int client_fd, client_port;
  client_fd=socket(AF_INET, SOCK_STREAM,0);
  if (client_fd < 0){
    cerr<<"ERROR opening socket";
    exit(2);
  }
 
  struct sockaddr_in server_addr;
  server_addr.sin_family=AF_INET;
  server_addr.sin_port=htons(server_port);
  server_addr.sin_addr.s_addr = inet_addr(server_ip_addr);

  if (connect(client_fd,(struct sockaddr*)& server_addr,sizeof(server_addr)) < 0){
    cerr<<"ERROR connecting"<<endl;
    exit(2);
  }
  else{
    cout<<"ConnectDone:"<<server_ip_addr_port<<endl;
  }

  string logins=string("User: ")+ username + string(" Pass: ")+passwd;  
  char login[logins.length()+1];
  strcpy(login,logins.c_str());
  login[logins.length()]='\0';
  int status=write(client_fd,login,strlen(login));
  if(status<0){
    cerr<<"Error sending message to server"<<endl;
    exit(3);
  }
  // cout<<login<<endl;
  // cout<<strlen(login)<<endl;
  // cout<<login[logins.length()]<<endl;
  
  char response[256];
  status=read(client_fd, response,255);
  if(status<0){
    cerr<<"Error reading message from server"<<endl;
    exit(3);
  }
  string response_string(response);
  response_string=response_string.substr(0,status);
  cout<<response_string<<endl;

  char quit[]="quit";
  status=write(client_fd,quit,strlen(quit));
  if(status<0){
    cerr<<"Error sending message to server"<<endl;
    exit(3);
  }
  
  close(client_fd);
  exit(0);
}
