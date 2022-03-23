//SMPTE LTC DECODER.

//Author: Camillo Sacco
//Thanks to: Bruno Sacco

//This project is based on Microchip PIC16f15313 and strictly relies on its CLC and NCO modules.
//SMPTE LTC uses biphase mark code wich is very similar to Manchester encoding.
//With reference to the Microchip application note (AN1470) and a few adaptations and optimizations we are able to decode the BMC in the same way, all in hardware, thanks to four CLC and NCO module. 
//For details about the configuration of input comparator (CMP), configurable logic cells (CLC), numerically controlled oscillator (NCO) please refer to the attached documentation and schematics.
//Decoded data and decoded clock are at the same time available on two external pins and fed inside a free running SPI client module.
//In this way we are able to read data 8bit per time. The drawback is that SPI module is not in sync with the beginning of the frame.
//We have to find the LTC sync word (0b0011111111111101) and then shift all the data properly.

#include "mcc_generated_files/mcc.h"

bool syncd = false;
uint8_t right_sh = 0;
uint8_t buffer[10];
uint8_t hh,HH,mm,MM,ss,SS,ff,FF;

uint8_t reverse_byte(uint8_t b);
void parse(void);
void printtime(void);
void trysync(void);


void main(void)
{
    // initialize the device
    SYSTEM_Initialize();
    static uint8_t index;
    unsigned char c;
    uint8_t i;

    INTERRUPT_GlobalInterruptEnable();
    INTERRUPT_PeripheralInterruptEnable();

    while (1)
    {
        SPI1_ClearReceiveOverflowStatus();
        SPI1_ClearWriteCollisionStatus();

        if(SPI1_IsBufferFull()){
            if(syncd){
                //In sync: receive data
                c = SSP1BUF;
                buffer[index] |= (c >> right_sh);
                index++;
                if(index > 9){
                    index = 0;
                    //verify if sync word correctly received
                    syncd = ((buffer[8] == 0x3f) && (buffer[9] == 0xfd));
                    if (syncd) {
                        //ok, frame valid
                        parse();
                        printtime();
                    }
                }
                //don't forget to handle the data relative to the next byte
                buffer[index] = (c << (8-right_sh));
            }else{
                trysync();
                if(syncd){
                    index = 0;
                }
            }          
        }   
    }
}

void trysync(void) {
    //This routine analizes a 3byte window to find the 2byte LTC sync word.
    //this task is done by checking each one of the eight possible positions of the 2bytes sync word inside the 3byte window.
    
    //    A         B        C      right_sh
    // x0011111 11111110 1xxxxxxx       7       (A & 0X7F == 0x1F) && (B == 0xFE) && (C | 0x7F == 0xFF)
    // xx001111 11111111 01xxxxxx       6       (A & 0x3F == 0x0F) && (B == 0xFF) && (C | 0x3F == 0x7F)
    // xxx00111 11111111 101xxxxx       5       (A & 0x1F == 0x07) && (B == 0xFF) && (C | 0x1F == 0xBF)
    // xxxx0011 11111111 1101xxxx       4       (A & 0x0F == 0x03) && (B == 0xFF) && (C | 0x0F == 0xDF)
    // xxxxx001 11111111 11101xxx       3       (A & 0x07 == 0x01) && (B == 0xFF) && (C | 0x07 == 0xEF)
    // xxxxxx00 11111111 111101xx       2       (A & 0x03 == 0x00) && (B == 0xFF) && (C | 0x03 == 0xF7)
    // xxxxxxx0 01111111 1111101x       1       (A & 0x01 == 0x00) && (B == 0x7F) && (C | 0x01 == 0xFB)
    // xxxxxxxx 00111111 11111101       0       (B == 0X3F) && (C == 0XFD)
    
    
    // 3Bytes buffer for sync word
    static uint8_t syncbuf[3];
    
    //read a byte
    syncbuf[2] = SSP1BUF;

    //list of checks to  find sync word and define bit alignment inside bytes.
    if (((syncbuf[0] & 0X7F) == 0x1F) && (syncbuf[1] == 0xFE) && ((syncbuf[2] | 0x7F) == 0xFF)) {
        right_sh = 7;
        syncd = true;
        buffer[0] = syncbuf[2] << 1;
    } else if (((syncbuf[0] & 0x3F) == 0x0F) && (syncbuf[1] == 0xFF) && ((syncbuf[2] | 0x3F) == 0x7F)) {
        right_sh = 6;
        syncd = true;
        buffer[0] = syncbuf[2] << 2;
    } else if (((syncbuf[0] & 0x1F) == 0x07) && (syncbuf[1] == 0xFF) && ((syncbuf[2] | 0x1F) == 0xBF)) {
        right_sh = 5;
        syncd = true;
        buffer[0] = syncbuf[2] << 3;
    } else if (((syncbuf[0] & 0x0F) == 0x03) && (syncbuf[1] == 0xFF) && ((syncbuf[2] | 0x0F) == 0xDF)) {
        right_sh = 4;
        syncd = true;
        buffer[0] = syncbuf[2] << 4;
    } else if (((syncbuf[0] & 0x07) == 0x01) && (syncbuf[1] == 0xFF) && ((syncbuf[2] | 0x07) == 0xEF)) {
        right_sh = 3;
        syncd = true;
        buffer[0] = syncbuf[2] << 5;
    } else if (((syncbuf[0] & 0x03) == 0x00) && (syncbuf[1] == 0xFF) && ((syncbuf[2] | 0x03) == 0xF7)) {
        right_sh = 2;
        syncd = true;
        buffer[0] = syncbuf[2] << 6;
    } else if (((syncbuf[0] & 0x01) == 0x00) && (syncbuf[1] == 0x7F) && ((syncbuf[2] | 0x01) == 0xFB)) {
        right_sh = 1;
        syncd = true;
        buffer[0] = syncbuf[2] << 7;
    } else if ((syncbuf[1] == 0X3F) && (syncbuf[2] == 0XFD)) {
        right_sh = 0;
        syncd = true;
        buffer[0] = 0;
    } else {
        
        //shift "ring buffer"
        syncbuf[0] = syncbuf[1];
        syncbuf[1] = syncbuf[2];
    }
}

void parse(void) {
    hh = 0x0F & reverse_byte(buffer[6]);    //h x1
    HH = 0x03 & reverse_byte(buffer[7]);    //h x10
    mm = 0x0F & reverse_byte(buffer[4]);    //m x1
    MM = 0x07 & reverse_byte(buffer[5]);    //m x10
    ss = 0x0F & reverse_byte(buffer[2]);    //s x1
    SS = 0x07 & reverse_byte(buffer[3]);    //s x10
    ff = 0x0F & reverse_byte(buffer[0]);    //f x1
    FF = 0x03 & reverse_byte(buffer[1]);    //f x10
}

void printtime(void) {
    EUSART1_Write(HH + '0');
    EUSART1_Write(hh + '0');
    EUSART1_Write(':');
    EUSART1_Write(MM + '0');
    EUSART1_Write(mm + '0');
    EUSART1_Write(':');
    EUSART1_Write(SS + '0');
    EUSART1_Write(ss + '0');
    EUSART1_Write(':');
    EUSART1_Write(FF + '0');
    EUSART1_Write(ff + '0');
    EUSART1_Write('\n');
}

uint8_t reverse_byte(uint8_t b){
    //reverse bit endianness
   b = (b & 0b11110000) >> 4 | (b & 0b00001111) << 4;
   b = (b & 0b11001100) >> 2 | (b & 0b00110011) << 2;
   b = (b & 0b10101010) >> 1 | (b & 0b01010101) << 1;
   return b;
}



