#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int 
sys_trace(void)
{
  int mask;
  if (argint(0, &mask) < 0) {
    return -1;
  }

  // int SYS_CALL_BITWISE [] = {
  //   0x2,        //fork 
  //   0x4,        //exit
  //   0x8,        //wait 
  //   0x10,       //pipe 
  //   0x20,       //read 
  //   0x40,       //kill 
  //   0x80,       //exec 
  //   0x100,      //fstat 
  //   0x200,      //chdir 
  //   0x400,      //dup 
  //   0x800,      //getpid 
  //   0x1000,     //sbrk 
  //   0x2000,     //sleep 
  //   0x4000,     //uptime 
  //   0x8000,     //open 
  //   0x10000,    //write 
  //   0x20000,    //mknod 
  //   0x40000,    //unlink 
  //   0x80000,    //link 
  //   0x100000,   //mkdir 
  //   0x200000,   //close 
  //   0x400000,   //trace
  // };

  struct proc* newProc = myproc();
  newProc ->trace_mask = mask;

  return 0;
}