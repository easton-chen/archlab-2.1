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
                long long tmp=0;
                for(int i=0;i<64-length;++i)
                 tmp=tmp|(sign<<i);
                tmp=tmp<<length;
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

REG  mulh(REG r1, REG r2)
{
	unsigned char ans[128]={0};
	unsigned char tmp1[128]={0};
	unsigned char tmp2[128]={0};
	long long a,b;
	memcpy(&a,&r1,sizeof(r1));
	memcpy(&b,&r2,sizeof(r2));

	int sign_a=(r1>>63)&0x1;//符号位
	int sign_b=(r2>>63)&0x1;
	int num=0;//记录负数个数
	if(sign_a==1)
		{
			a=-a;
			num++;
		}
	if(sign_b==1)
		{
			b=-b;
			num++;
		}//全部当做unsigned相乘
	unsigned long long ua_h,ua_l,ub_h,ub_l;
	ua_h=getbit64(r1,0,31); ua_l=getbit64(r1,32,63);
	ub_h=getbit64(r2,0,31); ub_l=getbit64(r2,32,63);
	
	unsigned long long res=0;
	int j=0,acc=0,temp;
	res=ua_l*ub_l; 
	for(int i=0;i<=63;++i)
		tmp1[i]=(unsigned char)((res>>i)&1);
	
	res=ua_l*ub_h;
	for(int i=0;i<=63;++i)
		tmp2[32+i]=(unsigned char)((res>>i)&1);
	
	j=0;
	for(;j<64;++j)
		{
			temp=(int)tmp1[j]+(int)tmp2[j]+acc;
			ans[j]=temp%2;
			if(temp>=2)
				acc=1;
			else
				acc=0;
		}
	for(;j<96;++j)
		{
			temp=tmp2[j]+acc;
			ans[j]=temp%2;
			if(temp>=2)
				acc=1;
			else
				acc=0;
		}
	if(acc==1)
		ans[j++]=1;
	
	memcpy(tmp1,ans,sizeof(ans));
	memset(ans,0,sizeof(ans));
	memset(tmp2,0,sizeof(tmp2));
	
	res=ua_h*ub_l;
  for(int i=0;i<=63;++i)
    tmp2[32+i]=(unsigned char)((res>>i)&1);
	
	j=0;temp=0;acc=0;
	for(;j<96;++j)
    {
      temp=(int)tmp1[j]+(int)tmp2[j]+acc;
      ans[j]=temp%2;
      if(temp>=2)
        acc=1;
      else
        acc=0;
    }
	if(acc==1)
		ans[j++]=1;

	memcpy(tmp1,ans,sizeof(ans));
	memset(ans,0,sizeof(ans));
	memset(tmp2,0,sizeof(tmp2));

	res=ua_h*ub_h;
	for(int i=0;i<=63;++i)
		tmp2[64+i]=(unsigned char)((res>>i)&1);
	
	j=0;temp=0;acc=0;
	for(;j<128;++j)
		{
			temp=(int)tmp1[j]+tmp2[j]+acc;
			ans[j]=temp%2;
			if(temp>=2)
				acc=1;
			else
				acc=0;
		}
	
	long long  ret=0;
	for (int i=0;i<=127;++i)
		ret=ret+((int)ans[i]<<i);
	
	if(num==1)
		ret=-ret;
	REG rr;
	memcpy(&rr,&ret,sizeof(ret));
	return rr;
}

void print_memory(long long addr, int cnt, int size)
{
	char res_char = 0;
	short res_short = 0;
	unsigned res_unsigned = 0;
	long long res_long = 0;
	printf("addr = %llx, cnt = %d, size = %d:\n",addr,cnt,size);
	switch(size)
	{
		case 1:
			for (int i = 0; i < cnt; i++)
			{
				memcpy(&res_char,&memory[addr+i*size],size);
				printf("memory[0x%llx]=%x='%c'\t",addr+i*size,res_char,res_char);
				if((i+1)%4 == 0) printf("\n");
			}
			break;
		case 2:
			for (int i = 0; i < cnt; i++)
			{
				memcpy(&res_short,&memory[addr+i*size],size);
				printf("memory[0x%llx]=%x\t",addr+i*size,res_short);
				if((i+1)%4 == 0) printf("\n");
			}
			break;
		case 4:
			for (int i = 0; i < cnt; i++)
			{
				memcpy(&res_unsigned,&memory[addr+i*size],size);
				printf("memory[0x%llx]=%x\t",addr+i*size,res_unsigned);
				if((i+1)%4 == 0) printf("\n");
			}
			break;
		case 8:
			for (int i = 0; i < cnt; i++)
			{
				memcpy(&res_long,&memory[addr+i*size],size);
				printf("memory[0x%llx]=%llx\t",addr+i*size,res_long);
				if((i+1)%4 == 0) printf("\n");
			}
			break;
		default:
			printf("illegal size!\n");
			break;
	}
}