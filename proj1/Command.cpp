#ifndef COMMAND
#define COMMAND
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <iostream>
#endif
using namespace std;
struct Job{
    bool isPipe = false;
    int *pipeIn = nullptr;
    int *pipeOut = nullptr;
    int in = -1;
    int out = -1;
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
    int number; //用來算還走幾個指令就要丟出去,預設-1
    vector<Job> jobs; //指令，用|N or | or !分割的
    int fd[2]; //pipe的fd
    int fd_in; //pipe的輸入
    int fd_out; //pipe的輸出
    int fd_err; //pipe的錯誤
    
    Command(){
        isErrorPipe = false;
        isNumPipe = false;
        number = -1;
        fd[0] = -1;
        fd[1] = -1;
        fd_in = -1;
        fd_out = -1;
        fd_err = -1;
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