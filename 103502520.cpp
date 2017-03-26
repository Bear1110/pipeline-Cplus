/**
    設計構想

	藉由產生5個物件(struct)
	分別為 IF_ID、ID_EX、EX_MEM、MEM_WB 及 一個pipeline (包含 4個register)
	每個register都可以access上一個register的信號、資料，再由這些資料去做運算(eg:ex_mem)
	運算完畢後將資料存在當前register以備下一個階段去取得上一個階段的數值。



	具體實作

	先將mem_wb 的資料(若需寫回)寫回暫存器
	ex_mem→mem_wb、id_ex→ex_mem、if_id→id_ex、if_id從檔案中取得instruction若無指令則nop。

*/


#include<iostream>
#include<fstream>
#include<cstdlib>
#include<math.h>
using namespace std;

int convert_decimal(bool sign,string str);
void general();    // 執行 General
void datahazrd();  // 執行 Datahazard
void lwhazard();  //  執行 Lwhazard
void branchhazrd();// 執行 Branchazard
int temp = 0;

string instructions[20];
ifstream fin;     // 讀檔案用
ofstream fout;    // 寫檔案用

void initializer_register();// 用來 初始化用

int mem[5] = { 5, 5, 6, 8, 8 };
int reg[9] = { 0, 8, 7, 6, 3, 9, 5, 2, 7 };

/**

 這裡拿來 instruction fetch  (IF_ID)

**/
struct IF_ID
{
    bool input;
    int pc;
    string instr;
    IF_ID()
    {
        input = false;
        pc = 0;
        instr = "00000000000000000000000000000000";
    }
    void Fetch()
    {
        pc = pc+4;
        if(instructions[pc/4]=="")
        {
            instr = "00000000000000000000000000000000";
        }
        else
        {
            instr = instructions[pc/4];
        }
        input = true;
    }
    void flush()
    {
        input = true;
        instr = "00000000000000000000000000000000";
        pc = pc-4;
    }
};
/**

 這裡拿來 instruction decode  (ID_EX)

**/
struct ID_EX
{
    //previous reg
    int pc;
    string instr;

    //deocde needed
    string opcode;
    string func;
    string control_sign;

    bool input;
    int ReadData1,ReadData2;
    int sign_ext;
    int Rs,Rt,Rd;

    ID_EX(){
        input = false;
        ReadData1 = ReadData2 = 0;
        sign_ext = 0;
        Rs = Rt = Rd = pc =0;
        control_sign = "000000000";
    }
    void flush(){
        input = false;
        ReadData1 = ReadData2= 0;
        sign_ext = 0;
        Rs = Rt = Rd = pc =0;
        control_sign = "000000000";
        input=true;
    }
    void decode(IF_ID &myifid){
        opcode = myifid.instr.substr(0, 6);    //opcode 為 machine code 的 前面 6碼
        input = myifid.input;
        myifid.input = false;

        func = myifid.instr.substr(26, 6);
        Rs = convert_decimal(false,myifid.instr.substr(6, 5));
        Rt = convert_decimal(false,myifid.instr.substr(11, 5));
        ReadData1 = reg[Rs];
        ReadData2 = reg[Rt];
        pc = myifid.pc;

        if(opcode == "000000")  //R-type
        {
            Rd = convert_decimal(false,myifid.instr.substr(16, 5));
            sign_ext = 0;
            if(func=="000000"&&Rs==0&&Rt==0)        //nop
            {
                control_sign = "000000000";
            }
            else
            {
                control_sign="110000010";

            }
        }
        else   // I-type
        {
            Rd=0;
            sign_ext=convert_decimal(true,myifid.instr.substr(16 , 16));

            if(opcode=="100011")   //lw
            {
                control_sign="000101011";
            }
            else if(opcode=="000100") //beq
            {
                control_sign="X0101000X";

            }else if(opcode=="001000")   //addi
            {
                control_sign = "000100010";

            }else if(opcode=="001101"){   //ori
                control_sign = "011100010";
            }
        }

    }

};
/**

 這裡拿來 execute (執行運算)  EX_MEM

**/
struct EX_MEM
{
    bool input;
    int ALUout;
    int writeData;
    int Rt;
    int pc;
    int tmp1,tmp2;
    string control_sign;
    string ALUop;

    EX_MEM()
    {
        input = false;
        ALUout = 0;
        writeData = 0;
        Rt = pc = 0;
        control_sign = "00000";
    }
    void flush()
    {
        EX_MEM();
        ALUout = writeData = Rt = pc = 0;
        input=true;
    }
    void execute(ID_EX &myidex)
    {
            input = myidex.input;
            myidex.input = false;

            writeData = myidex.ReadData2;
            control_sign = myidex.control_sign.substr(4,8);
            ALUop = myidex.control_sign.substr(1,2);
            if(myidex.control_sign.substr(0,1)=="1") //R-type
            {
                Rt = myidex.Rd;
                temp = 1;
            }
            else //others I-type
            {
                Rt = myidex.Rt;
                temp = 0;
            }
            tmp1 = myidex.ReadData1;
            if(myidex.control_sign[3]=='0') //get ALUOp0 R-type lw sw
            {
                tmp2 = myidex.ReadData2;
            }
            else //beq  addi
            {
                tmp2 = myidex.sign_ext;
            }
            if(ALUop == "10") //R-type
            {
                if(myidex.func == "100000")  //add
                    ALUout=tmp1+tmp2;
                else if(myidex.func == "100010")//sub
                    ALUout=tmp1-tmp2;
                else if(myidex.func == "100100")//and
                    ALUout=tmp1&tmp2;
                else if(myidex.func == "100101")//or
                    ALUout=tmp1|tmp2;
                else//nop
                    ALUout=0;

            }
            else if(ALUop=="00") // lw  addi
            {
                ALUout=tmp1+tmp2;
            }
            else if(ALUop=="01") //beq
            {
                ALUout=tmp1-tmp2;

            }else if(ALUop=="11") //ori
            {
                ALUout=tmp1|tmp2;
            }
            pc = myidex.pc+(myidex.sign_ext*4);

    }

};
/**

 這裡拿來 Memory access   MEM_WB

**/
struct MEM_WB
{
    bool input;
    bool branch;
    int readData;
    int ALUout;
    int Rt;
    string control_sign;
    MEM_WB()
    {
        ALUout = 0;
        readData = 0;
        Rt = 0;
        input = branch = false;
        control_sign = "00";
    }
    void memory_read(IF_ID &myifid,EX_MEM myexmem)   // 讀取
    {
        branch = false;
        input = myexmem.input;
        myifid.input=false;

        Rt = myexmem.Rt;

        ALUout = myexmem.ALUout;
        control_sign = myexmem.control_sign.substr(3,4);

        if (control_sign == "0X" && ALUout == 0) //beq
        {
            branch = true;
            myifid.pc = myexmem.pc;
        }
        if(control_sign=="11")    //load
        {
            readData=mem[ALUout/4];
        }else{
            readData=0;
        }
    }
};
/**

 這裡拿來 產生 pipeline class

**/
struct Pipeline
{
    int clockcycle;
    IF_ID ifid;
    ID_EX idex;
    EX_MEM exmem;
    MEM_WB memwb;

    Pipeline()                  //initial clockcycle
    {
        clockcycle=0;
    }

    void writeback_Reg(MEM_WB myback)    /**這裡是 WRB*/
    {
        if (myback.control_sign[0] == '1')
        {
            if (myback.Rt == 0)	return;
            if(myback.control_sign[1] == '1')
            {
                reg[myback.Rt] =  myback.readData;
            }
            else
            {
                reg[myback.Rt] = myback.ALUout;
            }
        }

    }
 /**
    這邊是來確認有沒有datahazard    MEM hazard   branch hazrd
 */
    void check()
    {
        /** data hazard */

        if(exmem.Rt == idex.Rs && exmem.Rt != 0 && exmem.control_sign[3] == '1')                   //ForwardA 10
            idex.ReadData1 = exmem.ALUout;
        if(exmem.Rt == idex.Rt && exmem.Rt != 0 && exmem.control_sign[3] == '1')
            idex.ReadData2 = exmem.ALUout;        //ForwardB 10
        /** MEM hazard */

        if(memwb.Rt == idex.Rs && memwb.Rt != 0 && memwb.control_sign[0] == '1')
            idex.ReadData1 = (memwb.control_sign[1] == '1') ? memwb.readData : memwb.ALUout ;          //ForwardA01
        if(memwb.Rt == idex.Rt && memwb.Rt != 0 && memwb.control_sign[0] == '1')
            idex.ReadData2 = (memwb.control_sign[1] == '1') ? memwb.readData : memwb.ALUout;    //ForwardB01

        if (idex.control_sign[5] == '1'                 //load-use
                && (idex.Rt == convert_decimal(false,ifid.instr.substr(6, 5))
                    || idex.Rt == convert_decimal(false,ifid.instr.substr(11, 5))))
        {
            // set NOP
            ifid.instr = "00000000000000000000000000000000";
            ifid.pc -= 4;
            ifid.input = true;
        }
        /** branch hazrd*/
        if (memwb.branch)
        {
            ifid.flush();
            idex.flush();
            exmem.flush();
        }
    }

    void printcycle()
    {

        fout<<"CC  "<<clockcycle<<" :\n";
        fout<<endl;
        fout<<"Registers: "<<endl;
        fout<<"$0: "<<reg[0]<<"	  $1: "<<reg[1]<<"	  $2: "<<reg[2]<<endl;
        fout<<"$3: "<<reg[3]<<"	  $4: "<<reg[4]<<"	  $5: "<<reg[5]<<endl;
        fout<<"$6: "<<reg[6]<<"	  $7: "<<reg[7]<<"	  $8: "<<reg[8]<<endl;
        fout<<endl;
        fout<<"Data memory:"<<endl;
        fout<<"0:       "<<mem[0]<<endl;
        fout<<"4:       "<<mem[1]<<endl;
        fout<<"8:       "<<mem[2]<<endl;
        fout<<"12:      "<<mem[3]<<endl;
        fout<<"16:      "<<mem[4]<<endl;
        fout<<endl;
        fout<<"IF/ID:"<<endl;
        fout<<"PC:                   "<<ifid.pc<<endl;
        fout<<"Instruction:          "<<ifid.instr<<endl;
        fout<<endl;
        fout<<"ID/EX :"<<endl;
        fout<<"ReadData1          "<<idex.ReadData1<<endl;
        fout<<"ReadData2          "<<idex.ReadData2<<endl;
        fout<<"sign_ext           "<<idex.sign_ext<<endl;
        fout<<"Rs                 "<<idex.Rs<<endl;
        fout<<"Rt                 "<<idex.Rt<<endl;
        fout<<"Rd                 "<<idex.Rd<<endl;
        fout<<"Control signals    "<<idex.control_sign<<endl;
        fout<<endl;
        fout<<"EX/MEM :"<<endl;
        fout<<"ALUout             "<<exmem.ALUout<<endl;
        fout<<"WriteData          "<<exmem.writeData<<endl;
        if(temp==1) //R-type
        {
            fout<<"Rd                 "<<exmem.Rt<<endl;
        }
        else //others I-type
        {
            fout<<"Rt                 "<<exmem.Rt<<endl;
        }
        fout<<"Control signals    "<<exmem.control_sign<<endl;

        fout<<endl;
        fout<<"MEM/WB :"<<endl;
        fout<<"Read Data          "<<memwb.readData<<endl;
        fout<<"ALUout             "<<memwb.ALUout<<endl;
        fout<<"Control signals    "<<memwb.control_sign<<endl;
        fout<<"================================================="<<endl;

    }
    /**
        這邊拿來重複推進 cycle
    */
    bool is_nextstep()
    {
        writeback_Reg(memwb);
        memwb.memory_read(ifid,exmem);
        exmem.execute(idex);
        idex.decode(ifid);
        ifid.Fetch();

        clockcycle++;
        printcycle();
        check();


        if((memwb.control_sign=="00"&&memwb.ALUout == 0 && memwb.readData==0)&&exmem.control_sign=="00000"&&idex.control_sign=="000000000"&&clockcycle!=1)
        {
            return false;
        }
        else
            return true;
    }

};
/**初始化 register,memory 值*/
void initializer_register(){
    mem[0] = 5;
        mem[1] = 5;
        mem[2] = 6;
        mem[3] = 8;
        mem[4] = 8;

        reg[0] = 0;
        reg[1] = 8;
        reg[2] = 7;
        reg[3] = 6;
        reg[4] = 3;
        reg[5] = 9;
        reg[6] = 5;
        reg[7] = 2;
        reg[8] = 7;
}

Pipeline p1;
Pipeline p2;
Pipeline p3;
Pipeline p4;
int main()
{
    /**真正執行在此*/
    general();
    datahazrd();
    lwhazard();
    branchhazrd();

    return 0;
}
/**
    以下是讀檔及執行準備
*/
void general()
{
    initializer_register();
    fin.open("General.txt", ifstream::in);
    fout.open("genResult.txt",  ifstream::out);
    int    n = 1;
    while (fin >> instructions[n])	n++;
    while(p1.is_nextstep());
    fin.close();
    fout.close();
}
void datahazrd()
{
    initializer_register();
    fin.open("Datahazard.txt", ifstream::in);
    fout.open("dataResult.txt",  ifstream::out);
    int    n = 1;
    while (fin >> instructions[n])	n++;
    while(p2.is_nextstep());
    fin.close();
    fout.close();
}
void lwhazard()
{
    initializer_register();
    fin.open("Lwhazard.txt", ifstream::in);
    fout.open("loadResult.txt",  ifstream::out);
    int    n = 1;
    while (fin >> instructions[n])	n++;
    while(p3.is_nextstep());
    fin.close();
    fout.close();
}

void branchhazrd()
{
    initializer_register();
    fin.open("Branchazard.txt", ifstream::in);
    fout.open("branchResult.txt",  ifstream::out);
    int    n = 1;
    while (fin >> instructions[n])	n++;
    while(p4.is_nextstep());
    fin.close();
    fout.close();
}
/**
    此為 convert 二進為變成十進位
*/
int convert_decimal( bool sign,string str)
{
    bool flag = false;
    if (str[0] == '1')
            flag = true;
    int res = 0;

    for (int i = str.length()-1 , a = 0; i >0 ; a++ ,i--)
    {
        res += (str[i]-'0')*pow(2,a);
    }
    if (flag && sign)                //for sign_ext
    {
        res = -res;
    }
    return res;
}

