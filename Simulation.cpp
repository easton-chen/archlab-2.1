#include "Simulation.h"
using namespace std;

extern void read_elf();
extern unsigned long long cadr; //代码段在解释文件中的偏移地址
extern unsigned long long csize; //代码段的长度
extern unsigned long long vcadr; //代码段在内存中的虚拟地址	

extern unsigned long long dadr; //数据段在解释文件中的偏移地址
extern unsigned long long dsize; //数据段的长度
extern unsigned long long bsize; //.bss和.sbss段的长度
extern unsigned long long vdadr; //数据段在内存中的虚拟地址	
extern unsigned long long gp; //全局数据段在内存的地址

extern unsigned long long madr; //main函数在内存中地址
extern unsigned long long msize;
extern unsigned long long endPC; //程序结束时的PC
extern unsigned long long entry; //程序的入口地址
extern FILE *file;


//指令运行数
long long inst_num=0;

//系统调用退出指示
int exit_flag=0;

//加载代码段
//初始化PC
void load_memory()
{
	memset(memory, 0, sizeof(memory));

	//映射.text段，cadr和csize在Read_Elf.cpp中维护
	fseek(file,cadr,SEEK_SET);
	fread(&memory[vcadr],1,csize,file);
	
	//for debugging
	printf("cadr = %llx\n", cadr);
	printf("csize = %llx\n", csize);
	printf("vcadr = %llx\n", vcadr);
	for(int i = 0; i < csize; i++)
	{
		printf("%02x",memory[i+vcadr]);
		if((i+1)%4 == 0) printf(" ");
		if((i+1)%16 == 0) printf("\n");
	}
	printf("\n.text finished!\n\n\n");
	
	//映射.data段
	fseek(file,dadr,SEEK_SET);
	//因为.bss段在elf文件中不占用空间，所以实际映射的是.data段的大小
	//由于之前的初始化，映射后内存中数据区的后bsize个字节仍是0
	fread(&memory[vdadr],1,dsize-bsize,file);	
	
	//for debugging
	printf("dadr = %llx\n", dadr);
	printf("dsize = %llx\n", dsize);
	printf("vdadr = %llx\n", vdadr);
	for(int i = 0; i < dsize; i++)
	{
		printf("%02x",memory[i+vdadr]);
		if((i+1)%4 == 0) printf(" ");
		if((i+1)%16==0) printf("\n");
	}
	printf("\n.data finished!\n\n\n");

	//维护一些值。
	entry = vcadr;
	endPC=madr+msize-3;
	//for debugging
	printf("entry = %llx\n", entry);
	printf("gp = %llx\n", gp);
	printf("madr = %llx\n", madr);


	fclose(file);
}

int main()
{
	//解析elf文件
	read_elf();
	
	//加载内存
	load_memory();

	//设置入口地址
	PC=madr;
	
	//设置全局数据段地址寄存器
	reg[3]=gp;
	
	reg[2]=MAX/2;//栈基址 （sp寄存器）

	simulate();

	cout <<"simulate over!"<<endl;

	return 0;
}

void simulate()
{
	//结束PC的设置
	//int end=(int)endPC/4-1;
	while(PC!=endPC)
	{
		printf("=======================================================\n");
		printf("instruction num: %d\n",inst_num);
		//Decoder* decoder = (Decoder*) malloc(sizeof(Decoder));
		//memset(decoder, 0, sizeof(Decoder));
		//运行
		IF();
		ID();
		EX();
		MEM();
		WB();

		//更新中间寄存器
		//IF_ID=IF_ID_old;
		//ID_EX=ID_EX_old;
		//EX_MEM=EX_MEM_old;
		//MEM_WB=MEM_WB_old;

        if(exit_flag==1)
            break;

        reg[0]=0;//一直为零

        //free(decoder);
        inst_num++;
        print_REG();
        printf("=======================================================\n\n\n");
	}
}


//取指令
void IF()
{
	printf("-------------IF--------------\n");
	//write IF_ID_old
	//IF_ID_old.inst=memory[PC];
	
	//For Debugging
	printf("PC:%llx\n",PC);
	//For Debugging
	memcpy(&IF_ID.inst,memory+PC,4);
	IF_ID.PC=PC;
	printf("instruction:%08x\n",IF_ID.inst);
	printf("IF over!\n");
	print_IFID();
	PC=PC+4;
}

//译码
void ID()
{
	printf("-------------ID--------------\n");
	//Read IF_ID
	unsigned int inst=IF_ID.inst;

	int EXTop=0;
	unsigned int Imm_length=0;//立即数的长度

	char RegDst,ALUop,ALUSrc;
	char Branch,MemRead,MemWrite;
	char RegWrite,MemtoReg;

	unsigned int OP=getbit(inst,25,31);
	printf("OP:  %02x \n",OP);
	unsigned int rs = 0;
	unsigned int rt = 0;
	unsigned int rd = 0;
	unsigned int fuc3 = 0;
	unsigned int fuc7 = 0;
	unsigned int Imm = 0;

	//rd=getbit(inst,7,11);
	//fuc3=getbit(inst,0,0);
	//....


	if(OP==OP_R)//R 0x33
	{
		printf("R-TYPE:\n");
		fuc7=getbit(inst,0,6);
		rt=getbit(inst,7,11);
		rs=getbit(inst,12,16);
		fuc3=getbit(inst,17,19);
		rd=getbit(inst,20,24);

		Imm_length=0;
		EXTop=0;
		RegDst=1;
		ALUop=0;
		ALUSrc=0;
		Branch=0;
		MemRead=0;
		MemWrite=0;
		RegWrite=1;
		MemtoReg=0;

		switch(fuc3){
			case 0:
				switch(fuc7){
					case 0x00:
						ALUop=1;// R[rd] ← R[rs1] + R[rs2]
						break;
					case 0x01:
						ALUop=2;//R[rd] ← (R[rs1] * R[rs2])[31:0]
						break;
					case 0x20:
						ALUop=3;//R[rd] ← R[rs1] - R[rs2]
						break;
					default: printf("Illegal Instruction\n"); break;
				}
				break;
			case 1:
				if(fuc7 == 0x00) ALUop=4;//R[rd] ← R[rs1] << R[rs2]
				else if(fuc7==0x01) ALUop=5;//R[rd] ← (R[rs1] * R[rs2])[63:32]
				else printf("Illegal Instruction\n");
				break;
			case 2:
				if(fuc7==0x00) ALUop=6;//R[rd] ← (R[rs1] < R[rs2]) ? 1 : 0
				else printf("Illegal Instruction\n");
				break;
			case 4:
				if(fuc7 == 0x00) ALUop=7;//R[rd] ← R[rs1] ^ R[rs2]
				else if(fuc7==0x01) ALUop=8;//R[rd] ← R[rs1] / R[rs2]
				else printf("Illegal Instruction\n");
				break;
			case 5:
				if(fuc7 == 0x00||fuc7==0x02) ALUop=9;//R[rd] ← R[rs1] >> R[rs2]
				else printf("Illegal Instruction\n");
				break;
			case 6:
				if(fuc7 == 0x00) ALUop=10;//R[rd] ← R[rs1] | R[rs2]
				else if(fuc7==0x01) ALUop=11;//R[rd] ← R[rs1] % R[rs2]
				else printf("Illegal Instruction\n");
				break;
			case 7:
				if(fuc7==0x00) ALUop=12;//R[rd] ← R[rs1] & R[rs2]
				else printf("Illegal Instruction\n");
				break;
			default: 
				printf("Illegal Instruction\n");
				break;
		}
	}
	else if(OP==OP_I)//I 0x13
	{
  		printf("I-TYPE:\n");
  		Imm=getbit(inst,0,11);
    	rs=getbit(inst,12,16);
    	fuc3=getbit(inst,17,19);
    	rd=getbit(inst,20,24);
 		fuc7=getbit(inst,0,6);
    	Imm_length=12;
    	EXTop=0;
		RegDst=1;
		ALUop=0;
		ALUSrc=1;
		Branch=0;
		MemRead=0;
		MemWrite=0;
		RegWrite=1;
		MemtoReg=0;

		switch(fuc3)
		{
			case 0:
				ALUop=17;//R[rd] ← R[rs1] + imm
				EXTop=1;
				break;
			case 1:
				if(fuc7==0x00)
				ALUop=18;//R[rd] ← R[rs1] << imm
				else printf("Illegal Instruction\n");
				break;
			case 2:
				ALUop=19;//R[rd] ← (R[rs1] < imm) ? 1 : 0
				break;
			case 4:
				ALUop=20;//R[rd] ← R[rs1] ^ imm
				break;
			case 5:
				if(fuc7 == 0x00||fuc7==0x20) ALUop=21;//R[rd] ← R[rs1] >> imm
				else printf("Illegal Instruction\n");
				break;
			case 6:
				
				ALUop=22;//R[rd] ← R[rs1] | imm
				break;
			case 7:		
				ALUop=23;//R[rd] ← R[rs1] & imm
				break;
			default:
				printf("Illegal Instruction\n"); 
				break;
		}
    }
    else if(OP==OP_SW)//S 0x23 
    {
    	printf("S-TYPE:\n");
    	Imm=(getbit(inst,0,6)<<5) + getbit(inst,20,24);
      	rt=getbit(inst,7,11);
      	rs=getbit(inst,12,16);
      	fuc3=getbit(inst,17,19);
  		Imm_length=12;
  		EXTop=1;
		RegDst=0;
		ALUop=0;
		ALUSrc=1;
		Branch=0;
		MemRead=0;
		MemWrite=1;
		RegWrite=0;
		MemtoReg=0;

		switch(fuc3)
		{
			case 0:	
				ALUop=27;//Mem(R[rs1] + offset) ← R[rs2][7:0]
				break;
			case 1:
				ALUop=28;//Mem(R[rs1] + offset) ← R[rs2][15:0]
				break;
			case 2:
				ALUop=29;//Mem(R[rs1] + offset) ← R[rs2][31:0]
				break;
			case 3:
				ALUop=30;//Mem(R[rs1] + offset) ← R[rs2][63:0]
				break;
			default: printf("Illegal Instruction\n"); break;
		}
    }
    else if(OP==OP_LW)//I 0x03  The LW instruction loads a 32-bit value from memory and sign-extends this to 64 bits before storing it in register rd for RV64I.
    {
    	Imm=getbit(inst,0,11);
    	rs=getbit(inst,12,16);
    	fuc3=getbit(inst,17,19);
    	rd=getbit(inst,20,24);
	   	Imm_length=12;
	   	EXTop=1;//sign-extend
		RegDst=1;
		ALUop=0;
		ALUSrc=1;
		Branch=0;
		MemRead=1;
		MemWrite=0;
		RegWrite=1;
		MemtoReg=1;

		switch(fuc3)
		{
			case 0:
				ALUop=13;//R[rd] ← SignExt(Mem(R[rs1] + offset, byte))
				break;
			case 1:
				ALUop=14;//R[rd] ← SignExt(Mem(R[rs1] + offset, half))
				break;
			case 2:
				ALUop=15;//R[rd] ← Mem(R[rs1] + offset, word)
				break;
			case 3:
				ALUop=16;//R[rd] ← Mem(R[rs1] + offset, doubleword)
				break;
			default: printf("Illegal Instruction\n");break;
		}
    }
    else if(OP==OP_BEQ)//SB  0x63
    {
   		Imm=(getbit(inst,0,0)<<12) + (getbit(inst,24,24)<<11) + (getbit(inst,1,6)<<5) + (getbit(inst,20,23)<<1);
    	rt=getbit(inst,7,11);
    	rs=getbit(inst,12,16);
    	fuc3=getbit(inst,17,19);
    	Imm_length=12;
    	EXTop=0;
		RegDst=0;
		ALUop=0;
		ALUSrc=0;
		Branch=1;
		MemRead=0;
		MemWrite=0;
		RegWrite=0;
		MemtoReg=0;

		switch(fuc3)
		{
			case 0:
				ALUop=31;//if(R[rs1] == R[rs2]) PC ← PC + {offset, 1b'0}
				break;
			case 1:
				ALUop=32;//if(R[rs1] != R[rs2]) PC ← PC + {offset, 1b'0}
				break;
			case 4:
				ALUop=33;//if(R[rs1] < R[rs2]) PC ← PC + {offset, 1b'0}
				break;
			case 5:
				ALUop=34;//if(R[rs1] >= R[rs2]) PC ← PC + {offset, 1b'0}
				break;
			default: printf("Illegal Instruction\n");break;
		}
    }
    else if(OP==OP_JAL)//UJ 0x6f R[rd] ← PC + 4 PC ← PC + {imm, 1b'0}
    {
    	Imm=(getbit(inst,0,0)<<20) + (getbit(inst,12,19)<<12) + (getbit(inst,11,11)<<11) + (getbit(inst,1,10)<<1);
    	rd=getbit(inst,20,24);
    	Imm_length=20;
    	EXTop=1;
		RegDst=1;
		ALUop=37;
		ALUSrc=1;
		Branch=1;
		MemRead=0;
		MemWrite=0;
		RegWrite=1;
		MemtoReg=0;
    }
    else if(OP==OP_IW)//I 0x1B R[rd] ← SignExt(R[rs1](31:0) + imm)
    {
		Imm=getbit(inst,0,11);
    	rs=getbit(inst,12,16);
    	fuc3=getbit(inst,17,19);
    	rd=getbit(inst,20,24);
		if(fuc3==0x00)
    	{
			Imm_length=12;
			EXTop=1;
			RegDst=1;
			ALUop=24;
			ALUSrc=1;
			Branch=0;
			MemRead=0;
			MemWrite=0;
			RegWrite=1;
			MemtoReg=0;
   	 	}
		else printf("Illegal Instruction\n");	
    }
    else if(OP==OP_JALR)//I 0x67 R[rd] ← PC + 4  PC ← R[rs1] + {imm, 1b'0}
    {
    	Imm=getbit(inst,0,11);
    	rs=getbit(inst,12,16);
    	fuc3=getbit(inst,17,19);
    	rd=getbit(inst,20,24);
		if(fuc3==0x00)
    	{
			Imm_length=12;
			EXTop=1;
			RegDst=1;
			ALUop=25;
			ALUSrc=1;
			Branch=1;
			MemRead=0;
			MemWrite=0;
			RegWrite=1;
			MemtoReg=0;
		}
		else printf("Illegal Instruction\n");
    }
    else if(OP==OP_SCALL)//I 0x73 (Transfers control to operating system)
    {
    	Imm=getbit(inst,0,11);
    	rs=getbit(inst,12,16);
    	fuc3=getbit(inst,17,19);
    	rd=getbit(inst,20,24);
		fuc7=getbit(inst,0,6);
		if(fuc3==0x0&&fuc7==0x0)
		{
    		Imm_length=12;
    		EXTop=0;
			RegDst=0;
			ALUop=26;
			ALUSrc=0;
			Branch=0;
			MemRead=0;
			MemWrite=0;
			RegWrite=0;
			MemtoReg=0;
		}
		else printf("Illegal Instruction\n");
    }
    else if(OP==OP_AUIPC)//U 0x17
    {
    	Imm=getbit(inst,0,19)<<12;
    	rd=getbit(inst,20,24);
    	Imm_length=32;
    	EXTop=1;
		RegDst=1;
		ALUop=35;
		ALUSrc=1;
		Branch=0;
		MemRead=0;
		MemWrite=0;
		RegWrite=1;
		MemtoReg=0;
    }
    else if(OP==OP_LUI)//U 0x37
    {
    	Imm=getbit(inst,0,19)<<12;
    	rd=getbit(inst,20,24);
    	Imm_length=32;
    	EXTop=1;
		RegDst=1;
		ALUop=36;
		ALUSrc=1;
		Branch=0;
		MemRead=0;
		MemWrite=0;
		RegWrite=1;
		MemtoReg=0;
    }

	//write ID_EX_old
	ID_EX.Rd=rd;
	ID_EX.Rt=rt;
	ID_EX.Reg_Rs=reg[rs];
	ID_EX.Reg_Rt=reg[rt];

	ID_EX.PC=IF_ID.PC;
	//printf("EXTop == %d\n",EXTop);
	//printf("Imm_length = %d\n",Imm_length);
	ID_EX.Imm=ext_signed(Imm,EXTop,Imm_length);

	ID_EX.Ctrl_EX_ALUSrc=ALUSrc;
	ID_EX.Ctrl_EX_ALUOp=ALUop;
	ID_EX.Ctrl_EX_RegDst=RegDst;

	ID_EX.Ctrl_M_Branch=Branch;
	ID_EX.Ctrl_M_MemWrite=MemWrite;
	ID_EX.Ctrl_M_MemRead=MemRead;

	ID_EX.Ctrl_WB_RegWrite=RegWrite;
	ID_EX.Ctrl_WB_MemtoReg=MemtoReg;
	
	
	//For Debugging
	printf("Inst: \n");
	switch (ALUop)
	{
		case 1: printf("R[rd] ← R[rs1] + R[rs2]\n"); break;
		case 2: printf("R[rd] ← (R[rs1] * R[rs2])[31:0]\n");break;
		case 3: printf("R[rd] ← R[rs1] - R[rs2]\n");break;
		case 4: printf("R[rd] ← R[rs1] << R[rs2]\n");break;
		case 5: printf("R[rd] ← (R[rs1] * R[rs2])[63:32]\n");break;
		case 6: printf("R[rd] ← (R[rs1] < R[rs2]) ? 1 : 0\n");break;
		case 7: printf("R[rd] ← R[rs1] ^ R[rs2]\n");break;
		case 8: printf("R[rd] ← R[rs1] / R[rs2]\n");break;
		case 9: printf("R[rd] ← R[rs1] >> R[rs2]\n");break;
		case 10: printf("R[rd] ← R[rs1] | R[rs2]\n");break;
		case 11: printf("R[rd] ← (R[rs1] %% R[rs2]\n");break;
		case 12: printf("R[rd] ← R[rs1] & R[rs2]\n");break;
		case 13: printf("R[rd] ← SignExt(Mem(R[rs1] + offset, byte))\n");break;
		case 14: printf("R[rd] ← SignExt(Mem(R[rs1] + offset, half))\n");break;
		case 15: printf("R[rd] ← Mem(R[rs1] + offset, word)\n");break;
		case 16: printf("R[rd] ← Mem(R[rs1] + offset, doubleword)\n");break;
		case 17: printf("R[rd] ← R[rs1] + imm\n");break;
		case 18: printf("[rd] ← R[rs1] << imm\n");break;
		case 19: printf("R[rd] ← (R[rs1] < imm) ? 1 : 0\n");break;
		case 20: printf("R[rd] ← R[rs1] ^ imm\n");break;
		case 21: printf("R[rd] ← R[rs1] >> imm\n");break;
		case 22: printf("R[rd] ← R[rs1] | imm\n");break;
		case 23: printf("R[rd] ← R[rs1] & imm\n");break;
		case 24: printf("R[rd] ← SignExt(R[rs1](31:0) + imm)\n");break;
		case 25: printf("R[rd] ← PC + 4\nPC ← R[rs1] + {imm, 1b'0}\n");break;
		case 26: printf("(Transfers control to operating system)\n");break;
		case 27: printf("Mem(R[rs1] + offset) ← R[rs2][7:0]\n");break;
		case 28: printf("Mem(R[rs1] + offset) ← R[rs2][15:0]\n");break;
		case 29: printf("Mem(R[rs1] + offset) ← R[rs2][31:0]\n");break;
		case 30: printf("Mem(R[rs1] + offset) ← R[rs2][63:0]\n");break;
		case 31: printf("if(R[rs1] == R[rs2])\n");break;
		case 32: printf("if(R[rs1] != R[rs2])\n");break;
		case 33: printf("if(R[rs1] < R[rs2])\n");break;
		case 34: printf("if(R[rs1] >= R[rs2])\n");break;
		case 35: printf("R[rd] ← PC + {offset, 12'b0}\n");break;
		case 36: printf("R[rd] ← {offset, 12'b0}\n");break;
		case 37: printf("R[rd] ← PC + 4 PC ← PC + {imm, 1b'0}\n");break;
		default: printf("Illegal Instruction\n");break;

	}

	printf("ID over!\n");
	print_IDEX();
}

//执行
void EX()
{
	printf("-------------EX--------------\n");
	unsigned int rd=ID_EX.Rd;
	unsigned int rt=ID_EX.Rt;
	long long Imm=ID_EX.Imm;

	REG Rs=ID_EX.Reg_Rs;
	REG Rt=ID_EX.Reg_Rt;

	char ALUSrc=ID_EX.Ctrl_EX_ALUSrc;
	char ALUop=ID_EX.Ctrl_EX_ALUOp;
	char RegDst=ID_EX.Ctrl_EX_RegDst;

	//read ID_EX
	int temp_PC=ID_EX.PC;


	//Branch PC calulate
	//...

	//choose ALU input number
	//...

	//alu calculate
	int Zero;
	REG ALUout;
	switch(ALUop){
		case 1:
			ALUout=Rs+Rt;
			break;
		case 2:
			ALUout=(Rs*Rt)&0xffffffff;
			break;
		case 3:
			ALUout=Rs-Rt;
			break;
		case 4:
			ALUout=Rs<<Rt;
			break;
		case 5:
			ALUout=((Rs*Rt)>>32)&0xffffffff;
			break;
		case 6:
			if(Rs<Rt) ALUout=1;
			else ALUout=0;
			break;
		case 7:
			ALUout=Rs^Rt;
			break;
		case 8:
			ALUout=Rs/Rt;
			break;
		case 9:
			ALUout=Rs>>Rt;
			break;
		case 10:
			ALUout=Rs|Rt;
			break;
		case 11:
			ALUout=Rs%Rt;
			break;
		case 12:
			ALUout=Rs&Rt;
			break;
		case 13:
			ALUout=Rs+Imm;
			break;
		case 14:
			ALUout=Rs+Imm;
			break;
		case 15:
			ALUout=Rs+Imm;
			break;
		case 16:
			ALUout=Rs+Imm;
			break;
		case 17:
			ALUout=Rs+Imm;
			break;
		case 18:
			ALUout=Rs<<Imm;
			break;
		case 19:
			if(Rs<Imm)ALUout=1;
			else ALUout=0;
			break;
		case 20:
			ALUout=Rs^Imm;
			break;
		case 21:
			ALUout=Rs>>Imm;
			break;
		case 22:
			ALUout=Rs|Imm;
			break;
		case 23:
			ALUout=Rs&Imm;
			break;
		case 24:
			//Reg[rs1] + sign-extented(Imm),结果取低32位后在符号扩展至64位。 
			ALUout=ext_signed((Rs+Imm)&0xffffffff,1,32);
			break;
		case 25:
			ALUout=temp_PC+4;
			PC=Rs+(Imm<<1);
			printf("case25!!!\n");
			break;
		case 26:
			printf("System call\n");
			break;
		case 27:
			ALUout=Rs+Imm;
			break;
		case 28:
			ALUout=Rs+Imm;
			break;
		case 29:
			ALUout=Rs+Imm;
			break;
		case 30:
			ALUout=Rs+Imm;
			break;
		case 31:
			if(Rs==Rt) PC=PC+Imm;
			printf("case31!!!\n");
			break;
		case 32:
			if(Rs!=Rt) PC=PC+Imm;
			printf("case32!!!\n");
			break;
		case 33:
			if(Rs<Rt) PC=PC+Imm;
			printf("case33!!!\n");
			break;
		case 34:
			if(Rs>=Rt) PC=PC+Imm;
			printf("case34!!!\n");
			break;
		case 35:
			ALUout=PC+Imm;
			printf("case35!!!\n");
			break;
		case 36:
			ALUout=Imm;
			break;
		case 37:
			ALUout=PC+4;
			PC=PC+Imm;
			printf("case37!!!\n");
			break;
		
		default:
			printf("Error: Not found in current ISA.\n");
			break;
	}

	//choose reg dst address
	int Reg_Dst;
	if(RegDst)
	{
		Reg_Dst=rd;
	}
	else
	{
		Reg_Dst=rt;
	}

	//write EX_MEM_old
	EX_MEM.PC=temp_PC;
	EX_MEM.Reg_dst=Reg_Dst;
	EX_MEM.ALU_out=ALUout;
	EX_MEM.Reg_Rt=Rt;

	EX_MEM.Ctrl_EX_ALUOp=ALUop;
	EX_MEM.Ctrl_M_Branch=ID_EX.Ctrl_M_Branch;
	EX_MEM.Ctrl_M_MemWrite=ID_EX.Ctrl_M_MemWrite;
	EX_MEM.Ctrl_M_MemRead=ID_EX.Ctrl_M_MemRead;

	EX_MEM.Ctrl_WB_RegWrite=ID_EX.Ctrl_WB_RegWrite;
	EX_MEM.Ctrl_WB_MemtoReg=ID_EX.Ctrl_WB_MemtoReg;
	printf("EX over!\n");
	print_EXMEM();
}

//访问存储器
	void MEM()
{
	printf("-------------MEM-------------\n");
	//read EX_MEM
	char Branch=EX_MEM.Ctrl_M_Branch;
	char MemWrite=EX_MEM.Ctrl_M_MemWrite;
	char MemRead=EX_MEM.Ctrl_M_MemRead;
	char ALUop=EX_MEM.Ctrl_EX_ALUOp;
	unsigned long long addr=EX_MEM.ALU_out;
	long long reg_rt=EX_MEM.Reg_Rt;
	long long val;
	//complete Branch instruction PC change (no idea what he is talking about)

	//read / write memory
	//read memory
	if(MemRead == 1)
	{
		switch(ALUop)
		{
			case 13:memcpy(&val,&memory[addr],1);ext_signed(val,1,8);break;
			case 14:memcpy(&val,&memory[addr],2);ext_signed(val,1,16);break;
			case 15:memcpy(&val,&memory[addr],4);ext_signed(val,1,32);break;
			case 16:memcpy(&val,&memory[addr],8);break;
		}
		
	}
	else if (MemWrite == 1)
	{
		
		printf("addr = %llx\n",addr);

		switch(ALUop)
		{
			case 27:val=getbit64(reg_rt,56,63);memcpy(&memory[addr],&val,1);break;
			case 28:val=getbit64(reg_rt,48,63);memcpy(&memory[addr],&val,2);break;
			case 29:val=getbit64(reg_rt,32,63);memcpy(&memory[addr],&val,4);break;
			case 30:val=getbit64(reg_rt,0,63);printf("val = %llx\n",val);memcpy(&memory[addr],&val,8);break;		
		}
		

	}
	//write MEM_WB_old
	if(MemRead == 0) 
		MEM_WB.ALU_out=EX_MEM.ALU_out;
	else 
		MEM_WB.ALU_out=val;
	MEM_WB.Mem_read=EX_MEM.Ctrl_M_MemRead;
	MEM_WB.Reg_dst=EX_MEM.Reg_dst;
	MEM_WB.Ctrl_WB_MemtoReg=EX_MEM.Ctrl_WB_MemtoReg;
	MEM_WB.Ctrl_WB_RegWrite=EX_MEM.Ctrl_WB_RegWrite;

	printf("MEM over!\n");
	print_MEMWB();
}


//写回
void WB()
{
	printf("-------------WB--------------\n");
	//read MEM_WB
  	unsigned int Mem_read=MEM_WB.Mem_read;
	REG ALU_out=MEM_WB.ALU_out;
	int Reg_dst=MEM_WB.Reg_dst;
	char Ctrl_WB_RegWrite=MEM_WB.Ctrl_WB_RegWrite;
	char Ctrl_WB_MemtoReg=MEM_WB.Ctrl_WB_MemtoReg;
	if(Ctrl_WB_RegWrite==1)
	{
			reg[Reg_dst]=ALU_out;
	}
	//write reg
	printf("WB over!\n");
}

