#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <bits/stdc++.h>
#include <stdlib.h>
#include <fstream>
#include <arpa/inet.h>
#include <unistd.h>
#include <regex>
#include<dirent.h>

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

  // const int enable=1;
  // if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
  //   cerr<<"setsockopt(SO_REUSEADDR) failed"<<endl;

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
  cout<<"ListenDone:"<<server_port<<endl<<endl;
  
  while(true){
    // listen(server_fd,4);
    //cout<<"ListenDone:"<<server_port<<endl;

    int client_addr_len=sizeof(client_addr);
    client_fd=accept(server_fd, (struct sockaddr*)& client_addr,(socklen_t *) &client_addr_len);
    cout<<"Client:"<<inet_ntoa(client_addr.sin_addr)<<":"<<(int) ntohs(client_addr.sin_port)<<endl;

    char response[256];
    int status=0;
    regex login_regex("^User: [^\\s]+ Pass: [^\\s]+$");
    string username("default"), passwd("default");
    bool auth=0;
    ifstream passwdfile (argv[2]);
    
    while(true){
  
      status=read(client_fd,response,255);
      if(status<0){
	cerr<<"Error receiving message from client"<<endl;
	close(client_fd);
	break;
      }
      
      string response_string(response);
      response_string=response_string.substr(0,status);
 
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
      else if(response_string=="LIST"){
	if(auth){
	  string user_dir_string=user_database_name+"/"+username;
	  char user_dir_char[user_dir_string.length()+1];
	  strcpy(user_dir_char,user_dir_string.c_str());

	  //cout<<user_dir_char<<endl;
    
	  DIR *user_dir;
	  if ((user_dir=opendir(user_dir_char))==NULL) {
	    cout<<username<<": Folder Read Fail"<<endl;
	    close(client_fd);
	    break;
	  }
	  else{
	    struct dirent *dp;
	    int N=0;
	    while ((dp = readdir(user_dir)) != NULL) {
	      N++;
	    }
	    N-=2;
	    //cout<<N<<endl;
	    string list_string=username+": No of messages "+to_string(N);
	    cout<<list_string<<endl;

	    char list_char[list_string.length()+1];
	    strcpy(list_char,list_string.c_str());
	    list_char[list_string.length()]='\0';

	    status = write(client_fd, list_char, list_string.length());
	    if (status < 0){
	      cerr<<"ERROR sending message to client"<<endl;
	      close(client_fd);
	      close(server_fd);
	      exit(5);
	    }
	  }
	}
	else{
	  cout<<"Do login first"<<endl;
	}
      }
      else if(response_string=="quit"){
	cout<<"Bye "<<username<<endl;
	break;
      }
      else{
	cout<<"Unknown Command"<<endl;
	close(client_fd);
	break;
      }

      
      /////

      

      // char welcome_char[welcome.length()+1];
      // strcpy(welcome_char,welcome.c_str());
      // welcome_char[welcome.length()]='\0';

  
  
      // status = write(client_fd, welcome_char, welcome.length());
      // if (status < 0){
      // 	cerr<<"ERROR sending message to client"<<endl;
      // 	close(client_fd);
      // 	close(server_fd);
      // 	exit(5);
      // }
      // status=read(client_fd,response,255);
      // if(status<0){
      // 	cerr<<"Error receiving message from client"<<endl;
      // 	close(client_fd);
      // 	close(server_fd);
      // 	exit(5);
      // }
      // response_string=response;
      // response_string=response_string.substr(0,status);
      // //cout<<response_string<<endl;

     
      // else{
      // 	cout<<"Unknown command"<<endl;
      // 	close(client_fd);
      // 	close(server_fd);
      // 	exit(0);
      // }

      // status=read(client_fd,response,255);
      // if(status<0){
      // 	cerr<<"Error receiving message from client"<<endl;
      // 	close(client_fd);
      // 	close(server_fd);
      // 	exit(5);
      // }
      // response_string=response;
      // response_string=response_string.substr(0,status);
      //cout<<response_string<<endl;
    }
    cout<<endl;
    passwdfile.close();
    close(client_fd);
  }
  
  close(server_fd);
  
}
