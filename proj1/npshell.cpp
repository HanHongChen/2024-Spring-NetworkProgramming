
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
#include <assert.h>
#include "Command.cpp"

#define DEBUG
// int size = 256;
using namespace std;
// vector<Command> numPipes;
// vector<int*> pipe_v;//讓main可以close

vector<Job> extractJob(vector<string> tmp);

vector<NumPipe> numPipes;

int state;
void sigchld_handler(int sig){
    int status;
    while (waitpid(-1, &status, 0) > 0) {
        // 處理子進程的退出
        if (WIFEXITED(status)) {
            //printf("child process ends. return：%d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            //printf("child process ended by signal. signal：%d\n", WTERMSIG(status));
        }
    }

}
void waitChildProcesses(){
    int status;
    while (waitpid(-1, &status, 0) > 0) {
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

int reviewNumPipe(){//因為寫入的時候已經把接到同指令的合併用同個pipe，所以只須考慮不同位置情況
    for(int i = 0; i < numPipes.size(); i++){
        assert(numPipes[i].number > 0);//若<=0則有問題
        numPipes[i].number = numPipes[i].number - 1;
        if(numPipes[i].number == 0){
            //怎麼處理?
#ifndef NUMPIPE
            cerr << __FILE__ << " " << __LINE__ << endl;
            printf("numPipe[%d] = [%d, %d] is ready\n", i, numPipes[i].pipe[0], numPipes[i].pipe[1]);
#endif
            return i;
        }
    }
    return -1; 
}
vector<Command> extractCommand(vector<string> spaceSplit){
    
    vector<Command> result;//存放切完的command，以|N or ! else 一行結束分割
    Command command;
    vector<string> tmp;
    
    for(int i = 0; i < spaceSplit.size(); i++){
        if(spaceSplit[i][0] == '|' || spaceSplit[i][0] == '!'){ //&& spaceSplit[i].size() > 1){ //找到number pipe || !
            
            if(spaceSplit[i].size() > 1){
                if(spaceSplit[i][0] == '|')
                    command.isNumPipe = true;
                else
                    command.isErrorPipe = true;
                
                int number = stoi(spaceSplit[i].substr(1));
                // command.number = number;

                int find = 0;
                //找現在有沒有相同的number pipe指向同個位置，如果有就用舊的pipe(j-th)即可不用重開
                for(int j = 0; j < numPipes.size(); j++){
                    if(numPipes[j].number == number){
                        find = 1;
                        command.number = j;
                    }
                }
                //開number Pipe的pipe
                if(find == 0){
                    NumPipe numPipe;
                    numPipe.pipe = new int[2];
                    if(pipe(numPipe.pipe) == -1){
                        perror("pipe failed");
                        exit(1);
                    }
                    numPipe.number = number;//這個存還有多少輪到我丟出去
                    numPipes.push_back(numPipe);
                    command.number = numPipes.size() - 1;//這個存在numPipes的位置
                }

                //以|分出jobs
                // vector<Job> jobs = extractJob(tmp);
                //number pipe的時候要把tmp存入Job的arg中
                Job jobs;
                jobs.arg = tmp;
                command.jobs.push_back(jobs);
                // //找完一個command存起來
                result.push_back(command);

                //找到新的command就更新現有number pipe
                reviewNumPipe();
                //reset
                command = Command();
                tmp.clear();
            }else if(i < spaceSplit.size() - 1){ //是一般的pipe情況
                Job jobs;
                jobs.arg = tmp;
                jobs.isPipe = true;
                command.jobs.push_back(jobs);
                tmp.clear();
            }
            

        }
        // else if(spaceSplit[i][0] == '|'){
        //     extractCommand(tmp);

        // }
        else {//其他先存起來
            tmp.push_back(spaceSplit[i]);
        }

    }
    //若有最後一個command則存起來
    if(tmp.size() > 0){
        // vector<Job> jobs = extractJob(tmp);
        // command.jobs = jobs;
        // result.push_back(command);
        // reviewNumPipe();
        Job jobs;
        jobs.arg = tmp;
        command.jobs.push_back(jobs);
        result.push_back(command);
    }
    
    return result;
}

void exec(Job job){
    char **args = new char*[job.arg.size() + 1];
    for(int i = 0; i < job.arg.size(); i++){
        args[i] = new char[job.arg[i].size() + 1];
        strcpy(args[i], job.arg[i].c_str());
    }
    args[job.arg.size()] = nullptr;
    if (execvp(args[0], args) == -1) {
        cerr << "Unknown command: [" << job.arg[0] << "]." << endl;
        exit(-1);
    }
}

bool isBuildIn(Job job){
    if(job.arg[0] == "setenv"){
        if(job.arg.size() == 3){
            setenv(job.arg[1].c_str(), job.arg[2].c_str(), 1);
        }else{
            printf("setenv: wrong number of arguments\n");
        }
        return true;
    }else if(job.arg[0] == "printenv"){
        if(job.arg.size() == 2){
            printf("%s\n", getenv(job.arg[1].c_str()));
        }else{
            printf("printenv: wrong number of arguments\n");
        }
        return true;
    }else if(job.arg[0] == "exit"){
        exit(0);
    }
    return false;
}

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

        //切command
        vector<Command> commands = extractCommand(spaceSplit);
        
        // if(spaceSplit[0] == "setenv"){
        //     if(spaceSplit.size() == 3){
        //         setenv(spaceSplit[1].c_str(), spaceSplit[2].c_str(), 1);
        //     }else{
        //         printf("setenv: wrong number of arguments\n");
        //     }
        //     cout << "% ";
        //     continue;
        // }else if(spaceSplit[0] == "printenv"){
        //     if(spaceSplit.size() == 2){
        //         printf("%s\n", getenv(spaceSplit[1].c_str()));
        //     }else{
        //         printf("printenv: wrong number of arguments\n");
        //     }
        //     cout << "% ";
        //     continue;
        // }else if(spaceSplit[0] == "exit"){
        //     exit(0);
        // }

#ifndef DEBUG
        cerr << __FILE__ << " " << __LINE__ << endl;
        for(int i = 0; i < commands.size(); i++){

            commands[i].print();
        }
        cout << endl;
#endif
        pid_t pid;
        int pipeArray[2][2];
        for(int i = 0; i < commands.size(); i++){
            // Command commands[i] = commands[i];
            // vector<Job> jobs = commands[i].jobs[i];
            //現有numPipes的記數-1


            //檢查是否為build in，是則跳過
            if(isBuildIn(commands[i].jobs[0]))
                break;

            for(int j = 0; j < commands[i].jobs.size(); j++){
                
#ifndef DEBUG
                cerr << __FILE__ << " " << __LINE__ << endl;
                cout << "Job " << j << " : ";
                for(int k = 0; k < commands[i].jobs[j].arg.size(); k++){
                    cout << commands[i].jobs[j].arg[k] << " ";
                }
                cout << endl;
                cout << "in = " << commands[i].jobs[j].in << " out = " << commands[i].jobs[j].out << endl;
#endif
                if(j > 0){
                    commands[i].jobs[j].pipeIn = pipeArray[(j - 1) % 2];
                    commands[i].jobs[j].in = pipeArray[(j - 1) % 2][0];
                }

                if(commands[i].jobs[j].isPipe){
                    commands[i].jobs[j].pipeOut = pipeArray[j % 2];
                    commands[i].jobs[j].out = pipeArray[j % 2][1];
                    if(pipe(pipeArray[j % 2]) == -1){
                        perror("pipe failed");
                        exit(1);
                    }
                }

                pid = fork();
                if(pid < 0){
                    perror("fork failed");
                    exit(1);
                }else if(pid == 0){
#ifndef DEBUG
                    cerr << __FILE__ << " " << __LINE__ << endl;
#endif
                    
                    if(commands[i].jobs[j].pipeIn != nullptr){
                        close(commands[i].jobs[j].pipeIn[1]);
                        dup2(commands[i].jobs[j].pipeIn[0], STDIN_FILENO);
                        // close(commands[i].jobs[j].pipeIn[1]);
                        // dup2(commands[i].jobs[j].pipeIn[0], STDIN_FILENO);
                        // close(commands[i].jobs[j].pipeIn[0]);
                    }

                    if(commands[i].jobs[j].pipeOut != nullptr){
                        close(commands[i].jobs[j].pipeOut[0]);
                        dup2(commands[i].jobs[j].pipeOut[1], STDOUT_FILENO);
                        close(commands[i].jobs[j].pipeOut[1]);

                    }
                    exec(commands[i].jobs[j]);
                }else if (pid > 0){//父
                    if (j > 0){
                        close(pipeArray[(j - 1) % 2][0]);
                        close(pipeArray[(j - 1) % 2][1]);
                        
                    }
                    int cpid;
                    while ((cpid = wait(NULL)) > 0) {
                    }
                    
                }
            }
        }


        cout << "% ";
        //開始處理pipe
        //需要當前的command 以及過去存起來的commands
        // handlePipe();
    }
    
    clearenv();
    return 0;
}
int main(){
    // signal(SIGCHLD, sigchld_handler);

    //預設PATH變數
    setenv("PATH", "bin:.", 1);

    runCommand();
    
    return 0;
}
