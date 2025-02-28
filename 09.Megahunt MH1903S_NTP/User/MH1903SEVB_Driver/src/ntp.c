#include "type.h"
#include "ntp.h"
#include "config.h"
#include "socket.h"
#include "w5500.h"

#include <string.h>
#include <stdio.h>

ntpformat NTPformat;
datetime Nowdatetime;
uint8 NTP_Message[48];
tstamp Total_Seconds=0;
uint8 Time_Zone = 39;
uint8 NTP_Server_IP[4]={202, 112, 10, 60};
uint8 NTP_Port = 123;
uint8 NTP_Timeouttimer_Start=0;
uint8 BUFPUB[2048];
CONFIG_MSG  ConfigMsg, RecvMsg;
volatile uint8_t ntptimer = 0;

void get_seconds_from_ntp_server(uint8* buf,uint16 idx)
{
    tstamp seconds = 0;
    uint8 i=0;
    for (i = 0; i < 4; i++)
    {
      seconds = (seconds << 8) | buf[idx + i];
    }
    switch (Time_Zone)
    {
      case 0:
        seconds -=  12*3600;
        break;
      case 1:
        seconds -=  11*3600;
        break;
      case 2:
        seconds -=  10*3600;
        break;
      case 3:
        seconds -=  (9*3600+30*60);
        break;
      case 4:
        seconds -=  9*3600;
        break;
      case 5:
      case 6:
        seconds -=  8*3600;
        break;
      case 7:
      case 8:
        seconds -=  7*3600;
        break;
      case 9:
      case 10:
        seconds -=  6*3600;
        break;
      case 11:
      case 12:
      case 13:
        seconds -= 5*3600;
        break;
      case 14:
        seconds -=  (4*3600+30*60);
        break;
      case 15:
      case 16:
        seconds -=  4*3600;
        break;
      case 17:
        seconds -=  (3*3600+30*60);
        break;
      case 18:
        seconds -=  3*3600;
        break;
      case 19:
        seconds -=  2*3600;
        break;
      case 20:
        seconds -=  1*3600;
        break;
      case 21:                            //��
      case 22:
        break;
      case 23:
      case 24:
      case 25:
        seconds +=  1*3600;
        break;
      case 26:
      case 27:
        seconds +=  2*3600;
        break;
      case 28:
      case 29:
        seconds +=  3*3600;
        break;
      case 30:
        seconds +=  (3*3600+30*60);
        break;
      case 31:
        seconds +=  4*3600;
        break;
      case 32:
        seconds +=  (4*3600+30*60);
        break;
      case 33:
        seconds +=  5*3600;
        break;
      case 34:
        seconds +=  (5*3600+30*60);
        break;
      case 35:
        seconds +=  (5*3600+45*60);
        break;
      case 36:
        seconds +=  6*3600;
        break;
      case 37:
        seconds +=  (6*3600+30*60);
        break;
      case 38:
        seconds +=  7*3600;
        break;
      case 39:
        seconds +=  8*3600;
        break;
      case 40:
        seconds +=  9*3600;
        break;
      case 41:
        seconds +=  (9*3600+30*60);
        break;
      case 42:
        seconds +=  10*3600;
        break;
      case 43:
        seconds +=  (10*3600+30*60);
        break;
      case 44:
        seconds +=  11*3600;
        break;
      case 45:
        seconds +=  (11*3600+30*60);
        break;
      case 46:
        seconds +=  12*3600;
        break;
      case 47:
        seconds +=  (12*3600+45*60);
        break;
      case 48:
        seconds +=  13*3600;
        break;
      case 49:
        seconds +=  14*3600;
        break;
    }
    
    Total_Seconds=seconds;
    //calculation for date
    calc_date_time(seconds); 
}

void ntpclient_init(void)
{
	uint8 flag;
   NTPformat.dstaddr[0] = NTP_Server_IP[0];
   NTPformat.dstaddr[1] = NTP_Server_IP[1];
   NTPformat.dstaddr[2] = NTP_Server_IP[2];
   NTPformat.dstaddr[3] = NTP_Server_IP[3];
   /*NTPformat.dstaddr[0] = ip[0];
   NTPformat.dstaddr[1] = ip[1];
   NTPformat.dstaddr[2] = ip[2];
   NTPformat.dstaddr[3] = ip[3];*/
  

   NTPformat.leap = 0;           /* leap indicator */ 
   NTPformat.version = 4;        /* version number */
   NTPformat.mode = 3;           /* mode */
   NTPformat.stratum = 0;        /* stratum */
   NTPformat.poll = 0;           /* poll interval */
   NTPformat.precision = 0;      /* precision */
   NTPformat.rootdelay = 0;      /* root delay */
   NTPformat.rootdisp = 0;       /* root dispersion */
   NTPformat.refid = 0;          /* reference ID */
   NTPformat.reftime = 0;        /* reference time */
   NTPformat.org = 0;            /* origin timestamp */
   NTPformat.rec = 0;            /* receive timestamp */
   NTPformat.xmt = 1;            /* transmit timestamp */
   
   flag = (NTPformat.leap<<6)+(NTPformat.version<<3)+NTPformat.mode; //one byte Flag
   memcpy(NTP_Message,(void const*)(&flag),1);
}

uint8 NTP_Retry_Cnt=0; //counting the ntp retry number

void do_ntp_client(void)
{
  if(Total_Seconds)
    return; 
  else
  {
    //ntp_timeouttimer_start=1; //no used here. using a retry counter
  
    uint16 len;
    uint8 * data_buf = BUFPUB;
    uint32 destip = 0;
    uint16 destport;
    uint16 startindex = 40; //last 8-byte of data_buf[size is 48 byte] is xmt, so the startindex should be 40
    switch(getSn_SR(SOCK_NTP))
    {
      case SOCK_UDP:
        if ((len = getSn_RX_RSR(SOCK_NTP)) > 0) 		
        {
          if (len > TX_RX_MAX_BUF_SIZE) len = TX_RX_MAX_BUF_SIZE;	// if Rx data size is lager than TX_RX_MAX_BUF_SIZE 
          recvfrom(SOCK_NTP, data_buf, len, (uint8*)&destip, &destport);	
          //printf("%d\r\n", data_buf);
          get_seconds_from_ntp_server(data_buf,startindex);
          printf("%d-%02d-%02d %02d:%02d:%02d\r\n",  
                    (ConfigMsg.date.year[0]<<8)+ConfigMsg.date.year[1],
                    ConfigMsg.date.month,
                    ConfigMsg.date.day,
                    ConfigMsg.date.hour,
                    ConfigMsg.date.minute,
                    ConfigMsg.date.second);
          NTP_Retry_Cnt=0;
        }
        if(Total_Seconds>0) return;
        if(NTP_Retry_Cnt<100)
        {  
          if(NTP_Retry_Cnt==0)//first send request, no need to wait
          {
            sendto(SOCK_NTP,NTP_Message, sizeof(NTP_Message), NTP_Server_IP, NTP_Port);
            NTP_Retry_Cnt++;
            NTP_Timeouttimer_Start=1;
            ntptimer=0;
            //if(ConfigMsg.debug) printf("ntp retry: %d\r\n", ntp_retry_cnt);
          }
          else // send request again? it should wait for a while
          {
            if(ntptimer>2) //wait time, 3 second
            {
              sendto(SOCK_NTP,NTP_Message,sizeof(NTP_Message),NTP_Server_IP, NTP_Port);
              if(ConfigMsg.debug) printf("ntp retry: %d\r\n", NTP_Retry_Cnt);
              NTP_Retry_Cnt++;
              ntptimer=0;
              
            }
          }
        }
        else //ntp retry fail
        {
          NTP_Retry_Cnt=0;
          if(ConfigMsg.debug) printf("ntp retry failed!\r\n");
        }
        break;
      case SOCK_CLOSED:
        socket(SOCK_NTP,Sn_MR_UDP,NTP_Port,0);
        break;
    }
  } 
}

void calc_date_time(tstamp seconds)
{
    uint8 yf=0;
	uint32 p_year_total_sec;
    uint32 r_year_total_sec;
    tstamp n=0,d=0,total_d=0,rz=0;
    uint16 y=0,r=0,yr=0;
    signed long long yd=0;
    
    n = seconds;
    total_d = seconds/(SECS_PERDAY);
    d=0;
    p_year_total_sec=SECS_PERDAY*365;
    r_year_total_sec=SECS_PERDAY*366;
    while(n>=p_year_total_sec) 
    {
      if((EPOCH+r)%400==0 || ((EPOCH+r)%100!=0 && (EPOCH+r)%4==0))
      {
        n = n -(r_year_total_sec);
        d = d + 366;
      }
      else
      {
        n = n - (p_year_total_sec);
        d = d + 365;
      }
      r+=1;
      y+=1;
    
    }
    
    y += EPOCH;
 
    ConfigMsg.date.year[0] = (uint8)((y & 0xff00)>>8);
    ConfigMsg.date.year[1] = (uint8)(y & 0xff);
    
    yd=0;
    yd = total_d - d;
    
    yf=1;
    while(yd>=28) 
    {
        
        if(yf==1 || yf==3 || yf==5 || yf==7 || yf==8 || yf==10 || yf==12)
        {
          yd -= 31;
          if(yd<0)break;
          rz += 31;
        }
    
        if (yf==2)
        {
          if (y%400==0 || (y%100!=0 && y%4==0)) 
          {
            yd -= 29;
            if(yd<0)break;
            rz += 29;
          }
          else 
          {
            yd -= 28;
            if(yd<0)break;
            rz += 28;
          }
        } 
        if(yf==4 || yf==6 || yf==9 || yf==11 )
        {
          yd -= 30;
          if(yd<0)break;
          rz += 30;
        }
        yf += 1;
        
    }
    ConfigMsg.date.month=yf;
    yr = total_d-d-rz;
   
    yr += 1;
    
    ConfigMsg.date.day=yr;
    
    //calculation for time
    seconds = seconds%SECS_PERDAY;
    ConfigMsg.date.hour = seconds/3600;
    ConfigMsg.date.minute = (seconds%3600)/60;
    ConfigMsg.date.second = (seconds%3600)%60;
    
}

tstamp change_datetime_to_seconds(void) 
{
  tstamp seconds=0;
  uint32 total_day=0;
  uint16 i=0,run_year_cnt=0,l=0;
  
  l = ConfigMsg.date.year[0];//high
  
  l = (l<<8);
  
  l = l + ConfigMsg.date.year[1];//low
 
  
  for(i=EPOCH;i<l;i++)
  {
    if((i%400==0) || ((i%100!=0) && (i%4==0))) 
    {
      run_year_cnt += 1;
    }
  }
  
  total_day=(l-EPOCH-run_year_cnt)*365+run_year_cnt*366;

  for(i=1;i<=ConfigMsg.date.month;i++)
  {
    if(i==5 || i==7 || i==10 || i==12)
    {
      total_day += 30;
    }
    if (i==3)
    {
      if (l%400==0 && l%100!=0 && l%4==0) 
      {
        total_day += 29;
      }
      else 
      {
        total_day += 28;
      }
    } 
    if(i==2 || i==4 || i==6 || i==8 || i==9 || i==11)
    {
      total_day += 31;
    }
  }
 
  seconds = (total_day+ConfigMsg.date.day-1)*24*3600;  
  seconds += ConfigMsg.date.second;//seconds
  seconds += ConfigMsg.date.minute*60;//minute
  seconds += ConfigMsg.date.hour*3600;//hour
 
  return seconds;
}
