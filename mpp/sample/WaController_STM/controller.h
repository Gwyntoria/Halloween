/*
 * controller.h:
 *
 ***********************************************************************
 * by Jessica Mao
 * Lotogram Inc,. 2020/04/17
 *
 ***********************************************************************
 */


#ifndef controller_h
#define controller_h

#ifdef __cplusplus
extern "C" {
#endif


void controller_init(int iRoomSubType);
void controller_user_drop(int iPosition);
void controller_add_scores(int iPositions[], int iCount);
void controller_user_option_ab(int iPosition);
void controller_user_option_click(int iPosition, int iOption);
void controller_user_option_down(int iPosition, int iOption);
void controller_user_option_up(int iPosition, int iOption);
void controller_user_shoot(int iPosition);
void controller_shoots(int iPositions[], int iCount);
void controller_user_shoot_strength(int iPosition);
void controller_user_clear_score(int iPosition);
void controller_admin_clear_scores(int iPositions[], int iCount);
void controller_admin_options(int iOption);
void controller_admin_open_settings();


#ifdef __cplusplus
}
#endif


#endif
