
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
//兩個Number pipe有問題
#define NUMPIPE
// #define CLOSEPIPE
// #define PIPE
#define DEBUG
using namespace std;
vector<NumPipe> numPipes;

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
int returnNumPipesZeroIdx(){
    for(int i = 0; i < numPipes.size(); i++){
        if(numPipes[i].number == 0){
            return i;
        }
    }
    return -1;
}
int reviewNumPipe(){//因為寫入的時候已經把接到同指令的合併用同個pipe，所以只須考慮不同位置情況
    for(int i = 0; i < numPipes.size(); i++){
        // assert(numPipes[i].number > 0);//若<=0則有問題
// #ifndef NUMPIPE
//         cerr << __FILE__ << " " << __LINE__ << endl;
//         printf("numPipe[%d] = [%d, %d] is ready\n", i, numPipes[i].pipe[0], numPipes[i].pipe[1]);
// #endif
        numPipes[i].number = numPipes[i].number - 1;
        if(numPipes[i].number == 0){

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
                int numPipeIdx = reviewNumPipe();//要在將此次的number pipe存起來之前先扣一次先前的
                Job jobs;
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
                    jobs.pipeOut = numPipe.pipe;
                }

                //以|分出jobs
                // vector<Job> jobs = extractJob(tmp);
                //number pipe的時候要把tmp存入Job的arg中
                // Job jobs;
                jobs.arg = tmp;
                
                if(numPipeIdx != -1){
#ifndef NUMPIPE
                    cerr << __FILE__ << " " << __LINE__ << endl;
                    printf("pipeIn = [%d, %d]\n", numPipes[numPipeIdx].pipe[0], numPipes[numPipeIdx].pipe[1]);
#endif
                    jobs.pipeIn = numPipes[numPipeIdx].pipe;
                    
                    // command.pipeIn = numPipes[numPipeIdx].pipe;
                    command.numberIn = numPipeIdx;
                }
                command.jobs.push_back(jobs);
                // //找完一個command存起來
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
        int numPipeIdx = reviewNumPipe();
        if(numPipeIdx != -1){
            jobs.pipeIn = numPipes[numPipeIdx].pipe;
            command.numberIn = numPipeIdx;
        }
        command.jobs.push_back(jobs);
        result.push_back(command);
    }    
    //ls |1 cat ，cat的pipeIn吃不進來 ???
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
        // if(job.arg.size() == 3){
        setenv(job.arg[1].c_str(), job.arg[2].c_str(), 1);
        return true;
    }else if(job.arg[0] == "printenv"){
        printf("%s\n", getenv(job.arg[1].c_str()));
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
            // int numPipeIdx = reviewNumPipe();//這裡先扣的話會造成 ls |1 cat直接變成0
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
                if(j > 0 ){//j>0表示一定有PIPE，因為ls , cat test.html各自為一個job
                    commands[i].jobs[j].pipeIn = pipeArray[(j - 1) % 2];
                }

                // if(numPipeIdx != -1 && j == 0){//處理numPipe:有numberPipe且為第一個job，則此指令要接numPipe的as input
                //     commands[i].jobs[j].pipeIn = numPipes[numPipeIdx].pipe;
                //     commands[i].jobs[j].in = numPipes[numPipeIdx].pipe[1];
                // }
                
                //normal pipe處理輸出
                if(commands[i].jobs[j].isPipe){
                    commands[i].jobs[j].pipeOut = pipeArray[j % 2];
                    if(pipe(pipeArray[j % 2]) == -1){
                        perror("pipe failed");
                        exit(1);
                    }
                }

                //處理number pipe的輸出
                // if(commands[i].isNumPipe && j == commands[i].jobs.size() - 1){
                //     commands[i].jobs[j].pipeOut = numPipes[commands[i].number].pipe;
                //     commands[i].jobs[j].out = numPipes[commands[i].number].pipe[1];
                // }
                //ls |1 number 到這裡的時候，number此job的pipeIn還是null，我存錯存到該command的pipeIn了=>看錯
                
                pid = fork();
                if(pid < 0){
                    perror("fork failed");
                    close(pipeArray[j % 2][0]);
                    close(pipeArray[j % 2][1]);
                    j--;
                    continue;
                }else if(pid == 0){
#ifndef PIPE
                    cerr << __FILE__ << " " << __LINE__ << endl;
                    cerr << "commands[i].jobs[j].arg[0] = " << commands[i].jobs[j].arg[0] << endl;
                    if(commands[i].jobs[j].pipeIn != nullptr){
                        cerr << "commands[i].jobs[j].pipeIn = [" << commands[i].jobs[j].pipeIn[0] << ", " << commands[i].jobs[j].pipeIn[1] << "]" << endl;
                    }
                    if(commands[i].jobs[j].pipeOut != nullptr)
                        cerr << "commands[i].jobs[j].pipeOut = [" << commands[i].jobs[j].pipeOut[0] << ", " << commands[i].jobs[j].pipeOut[1] << "]" << endl;
                    // printf("commands[%d].jobs[%d] = %s, pipeIn = [%d, %d], pipeOut = [%d, %d] \n", i, j, 
                    // commands[i].jobs[j].arg[0].c_str(), commands[i].jobs[j].pipeIn[0],
                    // commands[i].jobs[j].pipeIn[1], commands[i].jobs[j].pipeOut[0], commands[i].jobs[j].pipeOut[1]);
#endif
                    
                    if(commands[i].jobs[j].pipeIn != nullptr){
                        cerr << "commands[i].jobs[j].pipeIn = [" << commands[i].jobs[j].pipeIn[0] << ", " << commands[i].jobs[j].pipeIn[1] << "]" << endl;
                        dup2(commands[i].jobs[j].pipeIn[0], STDIN_FILENO);
                        close(commands[i].jobs[j].pipeIn[0]);
                        close(commands[i].jobs[j].pipeIn[1]);
                        if(commands[i].numberIn != -1){
                            numPipes[commands[i].numberIn].finish = true;
                        }
                        
                    }

                    if(commands[i].jobs[j].pipeOut != nullptr){
                        dup2(commands[i].jobs[j].pipeOut[1], STDOUT_FILENO);
                        close(commands[i].jobs[j].pipeOut[0]);
                        close(commands[i].jobs[j].pipeOut[1]);

                    }
                    exec(commands[i].jobs[j]);
                }else if (pid > 0){//父
#ifndef CLOSEPIPE
                    cerr << __FILE__ << " " << __LINE__ << " parent 000000" << endl;
#endif
                    if (j > 0){
                        close(pipeArray[(j - 1) % 2][0]);
                        close(pipeArray[(j - 1) % 2][1]);
                        
                    }
#ifndef CLOSEPIPE
                    cerr << __FILE__ << " " << __LINE__ << " parent 11111111" << endl;
#endif
                    int numPipeIdx = returnNumPipesZeroIdx();
                    cout << "numPipeIdx = " << numPipeIdx <<" numPipes[numPipeIdx].finish = " << numPipes[numPipeIdx].finish << endl;
                    // if(j == commands[i].jobs.size() && numPipeIdx != -1){//這裡close有問題???
                    if(j == 0 && numPipeIdx != -1){
                        cerr << "closing numbered pipe" << endl;
                        // close(numPipes[numPipeIdx].pipe[0]);
                        close(numPipes[numPipeIdx].pipe[1]);//關numPipe的寫入端

                        //沒有刪掉會不會造成問題?
                        // numPipes.erase(numPipes.begin() + numPipeIdx);
                    }
                    while (wait(NULL) > 0);
                    if(numPipeIdx != -1 && numPipes[numPipeIdx].finish){
                        close(numPipes[numPipeIdx].pipe[0]);
                        close(numPipes[numPipeIdx].pipe[1]);
                        // numPipes.erase(numPipes.begin() + numPipeIdx);
                    }
#ifndef CLOSEPIPE
                    cerr << __FILE__ << " " << __LINE__ << " parent 2222222" << endl;
#endif
                    // while (wait(NULL) > 0);

#ifndef CLOSEPIPE
                    cerr << __FILE__ << " " << __LINE__ << " parent 3333333" << endl;
#endif
                    
                }
            }
        }

        // while (wait(NULL) > 0);

        cout << "% ";
        //開始處理pipe
        //需要當前的command 以及過去存起來的commands
        // handlePipe();
    }
    
    clearenv();
    return 0;
}

void run(){
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
        
    }
}
int main(){
    // signal(SIGCHLD, sigchld_handler);

    //預設PATH變數
    setenv("PATH", "bin:.", 1);

    runCommand();
    
    return 0;
}
