#include "C:/Keil/EE319Kware/inc/tm4c123gh6pm.h"
#include "stdint.h"


#define RS 0x00 /* PORTA BIT5 mask */
#define RW 0x01 /* PORTA BIT6 mask */
#define EN 0x02 /* PORTA BIT7 mask */


enum room_status { reserved =1 ,free=2 ,clean =3};
enum door_status {closed =0 ,open=1};



//keybad
void keypad_init(void);
unsigned char getchar(void);
unsigned char getkey(void);


//delays
void delayMs		(int n);
void delayUs		(int n);

//lcd
void lcd_init(void);
void LCD_Data(unsigned char data);
void LCD_command(unsigned char command);
void LCD_Data_string(char *data);


// UART
void uart_init(void);
char uart_readChar(void);
void uart_writeChar(char data);
void printString(char *String);
void set_pass(int roomNum);
void uart_main_loop(void);


// Doors and rooms

//enum room_status doorScreen(unsigned char);

typedef struct {
	unsigned char roomNum;
	enum room_status roomSt;
	enum door_status doorSt;	
	unsigned char pass[4]; //global array password

}room;

room rooms[5];
void roomInit(void)
{
	int i;
	for (i=0;i<5;i++)
	{
		rooms[i].doorSt=closed;
		rooms[i].roomNum=i+1;
		rooms[i].roomSt=free;
	}
}
	unsigned char check_pass(int roomNum)
{
	int i;
	unsigned char arr[4];
	for (i=0;i<4;i++)
	{
		arr[i]= getchar(); //get password bits
		LCD_command(0xC6); /*to start write password from the center of the screen*/
		LCD_Data('*'); //write * at the screen
	}
	for (i=0;i<4;i++)
	{
		if (rooms[roomNum].pass[i]!=arr[i])
			return 0;
	}
	return 1;
}



void SystemInit(){}

void PORTF_init(void)
{
	SYSCTL_RCGCGPIO_R |=(1<<5);//ENABLE CLOCK FOR PORT F
	delayUs(3);
	GPIO_PORTF_LOCK_R = 0x4C4F434B; 
	GPIO_PORTF_CR_R |= 0x0E;
	GPIO_PORTF_AFSEL_R = 0;
	GPIO_PORTF_PCTL_R = 0;
	GPIO_PORTF_AMSEL_R = 0;
	GPIO_PORTF_DEN_R |= 0x0E;
	GPIO_PORTF_DIR_R |= 0x0E;
	
}
	
int main(void)
{
	keypad_init();
	lcd_init();
	uart_init();
	PORTF_init();
	__enable_irq();	
}




void keypad_init(void)
{
 //ordiary initilization
	SYSCTL_RCGCGPIO_R |=(1<<3);//ENABLE CLOCK FOR PORT D
	GPIO_PORTD_LOCK_R = 0x4C4F434B; 
	GPIO_PORTD_CR_R |= 0xFF; //uncommit the whole port 
	GPIO_PORTF_PCTL_R = 0; //no special considrations
	GPIO_PORTD_DIR_R = 0x0F; // 0:3 as rows(output) 4:7 as columns(inputs)
	GPIO_PORTD_AFSEL_R = 0; // use the whole port as GPIO(no altr function)
	GPIO_PORTD_DEN_R = 0xFF; // enable the whole port as digital enable
	GPIO_PORTD_AMSEL_R = 0; //disable analog 
	GPIO_PORTD_ODR_R |= 0x0F;// set row pins as open drain
	GPIO_PORTD_PUR_R |= 0xF0; //set pull-ups for input pins
	
	// here to interrupt 
	GPIO_PORTD_IS_R &=~0x0F; // edge sens 
	GPIO_PORTD_IBE_R &=~0x0F;  // only single edge sens
	GPIO_PORTD_IEV_R &=~0x0F; //falling edge
	GPIO_PORTD_ICR_R =0x0F; //clear flag
	GPIO_PORTD_IM_R |=0x0F; //enable interrupts for the pins 4:7 port f 
	NVIC_EN0_R =0x04; //enable intrrupt 3 in nvic (port d)
	NVIC_PRI0_R = (NVIC_PRI0_R&0x00FFFFFF)|0x20000000; //set priority for port f to 1;
	__enable_irq();
	
}
void GPIOPORTD_Handler(void)
{
	GPIO_PORTD_ICR_R =0x0F; //clear flag
	if (check_pass(1))
	{
		LCD_command(1);	/* clear display */
		LCD_command(0x82); /*to start write password from the center of the screen*/
		LCD_Data_string("door open!");
		GPIO_PORTF_DATA_R |=0x02; //red led indecates door opened
	}
}
	
unsigned char getchar(void)
{
	
	unsigned char key;
	/* wait until the previous key is released */
	do{
		while(getkey() != 0);
		delayMs(20); /* wait to debounce */
	}while(getkey() != 0);
	do{
		key = getkey();
		delayMs(20); /* wait to debounce */
	}while(getkey() != key);
	return key;
}

unsigned char getkey(void)
{
	const char keys[4][4]=
	{
	{'1','2','3','A'},
	{'4','5','6','B'},
	{'7','8','9','C'},
	{'*','0','#','D'}
	};
	int row,col;
	while (1)
		
{
	row = 0;
	GPIO_PORTD_DATA_R |= 0x0E; /* enable row 0 */
	delayUs(3); /* wait for signal to settle */
	col = GPIO_PORTD_DATA_R & 0xF0;
	if (col != 0xF0) break;
	
	row = 1;
	GPIO_PORTD_DATA_R |= 0x0D; /* enable row 1 */
	delayUs(3); /* wait for signal to settle */
	col = GPIO_PORTD_DATA_R & 0xF0;
	if (col != 0xF0) break;
	
	
	row = 2;
	GPIO_PORTD_DATA_R |= 0x0B; /* enable row 2 */
	delayUs(3); /* wait for signal to settle */
	col = GPIO_PORTD_DATA_R & 0xF0;
	if (col != 0xF0) break;
	
	row = 3;
	GPIO_PORTD_DATA_R |= 0x07; /* enable row 3 */
	delayUs(3); /* wait for signal to settle */
	col = GPIO_PORTD_DATA_R & 0xF0;
	if (col != 0xF0) break;
}
if (col == 0xE0) return keys[row][0]; /* key in column 0 */
if (col == 0xD0) return keys[row][1]; /* key in column 1 */
if (col == 0xB0) return keys[row][2]; /* key in column 2 */
if (col == 0x70) return keys[row][3]; /* key in column 3 */
return 0; //not needed 
}

/* delay n milliseconds (16 MHz CPU clock) */
void delayMs(int n)
{
int i, j;
for(i = 0 ; i < n; i++)
for(j = 0; j < 3180; j++)
{} /* do nothing for 1 ms */
}
/* delay n microseconds (16 MHz CPU clock) */
void delayUs(int n)
{
int i, j;
for(i = 0 ; i < n; i++)
for(j = 0; j < 3; j++)
{} /* do nothing for 1 us */
}



void LCD_command(unsigned char command)
{
	GPIO_PORTA_DATA_R &= 0X1F; /* RS = 0, R/W = 0 */
	GPIO_PORTB_DATA_R = command;
	GPIO_PORTA_DATA_R |= EN; /* pulse E */
	delayUs(1);
	GPIO_PORTA_DATA_R &= 0X1F;
	if (command < 4)
		delayMs(2); /* command 1 and 2 needs up to 1.64ms */
	else
		delayUs(45); /* all others 40 us */
}


void LCD_Data(unsigned char data)
{
	GPIO_PORTA_DATA_R |= RS;
	GPIO_PORTB_DATA_R = data;
	GPIO_PORTA_DATA_R |= EN;
	delayUs(1);
	GPIO_PORTA_DATA_R &= 0X1F;
	delayUs(45);
}
void LCD_Data_string(char *data)
{
	while(*data)
		LCD_Data(*(data++));
}


void lcd_init(void)
{
//	uint32_t delay;
	SYSCTL_RCGCGPIO_R |= 0x03; 
//	delay = 1;

//LCD DATA
	GPIO_PORTB_LOCK_R = 0x4C4F434B; //unlock port a
	GPIO_PORTB_CR_R = 0xFF;  //commit register
	GPIO_PORTB_PCTL_R = 0; //no special control consdiration
	GPIO_PORTB_DIR_R = 0xFF; //all port b output dir
	GPIO_PORTB_DEN_R = 0xFF; //digital enable
	GPIO_PORTB_AMSEL_R = 0; //no analog function
	GPIO_PORTB_AFSEL_R = 0; //no altr sel
//LCD COMMANND
	GPIO_PORTA_LOCK_R = 0x4C4F434B; //unlock port a 
	GPIO_PORTA_CR_R |= 0xE0; //commit pins 5:7
	GPIO_PORTA_PCTL_R &= 0X1F; //no special control consedration for pins 5:7
	GPIO_PORTA_DIR_R |= 0x0E; //pins 5:7  as output pins
	GPIO_PORTA_DEN_R |= 0x0E; //pins 5:7 as digital enable
	GPIO_PORTA_AMSEL_R |= 0; //no analog
	GPIO_PORTA_AFSEL_R |= 0; //no altr for here but for UART will be used
	
	// initialization sequence
	delayMs(20); 
	LCD_command(0x30);
	delayMs(5);
	LCD_command(0x30);
	delayUs(100);
	LCD_command(0x30);
	LCD_command(0x38); /* set 8-bit data, 2-line, 5x7 font */
	LCD_command(0x06); /* move cursor right */
	LCD_command(0x01); /* clear screen, move cursor to home */
	LCD_command(0x0F); /* turn on display, cursor blinking */
}

void uart_init(void)
{
	SYSCTL_RCGCUART_R |= 1; //enable uart 0
	SYSCTL_RCGCGPIO_R |= 0x01;// enable system clock for port a 
	
	GPIO_PORTA_LOCK_R = 0x4C4F434B; //unlock port a 
	GPIO_PORTA_CR_R |= 0x03; //commit pins 0:1
	GPIO_PORTA_AFSEL_R |= 0x03;
	GPIO_PORTA_PCTL_R =  (GPIO_PORTA_PCTL_R&0xFFFFFF00)+0x11;//pmc0&pmc1 
	GPIO_PORTA_DEN_R |= 0x03;//digital enable pins 0:1
		
	UART0_CTL_R &= ~0x01; //disable uart by clearing enable bit 
	
	//baud rate =80,000,000/16*9600=520.833
	UART0_IBRD_R =520;
	//fbr =(.833*64)+.5=53
	UART0_FBRD_R =53;
	UART0_LCRH_R = (0x0070); //0011_0000 no parity 8bits
	UART0_CC_R = 0; //internal clock source
	UART0_CTL_R = 0x301; //enable , send and receive 
	
	UART0_ICR_R = 0x10;
	UART0_IM_R = 0x10;
	NVIC_PRI1_R = (NVIC_PRI1_R & 0xFFFF00FF ) | 0x00000000; //set prioity for uart0 to 0
	NVIC_EN0_R |= (1<<5);
}

char uart_readChar(void)
{
	while( (UART0_FR_R & 0x0010)!=0);
	return (UART0_DR_R &0xFF);
}
void uart_writeChar(char data)
{
	while( (UART0_FR_R & 0x02)!=0);
	UART0_DR_R = data;
}
void printString(char *String)
{
	while (* String)
	{
		uart_writeChar(*(String++));
	}
}
void set_pass(int roomNum)
{
	unsigned char i;
	for (i=0;i<4;i++)
	{
		rooms[roomNum].pass[i]= uart_readChar(); //get password bits
	}
}
void UART0_Handler(void)
{
	enum room_status roomSt;
	unsigned char roomNum;
	printString("Enter room number :\t");
	roomNum =uart_readChar();
	roomSt=rooms[roomNum-1].roomSt;
	printString("Room current state :\t");
	if(roomSt == reserved)
	{
		printString("reserved");
	}
	else if(roomSt == clean)
	{
		printString("cleaning");
	}
	else if(roomSt == free)
	{
		printString("free");
	}
	printString("to set status enter 1 to exit enter 0 :\t");
	while (uart_readChar()!=1||uart_readChar()!=0)
			printString("to set status enter 1 to exit enter 0 :\t");
	if (uart_readChar()==1)
	{
		printString("Please enter room Status (1-reseve\n2-free\n3-clean)\n :\t");
		while (uart_readChar()!=1||uart_readChar()!=2||uart_readChar()!=3)
			printString("Please enter room Status (1-reseve\n2-free\n3-clean)\n :\t");
		if (uart_readChar()==1)
		{
			printString("Please enter room Password (4 integers only -in case more it will be implicit ro the first 4-) :\t");
			set_pass(roomNum-1);	
			rooms[roomNum-1].roomSt=reserved;
			rooms[roomNum-1].doorSt=closed;			
			printString("Room reserved successfully :\t");
			if (roomNum-1==1)
			{
				LCD_command(1);	/* clear display */
				LCD_command(0x84); /*to start write password from the center of the screen*/
				LCD_Data_string("reserved");
				GPIO_PORTF_DATA_R =0x08; //yellow red for reserved

			}	
			return;
		}
		else if (uart_readChar()==2)
		{
			printString("Room is now free thank you :)");
			//set room status to free and lock the solinoid
			rooms[roomNum-1].roomSt=free;
			rooms[roomNum-1].doorSt=closed;
			if (roomNum-1==1)
			{
				LCD_command(1);	/* clear display */
				LCD_command(0x86); /*to start write password from the center of the screen*/
				LCD_Data_string("free");
				GPIO_PORTF_DATA_R =0; //no leds for free
			}
			return;
		}
		else if (uart_readChar()==3)
		{
			printString("Room is now ready for cleaning thank you :)");
			//set room status to cleaning and unlock the solinoid
			rooms[roomNum-1].roomSt=clean;
			rooms[roomNum-1].doorSt=open;
			if (roomNum-1==1)
			{
				LCD_command(1);	/* clear display */
				LCD_command(0x83); /*to start write password from the center of the screen*/
				LCD_Data_string("room service");
				GPIO_PORTF_DATA_R |=0x02; //red led indecates door opened
				GPIO_PORTF_DATA_R |=0x04; //green led indecates door opened for room services
			}
			return;
		}
	}
	else 
		return;
	
}

