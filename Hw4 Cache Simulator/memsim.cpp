#include <fstream>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <stdio.h>
#include <cstring>
#include <cmath>
#include <unordered_set>

#define ADDRESS_SIZE 32

using namespace std;

typedef enum {Read_Hit, Read_Miss, Write_Hit, Write_Miss} HIT_TYPE;
typedef enum {NEW_MRU, NEW_LRU, LRU_OUT, SWAP, STAY} REPLACEMENT_TYPE;
typedef enum {MRU, LRU} WAY;
typedef struct {
	
	bool valid;
	int tag_value;	
	
} CACHE;

void change_address_base(char address_16[], char address_2[]);
void change_base_16_2(char bit_16, char bit_2[]);
int change_base_2_10(char address_2[], int address_bits);

int main(int argc, char* argv[]) 
{
  char *test_file_name = argv[1];

  int cache_size = 4096*1024, block_size = 16, associativity = 2;
  int offset_bits = (int)log2((double)block_size), block_num = (int)((double) cache_size / block_size);
  int index_num =  (int)((double) block_num / associativity), index_bits = (int)log2((double) index_num);
  int tag_bits = ADDRESS_SIZE - offset_bits - index_bits;
  int tag_max = (int)pow (2.0, (double) tag_bits) - 1;
  int hex_bits = (int)((double)ADDRESS_SIZE / 4);

  /*
    if((block_size * index_num * associativity == cache_size) && (ADDRESS_SIZE == (tag_bits + index_bits + offset_bits))){
    printf("Address (%d bits): tag %d, index %d, offset %d\n", ADDRESS_SIZE, tag_bits, index_bits, offset_bits);
  }
  else{
    printf("Some formula is wrong!!!Please check!!!\n");  
  }
    
  printf("\n");
    */
  
  CACHE cache[index_num][associativity];
  
  for(int j = 0; j < index_num; j++){
	  for(int k = 0; k < associativity; k++){
		  
		  cache[j][k].valid = 0;
		  cache[j][k].tag_value = -1;
	  }
  }
 
  FILE *pFile;
  char trace[hex_bits + 1]; //include '\0'
  char reference_type[1 + 1];
  char address[ADDRESS_SIZE], tag[tag_bits], index[index_bits], offset[offset_bits];
  char *bit_position;

  int i = 0, index_value, tag_value;
  int store_failure = 0, swap_failure = 0, invalid_tag = 0, invalid_index = 0;
  bool replacement;
  HIT_TYPE hit_type;
  REPLACEMENT_TYPE replacement_type;
  unordered_set<int> index_set;
  int Read_Hit_Count = 0, Read_Miss_Count = 0, Write_Hit_Count = 0, Write_Miss_Count = 0, New_MRU_Count = 0; 
  
  ifstream ifile (test_file_name,ios::in);
  
  if (!ifile.is_open()) printf("Fail to open the file.\n");
  else{
    while(ifile.getline(trace,sizeof(trace),' ')){
       
        if(strlen(trace) < hex_bits){
        	
        	printf("Catch an invalid trace. Please check.\n");
        	break;
        }
        else i++; //start from 1
        
        ifile.getline(reference_type,sizeof(reference_type));
        
        //cout << "#" << i << " trace: " << trace << " " << reference_type << endl;
        
        //translate base 16 address to base 2 address
        change_address_base(trace, address);
        bit_position = address;
        
        memcpy(tag, bit_position, tag_bits);
        bit_position = bit_position + tag_bits;
                
        memcpy(index, bit_position, index_bits);
        bit_position = bit_position + index_bits;
        
        memcpy(offset, bit_position, offset_bits);
        
        index_value = change_base_2_10(index, index_bits);
        if((index_value < 0)||(index_value > index_num - 1)) invalid_index++;
        index_set.insert(index_value);  
        
        tag_value = change_base_2_10(tag, tag_bits);
        if((tag_value < 0)||(tag_value > tag_max)) invalid_tag++;

		/*
        cout << "Index: " << index << ", Index Value: " << index_value << "; Tag: " << tag << ", Tag Value: " << tag_value << endl;
        
        printf("Before memory reference: ");
        
        if(cache[index_value][MRU].valid == 1){
            
            printf("MRU[%d]: %d ", index_value, cache[index_value][MRU].tag_value);   
            if((cache[index_value][MRU].tag_value < 0)||(cache[index_value][MRU].tag_value > tag_max)) invalid_tag++;
        }
       	
        if(cache[index_value][LRU].valid == 1){
        
           	printf("LRU[%d]: %d ", index_value, cache[index_value][LRU].tag_value);   
        	if((cache[index_value][LRU].tag_value < 0)||(cache[index_value][LRU].tag_value > tag_max)) invalid_tag++;
        }
 
        printf("\n");
        */
       
        replacement = 0;
        
        if(reference_type[0] == 'R'){

        	//read hit
        	if(((cache[index_value][MRU].valid == 1)&&(cache[index_value][MRU].tag_value == tag_value))
        			||((cache[index_value][LRU].valid == 1)&&(cache[index_value][LRU].tag_value == tag_value))){
        		
        		Read_Hit_Count++;
        		hit_type = Read_Hit;
        		
        		//if the trace referred is in LRU, move it to MRU
          		if((cache[index_value][LRU].valid == 1)&&(cache[index_value][LRU].tag_value == tag_value)){
        			
        			if((cache[index_value][MRU].valid == 1)&&(cache[index_value][MRU].tag_value == tag_value)){
        			    printf("Duplicate traces ");
        			    cout << tag;
               			printf(" in Index[%d]! Please check!\n", index_value);
               			break;
           			}
           			else{
           				cache[index_value][LRU].tag_value = cache[index_value][MRU].tag_value;
           				cache[index_value][MRU].tag_value = tag_value;
           				cache[index_value][MRU].valid = 1;
           				replacement_type = SWAP;
           			}

        		} //if((cache[index_value][LRU].valid == 1)&&(strcmp(cache[index_value][LRU].tag_value,tag) == 0))
          		else replacement_type = STAY;
          			
        	} //if((strcmp(cache[index_value][MRU].tag_value,tag) == 0)||(strcmp(cache[index_value][LRU].tag_value,tag) == 0))
        	else{ //read miss
        		
        		Read_Miss_Count++;
        		hit_type = Read_Miss;
        		
        		//perform replacement policy
        		replacement = 1;
        	}
        } //if(reference_type = 'R')
        else if(reference_type[0] == 'W'){ //if(reference_type = 'W')

        	//write hit
        	if(((cache[index_value][MRU].valid == 1)&&(cache[index_value][MRU].tag_value == tag_value))
        			||((cache[index_value][LRU].valid == 1)&&(cache[index_value][LRU].tag_value == tag_value))){
        		
        		Write_Hit_Count++;
        		hit_type = Write_Hit;
        		
        		//if the trace referred is in LRU, move it to MRU
          		if((cache[index_value][LRU].valid == 1)&&(cache[index_value][LRU].tag_value == tag_value)){
        			
        			if((cache[index_value][MRU].valid == 1)&&(cache[index_value][MRU].tag_value == tag_value)){
        			    printf("Duplicate traces ");
        			    cout << tag;
               			printf(" in Index[%d]! Please check!\n", index_value);
               			break;
           			}
           			else{
           				cache[index_value][LRU].tag_value = cache[index_value][MRU].tag_value;
           				cache[index_value][MRU].tag_value = tag_value;
           				cache[index_value][MRU].valid = 1;
           				replacement_type = SWAP;
           			}

        		} //if((cache[index_value][LRU].valid == 1)&&(strcmp(cache[index_value][LRU].tag_value,tag) == 0))
          		else replacement_type = STAY;
        		
        	} //if((strcmp(cache[index_value][MRU].tag_value,tag) == 0)||(strcmp(cache[index_value][LRU].tag_value,tag) == 0))
        	else{ //write miss
        		
        		Write_Miss_Count++;
        		hit_type = Write_Miss;
        		
        		//perform replacement policy
        		replacement = 1;
        	}
        } //if(reference_type = 'W')
        else {
        	printf("Catch an invalid reference type. Please check.\n");
        	break;
        }
        
        //Read miss or wirte miss
        if(replacement == 1){
        
        	 if(cache[index_value][MRU].valid == 0){
        
        		cache[index_value][MRU].tag_value = tag_value;
                cache[index_value][MRU].valid = 1;
                replacement_type = NEW_MRU;
                New_MRU_Count++;

             } //if(cache[index_value][MRU].valid == 0)
             else{ //cache[index_value][MRU].valid == 1
                
            	 if(cache[index_value][LRU].valid == 0){
                	
            		 cache[index_value][LRU].tag_value = cache[index_value][MRU].tag_value;
            		 cache[index_value][LRU].valid = 1;
            		 replacement_type = NEW_LRU;
                	
            		 cache[index_value][MRU].tag_value = tag_value;  
                   	
            	 } //if(cache[index_value][LRU].valid == 0)
            	 else{ //if(cache[index_value][LRU].valid == 1)
               		
            		 cache[index_value][LRU].tag_value = cache[index_value][MRU].tag_value;                	
            		 cache[index_value][MRU].tag_value = tag_value;
	                
            		 replacement_type = LRU_OUT;
                	            
            	 } //else of if(cache[index_value][LRU].valid == 0)
    
             } //else of if(cache[index_value][MRU].valid == 0)
        
        } //if(replacement == 1)
        
        //at this point, replacement (if necessary) should be completed
       
        /*
        printf("After memory reference: ");
        
        if(cache[index_value][MRU].valid == 1){
            
            printf("MRU[%d]: %d ", index_value, cache[index_value][MRU].tag_value);   
            if(cache[index_value][MRU].tag_value != tag_value) store_failure++;
        }
        else store_failure++;  
       	
        if(cache[index_value][LRU].valid == 1){
        
            printf("LRU[%d]: %d ", index_value, cache[index_value][LRU].tag_value); 
            if(cache[index_value][MRU].valid == 0) store_failure++;
            if((cache[index_value][LRU].tag_value < 0)||(cache[index_value][LRU].tag_value > tag_max)) invalid_tag++;  
            if(cache[index_value][LRU].tag_value == tag_value) swap_failure++; 
        }
        
        printf("\n");
        
        printf("Reference Summary: hit type %d, replacement type %d\n", hit_type, replacement_type); 
        printf("\n");
        */
                
    } //while((ifile.getline(trace,sizeof(trace),'\n'))&&(i<TRACE_COUNT))
        
    ifile.close();

  } //else of if (!ifile.is_open())
  
  /*
  store_failure = store_failure + abs(index_set.size() - New_MRU_Count);
  
  printf("Error Summary:\n");
  printf("Store Failure %d, Swap Failure %d, Invalid Index %d, Invalid Tag %d\n", store_failure, swap_failure, invalid_index, invalid_tag);
  printf("\n");
  */
  
  if(Read_Hit_Count + Read_Miss_Count + Write_Hit_Count + Write_Miss_Count == i){
	  printf("Total traces: %d\n", i);
	  printf("Reads: %d\n", Read_Hit_Count + Read_Miss_Count);
	  printf("Writes: %d\n", Write_Hit_Count + Write_Miss_Count );
	  printf("Cache hits: %d\n", Read_Hit_Count + Write_Hit_Count);
	  printf("Cache misses: %d\n", Read_Miss_Count + Write_Miss_Count);
  }
  else printf("Count Error\n");
  
  /*
  printf("Read Hit: %d, Read Miss: %d\n", Read_Hit_Count, Read_Miss_Count);
  printf("Write Hit: %d, Write Miss: %d\n", Write_Hit_Count, Write_Miss_Count);
  printf("\n");
  */

  return 0;
}

void change_address_base(char address_16[], char address_2[]){
    
    char *bit_position = address_2;
    char bit_2[] = "0000";
    
    for(int i = 0; i<ADDRESS_SIZE/4 ; i++){
        
    	change_base_16_2(address_16[i], bit_2);        
    	memcpy(bit_position, bit_2, 4);
        bit_position = bit_position + 4;
    } //for(i = 0; i<8 ; i++)
}

void change_base_16_2(char bit_16, char bit_2[]){

    switch(bit_16)
    {
        case '0':
                stpcpy(bit_2 , "0000");
                break;
        case '1':
                stpcpy(bit_2 , "0001");
                break;
        case '2':
                stpcpy(bit_2 , "0010");
                break;                          
        case '3':
                stpcpy(bit_2 , "0011");
                break;
        case '4':
                stpcpy(bit_2 , "0100");
                break;
        case '5':
                stpcpy(bit_2 , "0101");
                break;
        case '6':
                stpcpy(bit_2 , "0110");
                break;
        case '7':
                stpcpy(bit_2 , "0111");
                break;
        case '8':
                stpcpy(bit_2 , "1000");
                break;
        case '9':
                stpcpy(bit_2 , "1001");
                break;
        case 'a':
                stpcpy(bit_2 , "1010");
                break;
        case 'b':
                stpcpy(bit_2 , "1011");
                break;
        case 'c':
                stpcpy(bit_2 , "1100");
                break;
        case 'd':
                stpcpy(bit_2 , "1101");
                break;
        case 'e':
                stpcpy(bit_2 , "1110");
                break;
        case 'f':
                stpcpy(bit_2 , "1111");
                break;
        default:
        	cout << "Unknown character!!!" << endl;
    } //switch(bit_16)
}

int change_base_2_10(char address_2[], int address_bits){ 

    int number_10 = 0;
    bool bit_value;
    
    for(int i=0; i<address_bits; i++){

    	if(address_2[address_bits-1-i] == '1') {
    		bit_value = 1;
    	}
    	else if(address_2[address_bits-1-i] == '0'){
    		bit_value = 0;
    	}
    	else printf("Invalid bit value at bit %d\n", i);

    	number_10 = number_10 + bit_value * (int)pow(2.0, (double) i) ;
    
    }//for(int i=0; i<address_bits-1; i++)

    return number_10;
}

