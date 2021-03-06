#include<iostream>
#include<stdio.h>
#include<math.h>
//#include <io.h>
#include <pthread.h>
#include<time.h>
#include<stdlib.h>
#include<string.h>
#include"Reg_def.h"

#define OP_JAL 111
#define OP_R 51

#define F3_ADD 0
#define F3_MUL 0

#define F7_MSE 1
#define F7_ADD 0

#define OP_I 19
#define F3_ADDI 0

#define OP_SW 35
#define F3_SB 0

#define OP_LW 3
#define F3_LB 0

#define OP_BEQ 99
#define F3_BEQ 0

#define OP_IW 27
#define F3_ADDIW 0

#define OP_JALR 103

#define OP_SCALL 115
#define F3_SCALL 0
#define F7_SCALL 0

#define OP_AUIPC 23

#define OP_LUI 55

#define MAX 100000000

//主存
unsigned char memory[MAX]={0};
//寄存器堆
REG reg[32]={0};
//PC
long long PC=0;


//各个指令解析段
unsigned int OP=0;
unsigned int fuc3=0,fuc7=0;
int shamt=0;
int rs=0,rt=0,rd=0;
unsigned int imm12=0;
unsigned int imm20=0;
unsigned int imm7=0;
unsigned int imm5=0;

/*
typedef struct decoder{
	int OP;
	int fuc3;
	int fuc7;
	int rd;
	int rs;
	int rt;
	int I_imm;
	int S_imm;
	int SB_imm;
	int U_imm;
	int UJ_imm;
} Decoder;
*/


//加载内存
void load_memory();

void simulate();

void IF();

void ID();

void EX();

void MEM();

void WB();


//符号扩展
long long ext_signed(unsigned int src,int bit);

//获取指定位
//unsigned int getbit(int s,int e);

unsigned int getbit(unsigned int inst,int s,int e)
{
	unsigned int mask=0; 
	for(int i=31-e;i<=31-s;++i)
		mask |= (1<<i);
	return (inst & mask)>>(31-e);
}
unsigned long long getbit64(unsigned long long inst,int s,int e)
{
	unsigned long long mask=0; 
	for(int i=63-e;i<=63-s;++i)
		mask |= (1<<i);
	return (inst & mask)>>(63-e);
}
/*
unsigned int getbit(unsigned inst,int s,int e)
{
	unsigned int part = (inst >> (31-e)) & ((1 << (e-s+2)) - 1);
	return part;
}
*/

long long ext_signed(unsigned int src,int bit,int length)
{
        if(bit==0)
        return (long long)src;
        else
        {
                int sign=(src&(1<<(length-1)))>>(length-1);
                //printf("sign:%d\n",sign);
                long long tmp=0;
                for(int i=0;i<64-length;++i)
                 tmp=tmp|(sign<<i);
                //printf("tmp:%llx\n",tmp);
                tmp=tmp<<length;
                //printf("tmp:%llx\n",tmp);
                return tmp+(long long)src;
        }
}

void print_REG()
{
	printf("The register files are as follows:\n");
	printf("------------------------------------\n");
	for(int i = 0; i < 32; i++)
	{
		printf("REG[%d] = %llx\n", i, reg[i]);
	}
	printf("------------------------------------\n");
}