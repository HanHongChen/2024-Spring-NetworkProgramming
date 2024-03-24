
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
vector<Command> numberPipes;
vector<int*> pipe_v;//讓main可以close

vector<Job> extractJob(vector<string> tmp);

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

void reviewNumPipe(){
    for(int i = 0; i < numberPipes.size(); i++){
        assert(numberPipes[i].isNumPipe);
        numberPipes[i].number = numberPipes[i].number - 1;
        if(numberPipes[i].number == 0){
            //怎麼處理?
            cout << "倒數完了\n" << endl;
            numberPipes[i].print();
             
        }
    }
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
                for(int j = 0; j < numberPipes.size(); j++){
                    if(numberPipes[j].number == number){
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
                // numberPipes.push_back(command);

                //找到新的command就更新現有number pipe
                reviewNumPipe();
                //reset
                command = Command();
                tmp.clear();
            }else if(i < spaceSplit.size() - 1){ //是一般的pipe情況
                Job jobs;
                jobs.arg = tmp;
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
    vector<char*> args;
    for(int i = 0; i < job.arg.size(); i++){
        args.push_back(const_cast<char*>(job.arg[i].c_str()));
    }
    args.push_back(NULL);//execvp要求最後一個為NULL

    if(execvp(args[0], args.data())== -1){
        cerr << "Unknown command: [" << job.arg[0] << "]." << endl;
        exit(-1);
    }
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
            Command cmd = commands[i];
            // vector<Job> jobs = cmd.jobs[i];
            for(int j = 0; j < cmd.jobs.size(); j++){ 
#ifndef DEBUG
                cerr << __FILE__ << " " << __LINE__ << endl;
                cout << "Job " << j << " : ";
                for(int k = 0; k < cmd.jobs[j].arg.size(); k++){
                    cout << cmd.jobs[j].arg[k] << " ";
                }
                cout << endl;
                // cout << "pipeIn = " << cmd.jobs[j].pipeIn[0] << " " << cmd.jobs[j].pipeIn[1] << endl;
                cout << "in = " << cmd.jobs[j].in << " out = " << cmd.jobs[j].out << endl;
#endif
                if(j > 0){
                    cmd.jobs[j].pipeIn = pipeArray[(j - 1) % 2];
                    cmd.jobs[j].in = pipeArray[(j - 1) % 2][0];
                    cout << "jobs[" << j << "].pipeIn[0] = " << cmd.jobs[j].pipeIn[0] << " jobs[" << j << "].pipeIn[1] = " << cmd.jobs[j].pipeIn[1] << " in " << cmd.jobs[j].in << endl;
                    // cout << "pipeArray[" << (j - 1) % 2 << "][0] = " << pipeArray[(j - 1) % 2][0] << " pipeArray[" << (j - 1) % 2 << "][1] = " << pipeArray[(j - 1) % 2][1] << endl;
                }

                if(j != cmd.jobs.size() - 1){
                    if(pipe(pipeArray[j % 2]) == -1){
                        perror("pipe failed");
                        exit(1);
                    }
                    cmd.jobs[j].pipeOut = pipeArray[j % 2];
                    cmd.jobs[j].out = pipeArray[j % 2][1];
                    cout << "jobs[" << j << "].pipeOut[0] = " << cmd.jobs[j].pipeOut[0] << " jobs[" << j << "].pipeOut[1] = " << cmd.jobs[j].pipeOut[1] << " out " << cmd.jobs[j].out << endl;
                    // cout << "pipeArray[" << j % 2 << "][0] = " << pipeArray[j % 2][0] << " pipeArray[" << j % 2 << "][1] = " << pipeArray[j % 2][1] << endl;
                }
                // Job job = cmd.jobs[j];
                // for(int k = 0; k < pipe_v.size(); k++){
                //     cout << "close pipe 0 " << pipe_v[k][0] << " " << pipe_v[k][1] << endl;
                // }
                pid = fork();
                if(pid < 0){
                    perror("fork failed");
                    exit(1);
                }else if(pid == 0){
#ifndef DEBUG
                    cerr << __FILE__ << " " << __LINE__ << endl;
                    cout << cmd.jobs[j].arg[0] << " in = " << cmd.jobs[j].in << " out = " << cmd.jobs[j].out << endl;
                    // cout << "pipeIn = " << job.pipeIn[0] << " " << job.pipeIn[1] << endl;
#endif
                    // cout << "checkout pipein" << endl;
                    // if(job.pipeIn != nullptr){
                    //     // close(job.pipeIn[1]);
                    //     dup2(job.pipeIn[0], STDIN_FILENO);
                    // }
                    // cout << "checkout pipeout" << endl;

                    // if(job.pipeOut != nullptr){
                    //     // close(job.pipeOut[0]);
                    //     dup2(job.pipeOut[1], STDOUT_FILENO);
                    // }
                    if(cmd.jobs[j].pipeIn != nullptr){
                        close(cmd.jobs[j].pipeIn[1]);
                        dup2(cmd.jobs[j].pipeIn[0], STDIN_FILENO);
                        close(cmd.jobs[j].pipeIn[0]);
                    }

                    if(cmd.jobs[j].pipeOut != nullptr){
                        close(cmd.jobs[j].pipeOut[0]);
                        dup2(cmd.jobs[j].pipeOut[1], STDOUT_FILENO);
                        close(cmd.jobs[j].pipeOut[1]);
                    }
                    exec(cmd.jobs[j]);

                }else if (pid > 0){//父

                    if (j > 0){
                        close(pipeArray[(i - 1) % 2][0]);
                        close(pipeArray[(i - 1) % 2][1]);
                    }
                    int cpid;
                    cout << "進入parent\n";
                    if(!cmd.isNumPipe){
                        int cpid;
                        while ((cpid = wait(NULL)) > 0) {
                            cout << "子进程 " << cpid << " 结束了。\n";
                        }
                    }

                    // if(!cmd.isNumPipe){
                    //     cout << "不是numPipe\n";
                    //     cout << wait(NULL) << endl;

                    //     // while ((cpid = wait(NULL)) > 0)//如果此非numberPipe的指令，等待所有的child process結束
                    //     // {

                    //     //     cout << cpid << endl;
                    //     //     continue;
                    //     // }    
                    // }
                    // if (j == cmd.jobs.size() - 1) {
                        // waitpid(pid, NULL, 0);
                    // }
                // wait(NULL);
                    // waitChildProcesses();
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
