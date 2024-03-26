#ifndef COMMAND
#define COMMAND
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <iostream>
#endif
using namespace std;
struct NumPipe{//global的存現有的number pipe
    int number;//計算還有幾步要丟出去
    int *pipe;//直接存pipe的fd
};
struct Job{
    // pid_t pid;
    bool isPipe = false;
    int *pipeIn = nullptr;
    int *pipeOut = nullptr;
    // int in = -1;
    // int out = -1;
    vector<string> arg;
    // Job(){
    //     isPipe = false;
    // }
    // ~Job(){
    //     delete[] pipe;
    // }
};
class Command {    
private:
    
public:
    bool isErrorPipe;
    bool isNumPipe; //確認是否為number pipe
    int number; //若為nuber pipe，則number為存入NumPipe的index，即NumPipes的第i個是這個的pipe
    int numberIn; //若為number pipe，則numberIn為存入NumPipe的index，即NumPipes的第i個是這個的pipe
    vector<Job> jobs; //指令，用|N or | or !分割的
    int *pipeIn = nullptr;
    int *pipeOut = nullptr;
    // int fd_in; //pipe的輸入
    // int fd_out; //pipe的輸出
    // int fd_err; //pipe的錯誤
    
    Command(){
        isErrorPipe = false;
        isNumPipe = false;
        number = -1;
        numberIn = -1;
        // fd[0] = -1;
        // fd[1] = -1;
        // fd_in = -1;
        // fd_out = -1;
        // fd_err = -1;
    }

    void print(){
        printf("isErrorPipe: %d, isNumPipe = %d, number = %d\n", isErrorPipe, isNumPipe, number);
        // printf("fd[0] = %d, fd[1] = %d, fd_in = %d, fd_out = %d, fd_err = %d\n", fd[0], fd[1], fd_in, fd_out, fd_err);
        // for(int i = 0; i < jobs.size(); i++){
        //     Job job = jobs[i];
        //     printf("Job %d : ", i);
        //     printf("isPipe = %d ", job.isPipe);
        //     // cout << "isPipe = " << true << " ";
        //     printf("arg: ");
        //     for(int j = 0; j < job.arg.size(); j++){
        //         printf("%s ", job.arg[j].c_str());
        //     }
        //     printf("\n");
        // }
    }
    
    // void printJobs(){
    //     for(int i = 0; i < jobs.size(); i++){
    //         printf("%s\n", jobs[i].c_str());
    //     }
    // }
};