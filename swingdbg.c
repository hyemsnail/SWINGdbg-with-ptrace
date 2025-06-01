#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <unistd.h>
#include <errno.h>

void print_registers(struct user_regs_struct regs) { //레지스터 출력 함수 부분분
    printf("[+] Registers:\n");
    printf("RAX: 0x%llx\n", regs.rax);
    printf("RBX: 0x%llx\n", regs.rbx);
    printf("RCX: 0x%llx\n", regs.rcx);
    printf("RDX: 0x%llx\n", regs.rdx);
    printf("RSI: 0x%llx\n", regs.rsi);
    printf("RDI: 0x%llx\n", regs.rdi);
    printf("RSP: 0x%llx\n", regs.rsp);
    printf("RBP: 0x%llx\n", regs.rbp);
}

//명령어 메뉴를 처리하는 함수
void commandmenu(pid_t pid, char *command) {
    if (strcmp(command, "ni") == 0) { //ni는 next instruction
        struct user_regs_struct regs;

        //현재 레지스터 상태를 조회함
        ptrace(PTRACE_GETREGS, pid, NULL, &regs);
        unsigned long long rip = regs.rip; //명령어 포인터 RIP 저장

        //현재 RIP 위치에서 opcode 명령어 읽기
        errno = 0;
        unsigned long instr = ptrace(PTRACE_PEEKTEXT, pid, rip, NULL);
        if (errno != 0) {
            perror("ptrace PEEKTEXT");
        }

        //RIP와 opcode 출력
        printf("[+] RIP = 0x%llx | instr = 0x%lx\n", rip, instr);
        print_registers(regs); //레지스터 정보를 출력

        //한 줄 실행하기
        ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL);
        waitpid(pid, NULL, 0);
    }

    else if (strcmp(command, "c") == 0) {
        //자식 프로세스를 계속 실행하기
        ptrace(PTRACE_CONT, pid, NULL, NULL);
        waitpid(pid, NULL, 0); //중단될 때까지 대기함
    }

    else {
        //모르는 명령어
        printf("[!] Unknown command: %s\n", command);
    }
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        //딱히 인자가 없으면 사용법만 한줄 출력
        printf("Usage: ./swingdbg <target program>\n");
        return -1;
    }

    int pid = fork(); //부모 - 자식 프로세스 나누기
    char command[100]; //사용자가 입력한 명령 저장

    if (pid == 0) {
        // 자식 프로세스 -> 디버깅 대상 프로그램
        if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) {
            //디버깅 허용 요청 실패
            perror("ptrace TRACEME");
            return -1;
        }
        execl(argv[1], argv[1], NULL); //디버깅 대상을 실행
    }

    else if (pid > 0) {
        // 부모 프로세스 -> 디버거 역할
        int stat;
        wait(&stat);  // 자식 프로세스가 중단될 때까지 대기함

        // 디버깅 루프 -> 자식이 중단될 때마다 반복함
        while (WIFSTOPPED(stat)) {
            printf("\nSWINGdbg >>> "); //프롬포트에 나오는거
            fgets(command, sizeof(command), stdin); //명령어 여기에 입력
            command[strcspn(command, "\n")] = '\0'; //개행 문자는 제거함
            commandmenu(pid, command); //입력한 명령을 처리함
        }
    }

    else {
        //fork 실패
        perror("fork error");
        return -1;
    }

    return 0;
}
