#include <reg51.h>

sfr lcd_data_port=0x80;	
sfr pin_port_1=0x90;
sbit STATUS_LED=P1^4;
sbit ALARM_LED=P1^5;
sbit BUZZER=P1^6;
sbit ALARM_OUT=P1^7;
sbit rs=P2^0;			/* Register select pin */
sbit rw=P2^1;			/* Read/Write pin */
sbit en=P2^2;			/* Enable pin */

volatile bit alarm_status_marker;
volatile bit ext_interrupt_marker;
volatile bit uart_interrupt_marker;
volatile bit alarm_reset_marker;

unsigned char Alarm_value;
unsigned char Alarm_buffer;
// 14 Rows and 16 columns
char *textlist[] = {"   Krawczynski  ","ElekronicSystems","Alarm Status=OFF",
"System restarted","                ","ALARM AN","ALARM AUS","ON ","OFF","  BUZZER  TEST  ",
"RESET OK","FEHLER","  Alarm Zone    ","    No alarm    "};

void Delay(unsigned int time)
{
	int i,j;
	for (i=0;i<time;i++)
		for (j=0;j<112;j++);
}

void Compare(unsigned char value1, unsigned char value2)
{
	if((value1 != value2) && (value1 > value2))
	{
	 BUZZER = 1;
	 ALARM_OUT = 1;
	}
	else if((value1 != value2) && (value1 < value2))
	{
	 Alarm_buffer = Alarm_value;
	 BUZZER = 0;
	 ALARM_OUT = 0;
	}
	else
	{
	 BUZZER = 0;
	 ALARM_OUT = 0;
	}
}

void LCD_Command (unsigned char cmd)  /* LCD16x2 command funtion */
{
	lcd_data_port= cmd;
	rs=0;			/* command reg. */
	rw=0;
	en=1; 
	Delay(5); // 1
	en=0;
	Delay(5); // 5
}

void LCD_Char (unsigned char char_data)  /* LCD data write function */
{
	lcd_data_port=char_data;
	rs=1;			/* Data reg.*/
	rw=0;			/* Write operation*/
	en=1;   				
	Delay(1);
	en=0;
	Delay(5);
}

void LCD_String (unsigned char *str) /* Send string to LCD function */
{
	int i;
	for(i=0;str[i]!=0;i++)  /* Send each char of string till the NULL */
	{
		LCD_Char(str[i]);  /* Call LCD data write */
	}
}

void LCD_String_xy(unsigned char row,unsigned char pos,unsigned char *str)
{
	if (row == 0)
	LCD_Command((pos & 0x0F)|0x80);
	else if (row == 1)
	LCD_Command((pos & 0x0F)|0xC0);
	LCD_String(str);	
}

void LCD_String_Pos_Repeat(unsigned char *str, unsigned char pos) /* Send string to LCD function */
{
	int i;
	
	for(i=0;str[i]!=0;i++)  /* Send each char of string till the NULL */
	{
		LCD_Command((pos & 0x0F)|0xC0);
		LCD_Char (str[i]);  /* Call LCD data write */
		Delay(800);
	}
}

void LCD_String_xy_Pos_Repeat(unsigned char row, unsigned char pos,unsigned char *str)
{
	if (row == 0)
	LCD_Command((pos & 0x0F)|0x80);
	else if (row == 1)
	LCD_Command((pos & 0x0F)|0xC0);
	LCD_String_Pos_Repeat(str,pos);	
}


void LCD_Shifting(unsigned char shift)
{
	unsigned char i;
	for(i=0;i<shift;i++)				
  {
		LCD_Command(0x1c);  	/* shift display right */
		Delay(1000);
  }
	
	for(i=0;i<(shift*2);i++)
  {
		LCD_Command(0x18);  	/* shift display left */
		Delay(1000);
  }	
	
		for(i=0;i<shift;i++)				
  {
		LCD_Command(0x1c);  	/* shift display right */
		Delay(1000);
  }
}

void LCD_Init (void)		/* LCD Initialize function */
{	
	Delay(20);		/* LCD Power ON Initialization time >15ms */
	LCD_Command (0x38);	/* Initialization of 16X2 LCD in 8bit mode */
	LCD_Command (0x0C);	/* Display ON Cursor OFF */
	LCD_Command (0x06);	/* Auto Increment cursor */
	LCD_Command (0x01);	/* clear display */
	LCD_Command (0x80);	/* cursor at home position */
}

void Ext_Int_Init()				
{
	EA  = 1;		/* Enable global interrupt */
	EX0 = 1;		/* Enable extern interrupt 0 */
	EX1 = 1;		/* Enable extern interrupt 1 */
	ES = 1;  		/* Enable serial interrupt */	
  IT0 = 1;	  /* Interrupt1 on falling edge */	
	IT1 = 1;	  /* Interrupt1 on falling edge */
}

void Pin_In_Out_Init()
{
	P1 = 0x0F;
	STATUS_LED = 1;
  BUZZER = 0;
	ALARM_OUT = 0;
	ALARM_LED = 1;
}

void UART_Init()
{
	TMOD = 0x20;		/* Timer 1, 8-bit auto reload mode */
	TH1 = 0xFD;		  /* 9600 baud rate */
	SCON = 0x50;		/* Mode 1, reception enable */
	TR1 = 1;		   
}

void UART_TxChar(unsigned char Data)
{
	SBUF = Data;
	while (TI==0);
	TI = 0;
}

void UART_SendString(unsigned char *str)
{
	int i;
	for(i=0;str[i]!=0;i++)
	{
		UART_TxChar(str[i]);
  }
}

void LCD_Welcome()
{
	LCD_String_xy(0,0,textlist[0]);
	LCD_String_xy(1,0,textlist[1]);
	Delay(1000);
  LCD_Shifting(1); //15
	Delay(1000);
	LCD_String_xy(0,0,textlist[2]);
	LCD_String_xy(1,0,textlist[3]);
	Delay(1000);
	LCD_String_xy(1,0,textlist[4]);	
}

void Extern_Interrupt_0() interrupt 0
{
	Alarm_buffer = Alarm_value;
	alarm_reset_marker = 1;
}

void Extern_Interrupt_1() interrupt 2
{
	ext_interrupt_marker = 1;
	EX1 = 0;
	ES = 0;
}

void Serial_ISR() interrupt 4    
{
	uart_interrupt_marker = 1;
	EX1 = 0;
	ES = 0;
} 

unsigned char *LCD_Zone_Nr(unsigned char value)
{
	switch(value)
	{
		case 0x01:
			return "1 ";
		break;
		case 0x02:
			return "2 ";
		break;
		case 0x03:
			return "12";
		break;
		case 0x04:
			return "3 ";
		break;
		case 0x05:
			return "13";
		break;
		case 0x06:
			return "23";
		break;
		case 0x07:
			return "123";
		break;
		case 0x08:
			return "4 ";
		break;
		case 0x09:
			return "14";
		break;
		case 0x0A:
			return "12";
		break;
		case 0x0B:
			return "124";
		break;
		case 0x0C:
			return "34";
		break;
		case 0x0D:
			return "134";
		break;
		case 0x0E:
			return "234";
		break;
		case 0x0F:
			return "1234";
		break; 
		default:
			return "0 ";
	}	
}

void main()
{
	alarm_status_marker = 0;
	alarm_reset_marker = 0;
	ext_interrupt_marker = 0;
	uart_interrupt_marker = 0;
	Alarm_buffer = 0;
	
	Pin_In_Out_Init();
	LCD_Init();	
	LCD_Welcome();	
	UART_Init();	
	Ext_Int_Init(); 
	
	
	while(1)
	{
		if(uart_interrupt_marker)
		{
			while (RI==0);
			RI = 0; 
			if(SBUF == '1')
			{
				STATUS_LED = 0;
				alarm_status_marker = 1;
				UART_SendString(textlist[5]);
				LCD_String_xy(0,13,textlist[7]);		
			}
			else if(SBUF == '0')
			{
				STATUS_LED = 1;
				BUZZER = 0;
				ALARM_OUT = 0;
				ALARM_LED = 1;
				alarm_status_marker = 0;
				UART_SendString(textlist[6]);
				LCD_String_xy(0,13,textlist[8]);
				LCD_String_xy(1,0,textlist[4]);		
			}
			else if(SBUF == '2')
			{	
				LCD_String_xy(1,0,textlist[9]);
				BUZZER = 1;
				ALARM_OUT = 1;
				ALARM_LED = 0;
				Delay(1000);
				BUZZER = 0;
				ALARM_OUT = 0;
				ALARM_LED = 1;
				Delay(1000);
				BUZZER = 1;
				ALARM_OUT = 1;
				ALARM_LED = 0;
				Delay(1000);
				BUZZER = 0;
				ALARM_OUT = 0;
				ALARM_LED = 1;
				LCD_String_xy(1,0,textlist[4]);
			}
			else if(SBUF == '3')
			{
				Alarm_buffer = Alarm_value;
				alarm_reset_marker = 1;
				UART_SendString(textlist[10]);
			}
			else if(SBUF == '4')
			{
				if(!alarm_status_marker)
				{
					UART_SendString(textlist[6]);	
				}
				else
				{
					UART_SendString(textlist[5]);
				}
			}
			else
			{
				UART_SendString(textlist[11]);
			}
			uart_interrupt_marker = 0;
			EX1 = 1;
			IT1 = 1;
			ES = 1;		
		}
		else if(ext_interrupt_marker)
		{
			if(!alarm_status_marker)
			{		
				STATUS_LED = 0;
				alarm_status_marker = 1;
				LCD_String_xy(0,13,textlist[7]);	
			}
			else
			{
				STATUS_LED = 1;
				BUZZER = 0;
				ALARM_OUT = 0;
				ALARM_LED = 1;
				alarm_status_marker = 0;
				LCD_String_xy(0,13,textlist[8]);
				LCD_String_xy(1,0,textlist[4]);		
			}
			ext_interrupt_marker = 0;
			EX1 = 1;
			IT1 = 1;
			ES = 1;	
		} // Extern or UART interrupt loop
		
		 Alarm_value = pin_port_1 & 0x0F; 
		
		 if(alarm_status_marker == 1)
		 {
			 if(Alarm_value !=0)
			 {
				 LCD_String_xy(1,0,textlist[12]);
				 ALARM_LED = 0;
				 Compare(Alarm_value,Alarm_buffer);
				 LCD_String_xy_Pos_Repeat(1,13,LCD_Zone_Nr(Alarm_value));
			 }
			 else
			 {
				 LCD_String_xy(1,0,textlist[13]);
				 if(alarm_reset_marker)
				 {
					BUZZER = 0;
					ALARM_OUT = 0;
					ALARM_LED = 1;
					alarm_reset_marker = 0;
				 }
				Alarm_buffer = Alarm_value;
			 }
	   }
		 else
		 {
	    LCD_String_xy(1,0,textlist[4]);
		  BUZZER = 0;
			ALARM_OUT = 0;
			ALARM_LED = 1;
		 }
	} //while loop
} // main