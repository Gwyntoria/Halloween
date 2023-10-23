/*
 * lotoSerial.c:
 *	Handle a serial port
 ***********************************************************************
 * RS485 Protocal implementation - by Shawn Lin
 * Lotogram Inc,. 2019/11/11
 *
 ***********************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "lotoSerial.h"

static int gsfd ;
static int ret;

/*
 * serialOpen:
 *	Open and initialise the serial port, setting all the right
 *	port parameters - or as many as are required - hopefully!
 *********************************************************************************
 */

int serialOpen (const char *device, const int baud)
{
  struct termios options ;
  speed_t myBaud ;
  int     status ;

  switch (baud)
  {
    case      50:	myBaud =      B50 ; break ;
    case      75:	myBaud =      B75 ; break ;
    case     110:	myBaud =     B110 ; break ;
    case     134:	myBaud =     B134 ; break ;
    case     150:	myBaud =     B150 ; break ;
    case     200:	myBaud =     B200 ; break ;
    case     300:	myBaud =     B300 ; break ;
    case     600:	myBaud =     B600 ; break ;
    case    1200:	myBaud =    B1200 ; break ;
    case    1800:	myBaud =    B1800 ; break ;
    case    2400:	myBaud =    B2400 ; break ;
    case    4800:	myBaud =    B4800 ; break ;
    case    9600:	myBaud =    B9600 ; break ;
    case   19200:	myBaud =   B19200 ; break ;
    case   38400:	myBaud =   B38400 ; break ;
    case   57600:	myBaud =   B57600 ; break ;
    case  115200:	myBaud =  B115200 ; break ;
    case  230400:	myBaud =  B230400 ; break ;
    case  460800:	myBaud =  B460800 ; break ;
    case  500000:	myBaud =  B500000 ; break ;
    case  576000:	myBaud =  B576000 ; break ;
    case  921600:	myBaud =  B921600 ; break ;
    case 1000000:	myBaud = B1000000 ; break ;
    case 1152000:	myBaud = B1152000 ; break ;
    case 1500000:	myBaud = B1500000 ; break ;
    case 2000000:	myBaud = B2000000 ; break ;
    case 2500000:	myBaud = B2500000 ; break ;
    case 3000000:	myBaud = B3000000 ; break ;
    case 3500000:	myBaud = B3500000 ; break ;
    case 4000000:	myBaud = B4000000 ; break ;

    default:
      return -2 ;
  }

  if ((gsfd = open (device, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK)) == -1)
    return -1 ;

  fcntl (gsfd, F_SETFL, O_RDWR) ;

// Get and modify current options:

  tcgetattr (gsfd, &options) ;

  cfmakeraw   (&options) ;
  cfsetispeed (&options, myBaud) ;
  cfsetospeed (&options, myBaud) ;

  options.c_cflag |= (CLOCAL | CREAD) ;
  options.c_cflag &= ~PARENB ;
  options.c_cflag &= ~CSTOPB ;
  options.c_cflag &= ~CSIZE ;
  options.c_cflag |= CS8 ;
  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG) ;
  options.c_oflag &= ~OPOST ;

  options.c_cc [VMIN]  =   0 ;
  options.c_cc [VTIME] = 100 ;	// Ten seconds (100 deciseconds)

  tcsetattr (gsfd, TCSANOW, &options) ;

  ioctl (gsfd, TIOCMGET, &status);

  status |= TIOCM_DTR ;
  status |= TIOCM_RTS ;

  ioctl (gsfd, TIOCMSET, &status);

  usleep (10000) ;	// 10mS

  return gsfd ;
}


/*
 * serialFlush:
 *	Flush the serial buffers (both tx & rx)
 *********************************************************************************
 */

int serial485Open (const char *device, const int baud, const int nbit, const char parity, const int nstop)
{
  struct termios options ;
  speed_t myBaud ;
  int     status;
 
  switch (baud)
  {
    case      50:	myBaud =      B50 ; break ;
    case      75:	myBaud =      B75 ; break ;
    case     110:	myBaud =     B110 ; break ;
    case     134:	myBaud =     B134 ; break ;
    case     150:	myBaud =     B150 ; break ;
    case     200:	myBaud =     B200 ; break ;
    case     300:	myBaud =     B300 ; break ;
    case     600:	myBaud =     B600 ; break ;
    case    1200:	myBaud =    B1200 ; break ;
    case    1800:	myBaud =    B1800 ; break ;
    case    2400:	myBaud =    B2400 ; break ;
    case    4800:	myBaud =    B4800 ; break ;
    case    9600:	myBaud =    B9600 ; break ;
    case   19200:	myBaud =   B19200 ; break ;
    case   38400:	myBaud =   B38400 ; break ;
    case   57600:	myBaud =   B57600 ; break ;
    case  115200:	myBaud =  B115200 ; break ;
    case  230400:	myBaud =  B230400 ; break ;
    case  460800:	myBaud =  B460800 ; break ;
    case  500000:	myBaud =  B500000 ; break ;
    case  576000:	myBaud =  B576000 ; break ;
    case  921600:	myBaud =  B921600 ; break ;
    case 1000000:	myBaud = B1000000 ; break ;
    case 1152000:	myBaud = B1152000 ; break ;
    case 1500000:	myBaud = B1500000 ; break ;
    case 2000000:	myBaud = B2000000 ; break ;
    case 2500000:	myBaud = B2500000 ; break ;
    case 3000000:	myBaud = B3000000 ; break ;
    case 3500000:	myBaud = B3500000 ; break ;
    case 4000000:	myBaud = B4000000 ; break ;
 
    default:
      return -2 ;
  }
 
  /* O_NONBLOCK is none block, and waiting for vim vtime*/
 
  if ((gsfd = open (device, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK)) == -1)
    return -1 ;
 
  fcntl (gsfd, F_SETFL, O_RDWR) ;
 
// Get and modify current options:
 
  tcgetattr (gsfd, &options) ;
 
    cfmakeraw   (&options) ;
    cfsetispeed (&options, myBaud) ;
    cfsetospeed (&options, myBaud) ;
    
    //data bit
    switch (nbit)
    {
        case 7: options.c_cflag &= ~CSIZE ; options.c_cflag |= CS7; break;
        case 8: options.c_cflag &= ~CSIZE ; options.c_cflag |= CS8; break;
        default:  options.c_cflag &= ~CSIZE ; options.c_cflag |= CS8; break;
    }
    //data parity
    switch(parity)  
    {
      case 'n':
      case 'N':
	      options.c_cflag &= ~PARENB ;
	      options.c_cflag &= ~INPCK;
	      break;
      case 'o':
      case 'O': 
	      options.c_cflag |= PARENB ;
	      options.c_cflag |= PARODD ;
	      options.c_cflag |= INPCK  ;
	      options.c_cflag |= ISTRIP	; 
	      break;
      case 'e':
      case 'E': 
	      options.c_cflag |= PARENB ;
	      options.c_cflag &= ~PARODD;
	      options.c_cflag |= INPCK 	;
	      options.c_cflag |= ISTRIP	;
	      break;
      default:
	      options.c_cflag &= ~PARENB ;
	      options.c_cflag &= ~INPCK;
	      break;
   }
   //data stopbits
    switch(nstop)
    {
      case 1: options.c_cflag &= ~CSTOPB ; break;
      case 2: options.c_cflag |= CSTOPB ;  break; 
      default: options.c_cflag &= ~CSTOPB ; break;    
    }
 
    options.c_cflag |= (CLOCAL | CREAD) ;
    //options.c_cflag &= ~CRTSCTS; // Shawn - Disable hardware flow control
    //options.c_iflag &= ~(IXON | IXOFF | IXANY); // Shawn - Disable software flow control
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG) ;
    options.c_oflag &= ~OPOST ;
 
    options.c_cc [VMIN]  =   0 ;
    options.c_cc [VTIME] = 10 ;	// 0.5 seconds (100 deciseconds)
 
  tcsetattr (gsfd, TCSANOW, &options) ;
 
  ioctl (gsfd, TIOCMGET, &status);
 
  status |= TIOCM_DTR ;
  status |= TIOCM_RTS ;
 
  ioctl (gsfd, TIOCMSET, &status);
 
  usleep (10000) ;	// 10mS
 
  return gsfd ;
}

/*
 * serialFlush:
 *	Flush the serial buffers (both tx & rx)
 *********************************************************************************
 */

void serialFlush (void)
{
  tcflush (gsfd, TCIOFLUSH) ;
}


/*
 * serialClose:
 *	Release the serial port
 *********************************************************************************
 */

void serialClose (void)
{
  close (gsfd) ;
}


/*
 * serialPutchar:
 *	Send a single character to the serial port
 *********************************************************************************
 */

void serialPutchar (const unsigned char c)
{
  write (gsfd, &c, 1) ;
}


/*
 * serialPuts:
 *	Send a string to the serial port
 *********************************************************************************
 */

void serialPuts (const char *s, int strlen)
{
  write (gsfd, s, strlen) ;
}

/*
 * serialPrintf:
 *	Printf over Serial
 *********************************************************************************
 */

void serialPrintf (const char *message, ...)
{
  va_list argp ;
  char buffer [1024] ;

  va_start (argp, message) ;
    vsnprintf (buffer, 1023, message, argp) ;
  va_end (argp) ;

  serialPuts (buffer, strlen(buffer)) ;
}


/*
 * serialDataAvail:
 *	Return the number of bytes of data avalable to be read in the serial port
 *********************************************************************************
 */

int serialDataAvail (void)
{
  int result ;

  if (ioctl (gsfd, FIONREAD, &result) == -1)
    return -1 ;

  return result ;
}


/*
 * serialGetchar:
 *	Get a single character from the serial device.
 *	Note: Zero is a valid character and this function will time-out after
 *	10 seconds.
 *********************************************************************************
 */

int serialGetchar (void)
{
  uint8_t x ;

  if (read (gsfd, &x, 1) != 1)
    return -1 ;

  return ((int)x) & 0xFF ;
}

/*
 * serialDataRead:
 *	Read from buffer, join each of 8 bytes
 *********************************************************************************
 */

int serialDataRead( char *buf,  int *size)
{
  int size_i,i;
  i = 0;
  size_i = 0;
  while(1)
  {
  	i = read(gsfd, buf+size_i ,1024);
  	size_i += i;
  	if(i == 8)
  	{
  		
  	}
  	else if(i>0 && i <= 8)
  	{
  		*size = size_i;
  		return 0;
  	}
  	else
  	{
  		return -1;
  	}
  }
}


/*
 * serialDataWrite:
 *	Write strings...
 *********************************************************************************
 */

void serialDataWrite(const char *ptr, int size)
{
  while(size--)
  {
    serialPutchar(*(ptr++));
  } 
}


/*
 * serialReadline:
 *  We need read one line per command
 *********************************************************************************
 */
int serialReadline(char *buf)
{
    fd_set rfds ;
    struct timeval time ;
    ssize_t cnt = 0 ;
    int lenth = 255 ; // Max command per line
    
    // Set the file discription
    FD_ZERO(&rfds) ;
    FD_SET(gsfd,&rfds) ;

    // Timeout settings
    time.tv_sec = 15;
    time.tv_usec = 0;
    
    // Selet it
    ret = select(gsfd+1, &rfds ,NULL, NULL, &time) ;
    switch (ret) {
    case -1:
        fprintf(stderr,"select error!\n") ;
        break ;
    case 0:
        fprintf(stderr, "time over!\n") ;
        break ;
    default:
        {
            cnt = read(gsfd, buf, lenth) ;
            buf[cnt]='\0';

            if(cnt == -1)
            {
                fprintf(stderr, "safe read failed!\n") ;
                cnt = 0 ;
            }
            break ;
        }
    }
    return cnt ;
}




