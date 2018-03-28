#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <map>
#include <cstring>
#define STRING_LENGTH 100
#define REG_NUM 8

using namespace std;

typedef enum  {add, addi, sub, mul, div_1, b, beq, bnq} INSTR_TYPE;

typedef enum  {if_stage, ex_stage, wb_stage} PIPELINE;

typedef struct {

	string opc, opr1, opr2, opr3;
	
	int reg1, reg2, reg3; //need to be re-used when pipelining => put them into the member group

	INSTR_TYPE instr_type;
	
	int comp_cycle = 0; //0 means no-op; at least need 1 cycle to complete

} INSTR;

class solution {

	private:

		//variables
		bool DEBUG;
		int clck;
		vector<string> vect_lines;
		vector<int>* t_vars;

		int cycle_counter;
		vector<INSTR> instrs;
		map<string, int> label_map; //key: "label1", value: instruction #
		map<string, int>::iterator iter;
		bool ready_bit[REG_NUM];
		int temp_result[REG_NUM];
		int stall_cycles;

		//functions
		int opr_to_value(string opr); //reg: return reg #, immediate: return constant
		void fetch(INSTR &instr);
		int execute(INSTR &instr);
		void write_back(INSTR &instr);
		INSTR_TYPE opc_to_type(string opc);
		void print_register(void);
		void parse_vect_line(int i, INSTR &instr);

	public :

		solution(ifstream &file_in,int clck_in = 10 ,bool DEBUG_in = false);
		void dbg(const string &msg);
		vector<int>* alu();
		int mips_clock();

};

int solution::mips_clock() {

	chrono::milliseconds timespan(clck);

	this_thread::sleep_for(timespan);

	static int cycle = 0;

	if (cycle == 0 )
		cycle = 1;
	else
		cycle = 0;
	return cycle;

}

solution::solution(ifstream &file_in,int clck_in /* = 10*/ ,bool DEBUG_in /*= false*/){ //read the file and set up initial values
		
		  t_vars = new vector<int>();

	  	  clck = clck_in;

	  	  DEBUG = DEBUG_in;

		  char input[STRING_LENGTH];

		  int i = 0;

		  if (!file_in.is_open()) printf("Fail to open the file.\n");
		  else{

			  while(file_in.getline(input,sizeof(input),'\n')){

		    	  vect_lines.push_back (input);
		    	  //cout << vect_lines[i] << endl;
		    	  i++;

		    } //while((ifile.getline(trace,sizeof(trace),'\n'))&&(i<TRACE_COUNT))

		    file_in.close();

		  } //else of if (!ifile.is_open())

		  stringstream ss(vect_lines[0]);
		  string reg;

		  int x;

		  while (getline(ss, reg, ',')) {

			  char *reg_value;

			  reg_value = new char[reg.length() + 1];

			  size_t length = reg.copy(reg_value,reg.length());

			  reg_value[reg.length()] = '\0';

			  x = atoi(reg_value);

			  t_vars->push_back(x);

			  delete[] reg_value;

		  }

		  print_register();
		  
		  //initialize some variables
		  stall_cycles = 0;
		  
		  for(i = 0; i < REG_NUM; i++){

			  ready_bit[i] = 1; //because register values are initialized, all register values are ready
			  temp_result[i] = t_vars->at(i);
		  }

}

vector<int>* solution::alu(){

	  int instrs_num = vect_lines.size();
	  int last_instr = instrs_num - 2;
	  
	  //printf("Total # of instrs: %d\n", instrs_num);

	  int cycle_counter = 0; //actual cycle

	  int op_result = -1;

	  int i = 0; //instruction #, start from 0

	  INSTR instr;

	  instr.opc = "initialization";

	  instrs.push_back(instr);

	  for(i = 1; i < instrs_num - 1; i++){

		  parse_vect_line(i, instr);
		  instrs.push_back(instr);
		  //cout << "#" << i << " " << instrs[i].opc << " " << instrs[i].opr1 << "," << instrs[i].opr2 << "," << instrs[i].opr3 << endl;
	  }

	  //printf("0\n");

	  i = 1;
	  
	  int IF = -1, EX = -1, WB = -1;

	  printf("\n");
	  
	  while(i < instrs_num - 1){ //the last instrs is "end", so need to reduce by 1
	  
		  	  if(!mips_clock()) {
		  		  
		  		  continue;
		  	  
		  	  } 
		  	  else {
		  	  // process instructions
		  		  
		  		  //pipeline: This is a 3 stage pipeline with Fetch, Execute and Write-Back as the only stages. 

		  		  if(instrs[EX].instr_type == b || instrs[EX].instr_type == beq || instrs[EX].instr_type == bnq){ //means last cycle's EX is branch

		  			  //do not assign EX to WB
		  			  instrs[EX].comp_cycle = cycle_counter - 1;
		  			  WB = -1;

		  		  }
		  		  else {

		  			  WB = EX;

		  		  }
		  		  
		  		  if(EX == last_instr){

		  			  EX = -1;
		  		  }
		  		  else{

		  			  EX = IF;
		  		  }

		  		  if(stall_cycles > 0){
			  			  
			  			IF = -1; //do not fetch any instruction
			  			stall_cycles--;
			  			  
			  	  }
			  	  else{

			  		    IF = i;
				  		if(i != last_instr) i++; //in normal case, fetch next instruction in the next cycle

				  		if(EX == last_instr || WB == last_instr) IF = -1;

			  	  }

			  	  //perform pipelining
		  		  if(WB > 0) {

		  			  	write_back(instrs[WB]);
		  			    instrs[WB].comp_cycle = cycle_counter;
		  			    if(WB == last_instr) i++; //in normal case, fetch next instruction in the next cycle

		  		  }

			  	  if(EX > 0){
			  			  
			  			op_result = execute(instrs[EX]);
			  			
				  		if(op_result >= 0)  {

				  			i = op_result; //in case of branch, jump to the returned label

				  		}

			  	  }

		  		  if(IF > 0) fetch(instrs[IF]);

		  		  //print the current stages in the pipeline
		  		  printf("clock cycle: %d\n", cycle_counter);

		  		  if(IF > 0){

				  		printf("Fetch: ");
				  	    cout << vect_lines[IF] << endl;

		  		  }

			  	  if(EX > 0){

				  		printf("Execute: ");
				  	    cout << vect_lines[EX] << endl;

			  	  }

			  	  if(WB > 0){

				  		printf("Write_back: ");
					  	cout << vect_lines[WB] << endl;
					  			
					  	print_register(); //according to the sample provided by TA, print register only in the write_back stage

			  	   }
		  			
			  	   printf("\n");
			  		  
			  	   cycle_counter++;

		  	  } //else of if(!mips_clock())

	  } //while(i < instrs_num - 1)

	  printf("clock cycle: %d\n\n", cycle_counter);

	  return t_vars;

}

void solution::write_back(INSTR &instr){

	  t_vars->at(instr.reg1) = temp_result[instr.reg1]; //write value to the register

}

void solution::fetch(INSTR &instr){ //return -1 or next instruction
	
	string temp;

	temp = instr.opc;
	
	int x = 0, y = 0; //x: cycle when an input is used, y: cycle when the input is ready
	
	if(temp[0] != 'b'){ //ALU
		
		ready_bit[instr.reg1] = 0; //announce the register will be written out!

		if(ready_bit[instr.reg2] == 0 || ready_bit[instr.reg3] == 0) {
			
			x = ex_stage - if_stage - 1; //1 is one cycle backwards from the completion of the execution
			y = ex_stage - if_stage - 1; //1 is one cycle backwards from the start of fetching the instruction
			
			if(y > x) //will use the input in x cycle, and the input is ready in at least y cycles
			
			stall_cycles = y-x;
			
		}

	}
	else if(temp == "b"){ //unconditional branch

		//printf("Case: b\n");
		stall_cycles = ex_stage - if_stage; 

	}
	else if(temp == "beq" || temp == "bnq"){
		
		stall_cycles = 0;
		
		if(ready_bit[instr.reg1] == 0 || ready_bit[instr.reg2] == 0) {
			
			x = ex_stage - if_stage - 1; //1 is one cycle backwards from the completion of the execution
			y = ex_stage - if_stage - 1; //1 is one cycle backwards from the start of fetching the instruction
			
			if(y > x) //will use the input in x cycle, and the input is ready in at least y cycles
			
			stall_cycles = y-x;
			
		}
		
		stall_cycles = stall_cycles + ex_stage - if_stage; 
		
	}
	
	return;

}

int solution::execute(INSTR &instr){ //return -1 or next instruction

	string temp;

	temp = instr.opc;

	int value1, value2, value3;

	if(temp[0] != 'b'){ //ALU

		//printf("Case: ALU\n");

		//all values should be ready after stall; if not, throw an error
		if(ready_bit[instr.reg2] == 0 || ready_bit[instr.reg3] == 0){
			
			if(instr.reg1 != instr.reg2 && instr.reg1 != instr.reg3) //ignore
			{

				printf("Error: there is problem about ready bit handling!\n");
				return -1;

			}
			
		}
		
		value1 = 0;

		value2 = temp_result[instr.reg2]; //from data forwarding
		
		if(temp == "addi") value3 = opr_to_value(instr.opr3);
		else value3 = temp_result[instr.reg3];
		
		//cout << "Before:" << endl;
		//cout << "reg1 " << reg1 << " ,reg2 " << reg2 << " ,reg3 " << reg3 << endl;
		//cout << "value1 " << value1 << " ,value2 " << value2 << " ,value3 " << value3 << endl;

		switch(instr.instr_type){

			case add:
			case addi:
				value1 = value2 + value3;
				break;

			case sub:
				value1 = value2 - value3;
				break;

			case mul:
				value1 = value2 * value3;
				break;

			case div_1:
				if(value3 == 0) printf("Exception: divided by 0\n");
				else value1 = value2 / value3;
				break;
		}
		
		temp_result[instr.reg1] = value1;
		
		ready_bit[instr.reg1] = 1; //data forwarding

		//cout << "After:" << endl;
		//cout << "reg1_value " << t_vars_1[reg1] << " reg2_value " << t_vars_1[reg2] << " reg3_value " << t_vars_1[reg3] << endl;

		return -1;

	}
	else if(temp == "b"){ //unconditional branch

		//printf("Case: b\n");

		iter = label_map.find(instr.opr1);

		return iter->second;

	}
	else if(temp == "beq" || temp == "bnq"){

		//printf("Case: beq/bnq\n");
		
		//all values should be ready after stall; if not, throw an error
		if(ready_bit[instr.reg1] == 0 || ready_bit[instr.reg2] == 0){
			
			printf("Error: there is problem about ready bit handling!\n");
			return -1;
			
		}

		value1 = temp_result[instr.reg1];

		value2 = temp_result[instr.reg2];

		//cout << "reg1 " << reg1 << " reg2 " << reg2 << endl;
		//cout << "value1 " << value1 << " value2 " << value2 << endl;

		if((temp == "beq") && (value1 == value2)) {

			iter = label_map.find(instr.opr3);

			return iter->second;

		}

		if((temp == "bnq") && (value1 != value2)) {

			iter = label_map.find(instr.opr3);

			return iter->second;

		}
		
		return -1;

	} //else if(temp == "beq" || temp == "bnq")

}

INSTR_TYPE solution::opc_to_type(string opc){

	INSTR_TYPE instr_type;

	if(opc == "add")  instr_type = add;
	if(opc == "addi")  instr_type = addi;
	if(opc == "sub")  instr_type = sub;
	if(opc == "mul")  instr_type = mul;
	if(opc == "div")  instr_type = div_1;
	if(opc == "b")  instr_type = b;
	if(opc == "beq")  instr_type = beq;
	if(opc == "bnq")  instr_type = bnq;

	return instr_type;

}

void solution::print_register(void){

	int i = 0;

	for(i = 0; i < t_vars->size()-1 ; i++){
		cout << t_vars->at(i) << ",";
	}
	cout << t_vars->at(i) << endl;

}

void solution::parse_vect_line(int i, INSTR &instr){

      stringstream ss(vect_lines[i]);
      string temp;

	  getline(ss, temp, ' ');

	  instr.opc = temp;
	  instr.instr_type = opc_to_type(temp);

	  if(temp == "b"){ //b label

		    getline(ss, temp);

		    instr.opr1 = temp; //for "b", opr1 is the label's name
		    
		    instr.opr2 = "NULL"; 
		    
		    instr.opr3 = "NULL"; 

	  }
      else{

		     char* y;
		     y = new char[6];

		     for(int j = 0; j < 5; j++) {
		    	 y[j] = temp[j];
		     }
		     y[5]='\0';

		     if(strcmp(y, "label") == 0){

		    	  label_map[temp] = i;
		    	  
		    	  //iter = label_map.find(temp);

		  		  //cout << temp << " is mapped to " << iter->second << endl;

		    	  getline(ss, temp, ' ');

		      }

		      delete[] y;

		      instr.opc = temp;
		      instr.instr_type = opc_to_type(instr.opc);

		      getline(ss, temp, ',');
		      instr.opr1 = temp;

		      getline(ss, temp, ',');
		      instr.opr2 = temp;

		      getline(ss, temp);
		      instr.opr3 = temp; //for beq and bnq, opr3 is label name

		      //cout << instr.opc << " " << instr.opr1 << "," << instr.opr2 << "," << instr.opr3 << endl;
		      
	    	  instr.reg1 = opr_to_value(instr.opr1);
	    	  
	    	  instr.reg2 = opr_to_value(instr.opr2);
		      
		      if(instr.instr_type == addi || instr.instr_type == beq || instr.instr_type == bnq){
		    	  
		    	  instr.reg3 = 0; //useless
		    	  
		      }
		      else{
		    	  
		    	  instr.reg3 = opr_to_value(instr.opr3);
		    	  		    	  
		      }

	   } //else of if(temp == "b")

}

int solution::opr_to_value(string opr){

	char *temp;
	int x, i;

	if(opr[0] == '$'){

		temp = new char[opr.length()];

		for(i = 1; i <= opr.length() - 1 ; i++) temp[i-1] = opr[i];

		temp[opr.length() - 1] = '\0';

	}
	else {

		temp = new char[opr.length() + 1];

		for(i = 0; i <= opr.length() - 1 ; i++) temp[i] = opr[i];

		temp[opr.length()] = '\0';

	}

	x = atoi(temp);

	//cout << "String to Int: " << temp << "->" << x << endl;

	delete[] temp;

	return x;

}


