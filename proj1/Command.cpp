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
    int step;//計算還有幾步要丟出去
    int *pipe;//直接存pipe的fd
    vector<pid_t> pids;//存使用到此NP的job id
};
struct Job{
    // pid_t pid;
    bool isPipe = false;
    int *pipeIn = nullptr;
    int *pipeOut = nullptr;
    bool hasPipeIn = false;
    bool hasPipeOut = false;
    vector<string> arg;
};

// pipe_t pipe = NULL;
// for (command: commands) {
//     if (pipe != NULL) {
//         addRedirect(command, pipe.read, STDIN_FILENO);
//         pipe = NULL;
//     }

//     if (command.haspipeout()) {
//         pipe = create_pipe();
//         addRedirect(command, pipe.write, STDOUT_FILENO);
//     }
// }

class Command {    
private:
    
public:
    bool isErrorPipe = false;
    bool isNumPipe = false; //確認是否為number pipe
    int number = -1;
    int numPipesOutIdx = -1; //若為nuber pipe，則number為存入NumPipe的index，即NumPipes的第i個是這個的pipe
    int numPipesInIdx = -1; //若為number pipe，則numberIn為存入NumPipe的index，即NumPipes的第i個是這個的pipe
    vector<Job> jobs; //指令，用|N or | or !分割的
    int *pipeIn = nullptr;
    int *pipeOut = nullptr;
    
};