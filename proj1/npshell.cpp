
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <algorithm>
#include <fcntl.h>
#include <iostream>
#include <vector>
#include <string>
#include <assert.h>
#include "Command.cpp"
// #define COMMAND
// #define NUMPIPE
// #define CLOSEPIPE
// #define PIPE
// #define DEBUG
// #define WAIT
using namespace std;
vector<NumPipe> numPipes;

void sigchld_handler(int sig){
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0) {
        // 處理子進程的退出
        if (WIFEXITED(status)) {
#ifndef WAIT
            printf("child process ends. return：%d\n", WEXITSTATUS(status));
#endif
        } else if (WIFSIGNALED(status)) {
#ifndef WAIT    
            printf("child process ended by signal. signal：%d\n", WTERMSIG(status));
#endif
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
int returnNumPipesZeroIdx(){
    for(int i = 0; i < numPipes.size(); i++){
        if(numPipes[i].step == 0){
            return i;
        }
    }
    return -1;
}
int reviewNumPipe(){//因為寫入的時候已經把接到同指令的合併用同個pipe，所以只須考慮不同位置情況
    int idx = -1;
    for(int i = 0; i < numPipes.size(); i++){
        assert(numPipes[i].step > 0);//若<=0則有問題
#ifndef NUMPIPE
        cerr << __FILE__ << " " << __LINE__ << endl;
        printf("numPipe[%d] = [%d, %d] is ready\n", i, numPipes[i].pipe[0], numPipes[i].pipe[1]);
#endif
        numPipes[i].step = numPipes[i].step - 1;
        if(numPipes[i].step == 0){//找到不能直接return，後面的也要繼續--才行
            idx = i;
        }
        
    }
    return idx; 
}
vector<Command> extractCommand(vector<string> spaceSplit){
    
    vector<Command> result;//存放切完的command，以|N or ! else 一行結束分割
    Command command;
    vector<string> tmp;
    
    for(int i = 0; i < spaceSplit.size(); i++){
        if(spaceSplit[i][0] == '|' || spaceSplit[i][0] == '!'){ //&& spaceSplit[i].size() > 1){ //找到number pipe || !
            
            if(spaceSplit[i].size() > 1){
                Job jobs;
                if(spaceSplit[i][0] == '|')
                    command.isNumPipe = true;
                else
                    command.isErrorPipe = true;
                
                int number = stoi(spaceSplit[i].substr(1));
                command.number = number;

                jobs.arg = tmp;
                
                command.jobs.push_back(jobs);
                //找完一個command存起來
                result.push_back(command);

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
        else {//其他先存起來
            tmp.push_back(spaceSplit[i]);
        }

    }
    //若有最後一個command則存起來
    if(tmp.size() > 0){
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
        if(job.arg[i] == ">"){
            int fd = open(job.arg[i + 1].c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args[i] = nullptr;
            break;
        }
        args[i] = new char[job.arg[i].size() + 1];
        strcpy(args[i], job.arg[i].c_str());
    }
    args[job.arg.size()] = nullptr;
#ifndef EXEC
    for(int i = 0; args[i] != nullptr; i++){
        cerr << "i = " << i << " " << args[i] << " ";
    }
    cerr << endl<<flush;
#endif
    if (execvp(args[0], args) == -1) {
        cerr << "Unknown command: [" << job.arg[0] << "]." << endl;
        exit(-1);
    }
}

bool isBuildIn(Job job){
    if(job.arg[0] == "setenv"){
        setenv(job.arg[1].c_str(), job.arg[2].c_str(), 1);
        return true;
    }else if(job.arg[0] == "printenv"){
        //有可能參數塞不存在則會回傳nullptr，因此需要先判斷
        char* r = getenv(job.arg[1].c_str());
        if(r != nullptr){
            printf("%s\n", r);
        }
        return true;
    }else if(job.arg[0] == "exit"){
        exit(0);
    }
    return false;
}

int run(){
    string input;
    cout << "% ";
    while(getline(cin, input)){
        if(input == ""){
            cout << "% ";
            continue;
        }

        //以空白分割
        string delimiter = " ";
        vector<string> spaceSplit = split(input, delimiter);
        //切command        
        vector<Command> commands = extractCommand(spaceSplit);
#ifndef COMMAND
        cerr << __FILE__ << " " << __LINE__ << endl;
        for(int i = 0; i < commands.size(); i++){
            printf("Command %d, isNumPipe %d, isErrPipe %d : \n", i, commands[i].isNumPipe, commands[i].isErrorPipe);
            for(int j = 0; j < commands[i].jobs.size(); j++){
                printf("Job %d : ", j);
                for(int k = 0; k < commands[i].jobs[j].arg.size(); k++){
                    cout << commands[i].jobs[j].arg[k] << " ";
                }
                cout << endl;
            }
        }
        cout << endl;
#endif
        int pid;
        for(int cmdIndex = 0; cmdIndex < commands.size(); cmdIndex++){
            int numPipeIdx = reviewNumPipe();
            // Command command = commands[cmdIndex];
            //建立number pipe
            if(commands[cmdIndex].isNumPipe || commands[cmdIndex].isErrorPipe){

                int find = 0;
                //找現在有沒有相同的number pipe指向同個位置，如果有就用舊的pipe(j-th)即可不用重開
                for(int numPipesIterator = 0; numPipesIterator < numPipes.size(); numPipesIterator++){
                    if(numPipes[numPipesIterator].step == commands[cmdIndex].number){
                        find = 1;
                        commands[cmdIndex].numPipesOutIdx = numPipesIterator;
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
                    numPipe.step = commands[cmdIndex].number;//這個存還有多少輪到我丟出去
                    numPipes.push_back(numPipe);
                    commands[cmdIndex].numPipesOutIdx = numPipes.size() - 1;//這個存在numPipes的
                    commands[cmdIndex].pipeOut = numPipe.pipe;
                }
            }

            //Build in
            if(isBuildIn(commands[cmdIndex].jobs[0]))
                break;

            //Pipe
            int pipeArray[2][2];
            for(int jobIdx = 0; jobIdx < commands[cmdIndex].jobs.size(); jobIdx++){
                if(jobIdx > 0){
                    commands[cmdIndex].jobs[jobIdx].pipeIn = pipeArray[(jobIdx - 1) % 2];
                }

                if(jobIdx != commands[cmdIndex].jobs.size() - 1){
                    commands[cmdIndex].jobs[jobIdx].pipeOut = pipeArray[jobIdx % 2];
                    if(pipe(pipeArray[jobIdx % 2]) == -1){
                        perror("pipe failed");
                        exit(1);
                    }
                }

                if(jobIdx == 0 && numPipeIdx != -1){
                    commands[cmdIndex].jobs[jobIdx].pipeIn = numPipes[numPipeIdx].pipe;
                }

                if(jobIdx == commands[cmdIndex].jobs.size() - 1 && (commands[cmdIndex].isNumPipe || commands[cmdIndex].isErrorPipe)){
                    commands[cmdIndex].jobs[jobIdx].pipeOut = numPipes[commands[cmdIndex].numPipesOutIdx].pipe;
                }
            

                pid = fork();
                if(pid < 0){
                    close(pipeArray[jobIdx % 2][0]);
                    close(pipeArray[jobIdx % 2][1]);
                    jobIdx--;
                    continue;
                }else if(pid == 0){
                    if(commands[cmdIndex].jobs[jobIdx].pipeIn != nullptr){
                        dup2 (commands[cmdIndex].jobs[jobIdx].pipeIn[0], STDIN_FILENO);
                        close(commands[cmdIndex].jobs[jobIdx].pipeIn[0]);
                        close(commands[cmdIndex].jobs[jobIdx].pipeIn[1]);
                    }

                    if(commands[cmdIndex].jobs[jobIdx].pipeOut != nullptr){
                        dup2( commands[cmdIndex].jobs[jobIdx].pipeOut[1], STDOUT_FILENO);
                        if(commands[cmdIndex].isErrorPipe){
                            dup2(commands[cmdIndex].jobs[jobIdx].pipeOut[1], STDERR_FILENO);
                        }
                        close(commands[cmdIndex].jobs[jobIdx].pipeOut[0]);
                        close(commands[cmdIndex].jobs[jobIdx].pipeOut[1]);

                    }
                    exec(commands[cmdIndex].jobs[jobIdx]);
                }else{//pid > 0的情況
#ifndef CLOSEPIPE
                    cerr << __FILE__ << " " << __LINE__ << " parent 000000" << endl;
#endif
                    if (jobIdx > 0){
                        close(pipeArray[(jobIdx - 1) % 2][0]);
                        close(pipeArray[(jobIdx - 1) % 2][1]);
                    }
#ifndef CLOSEPIPE
                    cerr << __FILE__ << " " << __LINE__ << " parent 11111111" << endl;
#endif
                    if(jobIdx == 0  && numPipeIdx != -1){
                        close(numPipes[numPipeIdx].pipe[1]);//關numPipe的寫入端
                        close(numPipes[numPipeIdx].pipe[0]);
                        numPipes.erase(numPipes.begin() + numPipeIdx);
                    }
                }
            }
            if(!(commands[cmdIndex].isNumPipe || commands[cmdIndex].isErrorPipe)){
                while (wait(NULL) > 0);
            }
            
        }
        cout << "% ";
    }
    clearenv();
    return 0;
}
int main(){
    signal(SIGCHLD, sigchld_handler);

    //預設PATH變數
    setenv("PATH", "bin:.", 1);

    //讀取command;
    run();
    
    return 0;
}
