#include "usart.h"
#include "ult.h"
#include "socket.h"
#include "w5500.h"
#include "wol.h"
#include "config.h"
#include <string.h>



uint8 buffer[2048]={0};												// ����һ��2KB�����飬�������Socket��ͨ������

uint8 DIP[4]={0};
uint8 DHAR[6]={0};
uint16 DPORT=3000;

/*���绽�ѣ�������λ�����͵�ħ������0xffffffffffff + 16��MAC��ַ��*/
void do_wol()
{
   uint16 len=0;
   switch(getSn_SR(0))                          // ��ȡsocket0��״̬
   {
      case SOCK_UDP:                           	// Socket���ڳ�ʼ�����(��)״̬
        if(getSn_IR(0) & Sn_IR_RECV)
        {
         setSn_IR(0, Sn_IR_RECV);
        }
				if(getIR() & 0x10)
        {
          printf("get wake on line\r\n");
					setIR(0x10);
					setMR(0x00);
        }
			 	// ���ݻػ����Գ������ݴ�Զ����λ������W5500��W5500���յ����ݺ��ٻظ�Զ����λ��
        if((len=getSn_RX_RSR(0))>0)
        { 
           recvfrom(0, buffer, len, DIP,&DPORT);  	// W5500��������Զ����λ�������ݣ���ͨ��SPI���͸�MCU
					 printf("%d.%d.%d.%d:%d:\r\n", DIP[0],DIP[1],DIP[2],DIP[3],DPORT);
           printf("%s\r\n", buffer);                // ���ڴ�ӡ���յ�������
           sendto(0, buffer,len, DIP, DPORT);       // ���յ����ݺ��ٻظ�Զ����λ����������ݻػ�
					 memset(buffer,0,len+1);                  // ��ս��յ�������
         }
        break; 
      case SOCK_CLOSED:                          // Socket���ڹر�״̬
         socket(0,Sn_MR_UDP,5000,0);         
			   setIMR(0x10);
			   setMR(MR_WOL);
        break;
    }
}
