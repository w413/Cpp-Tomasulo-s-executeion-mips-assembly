//	Name: Wayne Benge
//	Date: 19 November, 2017
//	Program 4: Tomasulo's Scheduling and execution

#include <iostream>
#include <string>
#include <iomanip>
#include <map>
#include <fstream>
#include <sstream>

std::string whichReg(const unsigned int var)
{
	std::string ret[32] =
	{
		"$0", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3", "$t0", "$t1", 
		"$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$s0", "$s1", "$s2", "$s3",
		"$s4", "$s5", "$s6", "$s7", "$t8", "$t9", "$k0", "$k1",	"$gp", "$sp",
		"$fp", "$ra"
	};
	return ret[var];
}

std::string disassemble(const unsigned int var) {
	unsigned int s, t, i, buffer;
	std::string str;
	std::stringstream ss;
	
	// get the first 6 bits to identify the 32-bit instruction
	buffer = var >> 26;
	
	// select the instruction and parse arguments
	switch (buffer) 
	{
		case  0:
			str = "syscall\t\t";
			return str;
			
		case  2:
			str = "j\t";
			i = var & 0x03FFFFFF; 
			ss << std::setfill('0') << std::setw(8) << std::hex << i;	// Convert unsigned int into hex string using stringstream object
			str += "0x" + ss.str();						// Concatinate 0x and hex string to instruction
			return str;
			
		case  4:
			str = "beq\t";
			s = (var & 0x03E00000) >> 21;				// Bit logic to obtain register value s
			t = (var & 0x001f0000) >> 16;				// Same as above for register value t
			i = (var & 0x0000ffff);						// Bit logic to find immediate value
			ss << i;									// Update string stream object with new immediate integer value
			str += whichReg(s) + ", " + whichReg(t) + ", " + ss.str();			
			return str;
			
		case  8:
			str = "addi\t";
			s = (var & 0x03E00000) >> 21;				// Bit logic to obtain register value s
			t = (var & 0x001f0000) >> 16;				// Same as above for register value t
			i = (var & 0x0000ffff);						// Bit logic to find immediate value
			ss << i;
			str += whichReg(t) + ", " + whichReg(s) + ", " + ss.str();		
			return str;
			
		case 13:
			str = "ori\t";
			s = (var & 0x03E00000) >> 21;				// Bit logic to obtain register value s
			t = (var & 0x001f0000) >> 16;				// Same as above for register value t
			i = (var & 0x0000ffff);						// Bit logic to find immediate value
			ss << i;									// Update string stream object with new immediate integer value
			str += whichReg(t) + ", " + whichReg(s) + ", " + ss.str();			
			return str;
			
		case 15:
			str = "lui\t";
			t = (var & 0x001f0000) >> 16;				// Bit logic to obtain register value t
			i = (var & 0x0000ffff);						// Bit logic to find immediate value
			ss << i;									// Update string stream object with new immediate integer value
			str += whichReg(t) + ", " + ss.str();
			return str;
			
		case 35:
			str = "lw\t";
			s = (var & 0x03E00000) >> 21;				// Bit logic to obtain register value s
			t = (var & 0x001f0000) >> 16;				// Same as above for register value t
			i = (var & 0x0000ffff);						// Bit logic to find immediate value
			ss << i;									// Update string stream object with new immediate integer value
			str += whichReg(t) + ", " + ss.str() + "(" + whichReg(s) + ")";	
			return str;
			
		case 43:
			str = "sw\t";
			s = (var & 0x03E00000) >> 21;				// Bit logic to obtain register value s
			t = (var & 0x001f0000) >> 16;				// Same as above for register value t
			i = (var & 0x0000ffff);						// Bit logic to find immediate value
			ss << i;									// Update string stream object with new immediate integer value
			str += whichReg(t) + ", " + ss.str() + "(" + whichReg(s) + ")";
			return str;
			
		default:
			return "\0";
	}
}

class RegisterTracker {
	unsigned int visibleRegister[32];
	unsigned int physicalRegister[1024];
	bool available[1024];
	
	public:
		RegisterTracker() {
			for (int i=0; i<32; i++)
				visibleRegister[i] = 2000;
			for (int i=1; i<1024; i++) {
				physicalRegister[i] = 0;
				available[i] = true;
			}
			physicalRegister[0] = 0;
			available[0] = false;
		}
		
		bool isAvailable (unsigned int index) {
			if(index < 1024)
				return available[index];
			else
				return false;
		}
		
		void setAvailable(unsigned int index, bool value) {
			if (index < 1024)
				available[index] = value;
		}
		
		unsigned int AVRassign (unsigned int index) { // assigns an architectually visible register to a physical register
			if (index < 32) {
				if (visibleRegister[index] < 1024)
					PRrelease(visibleRegister[index]);
				unsigned int i;
				for (i=0; i<1024; i++)
					if (isAvailable(i))
						break;
				visibleRegister[index] = i;
				PRcatch(i);
				return i;
			} else
				return 2000;
		}
		
		unsigned int AVR (unsigned int index) {
			if (index < 32) {
				if (visibleRegister[index] == 2000)
					AVRassign(index);
				return visibleRegister[index];
			} else
				return 2000;
		}
		
		unsigned int PRget (unsigned int index) {
			if (index < 1024)
				return physicalRegister[index];
			else
				return 0;
		}
		
		unsigned int AVRget (unsigned int index) {
			if (index < 32)
				return physicalRegister[visibleRegister[index]];
			else
				return 0;
		}
		
		void PRset (unsigned int index, unsigned int value) {
			if (index < 1024)
				physicalRegister[index] = value;
		}
		
		void PRrelease (unsigned int index) {
			if (index < 1024)
				available[index] = true;
		}
		
		void PRcatch (unsigned int index) {
			if (index < 1024)
				available[index] = false;
		}
		
		void AVRrelease (unsigned int index) {
			if (index < 32) {
				unsigned int pIndex = visibleRegister[index];
				PRrelease(pIndex);
			}
		}
};

class Memory {
	std::map <unsigned int, unsigned int> Data;
	
	public:
		Memory() {}
		
		unsigned int firstMember() {
			return Data.begin()->first;
		}
	
		unsigned int get(unsigned int address) {
			if (Data.find(address) == Data.end()) {
  				return 0;
			} else {
  				return Data[address];
			}
		}
		
		bool set(unsigned int address, unsigned int value) {
			if(Data.find(address) == Data.end()) {
				return false;
			} else {
				Data[address] = value;
				return true;
			}
		}

		void create(unsigned int address, unsigned int value) {
			Data[address] = value;
		}

		void dump(std::fstream &f) {
			for (auto& x: Data) {
	    		f << "0x" << std::setw(8) << std::setfill('0') << std::right << std::hex << x.first << "\t" ;
	    		f << "0x" << std::setw(8) << std::setfill('0') << std::right << std::hex << x.second << std::endl;
	    		f << std::dec;
	  		}
		}
		
		
};

class InstructionMemory : public Memory {
};

class DataMemory : public Memory {
	std::map <unsigned int, unsigned int>readTicket;
	public:
		bool read(unsigned int address, unsigned int clockCycle) {
			if (readTicket.find(address) == readTicket.end()) {
				readTicket[address] = clockCycle + 3;
				return false;
			} else if (readTicket[address] <= clockCycle) {
				readTicket.erase(address);
				return true;
			} else {
				return false;
			}
		}
};
			
struct RS_Entry {
	std::string 	OrgInstruction;
	unsigned int	OpCode;
	unsigned int	FunctCode;
	unsigned int	RegA;
	unsigned int	DataA;
	bool			ReadyA;
	bool			RequiredA;
	unsigned int	RegB;
	unsigned int	DataB;
	bool			ReadyB;
	bool			RequiredB;
	bool			RequiredImm;
	unsigned int	DataImm;
	bool			Executed;
	bool			Ready;
	
	RS_Entry & operator=(const RS_Entry& a) {
			
		OrgInstruction = a.OrgInstruction;
		OpCode = a.OpCode;
		FunctCode = a.FunctCode;
		RegA = a.RegA;
		DataA = a.DataA;
		ReadyA = a.ReadyA;
		RequiredA = a.RequiredA;
		RegB = a.RegB;
		DataB = a.DataB;
		ReadyB = a.ReadyB;
		RequiredB = a.RequiredB;
		RequiredImm = a.RequiredImm;
		DataImm = a.DataImm;
		Executed = a.Executed;
		Ready = a.Ready;
		return *this;
	}
};

class MIPS_ISA {

	RS_Entry RS[100];
	const struct RS_Entry Zero = {"", 0, 0, 0, 0, false, false, 0, 0, false, false, true, 0, false, false};
	RegisterTracker Reg;
	InstructionMemory Instruction;
	DataMemory Data;
	unsigned int pc, rsSize, clockCycle;
	bool programContinue;
	
	public:
		
		MIPS_ISA() {
			pc = 0;
			rsSize = 0;
			clockCycle = 0;
			programContinue = true;
			for (int i = 0; i < 100; i++)
				RS[i] = Zero;
		}
		
		void issue() {
			if (pc == 0)
				pc = Instruction.firstMember();
			bool doneIssuing = false;
			do {
				unsigned int mipsInst = Instruction.get(pc);
				unsigned int OpCode = mipsInst >> 26;
				unsigned int FuncCode = mipsInst & 0x0000003f;
				unsigned int Rs = (mipsInst & 0x03E00000) >> 21;
				unsigned int Rt = (mipsInst & 0x001f0000) >> 16;
				unsigned int Imm = (mipsInst & 0x0000ffff);
				unsigned int JumpVal = (mipsInst & 0x03FFFFFF) << 2;
				
				RS[rsSize].OpCode = OpCode;
				RS[rsSize].OrgInstruction = disassemble(mipsInst);
				pc+=4;
				
				switch (OpCode) {
					case 0:// syscall
					//	std::cout << "syscall\n";
						if (FuncCode == 0) {
							RS[rsSize].RegA = Reg.AVR(2);
							RS[rsSize].RequiredA = true;
							RS[rsSize].RequiredImm = false;
							doneIssuing = true;
							rsSize++;				
						}
						break;
					case 2:// j
					//	std::cout << "j\n";
						pc = JumpVal;
						break;
					case 4:// beq
					//	std::cout << "beq\n";
						RS[rsSize].RegA = Reg.AVR(Rs);
						RS[rsSize].RequiredA = true;
						RS[rsSize].RegB = Reg.AVR(Rt);
						RS[rsSize].RequiredB = true;
						RS[rsSize].DataImm = Imm;
						doneIssuing = true;
						rsSize++;
						break;
					case 8:// addi
					//	std::cout << "addi\n";
						if(Rs == 0) {
							RS[rsSize].RegA = 0;
							RS[rsSize].ReadyA = true;
						} else {
							RS[rsSize].RegA = Reg.AVR(Rs);
						}
						RS[rsSize].RequiredA = true;
						RS[rsSize].RegB = Reg.AVRassign(Rt);
						RS[rsSize].DataImm = Imm;
						rsSize++;
						break;
					case 13:// ori
					//	std::cout << "ori\n";
						RS[rsSize].RegA = Reg.AVR(Rs);
						RS[rsSize].RequiredA = true;
						RS[rsSize].RegB = Reg.AVRassign(Rt);
						RS[rsSize].DataImm = Imm;
						rsSize++;
						break;
					case 15:// lui
					//	std::cout << "lui\n";
						RS[rsSize].RegB = Reg.AVRassign(Rt);
						RS[rsSize].DataImm = Imm;
						rsSize++;				
						break;
					case 35:// lw
					//	std::cout << "lw\n";
						RS[rsSize].RegA = Reg.AVR(Rs);
						RS[rsSize].RequiredA = true;
						RS[rsSize].RegB = Reg.AVRassign(Rt);
						RS[rsSize].DataImm = Imm;
						rsSize++;
						break;
					case 43:// sw
					//	std::cout << "sw\n";
						RS[rsSize].RegA = Reg.AVR(Rs);
						RS[rsSize].RequiredA = true;
						RS[rsSize].RegB = Reg.AVR(Rt);
						RS[rsSize].RequiredB = true;
						RS[rsSize].DataImm = Imm;
						rsSize++;
						break;
					default:
							break;
				}
							
			} while (!doneIssuing);
			
		}
		
		void execute(unsigned int rsIndex, unsigned int &instructionsExecuted) {
			switch (RS[rsIndex].OpCode) {
					case 0:// syscall
					//	std::cout << RS[rsIndex].OrgInstruction << std::endl;
						if (Reg.AVRget(2) == 10) {
							programContinue = false;
							RS[rsIndex].Executed = true;
							instructionsExecuted++;
						}
						return;
					case 4:// beq					
					//	std::cout << RS[rsIndex].OrgInstruction << std::endl;
						RS[rsIndex].DataA = Reg.PRget(RS[rsIndex].RegA);
						RS[rsIndex].DataB = Reg.PRget(RS[rsIndex].RegB);
						if (RS[rsIndex].DataA == RS[rsIndex].DataB)
							pc += 4 * RS[rsIndex].DataImm;
						RS[rsIndex].Executed = true;
						instructionsExecuted++;
						break;
					case 8:// addi
					//	std::cout << RS[rsIndex].OrgInstruction << std::endl;
						RS[rsIndex].DataA = Reg.PRget(RS[rsIndex].RegA);													// Get data from physical register indexed by RegA
						RS[rsIndex].DataB = 
						RS[rsIndex].DataA + RS[rsIndex].DataImm;										// Add DataA to DataImm and store in DataB
						Reg.PRset(RS[rsIndex].RegB, RS[rsIndex].DataB);														// Store data in DataB to physical register indexed by RegB
						RS[rsIndex].Executed = true;																		// Set Executed to true
						instructionsExecuted++;																				// Increment the number of instructions executed
						break;
					case 13:// ori
					//	std::cout << RS[rsIndex].OrgInstruction << std::endl;
						RS[rsIndex].DataA = Reg.PRget(RS[rsIndex].RegA);													// Get data from physical register indexed by RegA
						RS[rsIndex].DataB = RS[rsIndex].DataA | RS[rsIndex].DataImm;										// Or DataA with DataImm and store in DataB
						Reg.PRset(RS[rsIndex].RegB, RS[rsIndex].DataB);														// Store data in DataB to physical register indexed by RegB
						RS[rsIndex].Executed = true;																		// Set Executed to true
						instructionsExecuted++;																				// Increment the number of instructions executed
						break;
					case 15:// lui
					//	std::cout << RS[rsIndex].OrgInstruction << std::endl;
						RS[rsIndex].DataB = RS[rsIndex].DataImm	<< 16;														// Shift data in DataImm left 16 bits and store in DataB
						Reg.PRset(RS[rsIndex].RegB, RS[rsIndex].DataB);														// Store data in DataB to physical register indexed by RegB
						RS[rsIndex].Executed = true;																		// Set Executed to true
						instructionsExecuted++;																				// Increment the number of instructions executed
						break;
					case 35:// lw
					//	std::cout << RS[rsIndex].OrgInstruction << std::endl;
						RS[rsIndex].DataA = Reg.PRget(RS[rsIndex].RegA);
						RS[rsIndex].DataB = Reg.PRget(RS[rsIndex].RegB);
						if(Data.read(RS[rsIndex].RegA + RS[rsIndex].DataImm, clockCycle)) {									// Call read member of Data, when data is ready read returns true and stores data in DataB
							RS[rsIndex].DataB = Data.get(RS[rsIndex].DataA + RS[rsIndex].DataImm);
							Reg.PRset(RS[rsIndex].RegB, RS[rsIndex].DataB);													// Store data in DataB to physical register indexed by RegB
							RS[rsIndex].Executed = true;																	// Set Executed to true
							instructionsExecuted++;																			// Increment the number of instructions execute
							break;
						}
						return;
					case 43:// sw
					//	std::cout << RS[rsIndex].OrgInstruction << std::endl;
						RS[rsIndex].DataA = Reg.PRget(RS[rsIndex].RegA);
						RS[rsIndex].DataB = Reg.PRget(RS[rsIndex].RegB);
						Data.set(RS[rsIndex].DataA + RS[rsIndex].DataImm, RS[rsIndex].DataB);
						RS[rsIndex].Executed = true;
						instructionsExecuted++;						
						break;
					default:
						std::cout << "[ERROR] Unknown Instruction\n";
						return;
				}
			}
		
		void nextMatch(unsigned int rsIndex) {
			for (unsigned int i = 1; i + rsIndex < rsSize; i++) {														// For loops to make ready next data
					if (RS[rsIndex].RegA == RS[rsIndex+i].RegA) {
						if (RS[rsIndex+i].RequiredA)
							RS[rsIndex+i].ReadyA = true;
						break;
					} else if (RS[rsIndex].RegA == RS[rsIndex+i].RegB) {
						if (RS[rsIndex+i].RequiredB)
							RS[rsIndex+i].ReadyB = true;
						break;
					}
				}
				for (unsigned int i = 1; i + rsIndex < rsSize; i++) {
					if (RS[rsIndex].RegB == RS[rsIndex+i].RegA) {
						if (RS[rsIndex+i].RequiredA)
							RS[rsIndex+i].ReadyA = true;
						break;
					} else if (RS[rsIndex].RegB == RS[rsIndex+i].RegB) {
						if (RS[rsIndex+i].RequiredB)
							RS[rsIndex+i].ReadyB = true;
						break;
					}
				}
		}
		
		void run(std::fstream &f_Out) { // Either this or the execute members need to mark next registers as ready
			unsigned int instructionsExecuted = 0;
			
			while(programContinue) {
			//	std::cout << "Issuing...\n";
				issue();
			//	std::cout << "Printing Registration Station\n";				
				printRS(f_Out);
			//	std::cout << "Going to plaid...\n";
				while(instructionsExecuted != rsSize) {
			//		std::cout << "Marking Ready...\n";;
					for (unsigned int i = 0; i < rsSize; i++) {
						if ((RS[i].ReadyA == RS[i].RequiredA) && (RS[i].ReadyB == RS[i].RequiredB)) {
							RS[i].Ready = true;
						}
						// If executed, find the next match
						if (RS[i].Executed) {
							nextMatch(i);
						}
					}
			//		std::cout << "Executing marks...\n";
					for (unsigned int i = 0; i < rsSize; i++) {
						if((RS[i].Ready == true) && (RS[i].Executed == false)) {
							execute(i, instructionsExecuted);
							clockCycle++;
							f_Out << "Clock Cycle: " << clockCycle << std::endl;
							printRS(f_Out);
						}
					}
				}				
			}
		}
		
		void printRS(std::fstream& f_Out) {
			f_Out << "Issue\n";
			f_Out << std::setw(12) << std::setfill(' ') << std::left;
			f_Out << "OrgInstruction\t\t";
			f_Out << std::setw(12) << std::setfill(' ') << std::left;
			f_Out << "OpCode";
			f_Out << std::setw(12) << std::setfill(' ') << std::left;
			f_Out << "FunctCode";
			f_Out << std::setw(12) << std::setfill(' ') << std::left;
			f_Out << "RegA";
			f_Out << std::setw(12) << std::setfill(' ') << std::left;
			f_Out << "DataA";
			f_Out << std::setw(12) << std::setfill(' ') << std::left;
			f_Out << "ReadyA";
			f_Out << std::setw(12) << std::setfill(' ') << std::left;
			f_Out << "RequiredA";
			f_Out << std::setw(12) << std::setfill(' ') << std::left;
			f_Out << "RegB";
			f_Out << std::setw(12) << std::setfill(' ') << std::left;
			f_Out << "DataB";
			f_Out << std::setw(12) << std::setfill(' ') << std::left;
			f_Out << "ReadyB";
			f_Out << std::setw(12) << std::setfill(' ') << std::left;
			f_Out << "RequiredB";
			f_Out << std::setw(12) << std::setfill(' ') << std::left;
			f_Out << "RequiredImm";
			f_Out << std::setw(12) << std::setfill(' ') << std::left;
			f_Out << "DataImm";
			f_Out << std::setw(12) << std::setfill(' ') << std::left;
			f_Out << "Executed";
			f_Out << "Ready" << std::endl;

			for ( unsigned int i = 0; i < rsSize; i++ ) {
				f_Out << RS[i].OrgInstruction << "\t";
				f_Out << std::setw(12) << std::setfill(' ') << std::left;
				f_Out << RS[i].OpCode;
				f_Out << std::setw(12) << std::setfill(' ') << std::left;
				f_Out << RS[i].FunctCode;
				f_Out << std::setw(12) << std::setfill(' ') << std::left;
				f_Out << RS[i].RegA;
				f_Out << std::setw(12) << std::setfill(' ') << std::left;
				f_Out << RS[i].DataA;
				f_Out << std::setw(12) << std::setfill(' ') << std::left;
				RS[i].ReadyA ? f_Out << "true" : f_Out << "false";
				f_Out << std::setw(12) << std::setfill(' ') << std::left;
				RS[i].RequiredA ? f_Out << "true" : f_Out << "false";
				f_Out << std::setw(12) << std::setfill(' ') << std::left;
				f_Out << RS[i].RegB;
				f_Out << std::setw(12) << std::setfill(' ') << std::left;
				f_Out << RS[i].DataB;
				f_Out << std::setw(12) << std::setfill(' ') << std::left;
				RS[i].ReadyB ? f_Out << "true" : f_Out << "false";
				f_Out << std::setw(12) << std::setfill(' ') << std::left;
				RS[i].RequiredB ? f_Out << "true" : f_Out << "false";
				f_Out << std::setw(12) << std::setfill(' ') << std::left;
				RS[i].RequiredImm ? f_Out << "true" : f_Out << "false";
				f_Out << std::setw(12) << std::setfill(' ') << std::left;
				f_Out << RS[i].DataImm;
				f_Out << std::setw(12) << std::setfill(' ') << std::left;
				RS[i].Executed ? f_Out << "true" : f_Out << "false";
				f_Out << std::setw(12) << std::setfill(' ') << std::left;
				RS[i].Ready ? f_Out << "true" : f_Out << "false"; f_Out << std::endl;
			}
			f_Out << "\n\n";
		}
		
		void insLoad(unsigned int address, unsigned int instruction) {
			Instruction.create(address, instruction);
		}
		
		void datLoad(unsigned int address, unsigned int data) {
			Data.create(address, data);
		}
		
		void clearRS() {
			for (int i = 0; i < 100; i++)
				RS[i] = Zero;
			rsSize = 0;
		}
		
		void memDump(std::fstream &f) {
			Data.dump(f);
		}
		
		void insDump(std::fstream &f) {
			Instruction.dump(f);
		}
		
};

int main(void) {
	MIPS_ISA myObj;
	std::fstream file;
	
	// Load instructions from MachineInstruction.txt
	file.open("MachineInstructions.txt");
	while (!file.eof()) {
		std::string sBuffer;
		unsigned int address, instruction;
		file >> sBuffer;																		// grab address
		std::stringstream adr(sBuffer);														// update istringstream object with new value
		adr >> std::hex >> address;															// convert from hex to decimal using istringstream
		file >> sBuffer;
		std::stringstream val(sBuffer);
		val >> std::hex >> instruction;
		myObj.insLoad(address, instruction);
	}
	file.close();
	
	// Load data memory
	for (int i = 0; i < 9; i++)
		myObj.datLoad(0x10010000 + (i*4),0x00000000 + i);
		
	file.open("DMDumpBefore.txt");
	myObj.memDump(file);
	file.close();
	
	file.open("Reservations.txt");
	myObj.run(file);	
	file.close();
	
	file.open("DMDumpAfter.txt");
	myObj.memDump(file);
	file.close();
		
	return 0;
}
