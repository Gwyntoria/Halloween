/*
 * ConfigParser.h:
 *
 ***********************************************************************
 * by Jessica Mao
 * Lotogram Inc,. 2020/04/17
 *
 ***********************************************************************
 */


#ifndef configparser_h
#define configparser_h

#ifdef __cplusplus
extern "C" {
#endif

void delete_char(char str[],char target);
char *GetIniKeyString(char *title,char *key,char *filename);
int GetIniKeyInt(char *title,char *key,char *filename);
int PutIniKeyString(char *title,char *key,char *val,char *filename);
int PutIniKeyInt(char *title,char *key,int val,char *filename);

#ifdef __cplusplus
}
#endif


#endif
