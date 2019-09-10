#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <bits/stdc++.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
using namespace std;

int main(int argc, char** argv){

  string usage="use 'SimpleEmailClientPhase3 <serverIPAddr:port> <user_name> <passwd> <list> <folder_to_store> <interval>' \n  serverIPAddr:port : info about server ip addr with portNum \n   user_name : user name of client \n   passwd : password of client \n  list : list of messages is a list of numbers separated by comma (e.g. 3, 5, 9 or 4 or 1, 8) \n  folder_to_store : folder where the downloaded messages will be stored \n interval : time interval to sleep after login \n";
  
  if(argc!=7){
    cerr<<usage<<endl;
    exit(1);
  }

  string server_ip_addr_port(argv[1]), username(argv[2]), passwd(argv[3]); 
  int server_port=stoi(server_ip_addr_port.substr(server_ip_addr_port.find(':')+1,server_ip_addr_port.length()));
  //cerr<<1<<endl;
  const char* server_ip_addr=server_ip_addr_port.substr(0,server_ip_addr_port.find(':')).c_str();
  
  int client_fd, client_port;
  client_fd=socket(AF_INET, SOCK_STREAM,0);
  if (client_fd < 0){
    cerr<<"ERROR opening socket";
    exit(2);
  }

  vector<int> vec_msg_n;
  string string_msg_n(argv[4]);
  stringstream  ss(string_msg_n);
  int n;
  try{
    while(!ss.eof()){
      if(ss>>n){
	vec_msg_n.push_back(n);
	while(1){
	  if ((ss.peek() == ',') or (ss.peek() == ' '))
	    ss.ignore();
	  else break;
	}
      }
      else{
	throw invalid_argument("Invalid syntax.");
      }
    }
  }
  catch(...){
    cerr<<usage;
    exit(3);
  }

  
  string local_folder_string(argv[5]);
  DIR *local_folder_dir;
  try{
    if((local_folder_dir=opendir(local_folder_string.c_str()))!=NULL){
      struct dirent *dp;
      while ((dp = readdir(local_folder_dir)) != NULL) {
	string local_file=local_folder_string+'/'+dp->d_name;
	//cout<<local_file.c_str()<<endl;
	remove(local_file.c_str());
      }
    }
    else{
      int status=mkdir(local_folder_string.c_str(),0777);
      //cout<<status<<endl;
      if(status==-1) {
    	throw;
      }
    }
  }
  catch(...){
    cerr<<"Unable to create/access the "<<local_folder_string<<endl;
    exit(4);
  }
  int interval=0;
  try{
    interval=stoi(argv[6]);
    if(interval<0) throw ;
  }
  catch(...){
    cerr<<"interval argument not a positive integer"<<endl;
    exit(4);
  }
  //cerr<<2<<endl;
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
    exit(5);
  }
  // cout<<login<<endl;
  // cout<<strlen(login)<<endl;
  // cout<<login[logins.length()]<<endl;
  
  char response[256];
  bzero(response, 255);
  status=read(client_fd, response,255);
  if(status<0){
    cerr<<"Error reading message from server"<<endl;
    exit(5);
  }
  string response_string(response);
  response_string=response_string.substr(0,status);
  cout<<response_string<<endl;

  usleep(interval*1000000);

  for(int i=0;i<vec_msg_n.size();i++){
    string retrv_cmd_string="RETRV "+to_string(vec_msg_n[i]);
    bzero(response, 255);
    //char retrv_cmd_char[retrv_cmd_string.length()+1];
    strcpy(response,retrv_cmd_string.c_str());
    cerr<<"client "<<retrv_cmd_string<<endl;
    status=send(client_fd,response,retrv_cmd_string.length(),0);
    if(status<0){
      cerr<<"Error sending message to server"<<endl;
      exit(5);
    }
    bzero(response, 255);
    // cout<<1<<endl;
    status=recv(client_fd, response,255,0);
    if(status<0){
      cerr<<"Error reading message from server"<<endl;
      exit(3);
    }
    response_string=response;
    response_string=response_string.substr(0,status);
    // cout<<response_string<<endl;
    string file_loc=local_folder_string+"/"+response_string;

    bzero(response, 255);
    status=recv(client_fd, response,255,0);
    if(status<0){
      cerr<<"Error reading message from server"<<endl;
      exit(3);
    }
    response_string=response;
    response_string=response_string.substr(0,status);
    //cout<<response_string<<endl;

    status=send(client_fd,response_string.c_str(),strlen(response_string.c_str()),0);
    if(status<0){
      cerr<<"Error sending message to server"<<endl;
      exit(5);
    }
    
    int size=stoi(response_string);
    //cerr<<3<<endl;
    FILE *message_file = fopen(file_loc.c_str(), "a");
    if(message_file==NULL) {
      cerr<<"error in opening "<<file_loc<<endl;
       close(client_fd);
       exit(5);
    }

    char buffer[1000];
    bzero(buffer, 1000);
    
    while((status = recv(client_fd, buffer, 1000, 0)) >= 0){
	int written_bytes = fwrite(buffer, sizeof(char), status, message_file);
	if(written_bytes < status){
	  cerr<<"error writing "<<file_loc<<endl;
	   close(client_fd);
	   exit(5);
	}
	bzero(buffer,1000);
	//	cout<<status<<endl;
	if (status == 0 ||  status!= 1000) {
	    break;
	}
    }
    if(status<0){
      cerr<<"Error reading message from server"<<endl;
      exit(3);
    }
    cout<<"Downloaded Message "<<vec_msg_n[i]<<endl;
    fclose(message_file); 
  }

  char quit[]="quit";
  status=send(client_fd,quit,strlen(quit),0);
  if(status<0){
    cerr<<"Error sending message to server"<<endl;
    exit(3);
  }
  
  close(client_fd);
  exit(0);
}
