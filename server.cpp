#include <iostream>
#include <cstring>
#include <unistd.h> //close
#include <arpa/inet.h> //socket, bind, listen, accept
#include <thread>
#include <unordered_map>
#include <mutex>
#include <sstream>
#include <fstream>
#include <chrono>
#include <queue>
#include <condition_variable>
using namespace std;

void handle_client(int client_socket);
void append_to_file(const string &command_line);
void load_from_file();
void parse_command(string &input, string &command, string &key, string &value);
void cleanup_expired_keys();
void worker_thread();
void compact_file_background();
long get_file_size();
void auto_compaction_worker();

unordered_map<string, string> store;
unordered_map<string, chrono::steady_clock::time_point> expiry;
mutex mtx;
queue<int> task_queue;
mutex queue_mtx;
condition_variable cv;
bool is_compacting = false;
const size_t FILE_THRESHOLD = 50;

int main() {
  int server_fd, client_socket;
  int opt = 1;

  struct sockaddr_in address;
  int addrlen = sizeof(address);

  //1. creating socket
  server_fd = socket(AF_INET, SOCK_STREAM, 0);

  if(server_fd < 0){
    perror("socket failed");
    return 1;
  }

  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  //2. bind and adding port
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(8080);

  if(bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0){
    perror("Bind failed");
    return 1;
  }
  
  //3. Listen
  if(listen(server_fd, 3) < 0){
    perror("Listen failed");
    return 1;
  }
  cout<<"Server running on port 8080..."<<endl;

  load_from_file();
  thread cleaner(cleanup_expired_keys);
  cleaner.detach();

  int THREAD_POOL_SIZE = 4;
  vector<thread> workers;
  for(int i=0;i<THREAD_POOL_SIZE;i++){
    workers.emplace_back(worker_thread);
  }
  thread compaction_thread(auto_compaction_worker);
  compaction_thread.detach();
  
  while(true){
    client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);

    if(client_socket < 0){
      perror("Accept failed");
      return 1;
    }

    cout<<"New client connected"<<endl;

    //client per thread
    // thread t (handle_client, client_socket);
    // t.detach();
    {
      lock_guard<mutex> lock(queue_mtx);
      task_queue.push(client_socket);
    }

    cv.notify_one();
  }

  close(server_fd);
  return 0;
}

void handle_client (int client_socket){
  char buffer[1024] = {0};

  while(true){
    
    memset(buffer, 0, sizeof(buffer));
    int bytes = read(client_socket, buffer, 1024);
    
    if(bytes <= 0){
      cout<<"Client disconnected\n";
      break;
    }
    
    string input(buffer); //converted char in string
    cout<<"Client says: "<<input<<endl;

    string command, key, value;
    parse_command(input, command, key, value);
    
    string response;
    
    if(command == "SET"){
      lock_guard<mutex> lock(mtx);
      store[key] = value;
      response = "OK\n";

      append_to_file(input);
      
    } else if(command == "GET"){
      lock_guard<mutex> lock(mtx);
      
      // //check for expiry
      // if(expiry.find(key) != expiry.end()){
      // 	if(chrono::steady_clock::now() > expiry[key]){
      // 	  store.erase(key);
      // 	  expiry.erase(key);
      // 	  response = "KEY NOT FOUND\n";
      // 	  send(client_socket, response.c_str(), response.size(), 0);
      // 	  continue;
      // 	}
      // }
      
      if(store.find(key) != store.end()){
	response = store[key]+"\n";
      } else {
	response = "KEY NOT FOUND\n";
      }
      
    } else if(command == "EXPIRE"){
      
      int seconds = stoi(value);
      lock_guard<mutex> lock(mtx);

      if(store.find(key) !=  store.end()){
	expiry[key] = chrono::steady_clock::now() + chrono::seconds(seconds);
	response = "OK\n";
      } else {
	response = "KEY NOT FOUND\n";
      }
      
    } else if(command == "EXIT"){
      response = "Goodbye\n";
      send(client_socket, response.c_str(), response.size(), 0);
      break;
      
    } else if(command == "COMPACT"){
      
      if(!is_compacting){
	thread t(compact_file_background);
	t.detach();
	response = "COMPACTION STARTED\n";
      } else {
	response = "COMPACTION ALREADY STARTED\n";
      }
      
    }else{
      response = "INVALID COMMAND\n";
    }
    
    send(client_socket, response.c_str(), response.size(), 0);
  }
  
  close(client_socket);
}

void append_to_file(const string &command_line){

  ofstream file("data.txt", ios::app);
  file<<command_line<<"\n";
  
}

void load_from_file(){
  ifstream file("data.txt");

  if(!file){
    cout<<"Cannot open file\n";
    return;
  }
  
  string line;

  while(getline(file, line)){
    string command, key, value;
    parse_command(line, command, key, value);
    if(command == "SET"){
      store[key] = value;
    } else if(command == "DEL"){
      store.erase(key);
    }
    
  }
  
}

void parse_command(string &input, string &command, string &key, string &value){
  stringstream ss(input);
  ss>>command>>key;
  getline(ss, value);

  if(!value.empty() && value[0] == ' '){
    value.erase(0, 1);
  }
}

void cleanup_expired_keys(){

  while(true){
    this_thread::sleep_for(chrono::seconds(2)); //sleep for 2 seconds
    lock_guard<mutex> lock(mtx);

    auto now = chrono::steady_clock::now();

    for(auto it = expiry.begin(); it != expiry.end(); ){

      if(now > it->second){
	string key = it->first;

	store.erase(key);
	it = expiry.erase(it);
	
	append_to_file("DEL "+key);
	cout<<"Expired key removed: "<<key<<endl;
      } else {
	++it;
      }
    }
  }
}

void worker_thread(){
  while(true){
    int client_socket;

    {  
      unique_lock<mutex> lock(queue_mtx);
      cv.wait(lock, [] {return !task_queue.empty();});
      client_socket = task_queue.front();
      task_queue.pop();
    }
    
    handle_client(client_socket);
  }
}

void compact_file_background(){
  unordered_map<string, string> snapshot;

  {
    lock_guard<mutex> lock(mtx);
    snapshot = store;
  }

  ofstream file("data_temp.txt");

  for(auto &pair:snapshot){
    file<<"SET "<<pair.first<<" "<<pair.second<<"\n";
  }

  file.close();

  {
    lock_guard<mutex> lock(mtx);
    remove("data.txt");
    rename("data_temp.txt", "data.txt");
    is_compacting = false;
  }

  cout<<"Background  compaction done\n";
  
}

long get_file_size(){
  ifstream file("data.txt", ios::binary | ios::ate);

  if(!file){
    return 0; //file doesn't exist yet
  }
  
  return file.tellg();
}

void auto_compaction_worker(){
  while(true){
    this_thread::sleep_for(chrono::seconds(30));

    if(get_file_size() > FILE_THRESHOLD){
      bool should_start = false;

      {
	lock_guard<mutex> lock(mtx);
	if(!is_compacting){
	  is_compacting = true;
	  should_start = true;
	}
      }

      if(should_start){
	thread t(compact_file_background);
	t.detach();
      }

    }
  }
}
