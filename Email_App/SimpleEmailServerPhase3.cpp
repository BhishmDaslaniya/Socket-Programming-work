#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <bits/stdc++.h>
#include <stdlib.h>
#include <fstream>
#include <arpa/inet.h>
#include <unistd.h>
#include <regex>
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>

using namespace std;

int main(int argc, char** argv){
  if(argc!=4){
    cerr<<"use 'SimpleEmailServerPhase2 <portNUM> <passwdfile> <user_database>' \n  portNum : port number for server binding \n  passwdfile : file that contains usernames and passwords for clients \n  <user_database> : folder where user messages are stored \n";
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
  //cerr<<4<<endl;
  ifstream passwdfile (argv[2]);
  if (!passwdfile.is_open()){
    cerr << "Either passwdfile does not exist or is unreadable"<<endl;
    exit(3);
  }
  else passwdfile.close();

  string user_database_name(argv[3]);
  DIR *user_database;
  if ((user_database=opendir(argv[3]))==NULL) {
    cerr<<"Unable to access "<<user_database_name<<endl;
    exit(4);
  }
  
  
  int server_fd, client_fd;
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0){
    cerr<<"Error while opening socket";
    exit(5);
  }

  struct sockaddr_in server_addr, client_addr;
  server_addr.sin_family=AF_INET;
  server_addr.sin_port=htons(server_port);
  //server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  if(bind(server_fd, (struct sockaddr*)& server_addr, sizeof(server_addr)) < 0){
    cerr<<"Bind failed"<<endl;
    exit(2);
    close(client_fd);
    close(server_fd);
  }
  else{
    cout<<"BindDone:"<<server_port<<endl;
  }

  listen(server_fd,4);
  cout<<"ListenDone:"<<server_port<<endl;
  
  while(true){
    // listen(server_fd,4);
    //cout<<"ListenDone:"<<server_port<<endl;

    int client_addr_len=sizeof(client_addr);
    client_fd=accept(server_fd, (struct sockaddr*)& client_addr,(socklen_t *) &client_addr_len);
    cout<<"Client:"<<inet_ntoa(client_addr.sin_addr)<<":"<<(int) ntohs(client_addr.sin_port)<<endl;

   
   
    regex login_regex("^User: [^\\s]+ Pass: [^\\s]+$");
    regex retrv_regex("^RETRV [0-9]+$");
    string username("default"), passwd("default");
    bool auth=0;
    ifstream passwdfile (argv[2]);
    
    while(true){
      char response[256];
      int status=0;
      
      status=read(client_fd,response,255);
      if(status<0){
	cerr<<"Error receiving message from client"<<endl;
	close(client_fd);
	break;
      }
      
      string response_string(response);
      // cout<<response_string<<endl;
      response_string=response_string.substr(0,status);
      //cout<<response_string<<endl;
 
      if(regex_match(response_string, login_regex)){
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

	if (passwdfile.is_open())
	  {
	    string line;
	    bool user_found=0;
	    while ( getline (passwdfile,line) ){
	      int pos=line.find(username);
	      if(pos!=string::npos){
		user_found=1;
		break;
	      }
	    }

	    if(user_found){
	      if(passwd==line.substr(username.length()+1)){
		string welcome="Welcome "+username;
		cout<<welcome<<endl;
		auth=1;
		char welcome_char[welcome.length()+1];
		strcpy(welcome_char,welcome.c_str());
		welcome_char[welcome.length()]='\0';
		status = write(client_fd, welcome_char, welcome.length());
	      
		if (status < 0){
		  cerr<<"ERROR sending message to client"<<endl;
		  close(client_fd);
		  break;
		}
	      }
	      else{
		cout<<"Wrong Passwd"<<endl;
		close(client_fd);
		break;	  
	      }
	    }
	    else {
	      cout<<"Invalid User"<<endl;
	      close(client_fd);
	      break;
	    } 
	  }
      }
      else if(response_string=="quit"){
	cout<<"Bye "<<username<<endl;
	break;
      }
      else if(regex_match(response_string, retrv_regex)){
	smatch sm;
	regex word_regex("[^\\s]+");
	int file_n;
	regex_search(response_string, sm, word_regex);
	int i=0;
	while(regex_search(response_string,sm,word_regex)){
	  if(i==1){
	    file_n=stoi(sm[0]);
	    break;
	  }
	  i++;
	  response_string = sm.suffix().str();
	}
	//cerr<<5<<endl;
	//cout<<file_n<<endl;
	string user_dir_string=user_database_name+"/"+username;
	char user_dir_char[user_dir_string.length()+1];
	strcpy(user_dir_char,user_dir_string.c_str());
	DIR* user_dir_dir;
	if ((user_dir_dir=opendir(user_dir_char))!=NULL) {
	  struct dirent *dp;
	  bool found=0;
	  while ((dp = readdir((user_dir_dir))) != NULL) {
	    string message_name=dp->d_name;
	    //cout<<message_name<<endl;
	    message_name=message_name.substr(0,message_name.length()-4);
	    
	    if(message_name==to_string(file_n)){
	      message_name=dp->d_name; // fullname of the required message
	      message_name=user_dir_string+"/"+message_name;
	   
	      status = send(client_fd, dp->d_name, strlen(dp->d_name),0);
	      if (status < 0){
		cerr<<"ERROR sending message to client"<<endl;
		close(client_fd);
		exit(5);
	      }

	      FILE *message_file;
	      int size;
	      message_file = fopen(message_name.c_str(), "r");
	      if(message_file == NULL) {
		cerr<<"error in opening "<<message_name<<endl;
	      } 

	      fseek(message_file, 0, SEEK_END);
	      size = ftell(message_file);
	      fseek(message_file, 0, SEEK_SET);

	      status = send(client_fd, to_string(size).c_str() , strlen(to_string(size).c_str()),0);
	      //cout<<size<<endl;
	      if (status < 0){
	      	cerr<<"ERROR sending message to client"<<endl;
	      	close(client_fd);
	      	close(server_fd);
	      	exit(5);
	      }

	      bzero(response,255);
	      status=recv(client_fd,response,255,0);
	      if(status<0){
	      	cerr<<"Error receiving message from client"<<endl;
	      	close(client_fd);
	      	break;
	      }
	      response_string=response;
	      response_string=response_string.substr(0,status);
	      // cout<<response_string<<endl;
	      
	      if(size==stoi(response_string)){
	      	//cerr<<6<<endl;
		cout<<username<<": Transferring Message "<<file_n<<endl;
		char buffer[1000];
		bzero(buffer, 1000); 
		int chunk_size; 
		while((chunk_size = fread(buffer, sizeof(char), 1000, message_file)) > 0){
		  
		  status=send(client_fd, buffer, chunk_size, 0);
		  if(status<0){
		    cerr<<"Error sending message to client"<<endl;
		    close(client_fd);
		    break;
		  }
		  //	cout<<status<<endl;
		  bzero(buffer, 1000);
		}	 
		//cout<<"file sent"<<endl;
		found=1;
	      }
	      else{
		cerr<<"Couldnt send message"<<endl;
		exit(5);
	      }
	      fclose(message_file);
	      break;
	    }
	  }
	  if(!found){
	    cout<<"Message Read Fail"<<endl;
	    close(client_fd);
	    break;
	  }
	}
      }
      else{
	cout<<"Unknown Command"<<endl;
	close(client_fd);
	break;
      }

      
      
    }
    passwdfile.close();
    close(client_fd);
  }
  
  close(server_fd);
  
}
