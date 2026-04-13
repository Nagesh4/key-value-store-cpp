#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <sstream>
using namespace std;

enum Command{
  CMD_SET = 1,
  CMD_GET = 2,
  CMD_DEL = 3,
  CMD_EXPIRE = 4,
  CMD_COMPACT = 5
};

int send_all(int sock, const void* buffer, int length);
void send_set(int sock, string key, string value);
void send_get(int sock, string key);
void send_del(int sock, string key);
void send_expiry(int sock, string key, int seconds);
void send_compact(int sock);

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
    int command = 0; // key_len, val_len;
    string key, value;
    
    getline(cin, input);
    if(input == "exit"){
      break;
    }
    
    stringstream ss(input);

    ss>>command>>key;
    
    if(command == 0){
      cout<<"Command is empty\n";
      continue;
    }

    getline(ss, value);

    if(!value.empty() && value[0] == ' '){
      value.erase(0, 1);
    }
    
    if(command == CMD_SET){
      if(key.empty() || value.empty()){
	cout<<"Invalid SET command\n";
	cout<<"Usage: SET key value\n";
	continue;
      }
      
      send_set(sock, key, value);
      
    } else if(command == CMD_GET){
      if(key.empty()){
	cout<<"Invalid GET command\n";
	cout<<"Usage: GET key\n";
	continue;
      }

      send_get(sock, key);
      
    } else if(command == CMD_EXPIRE){
      if(key.empty() || value.empty()){
	cout<<"Invalid EXPIRY command\n";
	cout<<"Usage: EXPIRE key seconds\n";
	continue;
      }

      int seconds = stoi(value);
      send_expiry(sock, key, seconds);
      
    } else if (command == CMD_COMPACT){
      send_compact(sock);
      
    } else if(command == CMD_DEL){
      if(key.empty()){
	cout<<"Usage: DEL key\n";
	continue;
      }

      send_del(sock, key);

    } else {
      cout<<"Invalid command\n";
    }

    //read response
    char buffer[1024] = {0};
    
    int bytes = recv(sock, buffer, 1024, 0);

    if(bytes <= 0){
      cout<<"Server is disconnected\n";
      break;
    }

    cout<<"[Server]: "<<buffer;
    
  }

  close(sock);
  return 0;
}

void send_set(int sock, string key, string value){
  int cmd = CMD_SET;
  int key_len = key.size();
  int value_len = value.size();

  send_all(sock, &cmd, sizeof(cmd));
  send_all(sock, &key_len, sizeof(key_len));
  send_all(sock, key.c_str(), key_len);
  send_all(sock, &value_len, sizeof(value_len));
  send_all(sock, value.c_str(), value_len);
}

void send_get(int sock, string key){
  int cmd = CMD_GET;
  int key_len = key.size();
  int bytes = 0;
  send_all(sock, &cmd, sizeof(cmd));
  send_all(sock, &key_len, sizeof(key_len));
  send_all(sock, key.c_str(), key_len);

}

void send_del(int sock, string key){
  int cmd = CMD_DEL;

  int key_len = key.size();
  send_all(sock, &cmd, sizeof(cmd));
  send_all(sock, &key_len, sizeof(key_len));
  send_all(sock, key.c_str(), key_len);
}

void send_expiry(int sock, string key, int seconds){
  int cmd = CMD_EXPIRE;
  int key_len = key.size();

  send_all(sock, &cmd, sizeof(cmd));
  send_all(sock, &key_len, sizeof(key_len));
  send_all(sock, key.c_str(), key_len);
  send_all(sock, &seconds, sizeof(seconds));
}

void send_compact(int sock){
  int cmd = CMD_COMPACT;
  send_all(sock, &cmd, sizeof(cmd));
}

int send_all(int sock, const void* buffer, int length) {
  int total = 0;
  while(total < length){
    int n = send(sock, (char *) buffer + total, length - total, 0);
    total+=n;
  }

  return total;
}
