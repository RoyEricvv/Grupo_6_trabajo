#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#define RESET_VAL_MSB 0xfffd
#define RESET_VAL_LSB 0xfffc

#define FLAG_CARRY     0x01
#define FLAG_ZERO      0x02
#define FLAG_INTERRUPT 0x04
#define FLAG_DECIMAL   0x08
#define FLAG_BREAK     0x10
#define FLAG_CONSTANT  0x20
#define FLAG_OVERFLOW  0x40
#define FLAG_SIGN      0x80

uint16_t branchAdress;

typedef struct {
    uint8_t a, x, y;    // Registros A, X e Y.
    uint16_t pc;        // Program counter.
    uint8_t status;     // Registro de status.
    uint8_t sp;         // Stack pointer.
    uint8_t *mem;       // Memoria.

    long ciclos;        // Cantidad de ciclos totales de ejecucion.
} mos6502_t;

typedef struct {
    uint8_t ins;        // Opcode.
    uint16_t addr;      // Direccion del operando (si corresponde).
    short ciclos;       // Cantidad de ciclos de la instruccionn.
    uint8_t *m;         // Puntero al operando (registro o memoria).(#$22 o $02)
} instruccion_t;


// macros de cálculo de banderas
void zerocalc (uint16_t rpt,mos6502_t *mos) {
    if (rpt & 0x00FF) mos->status &= ~FLAG_ZERO ; 
    else mos->status |=  FLAG_ZERO; 
}

void signcalc(uint16_t rpt,mos6502_t *mos){
    if (rpt & 0x0080) mos->status |= FLAG_SIGN; 
    else mos->status &= ~FLAG_SIGN; 
}

void carrycalc(uint16_t rpt,mos6502_t *mos) {
    if (rpt & 0xFF00) mos->status |= FLAG_CARRY ; 
        else mos->status &= ~FLAG_CARRY ; 
}

void overflowcalc(uint16_t rpt,mos6502_t *mos,uint16_t o) {/* n = resultado, m = acumulador(mos->a), o = memoria */ 
    if (((rpt)^(uint16_t)(mos->a))&((rpt)^(o)) & 0x0080) mos->status |= FLAG_OVERFLOW; 
    else mos->status &= ~FLAG_OVERFLOW; 
}

void init_memory(mos6502_t *mos){
    mos->mem = (uint8_t*)malloc(sizeof(uint8_t)*256*256);
}

void reset_cpu(mos6502_t *mos){
    mos->mem[RESET_VAL_MSB] = 0x06;
    mos->mem[RESET_VAL_LSB] = 0x00;

    uint8_t pch = mos->mem[RESET_VAL_MSB];
    uint8_t pcl = mos->mem[RESET_VAL_LSB];
    mos->pc = (pch * 256) + pcl;
}

//Etapas Instrucciones://///////////////////////////////////////
void fetch(mos6502_t *mos, instruccion_t *instr){
    instr->ins = mos->mem[mos->pc]; //instr = opcode de la instruccion
    (mos->pc)++;
}

void decode(mos6502_t *mos, instruccion_t *instr){

    //Opcodes de la instruccion LDA
    uint8_t aux = instr->ins;
    uint16_t dirCompleta, auxDir;
    
    if(aux==0xa9||aux==0xa5 ||aux==0xb5||aux==0xa2||aux==0xa6||aux==0xb6||aux==0xa0||aux==0xa4||aux==0xb4||aux==0x4a||aux==0x46||aux==0x56
       ||aux==0x09||aux==0x05||aux==0x15|| aux== 0xC6 || aux== 0xD6 || aux== 0xE6 || aux== 0xF6 || aux== 0x84 || aux== 0x94 || aux== 0x86 ||aux== 0x96
            ||aux== 0x29 || aux== 0x25 || aux == 0x35|| aux == 0x49|| aux == 0x45|| aux == 0x55|| aux == 0x85|| aux == 0x95){
    	instr->m = &(mos->mem[mos->pc]);
        (mos->pc)++;
    }else if( aux==0xad || aux==0xbd || aux==0xb9 ||aux==0xae||aux==0xbe||aux==0xac||aux==0xbc||aux==0x4e||aux==0x5e||aux==0x0d||aux==0x1d
              ||aux==0x19 || aux==0xa1 || aux==0xb1 || aux==0xc6 || aux==0xd6 || aux==0xce || aux==0xde || aux==0xee || aux==0xfe || aux==0x8c || aux==0x8e
            || aux== 0x2d || aux== 0x3d || aux==0x39|| aux==0x4d|| aux==0x5d|| aux==0x59|| aux==0x8d|| aux==0x9d|| aux==0x99){
        auxDir = mos->mem[mos->pc++];
        dirCompleta = mos->mem[mos->pc];
        dirCompleta *=256;
        dirCompleta += auxDir;
        instr->addr = dirCompleta;
        (mos->pc)++;
    }
    else {
      (mos->pc)++;
      return;
    }
    
   
}





void execute(mos6502_t *mos,instruccion_t *instruc){
    uint16_t aux, rpt=-1;
  
    switch(instruc->ins){
                //LDA:///////////////////////////
         case 0xa9: //Inmediato
            mos->a = instruc->m[0]; 
            break;
        case 0xa5: //Zero Page
            aux = instruc->m[0];
            if(0x0000<=aux && aux<=0x00ff){
                mos->a = mos->mem[aux];
            }
            break;
        case 0xb5: //Zero Page, X
            aux = mos->x + instruc->m[0];
            if(0x0000<=aux && aux<=0x00ff){
                mos->a = mos->mem[aux];
            }
            break;
        case 0xad: //Absoluto
            aux = instruc->addr;
            mos->a = mos->mem[aux];
            break;
        case 0xbd: //Absoluto X
            aux = mos->x + instruc->addr;
            mos->a = mos->mem[aux];
            break;
        case 0xb9: //Absoluto Y
            aux = mos->y + instruc->addr;
            mos->a = mos->mem[aux];
            break;
        
        //////////////////////////////////////
        
        
        //LDX:///////////////////////////
        case 0xa2: //Inmediato
            mos->x = instruc->m[0]; 
        break;
            
        case 0xa6: //ZeroPage
            aux = instruc->m[0];
            if(0x0000<=aux && aux<=0x00ff){
                mos->x = mos->mem[aux];
            }
        break;   
       
        case 0xb6: //ZeroPageY
            aux = mos->y + instruc->m[0];
            if(0x0000<=aux && aux<=0x00ff){
                mos->x = mos->mem[aux];
            }
        break;
        
        case 0xae: //Absoluto
            aux = instruc->addr;
            mos->x = mos->mem[aux];
        break;
        
        case 0xbe: //AbsolutoY
            aux = mos->y + instruc->addr;
            mos->x = mos->mem[aux];
        break;
        ///////////////////////////////////
        
        
        //LDY:///////////////////////////
        case 0xa0: //Inmediato
            mos->y = instruc->m[0];  
        break;
        
        case 0xa4: //ZeroPage
            aux = instruc->m[0];
            if(0x0000<=aux && aux<=0x00ff){
                mos->y = mos->mem[aux];
            }
        break;
        
        case 0xb4: //ZeroPageX
            aux = mos->x + instruc->m[0];
            if(0x0000<=aux && aux<=0x00ff){
                mos->y = mos->mem[aux];
            }
        break;
        
        case 0xac: //Absoluto
            aux = instruc->addr;
            mos->y = mos->mem[aux];
        break;
       
        case 0xbc: //AbsolutoX
            aux = mos->x + instruc->addr;
            mos->y = mos->mem[aux];
        break;
        ///////////////////////////////////
        
        
        //LSR:///////////////////////////
        case 0x4a: //Acumulador
            mos->a = mos->a>>1;
            rpt = mos->a;
            if(rpt&1) mos->status |= FLAG_CARRY;
            else mos->status &= ~FLAG_CARRY;
            zerocalc(rpt,mos);
            signcalc(rpt,mos);
        break;
            
        case 0x46: //ZeroPage
            aux = instruc->m[0];
            if(0x0000<=aux && aux<=0x00ff){
                mos->mem[aux] = mos->mem[aux]>>1;
                rpt = mos->mem[aux];
            }
            if(rpt&1) mos->status |= FLAG_CARRY;
            else mos->status &= ~FLAG_CARRY;
            zerocalc(rpt,mos);
            signcalc(rpt,mos);
        break;
        
        case 0x56: //Zero Page X
            aux = mos->x + instruc->m[0];
            if(0x0000<=aux && aux<=0x00ff){
               mos->mem[aux] = mos->mem[aux]>>1;
               rpt = mos->mem[aux];
            }
            if(rpt&1) mos->status |= FLAG_CARRY;
            else mos->status &= ~FLAG_CARRY;
            zerocalc(rpt,mos);
            signcalc(rpt,mos);
        break;
        
        case 0x4e: //Absoluto
           aux = instruc->addr;
           mos->mem[aux] = mos->mem[aux]>>1;
           rpt = mos->mem[aux];
           if(rpt&1) mos->status |= FLAG_CARRY;
           else mos->status &= ~FLAG_CARRY;
           zerocalc(rpt,mos);
           signcalc(rpt,mos);
        break;
        
        case 0x5e: //Absoluto X
            aux = mos->x + instruc->addr;
            mos->mem[aux] = mos->mem[aux]>>1;
            rpt = mos->mem[aux];
            if(rpt&1) mos->status |= FLAG_CARRY;
            else mos->status &= ~FLAG_CARRY;
            zerocalc(rpt,mos);
            signcalc(rpt,mos);
        break;
        
        
        //ORA:///////////////////////////
        case 0x09: //Inmediato
            mos->a = mos->a | instruc->m[0];
            zerocalc(mos->a,mos);
            signcalc(mos->a,mos);
        break;    
          
        case 0x05: //ZeroPage
            aux = instruc->m[0];
            if(0x0000<=aux && aux<=0x00ff) mos->a = mos->a | mos->mem[aux];
            zerocalc(mos->a,mos);
            signcalc(mos->a,mos);
        break;
        
        case 0x15: //ZeroPage X
            aux = mos->x + instruc->m[0];
            if(0x0000<=aux && aux<=0x00ff) mos->a = mos->a | mos->mem[aux];
            zerocalc(mos->a,mos);
            signcalc(mos->a,mos);
            
        break;
        
        case 0x0d: //Absoluto
           aux = instruc->addr;
           mos->a = mos->a | mos->mem[aux];
           zerocalc(mos->a,mos);
           signcalc(mos->a,mos);
        break;
        
        case 0x1d: //Absoluto X
            aux = mos->x + instruc->addr;
            mos->a = mos->a | mos->mem[aux];
            zerocalc(mos->a,mos);
            signcalc(mos->a,mos);
        break;
            
        case 0x19: //Absoluto Y
            aux = mos->y + instruc->addr;
            mos->a = mos->a | mos->mem[aux];
            zerocalc(mos->a,mos);
            signcalc(mos->a,mos);
        break;
        /////////////////////////////////
        
        //NOP:///////////////////////////
        case 0xea: break;
        /////////////////////////////////
        
        /*************STA***************/     
        case 0x85:  //ZeroPage
            aux = instruc->m[0];
            if(0x0000<=aux && aux<=0x00ff) mos->mem[aux] = mos->a;
        	break;
        case 0x95:  //Zero Page, X
            aux = mos->x + instruc->m[0];
            if(0x0000<=aux && aux<=0x00ff){
                mos->mem[aux]=mos->a; 
            }
            break;	
        case 0x8d: //Absoluto
            aux = instruc->addr;
            mos->mem[aux] = mos->a;
            break;
        case 0x9d:  //Absoluto X
            aux = mos->x + instruc->addr;
            mos->mem[aux] = mos->a; 
            break;
        case 0x99: //Absoluto Y
            aux = mos->y + instruc->addr;
            mos->mem[aux] = mos->a;
            break;

        //AND:///////////////////////////
        case 0x29: //Inmediato
        	mos->a = mos ->a & instruc->m[0];
        	zerocalc(mos->a,mos);
            signcalc(mos->a,mos);
 			break; 
        case 0x25: //Zeropage
        	aux = instruc->m[0];
            if(0x0000<=aux && aux<=0x00ff) mos->a = mos->a & mos->mem[aux];
            zerocalc(mos->a,mos);
            signcalc(mos->a,mos);
            break;
        case 0x35: //ZeroPage X
        	aux = mos->x + instruc->m[0];
            if(0x0000<=aux && aux<=0x00ff) mos->a = mos->a & mos->mem[aux];
            zerocalc(mos->a,mos);
            signcalc(mos->a,mos);
            break;
    	case 0x2d: //Absoluto
    	   aux = instruc->addr;
           mos->a = mos->a & mos->mem[aux];
           zerocalc(mos->a,mos);
           signcalc(mos->a,mos);
       	   break;
        case 0x3d: //Absoluto X
            aux = mos->x + instruc->addr;
            mos->a = mos->a & mos->mem[aux];
            zerocalc(mos->a,mos);
            signcalc(mos->a,mos);
        	break;	        
        case 0x39: //Absoluto Y
			aux = mos->y + instruc->addr;
            mos->a = mos->a  & mos->mem[aux];
            zerocalc(mos->a,mos);
            signcalc(mos->a,mos);
        	break;
        //EOR:///////////////////////////	  
      	case 0x49: //Inmediato
        	mos->a = mos ->a ^ instruc->m[0];
        	zerocalc(mos->a,mos);
            signcalc(mos->a,mos);
 			break; 
        case 0x45: //Zeropage
        	aux = instruc->m[0];
            if(0x0000<=aux && aux<=0x00ff) mos->a = mos->a ^ mos->mem[aux];
            zerocalc(mos->a,mos);
            signcalc(mos->a,mos);
            break;
        case 0x55: //ZeroPage X
        	aux = mos->x + instruc->m[0];
            if(0x0000<=aux && aux<=0x00ff) mos->a = mos->a ^ mos->mem[aux];
            zerocalc(mos->a,mos);
            signcalc(mos->a,mos);
            break;
    	case 0x4D: //Absoluto
    	   aux = instruc->addr;
           mos->a = mos->a ^ mos->mem[aux];
           zerocalc(mos->a,mos);
           signcalc(mos->a,mos);
       	   break;
        case 0x5D: //Absoluto X
            aux = mos->x + instruc->addr;
            mos->a = mos->a ^ mos->mem[aux];
            zerocalc(mos->a,mos);
            signcalc(mos->a,mos);
        	break;	        
        case 0x59: //Absoluto Y
            aux = mos->y + instruc->addr;
            mos->a = mos->a  ^ mos->mem[aux];
            zerocalc(mos->a,mos);
            signcalc(mos->a,mos);
        	break;
        /****************************************BRK******************************************/
      	case 0x00:
            mos->status |= FLAG_INTERRUPT; //FLAG a 1
            exit(0);
            break;
        /****************************************CLC******************************************/
      	case 0x18:
            mos->status &= ~FLAG_CARRY;  //FLAG A 0
            break;
        /****************************************CLD******************************************/               
        case 0xD8:
            mos->status &= ~FLAG_DECIMAL;
            break;
                        
        /****************************************CLI******************************************/
        case 0x58:
            mos->status &= ~FLAG_INTERRUPT;
            break;
          /****************************************CLV******************************************/   
        case 0xB8:
            mos->status &= ~FLAG_OVERFLOW;
         /****************************************DEX******************************************/
        case 0xCA:
            mos->x--;
            mos->status |= mos->x && FLAG_SIGN;
            if(mos->x == 0) mos->status |= FLAG_ZERO;
            break;
         /****************************************DEY******************************************/
        case 0x88:
            mos->y--;
            mos->status |= mos->y && FLAG_SIGN;
            if(mos->y == 0) mos->status |= FLAG_ZERO;
            break;
        /****************************************DEC******************************************/
      	case 0xC6:  //Zero Page 
            aux = instruc->m[0]; 
            if(0x0000<=aux && aux<=0x00ff){
                mos->mem[aux]--;
            }
            mos->status |= mos->mem[aux] && FLAG_SIGN;
            if(mos->mem[aux] == 0) mos->status |= FLAG_ZERO;
            break;
        
      case 0xD6: //Zero Page, X
            aux = mos->x + instruc->m[0];
            if(0x0000<=aux && aux<=0x00ff){
              mos->mem[aux]--;
            }
            mos->status |= mos->mem[aux] && FLAG_SIGN;
            if(mos->mem[aux] == 0) mos->status |= FLAG_ZERO;
            break;
        
      case 0xCE: //Absolute
            aux = instruc->addr;
            mos->mem[aux]--;
            mos->status |= mos->mem[aux] && FLAG_SIGN;
            if(mos->mem[aux] == 0) mos->status |= FLAG_ZERO;
            break;
        
      case 0xDE: //Absolute, X
        
            aux = mos->x + instruc->addr;
            mos->mem[aux]--;
            mos->status |= mos->mem[aux] && FLAG_SIGN;
            if(mos->mem[aux] == 0) mos->status |= FLAG_ZERO;
            break;
        
            
        /****************************************INC******************************************/
      case 0xE6:  //Zero Page 
            aux = instruc->m[0]; 
            if(0x0000<=aux && aux<=0x00ff){
                mos->mem[aux]++;
            }
            mos->status |= mos->mem[aux] && FLAG_SIGN;
            if(mos->mem[aux] == 0) mos->status |= FLAG_ZERO;
            break;
        
      case 0xF6: //Zero Page, X
            aux = mos->x + instruc->m[0];
            if(0x0000<=aux && aux<=0x00ff){
              mos->mem[aux]++;
            }
            mos->status |= mos->mem[aux] && FLAG_SIGN;
            if(mos->mem[aux] == 0) mos->status |= FLAG_ZERO;
            break;
        
      case 0xEE: //Absolute
            aux = instruc->addr;
            mos->mem[aux]++;
            mos->status |= mos->mem[aux] && FLAG_SIGN;
            if(mos->mem[aux] == 0) mos->status |= FLAG_ZERO;
            break;
        
      case 0xFE: //Absolute, X
        
            aux = mos->x + instruc->addr;
            mos->mem[aux]++;
            mos->status |= mos->mem[aux] && FLAG_SIGN;
            if(mos->mem[aux] == 0) mos->status |= FLAG_ZERO;
            break;
        
        /****************************************INX******************************************/
      	case 0xE8:
            mos->x++;
            mos->status |= mos->x && 0x80;mos->x && FLAG_SIGN;
            if(mos->x==0) mos->status |= mos->x && FLAG_ZERO;
            break;
        /****************************************SEC******************************************/
      	case 0x38:
            mos->status |= FLAG_CARRY;
            break;
        /****************************************SED******************************************/
      	case 0xF8:
            mos->status |= FLAG_DECIMAL;
            break;
        /****************************************SEI******************************************/
      	case 0x78:
            mos->status |= FLAG_INTERRUPT;
            break;
        /****************************************TAX******************************************/
      	case 0xAA:
            mos->x = mos->a;
            if(mos->x<0) mos->status |= mos->x && FLAG_SIGN;
            if(mos->x==0) mos->status |= mos->x && FLAG_ZERO;
            break;
        /****************************************TAY******************************************/
      	case 0xA8:
            mos->y = mos->a;
            if(mos->x<0) mos->status |= mos->x && FLAG_SIGN;
            if(mos->x==0) mos->status |= mos->x && FLAG_ZERO;
            break;
        /****************************************TSX******************************************/
      	case 0xBA:
            mos->x = mos->sp ;
            if(mos->x<0) mos->status |= mos->x && FLAG_SIGN;
            if(mos->x==0) mos->status |= mos->x && FLAG_ZERO;
            break;
        /****************************************TXA******************************************/
      	case 0x8A:
            mos->a = mos->x;
            if(mos->x<0) mos->status |= mos->x && FLAG_SIGN;
            if(mos->x==0) mos->status |= mos->x && FLAG_ZERO;
            break;
        /****************************************TXS******************************************/
      	case 0x9A:
            mos->sp = mos->x;
            break;
        /****************************************TYA******************************************/
      	case 0x98:
            mos->a = mos->y;
            if(mos->x<0) mos->status |= mos->x && FLAG_SIGN;
            if(mos->x==0) mos->status |= mos->x && FLAG_ZERO;
            break;            
        /****************************************BCC******************************************/  
        case 0x90:
            if(mos->status & FLAG_CARRY == 0 ){
                mos->pc = branchAdress;
            }
            break; 
        /****************************************BCS******************************************/     
        case 0xB0:
            if(mos->status & FLAG_CARRY == FLAG_CARRY ){
                mos->pc = branchAdress;
            }
            break;
        /****************************************BEQ******************************************/     
        case 0xF0:
            if(mos->status & FLAG_ZERO == FLAG_ZERO ){
                mos->pc = branchAdress;
            }
            break;
        /****************************************BMI******************************************/     
        case 0x30:
            if(mos->status & FLAG_SIGN == FLAG_SIGN ){
                mos->pc = branchAdress;
            }
            break;
        /****************************************BNE******************************************/     
        case 0xD0:
            if(mos->status & FLAG_ZERO == 0 ){
                mos->pc = branchAdress;
            }
            break;
        /****************************************BPL******************************************/     
        case 0x10:
            if(mos->status & FLAG_SIGN == 0 ){
                mos->pc = branchAdress;
            }
            break;
        /****************************************BVC******************************************/     
        case 0x50:
            if(mos->status & FLAG_OVERFLOW == 0 ){
                mos->pc = branchAdress;
            }
            break;
        /****************************************BVS******************************************/     
        case 0x70:
            if(mos->status & FLAG_OVERFLOW == FLAG_OVERFLOW ){
                mos->pc = branchAdress;
            }
            break;
            
        /****************************************STX******************************************/
        case 0x86: //Zero page
            aux = instruc->m[0]; 
            if(0x0000<=aux && aux<=0x00ff){
                mos->mem[aux] = mos->x;
            }
            break;
        case 0x96: //Zero page Y
            aux = mos->y + instruc->m[0];
            if(0x0000<=aux && aux<=0x00ff){
              mos->mem[aux] = mos->x;
            }
            break;
        case 0x8E: //Aboslute
           aux = instruc->addr;
           mos->mem[aux] = mos->x | mos->mem[aux];
       	   break;
        /****************************************STY******************************************/
        case 0x84: //Zero page
            aux = instruc->m[0]; 
            if(0x0000<=aux && aux<=0x00ff){
                mos->mem[aux] = mos->y;
            }
            break;
        case 0x94: //Zero page X
            aux = mos->x + instruc->m[0];
            if(0x0000<=aux && aux<=0x00ff){
              mos->mem[aux] = mos->y;
            }
            break;
        case 0x8C: //Aboslute
           aux = instruc->addr;
           mos->mem[aux] = mos->y | mos->mem[aux];
       	   break;

                 
    }
}
////////////////////////////////////////////////////////////////


//Instrucciones:////////////////////////////////////////////////
void procesaIntrucc(uint16_t param,int modoInmediato,mos6502_t *mos,instruccion_t *instruc){
    uint16_t aux;
    if(modoInmediato==1){
        mos->mem[mos->pc+1] = param;
    }else if(0x0000<=param && param<=0x00ff){
        mos->mem[mos->pc+1] = param;
    }else{
        aux = param%256;
        mos->mem[mos->pc+1] = aux; //LSB
        aux = param/256;
        mos->mem[mos->pc+2] = aux; //MSB
    }
}

void InstruccionConDir(uint8_t opcode,uint16_t param,int modoInmediato,mos6502_t *mos,instruccion_t *instruc){
    mos->mem[mos->pc]   = opcode;    //Opcode de la funcion
    procesaIntrucc(param,modoInmediato,mos,instruc);
    fetch(mos,instruc);
    decode(mos,instruc);
    execute(mos,instruc);
}


void InstruccionSinDir(uint8_t opcode,mos6502_t *mos,instruccion_t *instruc){
    uint16_t aux;
    mos->mem[mos->pc]   = opcode;    //Opcode de la funcion
    fetch(mos,instruc);
    decode(mos,instruc);
    execute(mos,instruc);
}

void InstruccionConSalto(uint8_t opcode,mos6502_t *mos,instruccion_t *instruc,uint16_t direcciondeSalto){
    uint16_t aux;
    mos->mem[mos->pc]   = opcode;    //Opcode de la funcion
    branchAdress = direcciondeSalto;
    fetch(mos,instruc);
    decode(mos,instruc);
    execute(mos,instruc); 
}






////////////////////////////////////////////////////////////////



int main(){
    mos6502_t mos;
    instruccion_t datos;
    ////Arranque del Programa://////////////////////////////////
    init_memory(&mos);
    reset_cpu(&mos);
    ////////////////////////////////////////////////////////////

    InstruccionConDir(0xa2,0x21,1,&mos,&datos); //LDX Inmediato (1->representa que es dir del tipo inmediato)
    InstruccionConDir(0x8E,0xd010,0,&mos,&datos); //STX (Cargo el valor de X en 0xd010)
    
    
    InstruccionConDir(0xae,0xd010,0,&mos,&datos); //LDX Absoluto (Cargo el valor de 0xd010 a X)
    InstruccionSinDir(0xE8,&mos,&datos); //INCX
    InstruccionSinDir(0xE8,&mos,&datos); //INCX
    InstruccionSinDir(0xE8,&mos,&datos); //INCX
    InstruccionSinDir(0xE8,&mos,&datos); //INCX
    
    printf("%04hhx\n",mos.x);
    ///////////////////////////////////////////////////////////
	

    return 0;
}
