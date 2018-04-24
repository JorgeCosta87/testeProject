#include <C8051F340.h>
 
#define LCD P1
#define RS P3_1
#define RW P3_2
#define EN P3_3
#define LED1 P2_2
 
 
 
//LCD functions
void pulseEnable();
void configLCD();
void writeLCD(unsigned char *phrase, unsigned char size);
void changeline();
 
//
void ADCconversion();
void leds();
void convertVoltFormat(unsigned char *string, float value);
 
 
unsigned char msb=0,lsb=0, msg_1_Counter=0, msg_2_Counter=0;
 
 
__bit OnOff=1;
 
__data unsigned char counterPOT=0;
__data unsigned char counterLED=0;
__data unsigned char envioMSG1[4]={0x00,0x0,0,0};
__data unsigned char envioMSG2[4]={0x00,0x00,0x05,0};
unsigned char myName[]= "Jorge Costa";
 
unsigned char voltFormat[7] = {'V','p','=',0,'.',0,'V'};
unsigned char tempFormat[7] = {'T',':',0,0,'.',0,'C'};
 
float adcvalue, volt, temp;
 
void main(void)
{
 
    SP=0x60;
   
    //Device main configurations
    XBR1=0x40; //enables crossbar
    XBR0=0x01; //enables UART connection
    OSCICN=0x83; //internal oscillateor 12MHz
    PCA0MD &= 0x40; //disable watchdog timer;
    PCA0MD=0x00;
 
    //timer configuration
    TMOD=0x21;          //T1 8-bit auto reload T0 16bit
 
    //Timer 0 load
    TL0=0x58;   //conta 25000
    TH0=0x9E;   //conta 25000
 
    //Uart config  (256-(1000000/(2*baud)))
    SCON=0b01010000;             //8 bit mode REN enable TI=0 and RI=0 and mode 1 ( Determined by the timer 1)
    TH1=256-26;         //256-1000000/2*19200 =~ 230
    TL1=256-26;         //256-1000000/2*19200 =~ 230
 
    //pins configuration
    P2MDOUT= 0x0C;          //P2 push-pull for leds
    P2MDIN=0x9F;            //P2_5 and P2_6 as analog input
 
    // ADC configuration
    REF0CN = 0b00001100;         // ADC positive ref=VDD
    //AMX0P  = 0b00011110;         // define sensor de temperatrura como entrada do ADC (tabela 5.1)
    AMX0P     = 0x04;
    AMX0N  = 0x1F;               // single-ended mode
    ADC0CN = 0b10000000;         // enable ADC
 
    //config interrupts
    //IP        = 0x10;
    IE        = 0x93; // 0b10010011;    //EA,UART,Timer0 and Ext0
 
    TR0=1;  //Timer0 on
    TR1=1;  //Timer1 on
 
 
 
    configLCD();
// END configurations
 
    writeLCD(myName, 10);
 
    while(1)
    {
        if(counterPOT >= 20)
        {
            counterPOT = 0;
            AMX0P     = 0x04;
            ADCconversion();
           
            convertVoltFormat(voltFormat, volt);
 
            //print the data on the lcd
            changeLine();
            writeLCD(voltFormat, 7);
 
            //prepare to send to processing
            msb=ADC0H;
            lsb=ADC0L;
 
            if(lsb==0)
            {
                lsb++;
            }
 
            envioMSG1[2]=msb;
            envioMSG1[3]=lsb;
 
            msg_1_Counter=0;
            SBUF=envioMSG1[msg_1_Counter++]; // comeÃ§a o envio do valor ADC para o grÃ¡fico do processing
        }
 
        if(counterLED >= 10)
        {
            leds(); //toogle the led each 250ms 4hz
            counterLED = 0;
        }
    }
}
 
void convertVoltFormat(unsigned char *string, float value)
{
    int aux = value * 10;
 
    string[5] = aux%10 + 0x30;
    aux /= 10;
    string[3] = aux%10 + 0x30;
}
 
void convertTempFormat(unsigned char *string, float value)
{
    int aux = value * 10;
 
    string[5] = aux%10 + 0x30;
    aux /= 10;
    string[3] = aux%10 + 0x30;
    aux /= 10;
    string[2] = aux%10 + 0x30;
}
 
void RSI_button() __interrupt(0)
{
    AMX0P  = 0b00011110; // table 5.1, pag 48
    ADCconversion();
 
    temp = adcvalue*0.322;
    temp = (float)((adcvalue*5)/(1023.0))/0.01;
 
    RS = 0;
    LCD = 0xC8;
    pulseEnable();
 
    convertTempFormat(tempFormat,temp);
    writeLCD(tempFormat,7);
 
    msg_2_Counter = 0;
    SBUF=envioMSG2[3] = tempFormat[msg_2_Counter++];
}
 
void RSI_Timer0(void) __interrupt (1){
 
    TL0=0x58;   //conta 25000
    TH0=0x9E;   //conta 25000
   
    counterLED++;
    counterPOT++;
}
 
void RSISerie(void) __interrupt(4)
{
    if(TI)
    {
        TI=0;
 
        if(msg_1_Counter<4)
        { // envia mensagem tipo 1
            SBUF=envioMSG1[msg_1_Counter];
            msg_1_Counter++;
        }
 
        if(msg_2_Counter<7)
        { // envia mensagem tipo 1
            SBUF=envioMSG2[3] = tempFormat[msg_2_Counter];
            msg_1_Counter++;
        }
    }
 
    if(RI==1)
    {
        RI=0;
    }
}
 
void ADCconversion()
{
    AD0BUSY = 1;            //initialize conversion
 
    while(AD0INT == 0);         //waits for the end of the conversion
    AD0INT = 0;         // clear flag
 
    adcvalue = ADC0;
    volt = (adcvalue * 3.3)/1023.0;
}
 
void configLCD(void)
{
    RS=0;
    RW=0;
 
    LCD=0x38;       //2 lines and 5x7 matrix
    pulseEnable();
    LCD=0x0E;       //Display on, cursor off
    pulseEnable();
    LCD=0x01;       //clear display
    pulseEnable();
    LCD=0x06;       //Increment cursor / Sifht cursor to right
    pulseEnable();
}
 
void pulseEnable(void)
{
    unsigned int time;
 
    EN=1;
    for(time=0;time<10;time++);
    EN=0;
    for(time=0;time<2000;time++);       //aprox 2.7ms
}
 
void writeLCD(unsigned char *phrase, unsigned char size)
{
 
    unsigned char j=0;
    RS=1;
    RW=0;
 
    for(j=0;j<size;j++)
    {
        LCD=phrase[j];
        pulseEnable();
    }
}
void changeline(void){
 
    RS=0;
    LCD=0b10000000|1<<6;
    pulseEnable();
}
 
void leds(void) //troca o estado dos leds
{
    P2_2 = ! P2_2;
    OnOff = !OnOff;
   
    if (OnOff==1)
        P2_2=1;
    else
        P2_2=0;
}
 
 
 
 
 
 
 
 
 
 
 
 
/*
 
 
//variÃÂ¡veis globais auxiliares
unsigned char contagemOverflows=0;
unsigned char cc=0;
__bit OnOff=1;
__data __at(0x3B) unsigned char contadorOverflows=0;
unsigned char msb=0,lsb=0, nEnviados=0;
__data __at(0x40) unsigned char envio[4]={0x00,0x0,0,0}; // array de envio da mensagem tipo 1
 
//protÃÂ³tipos de funÃÂ§ÃÂµes para configurar timers e gerir estado dos leds
void configTimer(void);
void Leds(void);
void reload(void);
void configDevice(void);
void configADC(void);
void configInt(void);
void reloadTimer(void);
void configSerie(void);
 
void main()
{
        SP=0x60; //localiza a pilha
        configDevice(); //configura a placa
        configADC();
        configInt(); //configura as interrupÃÂ§ÃÂµes
        configTimer(); //configura o timer
        configSerie();
 
       
    while(1)
    {
 
    }
}
//----------------------
void configDevice(void)
{
    P2MDOUT = 0x04;     // define P2 como I/O digital em push-pull
   
    XBR1 = 0x40;        // permite a crossbar
    OSCICN = 0x83;      // configura oscilador interno para 12MHz
    XBR0 = 0x01;        // liga os pinos P0_4 e P0_5 Ã  UART do canal sÃ©rie
   
    PCA0MD &= 0xBF;     // estas duas instruÃÂ§ÃÂµes fazem o disable do watchdog
    PCA0MD = 0x00;
}
//----------------------
void configADC(void)
{
 
    REF0CN = 0b00001100;         // ADC positive ref=VDD
    AMX0P  = 0b00011110;         // define sensor de temperatrura como entrada do ADC (tabela 5.1)
    AMX0N  = 0x1F;           // single-ended mode
    ADC0CN = 0b10000001;       // enable do ADC com inicio de conversÃ£o pela int do timer 0
}
 
//----------------------
void configInt(void)
{
    IE = 0b10010010; // timer 0 configurado como fonte de interrupÃÂ§ÃÂ£o
}
 
//--------baud de 19200------256-(1000000/(2*baud))--------
void configSerie(void)
{
    SCON=0b01010000; // modo 1
    TH1=256-26;
    TL1=256-26;
    TR1=1; // timer 1 ON
}
 
//----------------------
void reloadTimer(void)
{
    unsigned int valorInicial=65536-50000;
    TL0 = valorInicial;
    TH0 = (valorInicial>>8);
    TF0=0;
}
 
//----------------------
void configTimer(void)
{
    TMOD=0b00100001;
    TF0 = 0;
    reloadTimer();
    TR0=1;
}
 
//----------------------
void Leds(void) //troca o estado dos leds
{
    OnOff= !OnOff;
    if (OnOff==1)
        P2_2=1;
    else
        P2_2=0;
}
 
//----------------------
void RSI_timer0(void) __interrupt(1)
{
    reloadTimer();
    contagemOverflows++;
 
    while(AD0INT==0);     // fica preso aqui enquanto decorre uma conversÃ£o do ADC, quando acaba a flag Ã© posta a 1
 
    AD0INT=0;            // limpa a flag de fim de conversÃ£o
 
    msb=ADC0H;
    lsb=ADC0L;
    if(lsb==0)
    { // no caso do LSB ser 0 soma-se 1
        lsb++;
    }
    envio[2]=msb;
    envio[3]=lsb;
 
    nEnviados=0;
    SBUF=envio[nEnviados]; // comeÃ§a o envio do valor ADC para o grÃ¡fico do processing
   
    if (contagemOverflows==5) //50msx20=1s
    {
        contagemOverflows=0;
        Leds(); // a cada 1s muda o estado dos leds
    }
}
 
void RSISerie(void) __interrupt(4)
{
    if(TI)
    {
        TI=0;
        nEnviados++;
        if(nEnviados<4){ // envia mensagem tipo 1
            SBUF=envio[nEnviados];
        }
    } // if TI end
 
    if(RI==1)
    {
        RI=0;
    } // if RI end
}
 
*/
