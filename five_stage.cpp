#include<iostream>
#include<string>
#include<vector>
#include<bitset>
#include<fstream>

using namespace std;

#define MemSize 1000

template<size_t initialSize, size_t finalSize>
bitset<finalSize> signExtend(const bitset<initialSize>& original) {
    static_assert(finalSize > initialSize, "Final bit size must be greater than the initial bit size.");
    
    bitset<finalSize> signExtended;
    bool signBit = original[initialSize - 1]; // Get the sign bit (the MSB of the original bitset)
    
    // Copy the original bits to the sign extended bitset
    for (size_t i = 0; i < initialSize; ++i) {
        signExtended[i] = original[i];
    }
    
    // Perform sign extension if necessary
    if (signBit) {
        for (size_t i = initialSize; i < finalSize; ++i) {
            signExtended.set(i);
        }
    }
    
    return signExtended;
}

template<std::size_t B>
long bitset_to_long(const std::bitset<B>& b) {
  struct {long x:B;} s;
  return s.x = b.to_ulong();
}

struct IFStruct {
    bitset<32>  PC;
    bool        nop = false;  
};

struct IDStruct {
    bitset<32>  Instr;
    bool        nop = true;  
};

struct EXStruct {
    bitset<32>  Read_data1;
    bitset<32>  Read_data2;
    bitset<16>  Imm;	// need to print 
	bitset<32>  Imm32;
    bitset<5>   Rs;
    bitset<5>   Rt;
    bitset<5>   Wrt_reg_addr;
	bitset<3>	funct3;
    bool        is_I_type = false;
    bool        rd_mem = false;
    bool        wrt_mem = false; 
    bool        alu_op = false;     //1 for addu, lw, sw, 0 for subu 
    bool        wrt_enable = false;
    bool        nop = true;  
};

struct MEMStruct {
    bitset<32>  ALUresult;
    bitset<32>  Store_data;
    bitset<5>   Rs;
    bitset<5>   Rt;    
    bitset<5>   Wrt_reg_addr;
    bool        rd_mem = false;
    bool        wrt_mem = false; 
    bool        wrt_enable = false;    
    bool        nop = true;    
};

struct WBStruct {
    bitset<32>  Wrt_data;
    bitset<5>   Rs;
    bitset<5>   Rt;     
    bitset<5>   Wrt_reg_addr;
    bool        wrt_enable = false;
    bool        nop = true;     
};

struct stateStruct {
    IFStruct    IF;
    IDStruct    ID;
    EXStruct    EX;
    MEMStruct   MEM;
    WBStruct    WB;
};

class InsMem
{
	public:
		string id, ioDir;
        InsMem(string name, string ioDir) {       
			id = name;
			IMem.resize(MemSize);
            ifstream imem;
			string line;
			int i = 0;
			imem.open(ioDir + "/imem.txt");

			if (imem.is_open())
			{
				while (getline(imem,line))
				{      
					IMem[i] = bitset<8>(line.substr(0,8)); 
					i++;
				}                    
			}
            else cout << "Unable to open IMEM input file.";
			imem.close();                     
		}

		bitset<32> readInstr(bitset<32> ReadAddress) {    // Construct a 32-bit instruction given the memory address of the instruction
        
			size_t startIndex = static_cast<size_t>(ReadAddress.to_ulong());   
            bitset<32> instruction; 

			for (size_t i = 0; i < 4; i++){
				instruction <<= 8;  // Shift left by 8 bits (one byte) to make space for the next byte
				instruction |= IMem[startIndex + i].to_ulong();     
			}
			return instruction;

		}      
      
    private:
        vector<bitset<8> > IMem;     
};
      
class DataMem    
{
    public: 
		string id, opFilePath, ioDir;
        DataMem(string name, string ioDir) : id(name), ioDir(ioDir) {
            DMem.resize(MemSize);
			opFilePath = ioDir + "/" + name + "_DMEMResult.txt";
            ifstream dmem;
            string line;
            int i = 0;
            dmem.open(ioDir + "/dmem.txt");
            if (dmem.is_open())
            {
                while (getline(dmem,line))
                {      
                    DMem[i] = bitset<8>(line.substr(0,8));
                    i++;
                }
            }
            else cout << "Unable to open DMEM input file.";
                dmem.close();          
        }
		
        bitset<32> readDataMem(bitset<32> Address) {	
			
			size_t startIndex = static_cast<size_t>(Address.to_ulong());
            bitset<32> memAddress;

			for (size_t i = 0; i < 4; i++){
				memAddress <<= 8;
				memAddress |= DMem[startIndex + i].to_ulong();
			}

			return memAddress;
		}
            
        void writeDataMem(bitset<32> Address, bitset<32> WriteData) {

            size_t startIndex = static_cast<size_t>(Address.to_ulong());    // This index represents the starting position in the memory array (DMem) where the data will be written.

            for (size_t i = 0; i < 4; i++){  

                DMem[startIndex + i].reset();
                DMem[startIndex + i] |= bitset<8>(WriteData.to_string().substr(0,8));
                WriteData <<= 8;
            
			}

        }    
                     
        void outputDataMem() {
            ofstream dmemout;
            dmemout.open(opFilePath, std::ios_base::trunc);
            if (dmemout.is_open()) {
                for (int j = 0; j < 1000; j++)
                {     
                    dmemout << DMem[j] << endl;
                }
                     
            }
            else cout << "Unable to open " << id << " DMEM result file." << endl;
            dmemout.close();
        }             

    private:
		vector<bitset<8> > DMem;      
};

class RegisterFile
{
    public:
		string outputFile;
     	RegisterFile(string ioDir): outputFile (ioDir + "RFResult.txt") {
			Registers.resize(32);  
			Registers[0] = bitset<32> (0);  
        }
	
        bitset<32> readRF(bitset<5> Reg_addr) {   
			return Registers[Reg_addr.to_ulong()];
        }
    
        void writeRF(bitset<5> Reg_addr, bitset<32> Wrt_reg_data) {
            if (Reg_addr.to_ulong() != 0){
                Registers[Reg_addr.to_ulong()] = Wrt_reg_data;
            }
        }
		 
		void outputRF(int cycle) {
			ofstream rfout;
			if (cycle == 0)
				rfout.open(outputFile, std::ios_base::trunc);
			else 
				rfout.open(outputFile, std::ios_base::app);
			if (rfout.is_open())
			{
				rfout << "State of RF after executing cycle:\t" << cycle << endl;
				for (int j = 0; j < 32; j++)
				{
					rfout << Registers[j] << endl;
				}
			}
			else cout << "Unable to open RF output file." << endl;
			rfout.close();               
		} 
			
	private:
		vector<bitset<32> >Registers;
};

class Core {
	public:
		RegisterFile myRF;
		uint32_t cycle = 0;
		uint32_t instructionCount = 0;
		bool halted = false;
		string ioDir;
		struct stateStruct state, nextState;
		InsMem ext_imem;
		DataMem *ext_dmem;
		
		Core(string ioDir, InsMem &imem, DataMem *dmem): myRF(ioDir), ioDir(ioDir), ext_imem (imem), ext_dmem (dmem) {}

		virtual void step(){}
		virtual void printState(){}
		
		void performanceMetrics(uint32_t cycle, uint32_t totalInstructions){
			ofstream printMetrics;
			string filePath = ioDir + "PerformanceMetrics.txt";
			float cpi = static_cast<float>(cycle)/static_cast<float>(totalInstructions);
			float ipc = 1/static_cast<float>(cpi);
			printMetrics.open(filePath, std::ios_base::trunc);
			if (printMetrics.is_open()){
				printMetrics << "Performance of Five Stage: " << endl;
				printMetrics << "#Cycles -> " << cycle << endl;
				printMetrics << "#Instructions -> " << totalInstructions << endl;
				printMetrics << "CPI -> " << cpi << endl;
				printMetrics << "IPC -> " << ipc << endl;
			}
			else cout << "Unable to open a performance metrics output file." << endl;
			printMetrics.close();
		}

		bitset<7> returnOpcode(const string instruction){
			bitset<7> opcode(instruction, 25, 7);	// Extract [6:0] from instruction
			return opcode;
		}

		bitset<7> returnFunct7(const string instruction){
			bitset<7> funct7(instruction, 0, 7);
			return funct7;
		}
		
		bitset<3> returnFunct3(const string instruction){
			bitset<3> funct3(instruction, 17, 3);	// Extract [14:12] from instruction
			return funct3;
		}

};

class FiveStageCore : public Core{
	public:
		FiveStageCore(string ioDir, InsMem &imem, DataMem *dmem): Core(ioDir + "/FS_", imem, dmem), opFilePath(ioDir + "/StateResult_FS.txt") {
			state.IF.nop = false;
			state.ID.nop = true;
			state.EX.nop = true;
			state.MEM.nop = true;
			state.WB.nop = true;
		}

		void decodeRType(const string instruction){
			bitset<5> rs1(instruction, 12, 5);	
			bitset<5> rs2(instruction, 7, 5);
			bitset<5> rd(instruction, 20, 5);
			bitset<7> funct7 = returnFunct7(instruction);

			// Set Registers
			nextState.EX.Rs = rs1;
			nextState.EX.Rt = rs2;	
			nextState.EX.Wrt_reg_addr = rd;
			nextState.EX.Read_data1 = myRF.readRF(nextState.EX.Rs);
			nextState.EX.Read_data2 = myRF.readRF(nextState.EX.Rt);
			nextState.EX.funct3 = returnFunct3(instruction);

			// Set Control Signals
			nextState.EX.is_I_type = false;
			nextState.EX.rd_mem = false;
			nextState.EX.wrt_mem = false;
			nextState.EX.wrt_enable = true;	
			nextState.EX.nop = state.ID.nop;

			if (funct7.to_ulong() == 0x20){		// Set alu_op to false for SUB
				nextState.EX.alu_op = false;
			}
			else{								// Set alu_op to true for ADD
				nextState.EX.alu_op = true;
			}
		}

		void decodeIType(const string instruction, const bitset<7> opcode){
			bitset<5> rs1(instruction, 12, 5);	
			bitset<5> rs2;	// Not used for I-type
			bitset<5> rd(instruction, 20, 5);
			bitset<12> imm(instruction, 0, 12);

			// Set Registers
			nextState.EX.Rs = rs1;
			nextState.EX.Rt = rs2;	// Not used for I-type
			nextState.EX.Wrt_reg_addr = rd;
			nextState.EX.Read_data1 = myRF.readRF(nextState.EX.Rs);
			nextState.EX.Read_data2 = myRF.readRF(nextState.EX.Rt);		// Not used for I-type
			nextState.EX.Imm32 = signExtend<12, 32>(imm);		// Sign-extend 12-bit Immediate
			nextState.EX.funct3 = returnFunct3(instruction);

			// Set Control Signals
			nextState.EX.is_I_type = true;
			nextState.EX.wrt_mem = false;
			nextState.EX.alu_op = false;
			nextState.EX.wrt_enable = true;		
			nextState.EX.nop = state.ID.nop;

			if (opcode.to_ulong() == 0x3 && nextState.EX.funct3.to_ulong() == 0x0){
				nextState.EX.rd_mem = true;		// Only true for LW
			}
			else{
				nextState.EX.rd_mem = false;
			}
		}

		void decodeJType(const string instruction){
			bitset<5> rs1;	// Not used for J-type
			bitset<5> rs2; 	// Not used for J-type
			bitset<5> rd(instruction, 20, 5);

			// Extract immediate bits
			bitset<10> imm10_1(instruction, 1, 10);
			bitset<1> imm11(instruction, 11, 1);
			bitset<8> imm19_12(instruction, 12, 8);
			bitset<1> imm20(instruction, 0, 1);
			
			// Construct 12-bit immediate
			bitset<20> imm;
			for (int i = 0; i < 10; ++i){
				imm[i] = imm10_1[i];
			}
			imm[10] = imm11[0];
			for (int i = 0; i < 8; ++i){
				imm[i + 11] = imm19_12[i];
			}
			imm[19] = imm20[0];

			nextState.EX.Imm32 = signExtend<20, 32>(imm);

			// Set registers
			nextState.EX.Rs = rs1;
			nextState.EX.Rt = rs2;
			nextState.EX.Wrt_reg_addr = rd;
			nextState.EX.Read_data1 = myRF.readRF(nextState.EX.Rs);
			nextState.EX.Read_data2 = myRF.readRF(nextState.EX.Rt);

			// Resolve in ID stage
			myRF.writeRF(nextState.EX.Wrt_reg_addr, state.IF.PC);
			state.IF.PC = state.IF.PC.to_ulong() - 4;	
			state.IF.PC = state.IF.PC.to_ulong() + bitset_to_long(state.EX.Imm32);

			// Clear ID stage
			nextState.ID.nop = true;
			nextState.ID.Instr = bitset<32>(0);

			// Set Control Signals
			nextState.EX.is_I_type = false;
			nextState.EX.rd_mem = false;
			nextState.EX.wrt_mem = false;
			nextState.EX.alu_op = false;
			nextState.EX.wrt_enable = false;		
			nextState.EX.nop = true; // !

		}

		void decodeBType(const string instruction){
			bitset<5> rs1(instruction, 12, 5);
			bitset<5> rs2(instruction, 7, 5);
			bitset<5> rd;	// Not used for B-type

			// Extract immediate bits
			bitset<1> imm12(instruction, 0, 1);
			bitset<1> imm11(instruction, 24, 1);
			bitset<6> imm10_5(instruction, 1, 6);
			bitset<4> imm4_1(instruction, 20, 4);

			// Construct 12-bit immediate	
			bitset<13> imm;
			for (int i = 0; i < 4; ++i){
				imm[i + 1] = imm4_1[i];
			}
			for (int i = 0; i < 6; ++i){
				imm[i + 5] = imm10_5[i];
			}
			for (int i = 0; i < 1; ++i){
				imm[i + 11] = imm11[i];
			}
			for (int i = 0; i < 1; ++i){
				imm[i + 12] = imm12[i];
			}

			nextState.EX.Imm32 = signExtend<13, 32>(imm);

			// Set Registers
			nextState.EX.Rs = rs1;
			nextState.EX.Rt = rs2;
			nextState.EX.Wrt_reg_addr = rd;
			nextState.EX.Read_data1 = myRF.readRF(nextState.EX.Rs);
			nextState.EX.Read_data2 = myRF.readRF(nextState.EX.Rt);
			nextState.EX.funct3 = returnFunct3(instruction);

			// Set Control Signals
			nextState.EX.is_I_type = false;
			nextState.EX.rd_mem = false;
			nextState.EX.wrt_mem = false;
			nextState.EX.alu_op = false;
			nextState.EX.wrt_enable = false;		

			// MEM/WB Hazards
			if (nextState.WB.nop == false && nextState.WB.Wrt_reg_addr != 0 && (nextState.WB.wrt_enable)){
				if (nextState.EX.Rs == nextState.WB.Wrt_reg_addr){
					nextState.EX.Read_data1 = nextState.WB.Wrt_data;
				}
				else if (nextState.EX.Rt == nextState.WB.Wrt_reg_addr){
					nextState.EX.Read_data2 = nextState.WB.Wrt_data;
				}
			}

			// EX/MEM Hazards
			if (nextState.MEM.nop == false && nextState.MEM.Wrt_reg_addr != 0 && (nextState.MEM.wrt_enable)){ 
				if (nextState.EX.Rs == nextState.MEM.Wrt_reg_addr){
					nextState.EX.Read_data1 = nextState.MEM.ALUresult;

					if (nextState.MEM.rd_mem) {
						nextState.EX.nop = true;
						nextState.ID = state.ID;
						state.IF.PC = state.IF.PC.to_ulong() - 4;
						instructionCount--;
					}
					else{
						switch (nextState.EX.funct3.to_ulong()){
						case 0x0:	// BEQ
						{
							if (nextState.EX.Read_data1 == nextState.EX.Read_data2){
								state.IF.PC = state.IF.PC.to_ulong() - 4;
								state.IF.PC = state.IF.PC.to_ulong() + bitset_to_long(nextState.EX.Imm32);
								nextState.ID.nop = true;
								nextState.ID.Instr = bitset<32>(0);
								nextState.EX.nop = true;
							} 
							break;
						}
						case 0x1:	// BNE
						{
							if (nextState.EX.Read_data1 != nextState.EX.Read_data2){
								state.IF.PC = state.IF.PC.to_ulong() - 4;
								state.IF.PC = state.IF.PC.to_ulong() + bitset_to_long(nextState.EX.Imm32);
								nextState.ID.nop = true;
								nextState.ID.Instr = bitset<32>(0);
								nextState.EX.nop = true;
							}
							break;
						}
						}	
					}
				}
					
				else if (nextState.EX.Rt == nextState.MEM.Wrt_reg_addr){
					nextState.EX.Read_data2 = nextState.MEM.ALUresult;

					if (nextState.MEM.rd_mem) {
						nextState.EX.nop = true;
						nextState.ID = state.ID;
						state.IF.PC = state.IF.PC.to_ulong() - 4;
						instructionCount--;
					}
					else{
					switch (nextState.EX.funct3.to_ulong()){
					case 0x0:	// BEQ
					{
						if (nextState.EX.Read_data1 == nextState.EX.Read_data2){
							state.IF.PC = state.IF.PC.to_ulong() - 4;
							state.IF.PC = state.IF.PC.to_ulong() + bitset_to_long(nextState.EX.Imm32);
							nextState.ID.nop = true;
							nextState.ID.Instr = bitset<32>(0);
							nextState.EX.nop = true;
						} 
						break;
					}
					case 0x1:	// BNE
					{
						if (nextState.EX.Read_data1 != nextState.EX.Read_data2){
							state.IF.PC = state.IF.PC.to_ulong() - 4;
							state.IF.PC = state.IF.PC.to_ulong() + bitset_to_long(nextState.EX.Imm32);
							nextState.ID.nop = true;
							nextState.ID.Instr = bitset<32>(0);
							nextState.EX.nop = true;
						}
						break;
					}
					}	
					}
				}
			}		
		}

		void decodeSType(const string instruction){ 
			bitset<5> rs1(instruction, 12, 5);	
			bitset<5> rs2(instruction, 7, 5);
			bitset<5> rd;	// Not used for S-type
			bitset<3> funct3 = returnFunct3(instruction);	
			bitset<7> imm11_5(instruction, 0, 7);
			bitset<5> imm4_0(instruction, 20, 5);
			bitset<12> imm;

			// Construct 12-bit immediate
			for (int i = 0; i < 5; ++i){
				imm[i] = imm4_0[i];
			}
			for (int i = 0; i < 7; ++i){
				imm[i + 5] = imm11_5[i];
			}

			// Set Registers
			nextState.EX.Rs = rs1;
			nextState.EX.Rt = rs2;	
			nextState.EX.Wrt_reg_addr = rd;		// Not used for S-type
			nextState.EX.Read_data1 = myRF.readRF(nextState.EX.Rs);
			nextState.EX.Read_data2 = myRF.readRF(nextState.EX.Rt);
			nextState.EX.Imm32 = signExtend<12, 32>(imm);		// Sign-extend 12-bit immediate
			nextState.EX.funct3 = funct3;

			// Set Control Signals
			nextState.EX.is_I_type = true;
			nextState.EX.wrt_mem = true;
			nextState.EX.wrt_enable = false;
			nextState.EX.alu_op = true;	
			nextState.EX.rd_mem = false;
			nextState.EX.nop = nextState.ID.nop;

		}

		void executeRType(){

			switch (state.EX.funct3.to_ulong()){
				case 0x0:
				{
					if (state.EX.alu_op){	// ADD
						nextState.MEM.ALUresult = bitset_to_long(state.EX.Read_data1) + bitset_to_long(state.EX.Read_data2);
					}
					else{	// SUB
						nextState.MEM.ALUresult = bitset_to_long(state.EX.Read_data1) - bitset_to_long(state.EX.Read_data2);
					}
					break;
				}
				case 0x4:	// XOR
				{
					nextState.MEM.ALUresult = state.EX.Read_data1 ^ state.EX.Read_data2;
					break;
				}
				case 0x6:	// OR
				{
					nextState.MEM.ALUresult = state.EX.Read_data1 | state.EX.Read_data2;
					break;
				}
				case 0x7:	// AND
				{
					nextState.MEM.ALUresult = state.EX.Read_data1 & state.EX.Read_data2;
					break;
				}

			}

			// Set Control Signals
			nextState.MEM.Rs = state.EX.Rs;
			nextState.MEM.Rt = state.EX.Rt;
			nextState.MEM.Wrt_reg_addr = state.EX.Wrt_reg_addr;
			nextState.MEM.rd_mem = state.EX.rd_mem;
			nextState.MEM.wrt_mem = state.EX.wrt_mem;
			nextState.MEM.wrt_enable = state.EX.wrt_enable;
			nextState.MEM.Store_data = state.EX.Read_data2;		// Not used  

		}

		void executeIType(){
			if (state.EX.rd_mem){	// LW
				nextState.MEM.ALUresult = bitset_to_long(state.EX.Read_data1) + bitset_to_long(state.EX.Imm32);		// Calculate memory address
			}
			else{
				switch(state.EX.funct3.to_ulong()){
					case 0x0:	// ADDI
					{
						nextState.MEM.ALUresult = bitset_to_long(state.EX.Read_data1) + bitset_to_long(state.EX.Imm32);	
						break;
					}
					case 0x4:	// XORI
					{
						nextState.MEM.ALUresult = state.EX.Read_data1 ^ state.EX.Imm32;
						break;
					}
					case 0x6:	// ORI
					{
						nextState.MEM.ALUresult = state.EX.Read_data1 | state.EX.Imm32;
						break;
					}
					case 0x7:	// ANDI
					{
						nextState.MEM.ALUresult = state.EX.Read_data1 & state.EX.Imm32; 
						break;
					}
					case 0x2:	// SW
					{
						executeSType();
						break;
					}
				}
			}

			// Set Control Signals
			nextState.MEM.Rs = state.EX.Rs;
			nextState.MEM.Rt = state.EX.Rt;
			nextState.MEM.Wrt_reg_addr = state.EX.Wrt_reg_addr;
			nextState.MEM.rd_mem = state.EX.rd_mem;	
			nextState.MEM.wrt_mem = state.EX.wrt_mem;	
			nextState.MEM.wrt_enable = state.EX.wrt_enable;

		}

		void executeSType(){
			nextState.MEM.Store_data = state.EX.Read_data2;		// Data to be stored 
			nextState.MEM.ALUresult = state.EX.Read_data1.to_ulong() + state.EX.Imm32.to_ulong();		// Memory address
			nextState.MEM.Rs = state.EX.Rs;
			nextState.MEM.Rt = state.EX.Rt;
			nextState.MEM.Wrt_reg_addr = state.EX.Wrt_reg_addr;

			// Set Control Signals
			nextState.MEM.rd_mem = state.EX.rd_mem;		
			nextState.MEM.wrt_mem = state.EX.wrt_mem;
			nextState.MEM.wrt_enable = state.EX.wrt_enable;
		}

		void IF(){
            nextState.ID.Instr = ext_imem.readInstr(state.IF.PC);	// Construct 32 bit instruction [31:0], pass to ID stage
			nextState.IF.PC = state.IF.PC.to_ulong() + 4;
		}

		void ID(){
		    string instrStr = state.ID.Instr.to_string();
		    bitset<7> opcode = returnOpcode(instrStr);

			switch(opcode.to_ulong()){
				case 0x33:	// R-type
				{
					decodeRType(instrStr);
					instructionCount++;
					break;
				}
				case 0x13:	// I-type
				{
					decodeIType(instrStr, opcode);
					instructionCount++;
					break;
				}
				case 0x3:	// I-type (LW)
				{
					decodeIType(instrStr, opcode); 
					instructionCount++;
					break;
				}
				case 0x6F:	// J-type
				{
					decodeJType(instrStr);
					instructionCount++;
					break;
				}
				case 0x63:	// B-type
				{
					decodeBType(instrStr);
					instructionCount++;
					break;
				}
				case 0x23:	// S-type
				{
					decodeSType(instrStr);
					instructionCount++;
					break;
				}

				case 0x7F:	// Halt instruction
				{
				    state.IF.nop = true;
					nextState.IF.nop = true;
					nextState.IF.PC = state.IF.PC.to_ulong() - 4;
				    nextState.ID.nop = true;
				    nextState.EX.nop = true;
					instructionCount++;
                    break;
				}
			}	
		}

		void EX(){
			if (state.EX.is_I_type){
				executeIType();	// execute I-type, including LW, SW
			}
			else{
				executeRType();
			}	
		}

		void MEM(){
		
			if (state.MEM.rd_mem){	// Mem read
				nextState.WB.Wrt_data = ext_dmem->readDataMem(state.MEM.ALUresult);
			}
			else if (state.MEM.wrt_mem){		// Mem write
				ext_dmem->writeDataMem(state.MEM.ALUresult, state.MEM.Store_data);
			}
			else {
				nextState.WB.Wrt_data = state.MEM.ALUresult; // R/I-type
			}

			// Set control signals
			nextState.WB.Rs = state.MEM.Rs;
			nextState.WB.Rt = state.MEM.Rt;
			nextState.WB.Wrt_reg_addr = state.MEM.Wrt_reg_addr;
			nextState.WB.wrt_enable = state.MEM.wrt_enable;

		}

		void WB(){
			
			if (state.WB.wrt_enable){
				myRF.writeRF(state.WB.Wrt_reg_addr, state.WB.Wrt_data); 
			}

		}		

		void step(){

			/* --------------------- WB stage --------------------- */
			if (!state.WB.nop){
				WB();
			}
			
			/* --------------------- MEM stage -------------------- */
			if (!state.MEM.nop){
				MEM();
			}
			nextState.WB.nop = state.MEM.nop;
			
			/* --------------------- EX stage --------------------- */
			if (!state.EX.nop){
				EX();
			}
			nextState.MEM.nop = state.EX.nop;
			
			/* --------------------- ID stage --------------------- */
			if (!state.ID.nop){
				ID();
				// MEM/WB Hazards
				if (nextState.WB.nop == false && nextState.WB.Wrt_reg_addr != 0 && (nextState.WB.wrt_enable)){
					if (nextState.EX.Rs == nextState.WB.Wrt_reg_addr){
						nextState.EX.Read_data1 = nextState.WB.Wrt_data;
					}
					else if (nextState.EX.Rt == nextState.WB.Wrt_reg_addr){
						nextState.EX.Read_data2 = nextState.WB.Wrt_data;
					}
				}
				// EX/MEM Hazards
				if (nextState.MEM.nop == false && nextState.MEM.Wrt_reg_addr != 0 && (nextState.MEM.wrt_enable)){ 
					if (nextState.EX.Rs == nextState.MEM.Wrt_reg_addr){
						nextState.EX.Read_data1 = nextState.MEM.ALUresult;

						if (nextState.MEM.rd_mem) {
							nextState.EX.nop = true;
							nextState.ID = state.ID;
							state.IF.PC = state.IF.PC.to_ulong() - 4;
							instructionCount--;
						}
					}
					else if (nextState.EX.Rt == nextState.MEM.Wrt_reg_addr){
						nextState.EX.Read_data2 = nextState.MEM.ALUresult;

						if (nextState.MEM.rd_mem) {
							nextState.EX.nop = true;
							nextState.ID = state.ID;
							state.IF.PC = state.IF.PC.to_ulong() - 4;
							instructionCount--;
						}
					}
				}
			}

			/* --------------------- IF stage --------------------- */
            if (!state.IF.nop){
				IF();
			}
			nextState.ID.nop = state.IF.nop;

			/* ---------------------------------------------------- */	

			if (state.IF.nop && state.ID.nop && state.EX.nop && state.MEM.nop && state.WB.nop){
				halted = true;
			}
        
            myRF.outputRF(cycle); // dump RF
			printState(nextState, cycle); //print states after executing cycle 0, cycle 1, cycle 2 ... 
       
			state = nextState; //The end of the cycle and updates the current state with the values calculated in this cycle
			cycle++;
            performanceMetrics(cycle, instructionCount);
            
		}

		void printState(stateStruct state, int cycle) {
		    ofstream printstate;
			if (cycle == 0)
				printstate.open(opFilePath, std::ios_base::trunc);
			else 
		    	printstate.open(opFilePath, std::ios_base::app);
		    if (printstate.is_open()) {
				printstate << "----------------------------------------------------------------------" << endl;
		        printstate << "State after executing cycle:\t" << cycle << endl; 

		        printstate << "IF.PC:\t" << state.IF.PC.to_ulong() << endl; 
		        printstate << "IF.nop:\t" << state.IF.nop << endl; 

		        printstate << "ID.Instr:\t" << state.ID.Instr << endl; 
		        printstate << "ID.nop:\t" << state.ID.nop << endl;

		        printstate << "EX.Read_data1:\t" << state.EX.Read_data1 << endl;
		        printstate << "EX.Read_data2:\t" << state.EX.Read_data2 << endl;
		        printstate << "EX.Imm:\t" << state.EX.Imm32 << endl; 
		        printstate << "EX.Rs:\t" << state.EX.Rs << endl;
		        printstate << "EX.Rt:\t" << state.EX.Rt << endl;
		        printstate << "EX.Wrt_reg_addr:\t" << state.EX.Wrt_reg_addr << endl;
		        printstate << "EX.is_I_type:\t" << state.EX.is_I_type << endl; 
		        printstate << "EX.rd_mem:\t" << state.EX.rd_mem << endl;
		        printstate << "EX.wrt_mem:\t" << state.EX.wrt_mem << endl;        
		        printstate << "EX.alu_op:\t" << state.EX.alu_op << endl;
		        printstate << "EX.wrt_enable:\t" << state.EX.wrt_enable << endl;
		        printstate << "EX.nop:\t" << state.EX.nop << endl;        

		        printstate << "MEM.ALUresult:\t" << state.MEM.ALUresult << endl;
		        printstate << "MEM.Store_data:\t" << state.MEM.Store_data << endl; 
		        printstate << "MEM.Rs:\t" << state.MEM.Rs << endl;
		        printstate << "MEM.Rt:\t" << state.MEM.Rt << endl;   
		        printstate << "MEM.Wrt_reg_addr:\t"<<state.MEM.Wrt_reg_addr << endl;              
		        printstate << "MEM.rd_mem:\t" << state.MEM.rd_mem << endl;
		        printstate << "MEM.wrt_mem:\t" << state.MEM.wrt_mem << endl; 
		        printstate << "MEM.wrt_enable:\t" << state.MEM.wrt_enable << endl;         
		        printstate << "MEM.nop:\t" << state.MEM.nop << endl;        

		        printstate << "WB.Wrt_data:\t"<<state.WB.Wrt_data << endl;
		        printstate << "WB.Rs:\t" << state.WB.Rs << endl;
		        printstate << "WB.Rt:\t" << state.WB.Rt << endl;
		        printstate << "WB.Wrt_reg_addr:\t" << state.WB.Wrt_reg_addr << endl;
		        printstate << "WB.wrt_enable:\t" << state.WB.wrt_enable << endl;
		        printstate << "WB.nop:\t" << state.WB.nop << endl; 
		    }
		    else cout << "Unable to open FS StateResult output file." << endl;
		    printstate.close();
		}
	private:
		string opFilePath;
};

int main(int argc, char* argv[]) {
	
	string ioDir = "";
    if (argc == 1) {
        cout << "Enter path containing the memory files: ";
        cin >> ioDir;
    }
    else if (argc > 2) {
        cout << "Invalid number of arguments. Machine stopped." << endl;
        return -1;
    }
    else {
        ioDir = argv[1];
        cout << "IO Directory: " << ioDir << endl;
    }

    InsMem imem = InsMem("Imem", ioDir);
	DataMem dmem_fs = DataMem("FS", ioDir);

	FiveStageCore FSCore(ioDir, imem, &dmem_fs);

    while (1) {
		if (!FSCore.halted)
			FSCore.step();

		if (FSCore.halted)
			break;
    }
    
	FSCore.ext_dmem -> outputDataMem();     // Dump FS data mem

	return 0;
}