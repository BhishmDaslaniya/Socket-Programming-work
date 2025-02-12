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


  int server_fd;
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0){
    cerr<<"Error while opening socket";
    exit(5);
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family=AF_INET;
  server_addr.sin_port=htons(server_port);
  //server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  if(bind(server_fd, (struct sockaddr*)& server_addr, sizeof(server_addr)) < 0){
    cerr<<"Bind failed"<<endl;
    close(server_fd);
    exit(2);
  }
  else{
    cout<<"BindDone:"<<server_port<<endl;
  }

  listen(server_fd,4);
  cout<<"ListenDone:"<<server_port<<endl;

  vector<int> client_fds;
  map<int, string> usernames;
  
  while(true){

    int maxsock = -1;
    fd_set read_fd_set;
    FD_ZERO(&read_fd_set);
    FD_SET(server_fd, &read_fd_set);  
    maxsock = max(maxsock,server_fd);

    for (int i=0; i<client_fds.size(); i++){
      FD_SET(client_fds[i], &read_fd_set);
      maxsock = max(client_fds[i],maxsock);
    }

    struct timeval timeout = {10, 0}; 
    int retval = select(maxsock+1, &read_fd_set, NULL, NULL, &timeout);
    if(retval >= 0){
      if(FD_ISSET(server_fd, &read_fd_set)){
	struct sockaddr_in client_addr;
	int client_addr_len=sizeof(client_addr);
	int newClientSock = accept(server_fd, (struct sockaddr*)& client_addr,(socklen_t *) &client_addr_len);
	if (newClientSock >= 0){
	  client_fds.push_back(newClientSock);
	  cout<<"Client:"<<inet_ntoa(client_addr.sin_addr)<<":"<<(int) ntohs(client_addr.sin_port)<<endl;
	}
	else perror("accept");
      }


      //client_fd=accept(server_fd, (struct sockaddr*)& client_addr,(socklen_t *) &client_addr_len);

      regex login_regex("^User: [^\\s]+ Pass: [^\\s]+$");
      regex retrv_regex("^RETRV [0-9]+$");
      string username("default"), passwd("default");
      bool auth=0;
      ifstream passwdfile (argv[2]);

      for (int i=client_fds.size()-1; i>=0; i--){
	if (FD_ISSET(client_fds[i], &read_fd_set)){
	  int client_fd=client_fds[i];
	  //cout<<client_fd<<endl;
	  // cout<<client_fds[i]<<endl;
	  char response[256];
	  int status=0;
	  bzero(response,255);
	  status=read(client_fd,response,255);
	  if(status<0){
	    cerr<<"Error receiving message from client"<<endl;
	    close(client_fd);
	    client_fds.erase(client_fds.begin()+i);
	    break;
	  }

	  string response_string(response);
	  response_string=response_string.substr(0,status);
	  cerr<<response_string<<endl;

	  if(regex_match(response_string, login_regex)){
	    smatch sm;
	    regex word_regex("[^\\s]+");
	    regex_search(response_string, sm, word_regex);
	    int j=0;
	    while(regex_search(response_string,sm,word_regex)){
	      if(j==1) {
		//cout<<"client_fd is "<<client_fd<<"  "<<usernames[client_fds[i]]<<"  "<<client_fds[i]<<endl;
		usernames[client_fds[i]]=sm[0];
		username=usernames[client_fds[i]];
	      }
	      else if(j==3) passwd=sm[0];
	      j++;
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
		      client_fds.erase(client_fds.begin()+i);
		      break;
		    }
		  }
		  else{
		    cout<<"Wrong Passwd"<<endl;
		    close(client_fd);
		    client_fds.erase(client_fds.begin()+i);
		    break;	  
		  }
		}
		else {
		  cout<<"Invalid User"<<endl;
		  close(client_fd);
		  client_fds.erase(client_fds.begin()+i);
		  break;
		} 
	      }
	  }
	  else if(response_string=="quit"){
	    //cout<<client_fd<<"  "<<usernames[client_fd]<<endl;
	    cout<<"Bye "<<usernames[client_fds[i]]<<endl;
	    close(client_fd);
	    client_fds.erase(client_fds.begin()+i);
	  }
	  else if(regex_match(response_string, retrv_regex)){
	    // cout<<client_fd<<"  "<<usernames[client_fd]<<endl;
	    //cout<<2<<endl;
	    smatch sm;
	    regex word_regex("[^\\s]+");
	    int file_n;
	    regex_search(response_string, sm, word_regex);
	    int j=0;
	    while(regex_search(response_string,sm,word_regex)){
	      if(j==1){
	      	cerr<<sm[0]<<endl;
			file_n=stoi(sm[0]);
			break;
	      }
	      j++;
	      response_string = sm.suffix().str();
	    }

	    string user_dir_string=user_database_name+"/"+usernames[client_fds[i]];
	    char user_dir_char[user_dir_string.length()+1];
	    strcpy(user_dir_char,user_dir_string.c_str());
	    DIR* user_dir_dir;
	    //cout<<user_dir_string<<endl;
	    if ((user_dir_dir=opendir(user_dir_char))!=NULL) {
	      //cout<<3<<endl;
	      struct dirent *dp;
	      bool found=0;
	      while ((dp = readdir((user_dir_dir))) != NULL) {
		string message_name=dp->d_name;
		message_name=message_name.substr(0,message_name.length()-4);
		if(message_name==to_string(file_n)){
		  message_name=dp->d_name; // fullname of the required message
		  message_name=user_dir_string+"/"+message_name;

		  status = send(client_fd, dp->d_name, strlen(dp->d_name),0);
		  if (status < 0){
		    cerr<<"ERROR sending message to client"<<endl;
		    close(client_fd);
		    client_fds.erase(client_fds.begin()+i);
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
		  //cout<<1<<endl;
		  status = send(client_fd, to_string(size).c_str() , strlen(to_string(size).c_str()),0);
		  if (status < 0){
		    cerr<<"ERROR sending message to client"<<endl;
		    close(client_fd);
		    client_fds.erase(client_fds.begin()+i);
		    // close(server_fd);
		    exit(5);
		  }

		  bzero(response,255);
		  status=recv(client_fd,response,255,0);
		  if(status<0){
		    cerr<<"Error receiving message from client"<<endl;
		    close(client_fd);
		    client_fds.erase(client_fds.begin()+i);
		  }
		  response_string=response;
		  response_string=response_string.substr(0,status);
		  // cout<<response_string<<endl;

		  if(size==stoi(response_string)){
		    cout<<usernames[client_fds[i]]<<": Transferring Message "<<file_n<<endl;
		    char buffer[1000];
		    bzero(buffer, 1000); 
		    int chunk_size; 
		    while((chunk_size = fread(buffer, sizeof(char), 1000, message_file)) > 0){
		      status=send(client_fd, buffer, chunk_size, 0);
		      if(status<0){
			cerr<<"Error sending message to client"<<endl;
			close(client_fd);
			client_fds.erase(client_fds.begin()+i);
			break;
		      }
		      //	cout<<status<<endl;
		      bzero(buffer, 1000);
		    }	 
		    //cout<<"file sent"<<endl;
		  }
		  else{
		    cerr<<"Couldnt send message"<<endl;
		    client_fds.erase(client_fds.begin()+i);
		    close(client_fd);
		  }	      
		  fclose(message_file);
		  found=1;
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
	    client_fds.erase(client_fds.begin()+i);
	  }
	  // for(map<int, string>::const_iterator it = usernames.begin();
	  //     it != usernames.end(); ++it)
	  //   {
	  //     std::cout << it->first << " " << it->second<< "\n";
	  //   }
	}
      }
    }
  }
  passwdfile.close();
  close(server_fd);
}
