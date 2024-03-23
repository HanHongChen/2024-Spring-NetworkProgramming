
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <algorithm>
#include <fcntl.h>
#include <iostream>
#include <vector>
#include <string>
#include "Command.cpp"

// #define DEBUG
// int size = 256;
using namespace std;
vector<Command> commands;
void sigchld_handler(int sig){
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0) {
        // 處理子進程的退出
        if (WIFEXITED(status)) {
            //printf("child process ends. return：%d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            //printf("child process ended by signal. signal：%d\n", WTERMSIG(status));
        }
    }

}
vector<string> split(string str, string delimiter){
    vector<string> result;
    size_t pos = 0;
    string token;
    while((pos = str.find(delimiter)) != string::npos){
        token = str.substr(0, pos);
        result.push_back(token);
        str.erase(0, pos + delimiter.length());
    }
    result.push_back(str);
    return result;
}
vector<Job> extractJob(vector<string> tmp){
    vector<Job> result;
    Job job;;
    vector<string> arg;
    for(int i = 0; i < tmp.size(); i++){
        if(tmp[i] == "|"){
            job.isPipe = true;
            job.arg = arg;
            result.push_back(job);

            //重新初始化
            job = Job();
            arg.clear();
        }else{
            arg.push_back(tmp[i]);
        }
    }
    //最後一個job會因為沒有pipe了所以需要被存起來
    if(arg.size() > 0){
        job.arg = arg;
        result.push_back(job);
    }
    return result;
}
void extractCommand(vector<string> spaceSplit){
    // vector<Command> result;//存放切完的command，以|N or ! else 一行結束分割
    Command command;
    vector<string> tmp;
    for(int i = 0; i < spaceSplit.size(); i++){
        if((spaceSplit[i][0] == '|' || spaceSplit[i][0] == '!') && spaceSplit[i].size() > 1){ //找到number pipe || !
            if(spaceSplit[i][0] == '|')
                command.isNumPipe = true;
            else
                command.isErrorPipe = true;
            
            command.number = stoi(spaceSplit[i].substr(1));
            //開Pipe

            //以|分出jobs
            vector<Job> jobs = extractJob(tmp);
            command.jobs = jobs;
            //找完一個command存起來
            commands.push_back(command);

            //reset
            command = Command();
            tmp.clear();

        }else{//其他先存起來
            tmp.push_back(spaceSplit[i]);
        }

    }
    //若有最後一個command則存起來
    if(tmp.size() > 0){
        vector<Job> jobs = extractJob(tmp);
        command.jobs = jobs;
        commands.push_back(command);
    }
    
    // return result;
}
// void handlePipe(){
//     for(int i = 0; i < commands.size(); i++){
//         Command command = commands[i];
//         // if(!command.isNumPipe){

//         // }
//         int fd[2];
//         int fd_in = -1;
//         int fd_out = -1;
//         int fd_err = -1;
//         for(int j = 0; j < command.jobs.size(); j++){
        
//         }
// }
int runCommand(){
    //讀取使用者輸入，command長度不超過256 chars
    string input;
    
    
    cout << "% ";
    while (getline(cin, input)){
        //如果使用者輸入空白，則再次輸入
        if(input == ""){
            cout << "% ";
            continue;
        }

        //以空白分割
        string delimiter = " ";
        vector<string> spaceSplit = split(input, delimiter);
        
        if(spaceSplit[0] == "setenv"){
            if(spaceSplit.size() == 3){
                setenv(spaceSplit[1].c_str(), spaceSplit[2].c_str(), 1);
            }else{
                printf("setenv: wrong number of arguments\n");
            }
            cout << "% ";
            continue;
        }else if(spaceSplit[0] == "printenv"){
            if(spaceSplit.size() == 2){
                printf("%s\n", getenv(spaceSplit[1].c_str()));
            }else{
                printf("printenv: wrong number of arguments\n");
            }
            cout << "% ";
            continue;
        }else if(spaceSplit[0] == "exit"){
            exit(0);
        }

        //切command
        extractCommand(spaceSplit);

#ifndef DEBUG
        for(int i = 0; i < commands.size(); i++){
            commands[i].print();
        }
#endif

        //開始處理pipe
        //需要當前的command 以及過去存起來的commands
        // handlePipe();



        cout << "% ";

    }
    
    
    clearenv();
    return 0;
}
int main(){
    //預設PATH變數
    setenv("PATH", "bin:.", 1);

    runCommand();
    // signal(SIGCHLD, sigchld_handler);
    
    return 0;
}
