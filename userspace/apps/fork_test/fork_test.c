#include <stdlib.h>

void main() {
    print("Fork Test Starting...\n");
    
    int pid = fork();
    
    if (pid == 0) {
        // Child
        print(" [Child] Hello! I am the child process.\n");
        print(" [Child] My PID is 0 (from my perspective).\n");
        print(" [Child] Exiting...\n");
        exit(123);
    } else if (pid > 0) {
        // Parent
        print(" [Parent] I forked a child. PID: ");
        char buf[16];
        // simple itoa
        int temp = pid; int i=0;
        if(temp==0) buf[i++]='0';
        while(temp>0) { buf[i++]=(temp%10)+'0'; temp/=10; }
        // reverse
        for(int j=0; j<i/2; j++) { char t=buf[j]; buf[j]=buf[i-1-j]; buf[i-1-j]=t; }
        buf[i]=0;
        print(buf);
        print("\n");
        
        print(" [Parent] Waiting for child...\n");
        int status = 0;
        int res = waitpid(pid, &status, 0);
        
        print(" [Parent] Child reaped. Result: ");
        // print res
        temp = res; i=0;
        if(temp==0) buf[i++]='0';
        while(temp>0) { buf[i++]=(temp%10)+'0'; temp/=10; }
        for(int j=0; j<i/2; j++) { char t=buf[j]; buf[j]=buf[i-1-j]; buf[i-1-j]=t; }
        buf[i]=0;
        print(buf);
        
        print(". Status: ");
        // print status
        temp = status; i=0;
        if(temp==0) buf[i++]='0';
        while(temp>0) { buf[i++]=(temp%10)+'0'; temp/=10; }
        for(int j=0; j<i/2; j++) { char t=buf[j]; buf[j]=buf[i-1-j]; buf[i-1-j]=t; }
        buf[i]=0;
        print(buf);
        print("\n");
    } else {
        print(" [Error] Fork failed.\n");
    }
    
    print("Test Finished.\n");
    while(1);
}
