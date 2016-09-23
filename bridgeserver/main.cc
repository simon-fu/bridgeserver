

// command:
// interface:
// 1. prepareSession(SessionID, localInfo)
// 2. startSession(SessionID, peerInfo)
// 3. joinSession(SessionID, peerInfo)
// 4. relaySession(SessionID, webrtcAnswer)	


// data:  relay data between kurento and mobile

#include "applog.h"
#include "AppServer.h"

#include <unistd.h>
#include <stdio.h>
#include <execinfo.h>
#include <signal.h> // SIGINT

static FILE * g_log_fp = NULL;

#define dbgv(...) do{  if(!g_log_fp) break; fprintf(g_log_fp, "<main>[D] " __VA_ARGS__); fprintf(g_log_fp, "\n"); fflush(g_log_fp); }while(0)
#define dbgi(...) do{  if(!g_log_fp) break; fprintf(g_log_fp, "<main>[I] " __VA_ARGS__); fprintf(g_log_fp, "\n"); fflush(g_log_fp); }while(0)
#define dbge(...) do{  if(!g_log_fp) break; fprintf(g_log_fp, "<main>[E] " __VA_ARGS__); fprintf(g_log_fp, "\n"); fflush(g_log_fp); }while(0)



static
void open_stack_log(const char * exeNamePath){
    if(g_log_fp){
        return;
    }
    
    time_t nowtime;
    struct tm *timeinfo;
    nowtime = time(NULL);
    timeinfo = localtime(&nowtime);
    
    char filename[1024];
    sprintf(filename, "%s.%04d%02d%02d-%02d%02d%02d.crash"
            , exeNamePath
            , timeinfo->tm_year+1900
            , timeinfo->tm_mon+1
            , timeinfo->tm_mday
            , timeinfo->tm_hour
            , timeinfo->tm_min
            , timeinfo->tm_sec
            );
    g_log_fp = fopen(filename, "wb");
    if(!g_log_fp) {
        printf("fail to open crash file %s\n", filename);
        return;
    }
    printf("successfully open crash file %s\n", filename);
}

static
void close_stack_log(){
    if(g_log_fp){
        fclose(g_log_fp);
        g_log_fp = NULL;
        printf("closed crash file\n");
    }
}

static
void print_call_stack()
{
    void* callstack[128];
    int i, frames = backtrace(callstack, 128);
    char** strs = backtrace_symbols(callstack, frames);
    dbgi("callstack frames %d", frames);
    for (i = 0; i < frames; ++i) {
        //		printf("%s\n", strs[i]);
        dbgi("callstack %d : %s\n", i, strs[i]);
//        LOG_ERROR("callstack " << i << " : " << strs[i]);
    }
    free(strs);
}

static int getResultFromSystemCall(const char* pCmd, char* pResult, int size)
{
    int fd[2];
    if(pipe(fd))   {
        dbgi("pipe error!");
        return -1;
    }
    
    //prevent content in stdout affect result
    fflush(stdout);
    
    //hide stdout
    int bak_fd = dup(STDOUT_FILENO);
    int new_fd = dup2(fd[1], STDOUT_FILENO);
    
    //the output of `pCmd` is write into fd[1]
    int ret = system(pCmd);
    int bytes = read(fd[0], pResult, size-1);
    if(bytes < 0){
        dbgi("read cmd result error!");
        return -1;
    }else {
        // remove \r \n
        while( bytes > 0){
            char c = pResult[bytes-1];
            if(c == '\r' || c == '\n'){
                bytes--;
            }else{
                break;
            }
        }
    }
    pResult[bytes] = 0;
    
    //resume stdout
    dup2(bak_fd, new_fd);
    
    return ret;
}


static int addr2line(char const * const program_name, void const * const addr)
{
    char addr2line_cmd[512] = {0};
    
    /* have addr2line map the address to the relent line in the code */
#ifdef __APPLE__
    /* apple does things differently... */
    sprintf(addr2line_cmd,"atos -o %.256s %p", program_name, addr);
#else
    sprintf(addr2line_cmd,"addr2line -f -p -e %.256s %p", program_name, addr);
#endif
    
    /* This will print a nicely formatted string specifying the
     function and source line of the address */
//    printf("exec: %s\n", addr2line_cmd);
//    return system(addr2line_cmd);
    
    static char msg[2048];
    int ret = getResultFromSystemCall(addr2line_cmd, msg, 2048);
    dbgi("=> %s", msg);
    return ret;
}

#define MAX_STACK_FRAMES 64
static void *stack_traces[MAX_STACK_FRAMES];
static char const * icky_global_program_name;
static void posix_print_stack_trace()
{
    int i, trace_size = 0;
    char **messages = (char **)NULL;
    
    trace_size = backtrace(stack_traces, MAX_STACK_FRAMES);
    messages = backtrace_symbols(stack_traces, trace_size);
    
    /* skip the first couple stack frames (as they are this function and
     our handler) and also skip the last frame as it's (always?) junk. */
    // for (i = 3; i < (trace_size - 1); ++i)
    // we'll use this for now so you can see what's going on
    for (i = 0; i < trace_size; ++i)
    {
        if (addr2line(icky_global_program_name, stack_traces[i]) != 0)
        {
            dbgi("  error determining line # for: %s\n", messages[i]);
//            LOG_ERROR("  error determining line # for:  " << messages[i]);
        }
        
    }
    if (messages) { free(messages); }
}

static void SignalHandle(int sig){
    open_stack_log(icky_global_program_name);
    print_call_stack();
//    LOG_ERROR("============================" );
    dbgi("============================\n");
    posix_print_stack_trace();
    close_stack_log();
    exit(1);
}

//static
//void crash_me(){
//	char *p = 0;
//	*p = 0;
//}

AppServer g_app;

int main(int argc, char *argv[])
{
    icky_global_program_name = argv[0];
    signal(SIGSEGV, SignalHandle);
//    crash_me();
    
	//使用damon，在后台运行
	// int iNoClose = 0;
	//if (daemon(1, iNoClose) == -1) 
	//{
	//	perror("daemon() fail.");
	//	return -1;
	//}
			
    if(!g_app.startAndLoop()){
        return -1;
    }

    return 0;
}
