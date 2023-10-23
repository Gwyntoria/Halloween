/*
 * lotoSerial.h:
 *	Handle a serial port
 ***********************************************************************
 * RS485 Protocal implementation - by Shawn Lin
 * Lotogram Inc,. 2019/11/11
 *
 ***********************************************************************
 */

#ifdef __cplusplus
extern "C" {
#endif

extern int   serialOpen      (const char *device, const int baud) ;
extern int 	 serial485Open   (const char *device, const int baud, const int nbit, const char parity, const int nstop) ;
extern void  serialClose     (void) ;
extern void  serialFlush     (void) ;
extern void  serialPutchar   (const unsigned char c) ;
extern void  serialPuts      (const char *s, int strlen) ;
extern void  serialPrintf    (const char *message, ...) ;
extern int   serialDataAvail (void) ;
extern int   serialGetchar   (void) ;
extern int   serialDataRead  (char *buf,  int *size) ;
extern void  serialDataWrite (const char *ptr, int size) ;
extern int   serialReadline	 (char *buf) ;

#ifdef __cplusplus
}
#endif