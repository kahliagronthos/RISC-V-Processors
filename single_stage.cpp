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
        for (size_t i = initialSize; i < finalSize; ++i){
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
    bool        nop;  
};

struct EXStruct {
    bitset<32>  Read_data1;
    bitset<32>  Read_data2;
	bitset<32>  Imm32;
    bitset<5>   Rs;
    bitset<5>   Rt;
    bitset<5>   Wrt_reg_addr;
	bitset<3>	funct3;
    bool        is_I_type;
    bool        rd_mem;
    bool        wrt_mem; 
    bool        alu_op;  
    bool        wrt_enable;
    bool        nop;  
};

struct MEMStruct {
    bitset<32>  ALUresult;
    bitset<32>  Store_data;
    bitset<5>   Rs;
    bitset<5>   Rt;    
    bitset<5>   Wrt_reg_addr;
    bool        rd_mem;
    bool        wrt_mem; 
    bool        wrt_enable;    
    bool        nop;    
};

struct WBStruct {
    bitset<32>  Wrt_data;
    bitset<5>   Rs;
    bitset<5>   Rt;     
    bitset<5>   Wrt_reg_addr;
    bool        wrt_enable;
    bool        nop;     
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
				printMetrics << "Performance of Single Stage: " << endl;
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

class SingleStageCore : public Core {
	public:
		SingleStageCore(string ioDir, InsMem &imem, DataMem *dmem): Core(ioDir + "/SS_", imem, dmem), opFilePath(ioDir + "/StateResult_SS.txt") {}
		
		void decodeRType(const string instruction){
			bitset<5> rs1(instruction, 12, 5);	
			bitset<5> rs2(instruction, 7, 5);
			bitset<5> rd(instruction, 20, 5);
			bitset<7> funct7 = returnFunct7(instruction);

			// Set Registers
			state.EX.Rs = rs1;
			state.EX.Rt = rs2;	
			state.EX.Wrt_reg_addr = rd;
			state.EX.Read_data1 = myRF.readRF(state.EX.Rs);
			state.EX.Read_data2 = myRF.readRF(state.EX.Rt);
			state.EX.funct3 = returnFunct3(instruction);

			// Set Control Signals
			state.EX.is_I_type = false;
			state.EX.wrt_mem = false;
			state.EX.wrt_enable = true;	
			state.EX.rd_mem = false;
			state.EX.nop = false;

			if (funct7.to_ulong() == 0x20){		// Set alu_op to false for SUB
				state.EX.alu_op = false;
			}
			else{								// Set alu_op to true for ADD
				state.EX.alu_op = true;
			}

		}

		void decodeIType(const string instruction, const bitset<7> opcode){
			bitset<5> rs1(instruction, 12, 5);	
			bitset<5> rs2;	// Not used for I-type
			bitset<5> rd(instruction, 20, 5);
			bitset<12> Imm32(instruction, 0, 12);

			// Set Registers
			state.EX.Rs = rs1;
			state.EX.Rt = rs2;	// Not used for I-type
			state.EX.Wrt_reg_addr = rd;
			state.EX.Read_data1 = myRF.readRF(state.EX.Rs);
			state.EX.Read_data2 = myRF.readRF(state.EX.Rt);		// Not used for I-type
			state.EX.Imm32 = signExtend<12, 32>(Imm32);		// Sign-extend 12-bit Immediate
			state.EX.funct3 = returnFunct3(instruction);

			// Set Control Signals
			state.EX.is_I_type = true;
			state.EX.wrt_mem = false;
			state.EX.alu_op = true;
			state.EX.wrt_enable = true;		
			state.EX.nop = false;

			if (opcode.to_ulong() == 0x3 && state.EX.funct3.to_ulong() == 0x0){
				state.EX.rd_mem = true;		// Only set to true for LW
			}
			else{
				state.EX.rd_mem = false;
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

			state.EX.Imm32 = signExtend<20,32>(imm);
			state.EX.Rs = rs1;
			state.EX.Rt = rs2;
			state.EX.Wrt_reg_addr = rd;
			state.EX.Read_data1 = myRF.readRF(state.EX.Rs);
			state.EX.Read_data2 = myRF.readRF(state.EX.Rt);

			// Resolve JAL in ID stage
			myRF.writeRF(state.EX.Wrt_reg_addr, state.IF.PC);
			state.IF.PC = state.IF.PC.to_ulong() - 4;	
			state.IF.PC = state.IF.PC.to_ulong() + bitset_to_long(state.EX.Imm32);
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

			state.EX.Imm32 = signExtend<13, 32>(imm);

			// Set Registers
			state.EX.Rs = rs1;
			state.EX.Rt = rs2;
			state.EX.Wrt_reg_addr = rd;
			state.EX.Read_data1 = myRF.readRF(state.EX.Rs);
			state.EX.Read_data2 = myRF.readRF(state.EX.Rt);
			state.EX.funct3 = returnFunct3(instruction);

			// Resolve branch in ID stage
			switch (state.EX.funct3.to_ulong()){
				case 0x0:	// BEQ
				{
					if (state.EX.Read_data1 == state.EX.Read_data2){
						state.IF.PC = state.IF.PC.to_ulong() - 4;
						state.IF.PC = state.IF.PC.to_ulong() + bitset_to_long(state.EX.Imm32);
					} 
					break;
				}
				case 0x1:	// BNE
				{
					if (state.EX.Read_data1 != state.EX.Read_data2){
						state.IF.PC = state.IF.PC.to_ulong() - 4;
						state.IF.PC = state.IF.PC.to_ulong() + bitset_to_long(state.EX.Imm32);
					}
					break;
				}
			}
		}

		void decodeSType(const string instruction){
			bitset<5> rs1(instruction, 12, 5);	
			bitset<5> rs2(instruction, 7, 5);
			bitset<5> rd;	// Not used for S-type
			bitset<3> funct3 = returnFunct3(instruction);	// Not used for S-type
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
			state.EX.Rs = rs1;
			state.EX.Rt = rs2;	
			state.EX.Wrt_reg_addr = rd;		// Not used for S-type
			state.EX.Read_data1 = myRF.readRF(state.EX.Rs);
			state.EX.Read_data2 = myRF.readRF(state.EX.Rt);
			state.EX.Imm32 = signExtend<12, 32>(imm);		// Sign-extend 12-bit immediate

			// // Set Control Signals
			state.EX.is_I_type = false;
			state.EX.wrt_mem = true;
			state.EX.wrt_enable = false;
			state.EX.alu_op = true;
			state.EX.rd_mem = false;
			state.EX.nop = false;
		}

		void executeRType(){

			switch (state.EX.funct3.to_ulong()){
				case 0x0:
				{
					if (state.EX.alu_op){	// ADD
						state.WB.Wrt_data = bitset_to_long(state.EX.Read_data1) + bitset_to_long(state.EX.Read_data2);
					}
					else{	// SUB
						state.WB.Wrt_data = bitset_to_long(state.EX.Read_data1) - bitset_to_long(state.EX.Read_data2);
					}
					break;
				}
				case 0x4:	// XOR
				{
					state.WB.Wrt_data = state.EX.Read_data1 ^ state.EX.Read_data2;
					break;
				}
				case 0x6:	// OR
				{
					state.WB.Wrt_data = state.EX.Read_data1 | state.EX.Read_data2;
					break;
				}
				case 0x7:	// AND
				{
					state.WB.Wrt_data = state.EX.Read_data1 & state.EX.Read_data2;
					break;
				}

			}

			// Set Control Signals
			state.WB.Rs = state.EX.Rs;
			state.WB.Rt = state.EX.Rt;
			state.WB.Wrt_reg_addr = state.EX.Wrt_reg_addr;
			state.WB.wrt_enable = state.EX.wrt_enable;
			state.WB.nop = state.EX.nop;

			state.MEM.rd_mem = state.EX.rd_mem;
			state.MEM.wrt_mem = state.EX.wrt_mem;
			state.MEM.wrt_enable = state.EX.wrt_enable;
			state.MEM.nop = state.EX.nop;

		}

		void executeIType(){

			if (state.EX.rd_mem){	// LW
				state.MEM.ALUresult = bitset_to_long(state.EX.Read_data1) + bitset_to_long(state.EX.Imm32);		// Calculate address
				state.MEM.rd_mem = state.EX.rd_mem;	// Allow LW to read memory
			}
			else{
				switch(state.EX.funct3.to_ulong()){
					case 0x0:	// ADDI
					{
						state.WB.Wrt_data = bitset_to_long(state.EX.Read_data1) + bitset_to_long(state.EX.Imm32);
						break;
					}
					case 0x4:	// XORI
					{
						state.WB.Wrt_data = state.EX.Read_data1 ^ state.EX.Imm32;
						break;
					}
					case 0x6:	// ORI
					{
						state.WB.Wrt_data = state.EX.Read_data1 | state.EX.Imm32;
						break;
					}
					case 0x7:	// ANDI
					{
						state.WB.Wrt_data = state.EX.Read_data1 & state.EX.Imm32; 
						break;
					}
				}
				state.MEM.rd_mem = state.EX.rd_mem;		// All other I-types not allowed to read from memory
				state.MEM.ALUresult = 0;	// Is this right?
				state.MEM.Store_data = 0; 	// Is this right?
			}

			// Set Control Signals
			state.WB.Rs = state.EX.Rs;
			state.WB.Rt = state.EX.Rt;
			state.WB.Wrt_reg_addr = state.EX.Wrt_reg_addr;
			state.WB.wrt_enable = state.EX.wrt_enable;
			state.WB.nop = state.EX.nop;	

			state.MEM.Rs = state.EX.Rs;
			state.MEM.Rt = state.EX.Rt;
			state.MEM.Wrt_reg_addr = state.EX.Wrt_reg_addr;
			state.MEM.wrt_mem = state.EX.wrt_mem;	
			state.MEM.wrt_enable = state.EX.wrt_enable;
			state.MEM.nop = state.EX.nop;

		}

		void executeSType(){
			state.MEM.Store_data = state.EX.Read_data2;     // Data to be stored 
			state.MEM.ALUresult = state.EX.Read_data1.to_ulong() + state.EX.Imm32.to_ulong();   // Memory address
			state.MEM.Rs = state.EX.Rs;
			state.MEM.Rt = state.EX.Rt;
			state.MEM.Wrt_reg_addr = state.EX.Wrt_reg_addr;

			// Set Control Signals
			state.MEM.rd_mem = state.EX.rd_mem;		
			state.MEM.wrt_mem = state.EX.wrt_mem;
			state.MEM.wrt_enable = state.EX.wrt_enable;
			state.MEM.nop = state.EX.nop;
		}

		void ID(string instrStr, bitset<7> opcode){

				switch(opcode.to_ulong()){
				case 0x33:	// R-type
				{
					decodeRType(instrStr);
					break;
				}
				case 0x13:	// I-type
				{
					decodeIType(instrStr, opcode);
					break;
				}
				case 0x3:	// I-type (LW)
				{
					decodeIType(instrStr, opcode); 
					break;
				}
				case 0x6F:	// J-type
				{
					decodeJType(instrStr);
					break;
				}
				case 0x63:	// B-type
				{
					decodeBType(instrStr);
					break;
				}
				case 0x23:	// S-type
				{
					decodeSType(instrStr);
					break;
				}
			}
		}

		void EX(bitset <7> opcode){
			switch(opcode.to_ulong()){
				case 0x33:	// R-type
				{
					executeRType();
					break;
				}
				case 0x13:	// I-type
				{
					executeIType();
					break;
				}
				case 0x3:	// I-type (LW)
				{
					executeIType();
					break;
				}
				case 0x23:	// S-type
				{
					executeSType();
					break;
				}
			}
		}

		void MEM(){
		
			if (state.MEM.rd_mem){	// Mem read
				state.WB.Wrt_data = ext_dmem -> readDataMem(state.MEM.ALUresult);
			}

			if (state.MEM.wrt_mem){		// Mem write
				ext_dmem -> writeDataMem(state.MEM.ALUresult, state.MEM.Store_data);
			}

		}

		void WB(){
			
			if (state.WB.wrt_enable){
				myRF.writeRF(state.WB.Wrt_reg_addr, state.WB.Wrt_data); 
			}

		}

		void step() {
			// *** FETCH INSTRUCTION ***
			state.ID.Instr = ext_imem.readInstr(state.IF.PC);	// Construct 32 bit instruction [31:0], pass to ID stage 

			//cout << "Current Instruction in bitset<32> format: " << state.ID.Instr << endl;
			string instrStr = state.ID.Instr.to_string();
			//cout << "As a string: " << instrStr << endl;
			bitset<7> opcode = returnOpcode(instrStr);
			//cout << "Opcode: " << opcode << endl;
			//cout << "Cycle: " << cycle << endl;
            //cout << "PC: " << state.IF.PC.to_ulong() << endl;

			if (opcode.to_ulong() == 0x7F){		// Halt instruction
				state.IF.nop = true;
			}

			if ((state.IF.nop == true && nextState.IF.nop == false)){
				state.IF.PC = state.IF.PC.to_ulong();
				instructionCount++;

			}
			else if (state.IF.nop == true && nextState.IF.nop == true){
				state.IF.PC = state.IF.PC.to_ulong();
			}
			else{
				state.IF.PC = state.IF.PC.to_ulong() + 4;
				instructionCount++;
			}

			// *** DECODE INSTRUCTION ***
			ID(instrStr, opcode);

			// *** EXECUTE INSTRUCTION ***
			EX(opcode);

			// *** ACCESS MEMORY ***
			MEM();

			// *** WRITEBACK ***
			WB();

			if (state.IF.nop && nextState.IF.nop){
				halted = true;
			}
			
			if (state.IF.nop){
                nextState.IF.nop = true;
            }

			myRF.outputRF(cycle); // Print RF 
			printState(state, cycle); // Print states after executing cycle 0, cycle 1, cycle 2 ... 
			
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
    		    printstate << "State after executing cycle:\t" << cycle << endl; 

    		    printstate << "IF.PC:\t" << state.IF.PC.to_ulong() << endl;
    		    printstate << "IF.nop:\t" << state.IF.nop << endl;
    		}
    		else cout << "Unable to open SS StateResult output file." << endl;
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
    DataMem dmem_ss = DataMem("SS", ioDir);
	SingleStageCore SSCore(ioDir, imem, &dmem_ss);

    while (1) {
		if (!SSCore.halted)
			SSCore.step();

		if (SSCore.halted)
			break;
    }
    
	SSCore.ext_dmem -> outputDataMem();     // Dump SS data mem

	return 0;
}