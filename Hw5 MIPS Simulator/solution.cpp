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

using namespace std;

typedef enum  {add, addi, sub, mul, div_1, b, beq, bnq} INSTR_TYPE;

typedef struct {

	string opc, opr1, opr2, opr3;

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

		//functions
		int opr_to_value(string opr); //reg: return reg #, immediate: return constant
		int perform_instr(INSTR &instr);
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

		  i = 0;

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

}

vector<int>* solution::alu(){

	  int instrs_num = vect_lines.size();
	  
	  //printf("Total # of instrs: %d\n", instrs_num);

	  int cycle_counter = 1; //actual cycle

	  int op_result;

	  int i = 0; //instruction #, start from 0

	  INSTR instr;

	  instr.opc = "initialization";

	  instrs.push_back(instr);

	  for(i = 1; i < instrs_num - 1; i++){

		  parse_vect_line(i, instr);
		  instrs.push_back(instr);
		  //cout << "#" << i << " " << instrs[i].opc << " " << instrs[i].opr1 << "," << instrs[i].opr2 << "," << instrs[i].opr3 << endl;
	  }

	  printf("0\n");

	  i = 1;
	  
	  
	  while(i < instrs_num - 1){
	  
		  	  if(!mips_clock()) {
		  		  
		  		  continue;
		  	  
		  	  } 
		  	  else {
		  	  // process instructions
		  		  
				  op_result = perform_instr(instrs[i]);
				  instrs[i].comp_cycle = cycle_counter;
				  
				  //print stage status
				  cout << vect_lines[i] << endl;

				  print_register();

				  cout << instrs[i].comp_cycle << endl;

				  cycle_counter++;
					  
				  if(op_result < 0) i++; //-1 => next instruction
				  else i = op_result; //>=0 => jump to instr # (= op_result)

		  	  }

	  } //while(i < instrs_num - 1)

	  printf("end\n");

	  return t_vars;

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

int solution::perform_instr(INSTR &instr){ //return -1 or next instruction

	string temp;

	temp = instr.opc;

	int reg1, reg2, reg3;
	int value1, value2, value3;

	if(temp[0] != 'b'){ //ALU

		//printf("Case: ALU\n");

		reg1 = opr_to_value(instr.opr1); //output
		value1 = 0;

		reg2 = opr_to_value(instr.opr2);
		value2 = t_vars->at(reg2);

		if(instr.instr_type == addi){

			value3 = opr_to_value(instr.opr3);
			reg3 = 0;
		}
		else{

			reg3 = opr_to_value(instr.opr3);
			value3 = t_vars->at(reg3);
		}
		
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
		
		t_vars->at(reg1) = value1;

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

		reg1 = opr_to_value(instr.opr1);

		reg2 = opr_to_value(instr.opr2);

		value1 = t_vars->at(reg1);

		value2 = t_vars->at(reg2);

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


