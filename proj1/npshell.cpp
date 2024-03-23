
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

// #define DEBUG
// int size = 256;
using namespace std;
vector<Command> numberPipes;
vector<int*> pipe_v;//讓main可以close
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
    Job job;
    vector<string> arg;
    cout << "tmp size = " << tmp.size() << endl;
    if(tmp.size() == 1){
        arg.push_back(tmp[0]);
        job.arg = arg;
        result.push_back(job);
        return result;
    }
    int pipefd[2];
    int *fdPtr = pipefd;
    int count = 0;
    
    for(int i = 0; i < tmp.size(); i++){

        if(tmp[i] == "|"){
            job.isPipe = true;
            job.arg = arg;
            if(count > 0){
                job.pipeIn = pipefd;
                job.in = pipefd[0];
            }
            if(pipe(pipefd) == -1){
                perror("pipe failed");
                exit(1);
            }
            pipe_v.push_back(fdPtr);
            job.pipeOut = pipefd;
            job.out = pipefd[1];
            
            count++;
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
        job.in = pipefd[0];
        result.push_back(job);
    }
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
    // for(int i = 0; i < numberPipes.size(); i++){
    //     Command command = numberPipes[i];
    //     if(command.number == 0){
    //         numberPipes.erase(numberPipes.begin() + i);
    //     }else{
    //         command.number = command.number - 1;
    //     }
    // }

}
vector<Command> extractCommand(vector<string> spaceSplit){
    
    vector<Command> result;//存放切完的command，以|N or ! else 一行結束分割
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
            result.push_back(command);
            numberPipes.push_back(command);

            //找到新的command就更新現有number pipe
            reviewNumPipe();
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
        result.push_back(command);
        reviewNumPipe();

    }
    
    return result;
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
void exec(Job job){
    vector<char*> args;
    for(int i = 0; i < job.arg.size(); i++){
        args.push_back(const_cast<char*>(job.arg[i].c_str()));
    }
    args.push_back(NULL);
    // if(job.in != -1){
    //     dup2(job.in, STDIN_FILENO);
    // }
    // if(job.out != -1){
        // dup2(job.out, STDOUT_FILENO);
    // }
    cout << args[0] << " " << args.data() << endl;
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
        // int pipeArray[2][2];
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
#endif
                // if(j > 0){
                //     jobs[j].in = pipeArray[(j - 1) % 2][0];
                // }
                Job job = cmd.jobs[j];
                pid = fork();
                if(pid < 0){
                    perror("fork failed");
                    exit(1);
                }else if(pid == 0){
#ifndef DEBUG
                    cerr << __FILE__ << " " << __LINE__ << endl;
                    cout << job.arg[0] << " in = " << job.in << " out = " << job.out << endl;
                    // cout << "pipeIn = " << job.pipeIn[0] << " " << job.pipeIn[1] << endl;
#endif
                    cout << "checkout pipein" << endl;
                    if(job.pipeIn != nullptr){
                        // close(job.pipeIn[1]);
                        dup2(job.pipeIn[0], STDIN_FILENO);
                    }
                    cout << "checkout pipeout" << endl;

                    if(job.pipeOut != nullptr){
                        // close(job.pipeOut[0]);
                        dup2(job.pipeOut[1], STDOUT_FILENO);
                    }
                    for(int k = 0; k < pipe_v.size(); k++){
                        close(pipe_v[k][0]);
                        close(pipe_v[k][1]);
                    }
                    cout << "exec\n";
                    exec(job);

                }
            }
            if(pid > 0){
                for(int k = 0; k < pipe_v.size(); k++){
                    close(pipe_v[k][0]);
                    close(pipe_v[k][1]);
                }
                wait(NULL);
                cout << "% ";
            }
        }
     

        //開始處理pipe
        //需要當前的command 以及過去存起來的commands
        // handlePipe();



        

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
