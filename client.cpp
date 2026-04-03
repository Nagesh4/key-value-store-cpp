#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <sstream>
using namespace std;

int main(){
  int sock = 0;
  struct sockaddr_in serv_addr;

  //1. creating socket
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if(sock < 0){
    cout<<"Socket creation failed\n";
    return 1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(8080);

  //connect to server (localhost)
  if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0){
    cout<<"Invalid address\n";
    return 1;
  }

  if(connect(sock, (struct sockaddr * )&serv_addr, sizeof(serv_addr)) < 0){
    cout<<"Connection failed\n";
    return 1;
  }

  cout<<"Connected to the server: Type commands:\n";
  
  while(true){
    cout<<">";

    string input;
    string command, key, value;
    
    getline(cin, input);
    if(input == "exit"){
      break;
    }
    
    stringstream ss(input);
    ss>>command>>key;

    if(command.empty()){
      cout<<"Command is empty\n";
      continue;
    }

    getline(ss, value);
    if(!value.empty() && value[0] == ' '){
      value.erase(0, 1);
    }
    
    if(command == "SET"){
      if(key.empty() || value.empty()){
	cout<<"Invalid SET command\n";
	cout<<"Usage: SET key value\n";
	continue;
      }	
    } else if(command == "GET"){
      if(key.empty()){
	cout<<"Invalid GET command\n";
	cout<<"Usage: GET key\n";
	continue;
      }	
    } else if(command == "EXPIRE"){
      if(key.empty() || value.empty()){
	cout<<"Invalid EXPIRY command\n";
	cout<<"Usage: EXPIRE key seconds\n";
	continue;
      }
    } else {
      cout<<"Invalid command\n";
      continue;
    }

    //send to the server
    int bytes  = send(sock, input.c_str(), input.size(), 0);

    if(bytes <= 0){
      cout<<"Send failed\n";
      break;
    }

    //read response
    char buffer[1024] = {0};
    
    bytes = read(sock, buffer, 1024);

    if(bytes <= 0){
      cout<<"Server is disconnected\n";
      break;
    }

    cout<<"[Server]: "<<buffer;
    
  }

  close(sock);
  return 0;
}
