#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <bits/stdc++.h>
#include <stdlib.h>
#include <fstream>
#include <arpa/inet.h>
#include <unistd.h>
#include <regex>

using namespace std;

int main(int argc, char** argv){
  if(argc!=3){
    cerr<<"use 'SimpleEmailServerPhase1 <portNUM> <passwdfile>' \n  portNum : port number for server binding \n  passwdfile : file that contains usernames and passwords for clients \n";
    exit(1);
  }

  int server_port=0;
  try{
    server_port=stoi(argv[1]);
  }
  catch(exception){
    cerr<<"Invalid portNum"<<endl;
    exit(2);
  }
  //cerr<<server_port<<endl;
  ifstream passwdfile (argv[2]);
  if (!passwdfile.is_open()){
    cerr << "Either passwdfile does not exist or is unreadable"<<endl;
    exit(3);
  }

  int server_fd, client_fd;
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  int opt=1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){ 
    cerr<<"setsockopt"<<endl; 
    exit(EXIT_FAILURE); 
  }
  if (server_fd < 0){
    cerr<<"Error while opening socket";
    exit(4);
  }

  struct sockaddr_in server_addr, client_addr;
  server_addr.sin_family=AF_INET;
  server_addr.sin_port=htons(server_port);
  //server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  // const int enable=1;
  // if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
  //   cerr<<"setsockopt(SO_REUSEADDR) failed"<<endl;

  if(bind(server_fd, (struct sockaddr*)& server_addr, sizeof(server_addr)) < 0){
    cerr<<"Bind failed"<<endl;
    close(client_fd);
    close(server_fd);
    exit(2);
  }
  else{
    cout<<"BindDone:"<<server_port<<endl;
  }

  listen(server_fd,4);
  cout<<"ListenDone:"<<server_port<<endl;

  int client_addr_len=sizeof(client_addr);
  client_fd=accept(server_fd, (struct sockaddr*)& client_addr,(socklen_t *) &client_addr_len);
  cout<<"Client:"<<inet_ntoa(client_addr.sin_addr)<<":"<<(int) ntohs(client_addr.sin_port)<<endl;

  char response[256];
  int status=0;
  status=read(client_fd,response,255);
  if(status<0){
    cerr<<"Error receiving message from client"<<endl;
    close(client_fd);
    close(server_fd);
    exit(5);
  }
  string response_string(response);
  response_string=response_string.substr(0,status);
 
  // cout<<response_string<<endl;
  // cout<<response_string.length()<<endl;
  
  regex login_regex("^User: [^\\s]+ Pass: [^\\s]+$");
  //regex login_regex("User:");
  string username, passwd;
  if(regex_match(response_string, login_regex)){
    //cout<<"matched "<<endl;
    smatch sm;
    regex word_regex("[^\\s]+");
    regex_search(response_string, sm, word_regex);
    int i=0;
    while(regex_search(response_string,sm,word_regex)){
      if(i==1) username=sm[0];
      else if(i==3) passwd=sm[0];
      i++;
      response_string = sm.suffix().str();
    }
    //cout<<username<<" "<<passwd<<endl;
  }
  else{
    cout<<"Unknown Command"<<endl;
    close(client_fd);
    close(server_fd);
    exit(0);
  }

  string welcome="Welcome "+username;
  if (passwdfile.is_open())
  {
    string line;
    bool user_found=0;
    while ( getline (passwdfile,line) )
    {
      //cout << line << '\n';
      int pos=line.find(username);
      if(pos!=string::npos){
	user_found=1;
	//	cout<<"Passwd is "<<line.substr(username.length()+1)<<endl;
	if(passwd==line.substr(username.length()+1)){
	  cout<<welcome<<endl;
	  break;
	}
	else{
	  cout<<"Wrong Passwd"<<endl;
	  close(client_fd);
	  close(server_fd);
	  exit(0);
	}
      }
    }
    if(!user_found){
      cout<<"Invalid User"<<endl;
      close(client_fd);
      close(server_fd);
      exit(0);
    }
    passwdfile.close();
  }

  char welcome_char[welcome.length()+1];
  strcpy(welcome_char,welcome.c_str());
  welcome_char[welcome.length()]='\0';

  
  
  status = write(client_fd, welcome_char, welcome.length());
  if (status < 0){
    cerr<<"ERROR sending message to client"<<endl;
    close(client_fd);
    close(server_fd);
    exit(5);
  }


  status=read(client_fd,response,255);
  if(status<0){
    cerr<<"Error receiving message from client"<<endl;
    close(client_fd);
    close(server_fd);
    exit(5);
  }
  response_string=response;
  response_string=response_string.substr(0,status);
  //cout<<response_string<<endl;

  if(response_string=="quit"){
    cout<<"Bye "<<username<<endl;
  }
  else cout<<"Unknown command"<<endl;
  
  close(client_fd);
  close(server_fd);
  
}
