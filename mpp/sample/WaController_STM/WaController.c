/*
 * WaController.c:
 *
 *	By Jessica Mao 2020/04/17
 *
 * Copyright (c) 2012-2020 Lotogram Inc. <lotogram.com, zhuagewawa.com>

 * Version 1.0.0.75	Details in update.log
 ***********************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs/legacy/constants_c.h>
#include <iostream>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <vector>

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <iostream>
#include <chrono>

#include "vector.h"
#include "WaInit.h"
#include "controller.h"
#include "cJSON.h"
#include "md5.h"
#include "Detection.h"
#include "http.h"
#include "ConfigParser.h"

using std::vector;

#define KEY_ACTION  "1"                     //  action
#define KEY_EVENT   "2"                     //  event
#define KEY_OPTION  "3"                     //  option
#define KEY_VERSION     "4"                 //  version
#define KEY_TIMESTAMP   "5"                 //  timestamp
#define KEY_SIGN    "6"                     //  sign
#define KEY_ACTION_FROM     "7"             //  action_from
#define KEY_COUNT   "8"                     //  count
#define KEY_COINS   "9"                     //  coins
#define KEY_V_COINS "a"                     //  v_coins
#define KEY_STATUS  "b"                     //  status
#define KEY_RESULT  "c"                     //  result
#define KEY_FAILED_COUNT    "d"             //  failed_count
#define KEY_COUNTDOWN   "e"                 //  countdown
#define KEY_USER_STATUS "f"                 //  user_status
#define KEY_WIN_STATUS  "g"                 //  win_status
#define KEY_WIN_RATE    "h"                 //  win_rate
#define KEY_WIN_COINS   "m"                 //  win_coins
#define KEY_WIN_GEMS    "n"                 //  win_gems
#define KEY_CODE    "o"                     //  code
#define KEY_FINISHED    "p"                 //  grab_finished
#define KEY_MESSAGE "q"                     //  message
#define KEY_WARNING "r"                     //  warning
#define KEY_LANGUAGE    "s"                 //  language
#define KEY_MAX_WIN_COUNT   "t"             //  max_win_count
#define KEY_WAIT_TIME   "u"                 //  waittime
#define KEY_HOLDING_USER    "v"             //  holding user
#define KEY_GRAB_USER   "w"                 //  grabuser
#define KEY_START_TIME  "x"                 //  startTime
#define KEY_END_TIME    "y"                 //  endTime
#define KEY_DURATION    "z"                 //  duration
#define KEY_TIMEOUT "A"                     //  timeout
#define KEY_REASON  "R"                     //  reason
#define KEY_SCORE   "S"                     //  score

#define KEY_USER_ID     "u1"                //  user_id
#define KEY_USER_UID    "u2"                //  user_uid
#define KEY_USER_NAME   "u3"                //  user_name
#define KEY_USER_AVATAR "u4"                //  user_avatar
#define KEY_USER_TOKEN  "u5"                //  user_token
#define KEY_USER_TYPE   "u6"                //  user_type
#define KEY_USER_VIP    "u7"                //  user_vip
#define KEY_USER_COINS  "u8"                //  user_coins
#define KEY_USER_GEMS   "u9"                //  user_gems

#define KEY_ROOM_ID "r1"                    //  room_id
#define KEY_ROOM_UID    "r2"                //  room_uid
#define KEY_ROOM_TYPE   "r3"                //  room_type
#define KEY_ROOM_SUBTYPE    "r4"            //  room_subtype
#define KEY_ROOM_PRICE  "r5"                //  room_price
#define KEY_ROOM_STATUS "r6"                //  room status
#define KEY_ROOM_MAINTAINED "r7"            //  room maintained

#define KEY_DOLL_ID "d1"                    //  doll_id
#define KEY_DOLL_UID    "d2"                //  doll_uid
#define KEY_DOLL_NAME   "d3"                //  doll_name
#define KEY_DOLL_IMG    "d4"                //  doll_img

#define KEY_GRAB_ID "g1"                    //  grab_id
#define KEY_BILL_ID "b1"                    //  bill_id

#define KEY_TIPS_OCCUPY "T1"                //  ban_tips

#define KEY_RETAIN  "e1"                    //  retain
#define KEY_RETAIN_TIME "e2"                //  retain_time

#define KEY_GOAL    "o1"                    //  goal
#define KEY_GOAL_USER   "o2"                //  goal{user_id
#define KEY_GOAL_ROOM   "o3"                //  goal{room_id
#define KEY_GOAL_PROGRESS   "o4"            //  goal{progress_id
#define KEY_GOAL_MAX    "o5"                //  goal{max

#define KEY_GAME_ID "c1"                    //  game_id
#define KEY_GAME_POS    "c2"                //  game_pos 机位1,2,3,4
#define KEY_GAME_RATE   "c3"                //  game rate 分数和金币比例

#define KEY_RESULT_TYPE "f1"                //  result_type 房间结果类型：1保夹

#define KEY_OCCUPY  "p0"                    //  occupy
#define KEY_OCCUPY_RATE "p1"                //  occupy_rate
#define KEY_OCCUPY_MAX_MINS "p2"            //  occupy_max_minute

#define KEY_ADMIN_TOKEN "a1"                //  admin_token

#define KEY_ONLY_SERVER "N"                 //  only_server

#define KEY_AUTO_MODE   "U"                 //  auto mode
#define KEY_PASSIVE "F"                     //  passive 被动结算

#define KEY_RATE    "V"              // rate 倍率
#define KEY_TYPE    "W"              // type 类型
#define KEY_NAME    "X"              // name 名字
#define KEY_SOUL    "Z"              // soul 魂值
#define KEY_ON      "G"              // stream switch on/off
#define KEY_GAME_FIRES  "cc"        //game_fires; 每分钟平均开火次数
#define KEY_BLACK_HOLE  "16"        //black_hole;

#define KEY_GAME_BETS   "ce"        //game_bets; 游戏武器倍率
#define KEY_EMPTY_SHOT  "1b"        //empty shot; 空炮

#define KEY_MAGIC_TYPE  "m1"        //maigc type
#define KEY_DURATION    "z"         //duration


#define KEY_PK_MODE         "t1"        //pk模式
#define KEY_PK_STATUS       "t2"        //pk房间状态
#define KEY_PK_SCORE        "t3"        //pk房间各机位分数
#define KEY_PK_ID           "t4"        //pk记录id
#define KEY_PK_POS          "t5"        //pk机位
#define KEY_PK_ALLOW        "t6"        //支持pk
#define KEY_PK_COINCOUNT    "t7"        //pk预先投币次数

#define ACTION_PK_START     0x0035      //PK开始
#define ACTION_PK_END       0x0036      //PK结束
#define ACTION_PK_OPERATE   0x0037      //PK相关的操作
#define ACTION_PK_RET       0x0038      //PK相关的返回
#define ACTION_PK_CONTROL   0x0040      //用户PK操作

#define EVENT_PK_IDLE       0x0082      //PK待机中
#define EVENT_PK_STANDBY    0x0083      //PK准备中
#define EVENT_PK_START      0x0084      //PK开始
#define EVENT_PK_END        0x0085      //PK结束
#define EVENT_PK_CAL        0x0086      //PK记分
#define EVENT_PK_CLOSE      0x0087      //PK关闭
#define EVENT_PK_UPDATE     0x0088      //PK分数更新


//大怪
#define TYPE_DEATH      0           //幻影死神
#define TYPE_PUMPKIN    1           //千年南瓜怪
#define TYPE_DRAGON     2           //狂暴飞龙
#define TYPE_TROLL      3           //修罗巨魔
#define TYPE_SKELETON   4           //骷髅暴君
#define TYPE_BALROG     5           //巨石炎魔

//小怪
#define TYPE_PUMPKIN_SMALL  100      //南瓜怪
#define TYPE_DEATH_SMALL    101      //死神
#define TYPE_GHOST          102      //床单幽灵
#define TYPE_UMBRELLA       103      //伞怪
#define TYPE_MUMMY          104      //木乃伊
#define TYPE_ZOMBIE_WESTERN 105      //西洋僵尸
#define TYPE_ZOMBIE_CHINESE 106      //中国僵尸
#define TYPE_CYBORG         107      //人造人
#define TYPE_VAMPIRE        108      //吸血鬼

//武器
#define TYPE_MSD            200     //魔旋弹
#define TYPE_DGP            201     //电光破
#define TYPE_LHBL           202     //连环爆裂
#define TYPE_FYSL           203     //锁链封印
#define TYPE_QMLY           204     //驱魔烈焰
#define TYPE_MFZH           205     //魔法召唤

#define TYPE_XTBJ           300     //系统爆机


#define RESULT_OK   1
#define RESULT_FAILED   0

#define ACTION_ROOM_IN  0x0001              //  房间登入
#define ACTION_ROOM_OUT 0x0002              //  房间登出
#define ACTION_ROOM_RET 0x0003              //  房间报告结果
#define ACTION_ROOM_OPT 0x0004              //  房间操作
#define ACTION_ADMIN_ON 0x0011              //  管理员登入
#define ACTION_ADMIN_OFF    0x0021          //  房管理员登出
#define ACTION_ADMIN_SET    0x0013          //  管理员设置
#define ACTION_ADMIN_OPT    0x0014          //  管理员操作

#define ACTION_USER_IN  0x0021              //  用户登入
#define ACTION_USER_OUT 0x0022              //  用户登出
#define ACTION_USER_DROP_COIN   0x0023      //  用户投币
#define ACTION_USER_OPT 0x0024              //  用户操作

#define ACTION_CAST_MAGIC   0x0034          //  用户使用道具

#define EVENT_USER_PRESS_DOWN   0x0000      //  用户按下 button press
#define EVENT_USER_PRESS_UP 0x0001          //  用户抬起 button release
#define EVENT_USER_PRESS_CLICKED    0x0002  //  用户点击，默认一次 button clicked
#define EVENT_USER_PRESS_LONG   0x0003      //  长按

#define EVENT_ROOM_MAINTAINED   0x0011      //  房间维护事件
#define EVENT_ROOM_UNMAINTAINED 0x0012      //  房间取消维护事件
#define EVENT_COUNTDOWN 0x0013              //  倒计时事件
#define EVENT_GAMEOVER  0x0014              //  游戏机位空闲
#define EVENT_USERSCORE 0x0015              //  当前用户结算分数
#define EVENT_OPENSETTINGS  0x0016          //  打开设置
#define EVENT_SETTINGCONTROLL   0x0017      //  设置控制
#define EVENT_USERSCORE_ALL 0x0018          //  所有用户结算分数
#define EVENT_STREAM_SWITCH 0x0022          //  视频流开关
#define EVENT_CHANGE_SERVER 0x0034          //  修改服务器
#define EVENT_REBOOT_SYSTEM 0x0035          //  重启机器
#define EVENT_SCORE_RELEASE 0x0039          //  释放分数
#define EVENT_RESTART_FRP   0x0040
#define EVENT_MONSTER 0x0060                // 抓怪事件

#define EVENT_MONSTER_DETECT    0x0061      //抓怪事件

#define OPTION_GAME_UP  0x0000              //  向上按钮
#define OPTION_GAME_DOWN    0x0001          //  向下按钮
#define OPTION_GAME_LEFT    0x0002          //  向左按钮
#define OPTION_GAME_RIGHT   0x0003          //  向右按钮
#define OPTION_GAME_A   0x0004              //  射击
#define OPTION_GAME_B   0x0005              //  功能（魔界游戏中就是增加发射倍数）
#define OPTION_GAME_C   0x0006              //  上分
#define OPTION_GAME_D   0x0007              //  下分（结算）
#define OPTION_GAME_AB  0x0100              //  AB组合键
#define OPTION_GAME_X   0x0101              //  P2 A&B, P1 A  台湾主机的隐藏功能

#define STATUS_DISCONNECTED     0
#define STATUS_CONNECTED        1
#define STATUS_CONNECTING       2
#define STATUS_DISCONNECTING    3

static int  s_user_in_room[8] = {0};
static int  s_action_from[8] = {0};
static int  s_is_auto_shoot[8] = {0};
static int  s_is_auto_shoot_strength[8] = {0};
static int  s_game_time[8] = {60};
static int  s_count_down[8] = {60};
static int  s_new_user[8] = {0};
static int  s_end_play[8] = {0};
static int  s_with_drawing[8] = {0};
static int  s_in_countdown_thread[8] = {0};
static bool s_save_user_info[8] = {false};
static cJSON* s_users_info[8] = {NULL};
static long long s_send_time_only_server[8] = {0};
static long long s_last_monster_time[8] = {0};
static long long s_start_play_time[8] = {0};
static uint  s_shoot_count[8] = {0};
static bool s_reset_countdown[8] = {true};
static bool s_resend_countdown[8] = {false};
static int  s_check_position = 1;
static int  s_black_hole_user[8] = {0};
static uint s_operate_count[8] = {0};
static uint s_direction_count[8] = {0};

static int s_stream_on = 1;
static int s_streamoff = 0;
static long long s_streamoff_time = 0;
static int  s_in_streamoff_thread = 0;
static int  s_auto_debug = 0;
static int  s_detect_log = 0;
static int  s_auto_monster = 0;
static int  s_auto_baoji = 0;
static int  s_board_error = 0;
static int  s_check_baoji_status[4] = {0, 0, 0, 0};
static int  s_pk_mode = 0;
static int  s_pk_started = 0;
static char s_pk_id[256] = "";
static long long    s_pk_start_time = 0;
static long long    s_pk_end_time = 0;
static int  s_pk_enable_pos[4] = {0, 0, 0, 0};
static int  s_pk_close = 0;

static int  s_shoot_interval[8] = {100000, 100000, 100000, 100000, 100000, 100000, 100000, 100000};
static long long s_auto_start_time[8] = {0};
static long long s_auto_baoji_starttime[8] = {0};

static int s_option_count = 0;
static int s_operation_count = 0;

static int s_push_option = 0;
static int s_pop_option = 0;


static long long s_ll_error_time = 0;

struct Detect_Data
{
    long long ll_time;
    int life_cycle;
};

static Detect_Data  s_monster_detect[4][6] = {{{0, 15000}, {0, 10000}, {0, 15000}, {0, 15000}, {0, 15000}, {0, 15000}},
                                              {{0, 15000}, {0, 10000}, {0, 15000}, {0, 15000}, {0, 15000}, {0, 15000}},
                                              {{0, 15000}, {0, 10000}, {0, 15000}, {0, 15000}, {0, 15000}, {0, 15000}},
                                              {{0, 15000}, {0, 10000}, {0, 15000}, {0, 15000}, {0, 15000}, {0, 15000}}};
static Detect_Data  s_weapon_detect[4][6] = {{{0, 43000}, {0, 21000}, {0, 40000}, {0, 35000}, {0, 120000}, {0, 10000}},
                                             {{0, 43000}, {0, 21000}, {0, 40000}, {0, 35000}, {0, 120000}, {0, 10000}},
                                             {{0, 43000}, {0, 21000}, {0, 40000}, {0, 35000}, {0, 120000}, {0, 10000}},
                                             {{0, 43000}, {0, 21000}, {0, 40000}, {0, 35000}, {0, 120000}, {0, 10000}}};
static Detect_Data  s_l_monster_detect[4][9] = {{{0, 20000}, {0, 20000}, {0, 20000}, {0, 20000}, {0, 20000}, {0, 20000}, {0, 20000}, {0, 20000}, {0, 20000}},
                                                {{0, 20000}, {0, 20000}, {0, 20000}, {0, 20000}, {0, 20000}, {0, 20000}, {0, 20000}, {0, 20000}, {0, 20000}},
                                                {{0, 20000}, {0, 20000}, {0, 20000}, {0, 20000}, {0, 20000}, {0, 20000}, {0, 20000}, {0, 20000}, {0, 20000}},
                                                {{0, 20000}, {0, 20000}, {0, 20000}, {0, 20000}, {0, 20000}, {0, 20000}, {0, 20000}, {0, 20000}, {0, 20000}}};

static pthread_t s_check_websocket_pid = 0;
static pthread_t s_auto_play_pid1 = 0;
static pthread_t s_auto_rock_pid = 0;
static pthread_t s_auto_play_pid2 = 0;
static pthread_t s_auto_play_pid3 = 0;
static pthread_t s_auto_play_pid4 = 0;
static pthread_t s_detection_pid = 0;
static pthread_t s_check_empty_cannon_pid = 0;
static pthread_t s_check_all_scores_pid = 0;
static pthread_t s_user_options_pid = 0;
static pthread_t s_user_drop_pid[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static pthread_t s_cast_magic_pid = 0;
static pthread_t s_auto_shoot_pid[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static int s_drop_count[8] = {0, 0, 0, 0, 0, 0, 0, 0};

struct STRENGTH_DATA
{
    int strength_value;
    long long ll_time;
};

static vector<vector<STRENGTH_DATA*>> s_strength_list;
static vector<cJSON*> s_users_options;
static vector<cJSON*> s_cast_magic_message;

static loto_room_info* room_info_ptr = NULL;

typedef websocketpp::client<websocketpp::config::asio_tls_client> client_wss;
typedef websocketpp::client<websocketpp::config::asio_client> client_ws;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// pull out the type of messages sent by our config
//typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

static cJSON* key_maps = NULL;
static cJSON* action_maps = NULL;
static cJSON* event_maps = NULL;
static cJSON* option_maps = NULL;

class perftest
{
public:
    typedef perftest type;
    typedef std::chrono::duration<int,std::micro> dur_type;

    perftest (std::string uri);
    ~perftest();

    void start();
    void on_open(websocketpp::connection_hdl hdl);
    void on_message(websocketpp::connection_hdl hdl, message_ptr msg);
    bool on_ping(websocketpp::connection_hdl hdl, std::string str);
    void on_fail(websocketpp::connection_hdl hdl);
    void on_close(websocketpp::connection_hdl);
    void on_termination_handler(websocketpp::connection_hdl hdl);
    void send_message(char* message, const char* title, bool is_log = false);
    void check_websocet();
    int close_socket(std::string str_reason, websocketpp::connection_hdl hdl);

private:
    static client_ws::connection_ptr m_con_ws;
    static client_wss::connection_ptr m_con_wss;
    static bool    m_wss_support;

private:
    client_ws  m_endpoint_ws;
    client_wss  m_endpoint_wss;
    std::string m_uri;
    int     m_reconnect_c;
    int     m_status;
    long long m_ping_time;
    long    m_reconnect_delay;

    websocketpp::connection_hdl m_hdl;
};

static perftest* endpoint = NULL;
client_ws::connection_ptr perftest::m_con_ws = NULL;
client_wss::connection_ptr perftest::m_con_wss = NULL;
bool perftest::m_wss_support = false;

static void generate_maps()
{
    key_maps = cJSON_CreateObject();
    cJSON_AddStringToObject(key_maps, KEY_ACTION, "action");
    cJSON_AddStringToObject(key_maps, KEY_EVENT, "event");
    cJSON_AddStringToObject(key_maps, KEY_OPTION, "option");
    cJSON_AddStringToObject(key_maps, KEY_VERSION, "version");
    cJSON_AddStringToObject(key_maps, KEY_TIMESTAMP, "timestamp");
    cJSON_AddStringToObject(key_maps, KEY_SIGN, "sign");
    cJSON_AddStringToObject(key_maps, KEY_ACTION_FROM, "action_from");
    cJSON_AddStringToObject(key_maps, KEY_COUNT, "count");
    cJSON_AddStringToObject(key_maps, KEY_COINS, "coins");
    cJSON_AddStringToObject(key_maps, KEY_V_COINS, "v_coins");
    cJSON_AddStringToObject(key_maps, KEY_STATUS, "status");
    cJSON_AddStringToObject(key_maps, KEY_RESULT, "result");
    cJSON_AddStringToObject(key_maps, KEY_FAILED_COUNT, "failed_count");
    cJSON_AddStringToObject(key_maps, KEY_COUNTDOWN, "countdown");
    cJSON_AddStringToObject(key_maps, KEY_USER_STATUS, "user_status");
    cJSON_AddStringToObject(key_maps, KEY_WIN_STATUS, "win_status");
    cJSON_AddStringToObject(key_maps, KEY_WIN_RATE, "win_rate");
    cJSON_AddStringToObject(key_maps, KEY_WIN_COINS, "win_coins");
    cJSON_AddStringToObject(key_maps, KEY_WIN_GEMS, "win_gems");
    cJSON_AddStringToObject(key_maps, KEY_CODE, "code");
    cJSON_AddStringToObject(key_maps, KEY_FINISHED, "grab_finished");
    cJSON_AddStringToObject(key_maps, KEY_MESSAGE, "message");
    cJSON_AddStringToObject(key_maps, KEY_WARNING, "warning");
    cJSON_AddStringToObject(key_maps, KEY_LANGUAGE, "language");
    cJSON_AddStringToObject(key_maps, KEY_MAX_WIN_COUNT, "max_win_count");
    cJSON_AddStringToObject(key_maps, KEY_WAIT_TIME, "wait_time");
    cJSON_AddStringToObject(key_maps, KEY_HOLDING_USER, "holding user");
    cJSON_AddStringToObject(key_maps, KEY_GRAB_USER, "grab_user");
    cJSON_AddStringToObject(key_maps, KEY_START_TIME, "startTime");
    cJSON_AddStringToObject(key_maps, KEY_END_TIME, "endTime");
    cJSON_AddStringToObject(key_maps, KEY_DURATION, "duration");
    cJSON_AddStringToObject(key_maps, KEY_TIMEOUT, "timeout");
    cJSON_AddStringToObject(key_maps, KEY_SCORE, "score");
    cJSON_AddStringToObject(key_maps, KEY_USER_ID, "user_id");
    cJSON_AddStringToObject(key_maps, KEY_USER_UID, "user_uid");
    cJSON_AddStringToObject(key_maps, KEY_USER_NAME, "user_name");
    cJSON_AddStringToObject(key_maps, KEY_USER_AVATAR, "user_avatar");
    cJSON_AddStringToObject(key_maps, KEY_USER_TOKEN, "user_token");
    cJSON_AddStringToObject(key_maps, KEY_USER_TYPE, "user_type");
    cJSON_AddStringToObject(key_maps, KEY_USER_VIP, "user_vip");
    cJSON_AddStringToObject(key_maps, KEY_USER_COINS, "user_coins");
    cJSON_AddStringToObject(key_maps, KEY_USER_GEMS, "user_gems");
    cJSON_AddStringToObject(key_maps, KEY_ROOM_ID, "room_id");
    cJSON_AddStringToObject(key_maps, KEY_ROOM_UID, "room_uid");
    cJSON_AddStringToObject(key_maps, KEY_ROOM_TYPE, "room_type");
    cJSON_AddStringToObject(key_maps, KEY_ROOM_SUBTYPE, "room_subtype");
    cJSON_AddStringToObject(key_maps, KEY_ROOM_PRICE, "room_price");
    cJSON_AddStringToObject(key_maps, KEY_ROOM_STATUS, "room status");
    cJSON_AddStringToObject(key_maps, KEY_ROOM_MAINTAINED, "room maintained");
    cJSON_AddStringToObject(key_maps, KEY_DOLL_ID, "doll_id");
    cJSON_AddStringToObject(key_maps, KEY_DOLL_UID, "doll_uid");
    cJSON_AddStringToObject(key_maps, KEY_DOLL_NAME, "doll_name");
    cJSON_AddStringToObject(key_maps, KEY_DOLL_IMG, "doll_img");
    cJSON_AddStringToObject(key_maps, KEY_GRAB_ID, "grab_id");
    cJSON_AddStringToObject(key_maps, KEY_BILL_ID, "bill_id");
    cJSON_AddStringToObject(key_maps, KEY_TIPS_OCCUPY, "tips_occupy");
    cJSON_AddStringToObject(key_maps, KEY_RETAIN, "retain");
    cJSON_AddStringToObject(key_maps, KEY_RETAIN_TIME, "retain_time");
    cJSON_AddStringToObject(key_maps, KEY_GOAL, "goal");
    cJSON_AddStringToObject(key_maps, KEY_GOAL_USER, "goal_user_id");
    cJSON_AddStringToObject(key_maps, KEY_GOAL_ROOM, "goal_room_id");
    cJSON_AddStringToObject(key_maps, KEY_GOAL_PROGRESS, "goal_progress_id");
    cJSON_AddStringToObject(key_maps, KEY_GOAL_MAX, "goal_max");
    cJSON_AddStringToObject(key_maps, KEY_GAME_ID, "game_id");
    cJSON_AddStringToObject(key_maps, KEY_GAME_POS, "game_pos");
    cJSON_AddStringToObject(key_maps, KEY_GAME_RATE, "game rate");
    cJSON_AddStringToObject(key_maps, KEY_RESULT_TYPE, "result_type");
    cJSON_AddStringToObject(key_maps, KEY_OCCUPY, "occupy");
    cJSON_AddStringToObject(key_maps, KEY_OCCUPY_RATE, "occupy_rate");
    cJSON_AddStringToObject(key_maps, KEY_OCCUPY_MAX_MINS, "occupy_max_minute");
    cJSON_AddStringToObject(key_maps, KEY_ADMIN_TOKEN, "admin_token");
    cJSON_AddStringToObject(key_maps, KEY_ONLY_SERVER, "only server");
    cJSON_AddStringToObject(key_maps, KEY_AUTO_MODE, "auto mode");
    cJSON_AddStringToObject(key_maps, KEY_TYPE, "type");
    cJSON_AddStringToObject(key_maps, KEY_PASSIVE, "passive");
    cJSON_AddStringToObject(key_maps, KEY_RATE, "rate");
    cJSON_AddStringToObject(key_maps, KEY_NAME, "name");
    cJSON_AddStringToObject(key_maps, KEY_SOUL, "soul");
    cJSON_AddStringToObject(key_maps, KEY_ON, "switch on/off");
    cJSON_AddStringToObject(key_maps, KEY_GAME_FIRES, "game fires");
    cJSON_AddStringToObject(key_maps, KEY_BLACK_HOLE, "black hole");
    cJSON_AddStringToObject(key_maps, KEY_GAME_BETS, "game bets");
    cJSON_AddStringToObject(key_maps, KEY_EMPTY_SHOT, "empty shot");
    cJSON_AddStringToObject(key_maps, KEY_MAGIC_TYPE, "maigc type");
    cJSON_AddStringToObject(key_maps, KEY_DURATION, "duration");
    cJSON_AddStringToObject(key_maps, KEY_PK_MODE, "pk mode");
    cJSON_AddStringToObject(key_maps, KEY_PK_STATUS, "pk status");
    cJSON_AddStringToObject(key_maps, KEY_PK_SCORE, "pk score");
    cJSON_AddStringToObject(key_maps, KEY_PK_ID, "pk id");
    cJSON_AddStringToObject(key_maps, KEY_PK_POS, "pk pos");
    cJSON_AddStringToObject(key_maps, KEY_PK_COINCOUNT, "pk coin count");

    action_maps = cJSON_CreateObject();
    char szKey[64];
    sprintf(szKey, "%d", ACTION_ROOM_IN);
    cJSON_AddStringToObject(action_maps, szKey, "room in");
    sprintf(szKey, "%d", ACTION_ROOM_OUT);
    cJSON_AddStringToObject(action_maps, szKey, "room out");
    sprintf(szKey, "%d", ACTION_ROOM_RET);
    cJSON_AddStringToObject(action_maps, szKey, "room ret");
    sprintf(szKey, "%d", ACTION_ROOM_OPT);
    cJSON_AddStringToObject(action_maps, szKey, "room opt");
    sprintf(szKey, "%d", ACTION_ADMIN_ON);
    cJSON_AddStringToObject(action_maps, szKey, "admin On");
    sprintf(szKey, "%d", ACTION_ADMIN_OFF);
    cJSON_AddStringToObject(action_maps, szKey, "admin Off");
    sprintf(szKey, "%d", ACTION_ADMIN_SET);
    cJSON_AddStringToObject(action_maps, szKey, "admin set");
    sprintf(szKey, "%d", ACTION_ADMIN_OPT);
    cJSON_AddStringToObject(action_maps, szKey, "admin opt");
    sprintf(szKey, "%d", ACTION_USER_IN);
    cJSON_AddStringToObject(action_maps, szKey, "user in");
    sprintf(szKey, "%d", ACTION_USER_OUT);
    cJSON_AddStringToObject(action_maps, szKey, "user out");
    sprintf(szKey, "%d", ACTION_USER_DROP_COIN);
    cJSON_AddStringToObject(action_maps, szKey, "user drop coin");
    sprintf(szKey, "%d", ACTION_USER_OPT);
    cJSON_AddStringToObject(action_maps, szKey, "user opt");
    sprintf(szKey, "%d", ACTION_CAST_MAGIC);
    cJSON_AddStringToObject(action_maps, szKey, "cast magic");
    sprintf(szKey, "%d", ACTION_PK_START);
    cJSON_AddStringToObject(action_maps, szKey, "pk start");
    sprintf(szKey, "%d", ACTION_PK_END);
    cJSON_AddStringToObject(action_maps, szKey, "pk end");
    sprintf(szKey, "%d", ACTION_PK_OPERATE);
    cJSON_AddStringToObject(action_maps, szKey, "pk operate");
    sprintf(szKey, "%d", ACTION_PK_RET);
    cJSON_AddStringToObject(action_maps, szKey, "pk ret");
    sprintf(szKey, "%d", ACTION_PK_CONTROL);
    cJSON_AddStringToObject(action_maps, szKey, "pk controller");

    event_maps = cJSON_CreateObject();
    sprintf(szKey, "%d", EVENT_USER_PRESS_DOWN);
    cJSON_AddStringToObject(event_maps, szKey, "press down");
    sprintf(szKey, "%d", EVENT_USER_PRESS_UP);
    cJSON_AddStringToObject(event_maps, szKey, "press up");
    sprintf(szKey, "%d", EVENT_USER_PRESS_CLICKED);
    cJSON_AddStringToObject(event_maps, szKey, "press clicked");
    sprintf(szKey, "%d", EVENT_USER_PRESS_LONG);
    cJSON_AddStringToObject(event_maps, szKey, "press long");
    sprintf(szKey, "%d", EVENT_COUNTDOWN);
    cJSON_AddStringToObject(event_maps, szKey, "event countdown");
    sprintf(szKey, "%d", EVENT_ROOM_MAINTAINED);
    cJSON_AddStringToObject(event_maps, szKey, "room maintained");
    sprintf(szKey, "%d", EVENT_ROOM_UNMAINTAINED);
    cJSON_AddStringToObject(event_maps, szKey, "room unmaintained");
    sprintf(szKey, "%d", EVENT_GAMEOVER);
    cJSON_AddStringToObject(event_maps, szKey, "game over");
    sprintf(szKey, "%d", EVENT_USERSCORE);
    cJSON_AddStringToObject(event_maps, szKey, "user score");
    sprintf(szKey, "%d", EVENT_OPENSETTINGS);
    cJSON_AddStringToObject(event_maps, szKey, "open settings");
    sprintf(szKey, "%d", EVENT_SETTINGCONTROLL);
    cJSON_AddStringToObject(event_maps, szKey, "setting Control");
    sprintf(szKey, "%d", EVENT_USERSCORE_ALL);
    cJSON_AddStringToObject(event_maps, szKey, "user score all");
    sprintf(szKey, "%d", EVENT_STREAM_SWITCH);
    cJSON_AddStringToObject(event_maps, szKey, "stream switch");
    sprintf(szKey, "%d", EVENT_CHANGE_SERVER);
    cJSON_AddStringToObject(event_maps, szKey, "change server");
    sprintf(szKey, "%d", EVENT_REBOOT_SYSTEM);
    cJSON_AddStringToObject(event_maps, szKey, "reboot system");
    sprintf(szKey, "%d", EVENT_SCORE_RELEASE);
    cJSON_AddStringToObject(event_maps, szKey, "release score");
    sprintf(szKey, "%d", EVENT_MONSTER);
    cJSON_AddStringToObject(event_maps, szKey, "monster");
    sprintf(szKey, "%d", EVENT_MONSTER_DETECT);
    cJSON_AddStringToObject(event_maps, szKey, "monster detect");
    sprintf(szKey, "%d", EVENT_PK_IDLE);
    cJSON_AddStringToObject(event_maps, szKey, "pk idle");
    sprintf(szKey, "%d", EVENT_PK_STANDBY);
    cJSON_AddStringToObject(event_maps, szKey, "pk standby");
    sprintf(szKey, "%d", EVENT_PK_START);
    cJSON_AddStringToObject(event_maps, szKey, "pk start");
    sprintf(szKey, "%d", EVENT_PK_END);
    cJSON_AddStringToObject(event_maps, szKey, "pk end");
    sprintf(szKey, "%d", EVENT_PK_CAL);
    cJSON_AddStringToObject(event_maps, szKey, "pk cal");
    sprintf(szKey, "%d", EVENT_PK_CLOSE);
    cJSON_AddStringToObject(event_maps, szKey, "pk close");
    sprintf(szKey, "%d", EVENT_PK_UPDATE);
    cJSON_AddStringToObject(event_maps, szKey, "pk update");

    option_maps = cJSON_CreateObject();
    sprintf(szKey, "%d", OPTION_GAME_UP);
    cJSON_AddStringToObject(option_maps, szKey, "up");
    sprintf(szKey, "%d", OPTION_GAME_DOWN);
    cJSON_AddStringToObject(option_maps, szKey, "down");
    sprintf(szKey, "%d", OPTION_GAME_LEFT);
    cJSON_AddStringToObject(option_maps, szKey, "left");
    sprintf(szKey, "%d", OPTION_GAME_RIGHT);
    cJSON_AddStringToObject(option_maps, szKey, "right");
    sprintf(szKey, "%d", OPTION_GAME_A);
    cJSON_AddStringToObject(option_maps, szKey, "shoot");
    sprintf(szKey, "%d", OPTION_GAME_B);
    cJSON_AddStringToObject(option_maps, szKey, "shoot strength");
    sprintf(szKey, "%d", OPTION_GAME_C);
    cJSON_AddStringToObject(option_maps, szKey, "add scores");
    sprintf(szKey, "%d", OPTION_GAME_D);
    cJSON_AddStringToObject(option_maps, szKey, "clear scores");
    sprintf(szKey, "%d", OPTION_GAME_AB);
    cJSON_AddStringToObject(option_maps, szKey, "AB exit");
    sprintf(szKey, "%d", OPTION_GAME_X);
    cJSON_AddStringToObject(option_maps, szKey, "P2 A&B, P1 A");
}

static char* get_value_map(const char* jsonStr)
{
    cJSON* json = cJSON_Parse(jsonStr);
    cJSON* newJson = cJSON_CreateObject();

    cJSON *next = NULL, *item = json->child;
    char szKey[64];
    while (item != NULL)
    {
        next = item->next;
        cJSON* keyPtr = cJSON_GetObjectItemCaseSensitive(key_maps, item->string);
        if (keyPtr != NULL)
        {
            sprintf(szKey, "%d", item->valueint);
            if (strcmp(item->string, KEY_ACTION) == 0)
            {
                cJSON* actionPtr = cJSON_GetObjectItemCaseSensitive(action_maps, szKey);
                if (actionPtr != NULL)
                    cJSON_AddStringToObject(newJson, keyPtr->valuestring, actionPtr->valuestring);
                else
                    cJSON_AddNumberToObject(newJson, keyPtr->valuestring, item->valueint);
            }
            else if (strcmp(item->string, KEY_EVENT) == 0)
            {
                cJSON* eventPtr = cJSON_GetObjectItemCaseSensitive(event_maps, szKey);
                if (eventPtr != NULL)
                    cJSON_AddStringToObject(newJson, keyPtr->valuestring, eventPtr->valuestring);
                else
                    cJSON_AddNumberToObject(newJson, keyPtr->valuestring, item->valueint);
            }
            else if (strcmp(item->string, KEY_OPTION) == 0)
            {
                cJSON* optionPtr = cJSON_GetObjectItemCaseSensitive(option_maps, szKey);
                if (optionPtr != NULL)
                    cJSON_AddStringToObject(newJson, keyPtr->valuestring, optionPtr->valuestring);
                else
                    cJSON_AddNumberToObject(newJson, keyPtr->valuestring, item->valueint);
            }
            else
            {
                if (cJSON_IsString(item))
                    cJSON_AddStringToObject(newJson, keyPtr->valuestring, item->valuestring);
                else if (cJSON_IsNumber(item))
                    cJSON_AddNumberToObject(newJson, keyPtr->valuestring, item->valueint);
                else if (cJSON_IsArray(item) || cJSON_IsObject(item))
                {
                    cJSON* newItem = cJSON_Duplicate(item, 1);
                    cJSON_AddItemToObject(newJson, keyPtr->valuestring, newItem);
                }
            }
        }
        else
        {
            if (cJSON_IsString(item))
                cJSON_AddStringToObject(newJson, item->string, item->valuestring);
            else if (cJSON_IsNumber(item))
                cJSON_AddNumberToObject(newJson, item->string, item->valueint);
            else if (cJSON_IsArray(item) || cJSON_IsObject(item))
            {
                cJSON* newItem = cJSON_Duplicate(item, 1);
                cJSON_AddItemToObject(newJson, item->string, newItem);
            }
        }
        item = next;
    }
    char* newJsonStr = NULL;
    if (newJson != NULL)
    {
        newJsonStr = cJSON_Print(newJson);
        cJSON_Delete(newJson);
    }
    cJSON_Delete(json);

    return newJsonStr;
}

static void save_users_info(int iPosition, bool is_add = false)
{
    LOGD ("[%s] [Save Users Info] Position: %d, add new user: %d.", log_Time(), iPosition, is_add);

    char sz_user_info_file[512];
    sprintf(sz_user_info_file, "%sWaController/user_info_%d.ini", WORK_FOLDER, iPosition);
    if (is_add)
    {
        if (s_save_user_info[iPosition-1] == false)
        {
            if (access(sz_user_info_file, 0) == 0)
                remove(sz_user_info_file);

            s_save_user_info[iPosition-1] = true;
            if (s_user_in_room[iPosition-1] == 1 && s_users_info[iPosition-1] != NULL)
            {
                char* user_info = cJSON_Print(s_users_info[iPosition-1]);
                delete_char(user_info,'\n');
                char section[32];
                sprintf(section, "position%d", iPosition);
                PutIniKeyString(section, (char*)"UserInfo", user_info, sz_user_info_file);
            }
            s_save_user_info[iPosition-1] = false;
        }
    }
    else
    {
        if (access(sz_user_info_file, 0) == 0)
            remove(sz_user_info_file);
    }
}

static void* pthread_check_websocket(void *in)
{
    LOGD ("[%s] [Check Websocket]", log_Time());
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);
    while(1)
    {
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
        endpoint->check_websocet();
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
        int i = 0;
        while (i < 5)
        {
            pthread_testcancel();
            sleep(1);
            i += 1;
        }
        pthread_testcancel();
    }
    return NULL;
}

static int CreateDir(const char *sPathName)
{
    char DirName[256];
    strcpy(DirName, sPathName);
    int i,len = strlen(DirName);
    for(i=1; i<len; i++)
    {
        if(DirName[i]=='/')
        {
            DirName[i] = 0;
            //LOGD ("[%s] [CreateDir] %s", log_Time(), DirName);
            if(access(DirName, 0)!=0)
            {
                //LOGD ("[%s] [CreateDir] no dir %s", log_Time(), DirName);
                if(mkdir(DirName, 0755)==-1)
                {
                    LOGD ("[%s] [CreateDir] mkdir error", log_Time());
                    return -1;
                }
            }
            DirName[i] = '/';
        }
    }
    return 0;
}

static char* get_video_file(int is_check)
{
    char sz_image_folder[512];
    sprintf(sz_image_folder, "%sWithDraw_Image/", WORK_FOLDER);
    CreateDir(sz_image_folder);

    struct  tm      *ptm;
    struct  timeb   stTimeb;
    static  char    szTime[256] = {0};
    static  char    szFileName[256] = "";

    ftime(&stTimeb);
    ptm = localtime(&stTimeb.time);
    sprintf(szTime, "%04d-%02d-%02d-%02d-%02d-%02d", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
    if (is_check == 1)
        sprintf(szFileName, "%s%s-check-%s.jpg", sz_image_folder, room_info_ptr->szName, szTime);
    else
        sprintf(szFileName, "%s%s-score-%s.jpg", sz_image_folder, room_info_ptr->szName, szTime);
    return szFileName;
}

static char* get_empty_video_file(int is_empty)
{
    char sz_image_folder[512];
    sprintf(sz_image_folder, "%Empty_Image/", WORK_FOLDER);
    CreateDir(sz_image_folder);

    struct  tm      *ptm;
    struct  timeb   stTimeb;
    static  char    szTime[256] = {0};
    static  char    szFileName[256] = "";

    ftime(&stTimeb);
    ptm = localtime(&stTimeb.time);
    sprintf(szTime, "%04d-%02d-%02d-%02d-%02d-%02d", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
    if (is_empty == 1)
        sprintf(szFileName, "%s%s-emtpy-%s.jpg", sz_image_folder, room_info_ptr->szName, szTime);
    else {
        sprintf(szFileName, "%s%s-%s.jpg", sz_image_folder, room_info_ptr->szName, szTime);
    }
    return szFileName;
}

static Mat get_frame_image()
{
    ft_http_init();
    ft_http_client_t* http = ft_http_new();
    const char* image_data = ft_http_sync_request(http, room_info_ptr->szScreenShot, M_GET, 0, 0, 0, 0);
    int data_len = ft_http_get_body_len(http);

    Mat source_image;
    if (data_len > 0)
    {
        vector<uchar> u_data;
        for (int i = 0; i < data_len; ++i){
            u_data.push_back(image_data[i]);
        }
        try
        {
            source_image = imdecode(u_data, CV_LOAD_IMAGE_COLOR);
            source_image.channels();
        }
        catch (cv::Exception const & e) {
            LOGD ("[%s] [Get Frame Image] cv::exception: %s", log_Time(), e.what());
        }
        catch (...) {
            LOGD ("[%s] [Get Frame Image] other exception", log_Time());
        }
    }

    if (http)
        ft_http_destroy(http);
    ft_http_deinit();

    return source_image;
}

static int get_score(int iPosition, int is_check)
{
    LOGD ("[%s] [Get Score] screenshot: %s", log_Time(), room_info_ptr->szScreenShot);

    Mat source_image = get_frame_image();
    //char* file_name = get_video_file(is_check);
    //imwrite(file_name, source_image);

    LOGD ("[%s] [Get Score] Position %d, is_check: %d", log_Time(), iPosition, is_check);
    int iScore = read_score_by_image(source_image, iPosition);
    source_image.release();
    if (iScore == -3)
    {
        iScore = -1;
        LOGD ("[%s] [Get Score] source_image is less than template_image", log_Time());
    }
    return iScore;
}

static void send_countdown(int iPosition, int countdown, int is4Server, int isEmptyShot = 0)
{
    long long timestamp = get_timestamp(NULL, 1);
    long long send_time = 0;
    if (is4Server)
        send_time = s_send_time_only_server[iPosition - 1];

   if ((timestamp - send_time) >= 3000)
    {
        cJSON *root = cJSON_CreateObject();
        cJSON* itemPtr = cJSON_GetObjectItemCaseSensitive(s_users_info[iPosition-1], KEY_COINS);
        if (itemPtr != NULL)
            cJSON_AddNumberToObject(root, KEY_COINS, itemPtr->valueint);

        itemPtr = cJSON_GetObjectItemCaseSensitive(s_users_info[iPosition-1], KEY_ROOM_SUBTYPE);
        if (itemPtr != NULL)
            cJSON_AddNumberToObject(root, KEY_ROOM_SUBTYPE, itemPtr->valueint);

        itemPtr = cJSON_GetObjectItemCaseSensitive(s_users_info[iPosition-1], KEY_ROOM_PRICE);
        if (itemPtr != NULL)
            cJSON_AddNumberToObject(root, KEY_ROOM_PRICE, itemPtr->valueint);

        itemPtr = cJSON_GetObjectItemCaseSensitive(s_users_info[iPosition-1], KEY_ROOM_ID);
        if (itemPtr != NULL)
            cJSON_AddStringToObject(root, KEY_ROOM_ID, itemPtr->valuestring);

        itemPtr = cJSON_GetObjectItemCaseSensitive(s_users_info[iPosition-1], KEY_ROOM_TYPE);
        if (itemPtr != NULL)
            cJSON_AddNumberToObject(root, KEY_ROOM_TYPE, itemPtr->valueint);

        itemPtr = cJSON_GetObjectItemCaseSensitive(s_users_info[iPosition-1], KEY_USER_VIP);
        if (itemPtr != NULL)
            cJSON_AddNumberToObject(root, KEY_USER_VIP, itemPtr->valueint);

        itemPtr = cJSON_GetObjectItemCaseSensitive(s_users_info[iPosition-1], KEY_USER_ID);
        if (itemPtr != NULL)
            cJSON_AddStringToObject(root, KEY_USER_ID, itemPtr->valuestring);

        itemPtr = cJSON_GetObjectItemCaseSensitive(s_users_info[iPosition-1], KEY_GAME_RATE);
        if (itemPtr != NULL)
            cJSON_AddNumberToObject(root, KEY_GAME_RATE, itemPtr->valueint);

        cJSON_AddNumberToObject(root, KEY_GAME_POS, iPosition);
        cJSON_AddNumberToObject(root, KEY_COUNTDOWN, countdown);
        cJSON_AddNumberToObject(root, KEY_ACTION, ACTION_ROOM_OPT);
        cJSON_AddNumberToObject(root, KEY_EVENT, EVENT_COUNTDOWN);
        cJSON_AddNumberToObject(root, KEY_STATUS, 1);

        char        szT[64] = "";
        get_timestamp(szT, 1);
        cJSON_AddStringToObject(root, KEY_TIMESTAMP, szT);

        if (is4Server)
            cJSON_AddNumberToObject(root, KEY_ONLY_SERVER, 1);

        if (isEmptyShot == 1)
            cJSON_AddNumberToObject(root, KEY_EMPTY_SHOT, 1);

        char* message = cJSON_Print(root);
        cJSON_Delete(root);
        endpoint->send_message(message, (const char*)"COUNT DOWN", false);

        if (is4Server)
            s_send_time_only_server[iPosition - 1] = timestamp;
    }
}

static void send_countdown_pk(int countdown)
{
    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, KEY_ROOM_ID, room_info_ptr->szRoomId);
    cJSON_AddStringToObject(root, KEY_PK_ID, s_pk_id);
    cJSON_AddNumberToObject(root, KEY_COUNTDOWN, countdown);
    cJSON_AddNumberToObject(root, KEY_ACTION, ACTION_PK_RET);
    cJSON_AddNumberToObject(root, KEY_EVENT, EVENT_COUNTDOWN);
    cJSON_AddNumberToObject(root, KEY_STATUS, 1);

    char* message = cJSON_Print(root);
    cJSON_Delete(root);
    endpoint->send_message(message, (const char*)"COUNT DOWN for PK", false);
}

static void* pthread_withdraw_lock(void *inpos_ptr)
{
    int* pos_ptr = (int*)inpos_ptr;
    if (pos_ptr == NULL)
    {
        LOGD ("[%s] [With Draw Lock] No user.", log_Time());
        return NULL;
    }

    int iPosition = *pos_ptr;
    free(pos_ptr);

    LOGD ("[%s] [With Draw Lock] Position: %d", log_Time(), iPosition);

    int sleep_count = 30;
    while (s_with_drawing[iPosition-1] == 1)
    {
        sleep(1);
        sleep_count = sleep_count-1;
        if (sleep_count <= 0)
            s_with_drawing[iPosition-1] = 0;
    }
    LOGD ("[%s] [With Draw Lock] End, Position: %d", log_Time(), iPosition);
    return NULL;
}

static void* pthread_find_monster(void *inpos_ptr)
{
    int* pos_ptr = (int*)inpos_ptr;
    if (pos_ptr == NULL)
    {
        LOGD ("[%s] [Find Monster] No user.", log_Time());
        return NULL;
    }

    int iPosition = *pos_ptr;
    free(pos_ptr);

    long long find_time = get_timestamp(NULL, 1);
    int find_count = 3;
    bool strength_is_available = true;

    cJSON *root = cJSON_CreateObject();
    cJSON* itemPtr = cJSON_GetObjectItemCaseSensitive(s_users_info[iPosition-1], KEY_ROOM_ID);
    if (itemPtr != NULL)
        cJSON_AddStringToObject(root, KEY_ROOM_ID, itemPtr->valuestring);

    itemPtr = cJSON_GetObjectItemCaseSensitive(s_users_info[iPosition-1], KEY_USER_ID);
    if (itemPtr != NULL)
        cJSON_AddStringToObject(root, KEY_USER_ID, itemPtr->valuestring);

    itemPtr = cJSON_GetObjectItemCaseSensitive(s_users_info[iPosition-1], KEY_GRAB_ID);
    if (itemPtr != NULL)
        cJSON_AddStringToObject(root, KEY_GRAB_ID, itemPtr->valuestring);

    cJSON_AddNumberToObject(root, KEY_GAME_POS, iPosition);
    cJSON_AddNumberToObject(root, KEY_ACTION, ACTION_ROOM_OPT);
    cJSON_AddNumberToObject(root, KEY_EVENT, EVENT_MONSTER);
    cJSON_AddNumberToObject(root, KEY_STATUS, 1);

    LOGD ("[%s] [Find Monster] Position: %d, last find time: %lld", log_Time(), iPosition, s_last_monster_time[iPosition-1]);

    if ((find_time-s_last_monster_time[iPosition-1]) < 15000)
    {
        cJSON_AddNumberToObject(root, KEY_RESULT, RESULT_FAILED);
        find_count = 0;
        LOGD ("[%s] [Find Monster] Position: %d, last find time is less than 15 seconds", log_Time(), iPosition);
    }

    if (s_strength_list[iPosition-1].size() > 0)
    {
        long long last_change_time = s_strength_list[iPosition-1][s_strength_list[iPosition-1].size()-1]->ll_time;
        if ((find_time-last_change_time) < 10000)
        {
            cJSON_AddNumberToObject(root, KEY_RESULT, RESULT_FAILED);
            strength_is_available = false;
            LOGD ("[%s] [Find Monster] Position: %d, strength is unavailable, not check monster", log_Time(), iPosition);
        }
    }

    while (find_count > 0 && strength_is_available)
    {
        find_count -= 1;
        Mat source_image = get_frame_image();
        if (!source_image.empty())
        {
            int strength = get_strength_value(source_image, iPosition);
            int monster_type = get_monster_type(source_image, iPosition);
            source_image.release();
            if (s_strength_list[iPosition-1].size() > 0)
            {
                long long last_change_time = s_strength_list[iPosition-1][s_strength_list[iPosition-1].size()-1]->ll_time;
                if ((find_time-last_change_time) < 10000)
                {
                    cJSON_AddNumberToObject(root, KEY_RESULT, RESULT_FAILED);
                    LOGD ("[%s] [Find Monster] Position: %d, strength is unavailable", log_Time(), iPosition);
                    break;
                }
            }

            if (monster_type >= 0)
            {
                cJSON_AddNumberToObject(root, KEY_RESULT, RESULT_OK);
                cJSON_AddNumberToObject(root, KEY_TYPE, monster_type);
                cJSON_AddStringToObject(root, KEY_NAME, get_monster_name(monster_type));
                cJSON_AddNumberToObject(root, KEY_SOUL, strength);
                s_last_monster_time[iPosition-1] = get_timestamp(NULL, 1);
                break;
            }
            else
            {
                cJSON_AddNumberToObject(root, KEY_RESULT, RESULT_FAILED);
            }
        }
        else
        {
            LOGD ("[%s] [Find Monster] Position: %d, No Image", log_Time(), iPosition);
            cJSON_AddNumberToObject(root, KEY_RESULT, RESULT_FAILED);
        }
        LOGD ("[%s] [Find Monster] Position: %d, not find and left find_count: %ld", log_Time(), iPosition, find_count);
        if (find_count > 0)
            sleep(4);
    }

    char* message = cJSON_Print(root);
    cJSON_Delete(root);
    endpoint->send_message(message, (const char*)"FIND MONSTER", true);

    return NULL;
}

static void* pthread_check_empty_cannon(void *in_ptr)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);
    while (true)
    {
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
        Mat source_image;
        int check_position = 1;
        while (check_position <= room_info_ptr->iPlayerCount)
        {
            if (s_user_in_room[check_position-1] == 1 && s_end_play[check_position-1] == 0 && s_with_drawing[check_position-1] == 0)
            {
                if (source_image.empty())
                {
                    source_image = get_frame_image();
                }
                if (!source_image.empty())
                {
                    bool reset_countdown = s_reset_countdown[check_position-1];
                    if (check_empty_cannon(source_image, check_position))
                    {
                        s_reset_countdown[check_position-1] = false;
                    }
                    else
                    {
                        int strength = get_strength_value(source_image, check_position);
                        int iScore = read_score_by_image(source_image, check_position);
                        if (iScore >= 0 && iScore < strength)
                        {
                            s_reset_countdown[check_position-1] = false;
                        }
                        else
                        {
                            s_reset_countdown[check_position-1] = true;
                        }
                    }

                    if (reset_countdown == true && s_reset_countdown[check_position-1] == false)
                    {
                        send_countdown(check_position, s_count_down[check_position-1], 0, 1);
                    }
                    if (reset_countdown == false && s_reset_countdown[check_position-1] == true)
                        s_resend_countdown[check_position-1] = true;
                }
            }
            check_position += 1;
        }
        if (!source_image.empty())
        {
            source_image.release();
            //LOGD ("[%s] [Check Empty Cannon] ", log_Time());
        }
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
        int i = 0;
        while (i < 30)
        {
            pthread_testcancel();
            sleep(1);
            i += 1;
        }
        pthread_testcancel();
    }
    return  NULL;
}

static void* pthread_check_all_scores(void *in_ptr)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);
    sleep(30);
    while (s_pk_started == 1)
    {
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);

        cJSON* root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, KEY_PK_STATUS, 1);
        cJSON_AddStringToObject(root, KEY_ROOM_ID, room_info_ptr->szRoomId);
        cJSON_AddStringToObject(root, KEY_PK_ID, s_pk_id);
        cJSON_AddNumberToObject(root, KEY_ACTION, ACTION_PK_RET);
        cJSON_AddNumberToObject(root, KEY_EVENT, EVENT_PK_UPDATE);

        Mat source_image;
        int check_position = 1;
        int iScoreArray[room_info_ptr->iPlayerCount] = {0};
        while (check_position <= room_info_ptr->iPlayerCount)
        {
            if (s_pk_started == 1)
            {
                if (source_image.empty())
                {
                    source_image = get_frame_image();
                }
                if (!source_image.empty())
                {
                    int iScore = read_score_by_image(source_image, check_position);
                    iScoreArray[check_position-1] = iScore;
                }
            }
            check_position += 1;
        }

        if (!source_image.empty())
        {
            source_image.release();
        }

        cJSON* scorePtr = cJSON_CreateIntArray(iScoreArray, room_info_ptr->iPlayerCount);
        cJSON_AddItemToObject(root, KEY_PK_SCORE, scorePtr);

        char* message = cJSON_Print(root);
        endpoint->send_message(message, (char*)"Return PK Scores", true);
        cJSON_Delete(root);

        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
        int i = 0;
        while (i < 30)
        {
            pthread_testcancel();
            sleep(1);
            i += 1;
        }
        pthread_testcancel();
    }
    return  NULL;
}

static void* pthread_user_exit(void *inpos_ptr)
{
    int* pos_ptr = (int*)inpos_ptr;
    if (pos_ptr == NULL)
    {
        LOGD ("[%s] [User Exit] No user.", log_Time());
        return NULL;
    }

    int iPosition = *pos_ptr;
    free(pos_ptr);
    while (s_with_drawing[iPosition-1] == 1)
        continue;

    sleep(5);

    do {

        int i = 0, k = 0;

        for (i = 0 ; i < 6; i ++)
        {
            Detect_Data detect_data = s_weapon_detect[iPosition-1][i];
            if (detect_data.ll_time != 0)
            {
//                LOGD ("[%s] [User Exit] weapon detect position = %d, weapon type = %d", log_Time(), iPosition, i);
                break;
            }
        }

        for (k = 0; k < 6; k ++)
        {
            Detect_Data detect_data = s_monster_detect[iPosition-1][k];
            if (detect_data.ll_time != 0)
            {
//                LOGD ("[%s] [User Exit] monster detect position = %d, monster type = %d", log_Time(), iPosition, k);
                break;
            }
        }
        if (i >= 6 && k >= 6)
        {
            LOGD ("[%s] [User Exit] No weapon or monster, user can exit.", log_Time());
            break;
        }
        sleep(1);
    }while(true);

    s_with_drawing[iPosition-1] = 1;
    pthread_t pid;
    int* withdraw_pos_ptr = (int*)malloc(sizeof(int));
    *withdraw_pos_ptr = iPosition;
    pthread_create(&pid, NULL, pthread_withdraw_lock, withdraw_pos_ptr);
    pthread_detach(pid);

    if (s_is_auto_shoot[iPosition-1] == 1)
        s_is_auto_shoot[iPosition-1] = 0;

    if (s_is_auto_shoot_strength[iPosition-1] == 1)
        s_is_auto_shoot_strength[iPosition-1] = 0;

    s_count_down[iPosition-1] = 30;
    send_countdown(iPosition, s_count_down[iPosition-1], 1);

    cJSON* game_over_msg = cJSON_Duplicate(s_users_info[iPosition-1], 1);

    int check_count = 0;
    int check_same_count = 0;
    int is_failed_snap = 0;
    int iScore = 0;

    while (1)
    {
        iScore = get_score(iPosition, 0);
        if (iScore > 0 || (iScore == 0 && check_count >= 1))
            break;
        if (iScore == -2)
        {
            check_same_count += 1;
            sleep(1);
            if (check_same_count > 10)
            {
                LOGD ("[%s] [User Exit] Position: %d check same picture while get score.", log_Time(), iPosition);
                is_failed_snap = true;
                break;
            }
            else
                continue;
        }

        check_count += 1;
        LOGD ("[%s] [User Exit] check user score count: %d", log_Time(), check_count);
        if (check_count > 5)
            break;
        check_same_count = 0;
        usleep(500000);
    }

    LOGD ("[%s] [User Exit] Position: %d; user has score: %d", log_Time(), iPosition, iScore);

    if (iScore >= 0)
    {
        char        szT[64] = "";
        long long exit_time = get_timestamp(szT, 1);

        cJSON* msg = cJSON_Duplicate(s_users_info[iPosition-1], 1);
        if (cJSON_HasObjectItem(msg, KEY_TIMESTAMP))
            cJSON_DeleteItemFromObject(msg, KEY_TIMESTAMP);
        cJSON_AddStringToObject(msg, KEY_TIMESTAMP, szT);
        if (cJSON_HasObjectItem(msg, KEY_ACTION))
            cJSON_DeleteItemFromObject(msg, KEY_ACTION);
        cJSON_AddNumberToObject(msg, KEY_ACTION, ACTION_ROOM_RET);
        if (cJSON_HasObjectItem(msg, KEY_EVENT))
            cJSON_DeleteItemFromObject(msg, KEY_EVENT);
        cJSON_AddNumberToObject(msg, KEY_EVENT, EVENT_USERSCORE);
        if (cJSON_HasObjectItem(msg, KEY_SCORE))
            cJSON_DeleteItemFromObject(msg, KEY_SCORE);
        cJSON_AddNumberToObject(msg, KEY_SCORE, iScore);
        long long user_play_time = exit_time - s_start_play_time[iPosition-1];
        if (user_play_time > 5*60*1000)
        {
            uint average_shoot_count = s_shoot_count[iPosition-1]/(user_play_time/(60*1000));
            cJSON_AddNumberToObject(msg, KEY_GAME_FIRES, average_shoot_count);
        }

        char* message = cJSON_Print(msg);
        endpoint->send_message(message, (const char*)"USER EXIT", true);
        cJSON_Delete(msg);

        if (iScore > 0)
        {
            controller_user_clear_score(iPosition);

            int is_sub_scores = 0;
            int iCheck_Score = 0;
            check_count = 0;
            check_same_count = 0;
            while (true)
            {
                iCheck_Score = get_score(iPosition, 1);
                LOGD ("[%s] [User Exit] Position: %d, check score: %d,  check count: %d", log_Time(), iPosition, iCheck_Score, check_count);
                if (is_sub_scores == false && ((iCheck_Score > 0 && iCheck_Score != iScore) || (iCheck_Score == iScore && check_count == 2)))
                {
                    controller_user_clear_score(iPosition);
                    is_sub_scores = 1;
                }
                else if (iCheck_Score == 0)
                    break;

                if (iCheck_Score == -1)
                {
                    check_same_count += 1;
                    sleep(1);
                    if (check_same_count > 10)
                    {
                        is_failed_snap = 1;
                        LOGD ("[%s] [User Exit] Position: %d check same picture while check score", log_Time(), iPosition);
                        break;
                    }
                    else
                        continue;
                }

                check_count += 1;
                if (check_count > 5)
                    break;
                check_same_count = 0;
                sleep(1);
            }

            if (iCheck_Score == -2)
            {
                if (cJSON_HasObjectItem(game_over_msg, KEY_MESSAGE))
                    cJSON_DeleteItemFromObject(game_over_msg, KEY_MESSAGE);
                cJSON_AddStringToObject(game_over_msg, KEY_MESSAGE, "树莓派截图问题");
                if (cJSON_HasObjectItem(game_over_msg, KEY_STATUS))
                    cJSON_DeleteItemFromObject(game_over_msg, KEY_STATUS);
                cJSON_AddNumberToObject(game_over_msg, KEY_STATUS, 0);
                if (cJSON_HasObjectItem(game_over_msg, KEY_CODE))
                    cJSON_DeleteItemFromObject(game_over_msg, KEY_CODE);
                cJSON_AddNumberToObject(game_over_msg, KEY_CODE, 2);
            }
            else if (iCheck_Score != 0)
            {
                char    szMessage[128];
                sprintf(szMessage, "机位%d下分失败", iPosition);
                if (cJSON_HasObjectItem(game_over_msg, KEY_MESSAGE))
                    cJSON_DeleteItemFromObject(game_over_msg, KEY_MESSAGE);
                cJSON_AddStringToObject(game_over_msg, KEY_MESSAGE, szMessage);
                if (cJSON_HasObjectItem(game_over_msg, KEY_STATUS))
                    cJSON_DeleteItemFromObject(game_over_msg, KEY_STATUS);
                cJSON_AddNumberToObject(game_over_msg, KEY_STATUS, 0);
                if (cJSON_HasObjectItem(game_over_msg, KEY_CODE))
                    cJSON_DeleteItemFromObject(game_over_msg, KEY_CODE);
                cJSON_AddNumberToObject(game_over_msg, KEY_CODE, 1);
            }
        }
    }
    else if (iScore == -1)
    {
        char    szMessage[128];
        sprintf(szMessage, "机位%d无法取得结算的分数", iPosition);
        if (cJSON_HasObjectItem(game_over_msg, KEY_MESSAGE))
            cJSON_DeleteItemFromObject(game_over_msg, KEY_MESSAGE);
        cJSON_AddStringToObject(game_over_msg, KEY_MESSAGE, szMessage);
        if (cJSON_HasObjectItem(game_over_msg, KEY_STATUS))
            cJSON_DeleteItemFromObject(game_over_msg, KEY_STATUS);
        cJSON_AddNumberToObject(game_over_msg, KEY_STATUS, 0);
        if (cJSON_HasObjectItem(game_over_msg, KEY_CODE))
            cJSON_DeleteItemFromObject(game_over_msg, KEY_CODE);
        cJSON_AddNumberToObject(game_over_msg, KEY_CODE, 0);
    }
    else if (iScore == -2)
    {
        if (cJSON_HasObjectItem(game_over_msg, KEY_MESSAGE))
            cJSON_DeleteItemFromObject(game_over_msg, KEY_MESSAGE);
        cJSON_AddStringToObject(game_over_msg, KEY_MESSAGE, "树莓派截图问题");
        if (cJSON_HasObjectItem(game_over_msg, KEY_STATUS))
            cJSON_DeleteItemFromObject(game_over_msg, KEY_STATUS);
        cJSON_AddNumberToObject(game_over_msg, KEY_STATUS, 0);
        if (cJSON_HasObjectItem(game_over_msg, KEY_CODE))
            cJSON_DeleteItemFromObject(game_over_msg, KEY_CODE);
        cJSON_AddNumberToObject(game_over_msg, KEY_CODE, 2);
    }

    char        szT[64] = "";
    long long exit_time = get_timestamp(szT, true);

    if (cJSON_HasObjectItem(game_over_msg, KEY_TIMESTAMP))
        cJSON_DeleteItemFromObject(game_over_msg, KEY_TIMESTAMP);
    cJSON_AddStringToObject(game_over_msg, KEY_TIMESTAMP, szT);
    if (cJSON_HasObjectItem(game_over_msg, KEY_EVENT))
        cJSON_DeleteItemFromObject(game_over_msg, KEY_EVENT);
    cJSON_AddNumberToObject(game_over_msg, KEY_EVENT, EVENT_GAMEOVER);
    if (cJSON_HasObjectItem(game_over_msg, KEY_ACTION))
        cJSON_DeleteItemFromObject(game_over_msg, KEY_ACTION);
    cJSON_AddNumberToObject(game_over_msg, KEY_ACTION, ACTION_ROOM_RET);
    if (cJSON_HasObjectItem(game_over_msg, KEY_ACTION_FROM))
        cJSON_DeleteItemFromObject(game_over_msg, KEY_ACTION_FROM);
    cJSON_AddNumberToObject(game_over_msg, KEY_ACTION_FROM, s_action_from[iPosition-1]);

    long long user_play_time = exit_time - s_start_play_time[iPosition-1];
    if (user_play_time > 5*60*1000)
    {
        uint average_shoot_count = s_shoot_count[iPosition-1]/(user_play_time/(60*1000));
        cJSON_AddNumberToObject(game_over_msg, KEY_GAME_FIRES, average_shoot_count);
    }

    cJSON_Delete(s_users_info[iPosition -1]);
    s_users_info[iPosition -1] = NULL;

    s_user_in_room[iPosition-1] = 0;
    s_operate_count[iPosition-1] = 0;
    s_direction_count[iPosition-1] = 0;
    s_black_hole_user[iPosition-1] = 0;
    s_check_baoji_status[iPosition-1] = 0;
    s_auto_start_time[iPosition-1] = 0;
    s_auto_baoji_starttime[iPosition-1] = 0;
    save_users_info(iPosition);

    while (s_strength_list[iPosition-1].size() > 0)
    {
        const vector<STRENGTH_DATA*>::iterator it = s_strength_list[iPosition-1].begin();
        if (it != s_strength_list[iPosition-1].end())
        {
            STRENGTH_DATA* s_data = *it;
            s_strength_list[iPosition-1].erase(it);
            if (s_data != NULL)
                free(s_data);
        }
    }

    sleep(10);

    char* message = cJSON_Print(game_over_msg);
    endpoint->send_message(message, (const char*)"USER EXIT", true);
    cJSON_Delete(game_over_msg);

    s_with_drawing[iPosition-1] = 0;

    if (is_failed_snap)
    {

    }
    return NULL;
}


static void* pthread_end_pk(void *in)
{
    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, KEY_ROOM_ID, room_info_ptr->szRoomId);
    cJSON_AddStringToObject(root, KEY_PK_ID, s_pk_id);
    cJSON_AddNumberToObject(root, KEY_ACTION, ACTION_PK_RET);
    cJSON_AddNumberToObject(root, KEY_EVENT, EVENT_PK_END);
    cJSON_AddNumberToObject(root, KEY_STATUS, 1);

    char* message = cJSON_Print(root);
    cJSON_Delete(root);
    endpoint->send_message(message, (const char*)"PK END", false);

    sleep(2);

    cJSON *msg = cJSON_CreateObject();

    cJSON_AddStringToObject(msg, KEY_ROOM_ID, room_info_ptr->szRoomId);
    cJSON_AddStringToObject(msg, KEY_PK_ID, s_pk_id);
    cJSON_AddNumberToObject(msg, KEY_ACTION, ACTION_PK_RET);
    cJSON_AddNumberToObject(msg, KEY_EVENT, EVENT_PK_CAL);
    cJSON_AddNumberToObject(msg, KEY_STATUS, 1);

    Mat source_image;
    int check_position = 1;
    int check_count = 0;
    int iScoreArray[room_info_ptr->iPlayerCount] = {0};
    while (check_position <= room_info_ptr->iPlayerCount && check_count < 5)
    {
        if (source_image.empty())
        {
            source_image = get_frame_image();
        }
        if (!source_image.empty())
        {
            int iScore = read_score_by_image(source_image, check_position);
            if (iScore >= 0)
            {
                iScoreArray[check_position-1] = iScore;
            }
            else
            {
                if (!source_image.empty())
                {
                    source_image.release();
                }

                check_position = 1;
                check_count += 1;
                LOGD ("[%s] [pthread_end_pk] check user score count: %d", log_Time(), check_count);
                usleep(500000);
                continue;
            }
        }
        check_position += 1;
    }

    if (!source_image.empty())
    {
        source_image.release();
    }

    if (check_count >= 5 || check_position < room_info_ptr->iPlayerCount)
    {
        for (int i = 0; i < room_info_ptr->iPlayerCount; i ++)
        {
            iScoreArray[i] = -1;
        }
    }

    cJSON* scorePtr = cJSON_CreateIntArray(iScoreArray, room_info_ptr->iPlayerCount);
    cJSON_AddItemToObject(msg, KEY_PK_SCORE, scorePtr);

    char* message1 = cJSON_Print(msg);
    endpoint->send_message(message1, (char*)"PK End & Return Scores", true);
    cJSON_Delete(msg);

    int* pos_ptr = (int*)malloc(sizeof(int)*room_info_ptr->iPlayerCount);
    for(int i = 0; i < room_info_ptr->iPlayerCount; i ++)
        pos_ptr[i] = i+1;

    controller_admin_clear_scores(pos_ptr, room_info_ptr->iPlayerCount);
    free(pos_ptr);

    return NULL;
}

static void end_game(int iPosition)
{
    if (s_pk_mode == 1)
    {
        s_pk_end_time = 0;
        s_pk_start_time = 0;
        s_pk_started = 0;

        if (s_pk_close == 0)
        {
            pthread_t pid;
            pthread_create(&pid, NULL, pthread_end_pk, NULL);
            pthread_detach(pid);
        }

        for (int i = 0; i < 4; i ++)
        {
            s_pk_enable_pos[i] = 0;
        }
    }
    else
    {
        s_end_play[iPosition-1] = 1;
        LOGD ("[%s] [End Game] Position: %d, end_play = %d", log_Time(), iPosition, s_end_play[iPosition-1]);

        send_countdown(iPosition, 0, 0);

        sleep(1);
        if (s_user_in_room[iPosition-1] == 1)
        {
            pthread_t pid;
            int* pos_ptr = (int*)malloc(sizeof(int));
            *pos_ptr = iPosition;
            pthread_create(&pid, NULL, pthread_user_exit, pos_ptr);
            pthread_detach(pid);
        }
    }
}

static void* pthread_countdown(void *inpos_ptr)
{
    int* pos_ptr = (int*)inpos_ptr;
    if (pos_ptr == NULL)
    {
        LOGD ("[%s] [Count Down] No user.", log_Time());
        return NULL;
    }
    LOGD ("[%s] [Count Down]", log_Time());

    int iPosition = *pos_ptr;
    free(pos_ptr);
    pos_ptr = NULL;

    s_in_countdown_thread[iPosition-1] = 1;
    s_count_down[iPosition-1] = s_game_time[iPosition-1];
    while (s_end_play[iPosition-1] == 0)
    {
        if (s_count_down[iPosition-1] <= 0)
        {
            end_game(iPosition);
            break;
        }
        sleep(1);
        s_count_down[iPosition-1] = s_count_down[iPosition-1]-1;
        if (s_count_down[iPosition-1] > 0 && s_count_down[iPosition-1] < 60 && (s_count_down[iPosition-1] % 10 == 0))
        {
            LOGD ("[%s] [Count Down] Position: %d, countdown = %d", log_Time(), iPosition, s_count_down[iPosition-1]);
            send_countdown(iPosition, s_count_down[iPosition-1], 0);
        }
    }
    s_in_countdown_thread[iPosition-1] = 0;
    LOGD ("[%s] [Count Down] End", log_Time());

    return NULL;
}

static void* pthread_countdown_pk(void *in)
{
    LOGD ("[%s] [Count Down for PK]", log_Time());

    long long current_time = get_timestamp(NULL, 1);
    while ((s_pk_end_time-current_time) > 0)
    {
        long long remaining_time = (s_pk_end_time-current_time)/1000;
        if (remaining_time <= 60 && (remaining_time % 10 == 0))
        {
            LOGD ("[%s] [Count Down for PK] countdown = %lld", log_Time(), remaining_time);
            send_countdown_pk(remaining_time);
        }
        sleep(1);
        current_time = get_timestamp(NULL, 1);
    }

    end_game(0);

    LOGD ("[%s] [Count Down for PK] End", log_Time());

    return NULL;
}

static void* pthread_userdrop(void *inpos_ptr)
{
    int* pos_ptr = (int*)inpos_ptr;
    if (pos_ptr == NULL)
    {
        LOGD ("[%s] [User Drop] No user.", log_Time());
        return NULL;
    }

    int iPosition = *pos_ptr;
    free(pos_ptr);
    pos_ptr = NULL;

    LOGD ("[%s] [pthread_userdrop] iPosition: %d Begin.", log_Time(), iPosition);

    while (s_user_in_room[iPosition-1] == 1)
    {
        if (s_drop_count[iPosition-1] > 0)
        {
            s_drop_count[iPosition-1] --;

            LOGD ("[%s] [User Drop] Player %d add Scores", log_Time(), iPosition);

            controller_user_drop(iPosition);
            s_reset_countdown[iPosition-1] = true;
            s_resend_countdown[iPosition-1] = false;
            s_count_down[iPosition-1] = s_game_time[iPosition-1];
            s_with_drawing[iPosition-1] = 0;

            if (s_in_countdown_thread[iPosition-1] == 0)
            {
                pthread_t pid;
                int* pos_ptr = (int*)malloc(sizeof(int));
                *pos_ptr = iPosition;
                pthread_create(&pid, NULL, pthread_countdown, pos_ptr);
                pthread_detach(pid);
            }
        }
        usleep(70000);
    }

    s_user_drop_pid[iPosition-1] = 0;
    s_drop_count[iPosition-1] = 0;

    LOGD ("[%s] [pthread_userdrop] iPosition: %d End.", log_Time(), iPosition);

    return NULL;
}

static void user_drop(cJSON* jsonPtr)
{
    cJSON* gamePosPtr = cJSON_GetObjectItemCaseSensitive(jsonPtr, KEY_GAME_POS);
    if (gamePosPtr != NULL && gamePosPtr->valueint >= 1 && gamePosPtr->valueint <= room_info_ptr->iPlayerCount)
    {
        if (s_with_drawing[gamePosPtr->valueint - 1] == 1)
        {
            LOGD ("[%s] [User Drop] with drawing, can't user drop in position: %d", log_Time(), gamePosPtr->valueint);
            return ;
        }
        cJSON* countdownPtr = cJSON_GetObjectItemCaseSensitive(jsonPtr, KEY_COUNTDOWN);
        s_game_time[gamePosPtr->valueint-1] = countdownPtr->valueint;
        if (s_users_info[gamePosPtr->valueint-1] == NULL)
        {
            s_new_user[gamePosPtr->valueint-1] = 1;
            s_start_play_time[gamePosPtr->valueint-1] = get_timestamp(NULL, 1);
            s_shoot_count[gamePosPtr->valueint-1] = 0;
            s_check_baoji_status[gamePosPtr->valueint-1] = 0;
        }
        else
        {
            if (strcmp(cJSON_GetObjectItemCaseSensitive(jsonPtr, KEY_USER_ID)->valuestring, cJSON_GetObjectItemCaseSensitive(s_users_info[gamePosPtr->valueint-1], KEY_USER_ID)->valuestring) != 0)
                s_new_user[gamePosPtr->valueint-1] = 1;
            else
                s_new_user[gamePosPtr->valueint-1] = 0;
        }

        if (s_new_user[gamePosPtr->valueint-1] == 1 && room_info_ptr->iSupportMonster)
        {
            while (s_strength_list[gamePosPtr->valueint-1].size() > 0)
            {
                const vector<STRENGTH_DATA*>::iterator it = s_strength_list[gamePosPtr->valueint-1].begin();
                if (it != s_strength_list[gamePosPtr->valueint-1].end())
                {
                    STRENGTH_DATA* s_data = *it;
                    s_strength_list[gamePosPtr->valueint-1].erase(it);
                    if (s_data != NULL)
                        free(s_data);
                }
            }
            long long ll_change_time = get_timestamp(NULL, 1);
            int strength = 0;
            Mat source_image = get_frame_image();
            if (!source_image.empty())
            {
                strength = get_strength_value(source_image, gamePosPtr->valueint);
                source_image.release();
            }
            if (strength > 0)
            {
                STRENGTH_DATA* strength_data = (STRENGTH_DATA*)malloc(sizeof(STRENGTH_DATA));
                strength_data->ll_time = ll_change_time;
                strength_data->strength_value = strength;
                s_strength_list[gamePosPtr->valueint-1].push_back(strength_data);
            }
        }
        if (s_users_info[gamePosPtr->valueint-1] != NULL)
        {
            cJSON_Delete(s_users_info[gamePosPtr->valueint-1]);
            s_users_info[gamePosPtr->valueint-1] = NULL;
        }
        s_users_info[gamePosPtr->valueint-1] = cJSON_Duplicate(jsonPtr, 1);

        s_user_in_room[gamePosPtr->valueint-1] = 1;
        s_action_from[gamePosPtr->valueint-1] = 0;
        if (s_new_user[gamePosPtr->valueint-1] == 1)
        {
            if (s_end_play[gamePosPtr->valueint-1] == 1)
                s_end_play[gamePosPtr->valueint-1] = 0;
            save_users_info(gamePosPtr->valueint, true);
        }

        cJSON* itemPtr = cJSON_GetObjectItemCaseSensitive(s_users_info[gamePosPtr->valueint-1], KEY_BLACK_HOLE);
        if (itemPtr != NULL)
            s_black_hole_user[gamePosPtr->valueint-1] = itemPtr->valueint;

        s_drop_count[gamePosPtr->valueint-1] ++;
        if (s_user_drop_pid[gamePosPtr->valueint-1] == 0)
        {
            int* pos_ptr = (int*)malloc(sizeof(int));
            *pos_ptr = gamePosPtr->valueint;
            pthread_create(&s_user_drop_pid[gamePosPtr->valueint-1], NULL, pthread_userdrop, pos_ptr);
            pthread_detach(s_user_drop_pid[gamePosPtr->valueint-1]);
        }
    }
}

static void user_out_manually(int iPosition)
{
    LOGD ("[%s] [User Out Manually] position: %d ", log_Time(), iPosition);
    if (s_is_auto_shoot[iPosition-1] == 1)
        s_is_auto_shoot[iPosition-1] = 0;
    if (s_is_auto_shoot_strength[iPosition-1] == 1)
        s_is_auto_shoot_strength[iPosition-1] = 0;

    if (s_count_down[iPosition-1] > 0)
    {
        s_action_from[iPosition-1] = 1;
        s_count_down[iPosition-1] = 0;
    }
}

static void* pthread_with_draw(void *inpos_ptr)
{
    int* pos_ptr = (int*)inpos_ptr;
    if (pos_ptr == NULL)
    {
        LOGD ("[%s] [With Draw] No user.", log_Time());
        return NULL;
    }

    int iPosition = *pos_ptr;
    free(pos_ptr);
    pos_ptr = NULL;

    if (s_with_drawing[iPosition-1] == 0)
    {
        s_with_drawing[iPosition-1] = 1;
        pthread_t pid;
        int* withdraw_pos_ptr = (int*)malloc(sizeof(int));
        *withdraw_pos_ptr = iPosition;
        pthread_create(&pid, NULL, pthread_withdraw_lock, withdraw_pos_ptr);
        pthread_detach(pid);

        if (s_is_auto_shoot_strength[iPosition-1] == 1)
            s_is_auto_shoot_strength[iPosition-1] = 0;
        if (s_is_auto_shoot[iPosition-1] == 1)
            s_is_auto_shoot[iPosition-1] = 0;

        s_count_down[iPosition-1] = s_game_time[iPosition-1];
        send_countdown(iPosition, s_count_down[iPosition-1], 1);

        LOGD ("[%s] [User WithDraw] Position: %d", log_Time(), iPosition);

        cJSON* game_over_msg = cJSON_Duplicate(s_users_info[iPosition-1], 1);

        int check_count = 0;
        int check_same_count = 0;
        int is_failed_snap = 0;
        int iScore = 0;
        int is_success = 0;

        while (1)
        {
            iScore = get_score(iPosition, 0);
            if (iScore > 0 || (iScore == 0 && check_count >= 1))
                break;
            if (iScore == -2)
            {
                check_same_count += 1;
                sleep(1);
                if (check_same_count > 10)
                {
                    LOGD ("[%s] [User WithDraw] Position: %d check same picture while get score.", log_Time(), iPosition);
                    is_failed_snap = true;
                    break;
                }
                else
                    continue;
            }

            check_count += 1;
            LOGD ("[%s] [User WithDraw] check user score count: %d", log_Time(), check_count);
            if (check_count > 5)
                break;
            check_same_count = 0;
            usleep(500000);
        }
        LOGD ("[%s] [User WithDraw] Position: %d,  user has score: %d", log_Time(), iPosition, iScore);

        if (iScore >= 0)
        {
            char        szT[64] = "";
            get_timestamp(szT, true);

            cJSON* msg = cJSON_Duplicate(s_users_info[iPosition-1], 1);
            if (cJSON_HasObjectItem(msg, KEY_TIMESTAMP))
                cJSON_DeleteItemFromObject(msg, KEY_TIMESTAMP);
            cJSON_AddStringToObject(msg, KEY_TIMESTAMP, szT);
            if (cJSON_HasObjectItem(msg, KEY_ACTION))
                cJSON_DeleteItemFromObject(msg, KEY_ACTION);
            cJSON_AddNumberToObject(msg, KEY_ACTION, ACTION_ROOM_RET);
            if (cJSON_HasObjectItem(msg, KEY_EVENT))
                cJSON_DeleteItemFromObject(msg, KEY_EVENT);
            cJSON_AddNumberToObject(msg, KEY_EVENT, EVENT_USERSCORE);
            if (cJSON_HasObjectItem(msg, KEY_SCORE))
                cJSON_DeleteItemFromObject(msg, KEY_SCORE);
            cJSON_AddNumberToObject(msg, KEY_SCORE, iScore);

            if (cJSON_HasObjectItem(msg, KEY_TIMEOUT))
                cJSON_DeleteItemFromObject(msg, KEY_TIMEOUT);
            if (iScore > 0)
                cJSON_AddNumberToObject(msg, KEY_TIMEOUT, 20);

            char* message = cJSON_Print(msg);
            endpoint->send_message(message, (const char*)"User WithDraw", true);
            cJSON_Delete(msg);

            if (iScore == 0)
            {
                is_success = 1;
            }
            else if (iScore > 0)
            {
                controller_user_clear_score(iPosition);

                bool is_sub_scores = false;
                int iCheck_Score = 0;
                check_count = 0;
                check_same_count = 0;
                while (true)
                {
                    iCheck_Score = get_score(iPosition, 1);
                    LOGD ("[%s] [User WithDraw] Position: %d, check score: %d,  check count: %d", log_Time(), iPosition, iCheck_Score, check_count);
                    if (is_sub_scores == false && ((iCheck_Score > 0 && iCheck_Score != iScore) || (iCheck_Score == iScore && check_count == 2)))
                    {
                        controller_user_clear_score(iPosition);
                        is_sub_scores = true;
                    }
                    else if (iCheck_Score == 0)
                        break;

                    if (iCheck_Score == -1)
                    {
                        check_same_count += 1;
                        sleep(1);
                        if (check_same_count > 10)
                        {
                            is_failed_snap = true;
                            LOGD ("[%s] [User WithDraw] Position: %d check same picture while check score", log_Time(), iPosition);
                            break;
                        }
                        else
                            continue;
                    }

                    check_count += 1;
                    if (check_count > 5)
                        break;
                    check_same_count = 0;
                    sleep(1);
                }

                if (iCheck_Score == 0)
                {
                    char        szT[64] = "";
                    get_timestamp(szT, true);

                    cJSON* msg = cJSON_Duplicate(s_users_info[iPosition-1], 1);
                    if (cJSON_HasObjectItem(msg, KEY_TIMESTAMP))
                        cJSON_DeleteItemFromObject(msg, KEY_TIMESTAMP);
                    cJSON_AddStringToObject(msg, KEY_TIMESTAMP, szT);
                    if (cJSON_HasObjectItem(msg, KEY_ACTION))
                        cJSON_DeleteItemFromObject(msg, KEY_ACTION);
                    cJSON_AddNumberToObject(msg, KEY_ACTION, ACTION_ROOM_RET);
                    if (cJSON_HasObjectItem(msg, KEY_EVENT))
                        cJSON_DeleteItemFromObject(msg, KEY_EVENT);
                    cJSON_AddNumberToObject(msg, KEY_EVENT, EVENT_USERSCORE);
                    if (cJSON_HasObjectItem(msg, KEY_SCORE))
                        cJSON_DeleteItemFromObject(msg, KEY_SCORE);
                    cJSON_AddNumberToObject(msg, KEY_SCORE, 0);

                    char* message = cJSON_Print(msg);
                    endpoint->send_message(message, (const char*)"User WithDraw", true);
                    cJSON_Delete(msg);
                    is_success = 1;
                }
                else
                {
                    if (iCheck_Score == -2)
                    {
                        if (cJSON_HasObjectItem(game_over_msg, KEY_MESSAGE))
                            cJSON_DeleteItemFromObject(game_over_msg, KEY_MESSAGE);
                        cJSON_AddStringToObject(game_over_msg, KEY_MESSAGE, "树莓派截图问题");
                        cJSON_AddNumberToObject(game_over_msg, KEY_STATUS, 0);
                        cJSON_AddNumberToObject(game_over_msg, KEY_CODE, 2);
                    }
                    else
                    {
                        char    szMessage[128];
                        sprintf(szMessage, "机位%d下分失败", iPosition);
                        if (cJSON_HasObjectItem(game_over_msg, KEY_MESSAGE))
                            cJSON_DeleteItemFromObject(game_over_msg, KEY_MESSAGE);
                        cJSON_AddStringToObject(game_over_msg, KEY_MESSAGE, szMessage);
                        if (cJSON_HasObjectItem(game_over_msg, KEY_STATUS))
                            cJSON_DeleteItemFromObject(game_over_msg, KEY_STATUS);
                        cJSON_AddNumberToObject(game_over_msg, KEY_STATUS, 0);
                        if (cJSON_HasObjectItem(game_over_msg, KEY_CODE))
                            cJSON_DeleteItemFromObject(game_over_msg, KEY_CODE);
                        cJSON_AddNumberToObject(game_over_msg, KEY_CODE, 1);
                    }
                }
            }
        }
        else if (iScore == -1)
        {
            char    szMessage[128];
            sprintf(szMessage, "机位%d无法取得结算的分数", iPosition);
            if (cJSON_HasObjectItem(game_over_msg, KEY_MESSAGE))
                cJSON_DeleteItemFromObject(game_over_msg, KEY_MESSAGE);
            cJSON_AddStringToObject(game_over_msg, KEY_MESSAGE, szMessage);
            if (cJSON_HasObjectItem(game_over_msg, KEY_STATUS))
                cJSON_DeleteItemFromObject(game_over_msg, KEY_STATUS);
            cJSON_AddNumberToObject(game_over_msg, KEY_STATUS, 0);
            if (cJSON_HasObjectItem(game_over_msg, KEY_CODE))
                cJSON_DeleteItemFromObject(game_over_msg, KEY_CODE);
            cJSON_AddNumberToObject(game_over_msg, KEY_CODE, 0);
        }
        else if (iScore == -2)
        {
            if (cJSON_HasObjectItem(game_over_msg, KEY_MESSAGE))
                cJSON_DeleteItemFromObject(game_over_msg, KEY_MESSAGE);
            cJSON_AddStringToObject(game_over_msg, KEY_MESSAGE, "树莓派截图问题");
            if (cJSON_HasObjectItem(game_over_msg, KEY_STATUS))
                cJSON_DeleteItemFromObject(game_over_msg, KEY_STATUS);
            cJSON_AddNumberToObject(game_over_msg, KEY_STATUS, 0);
            if (cJSON_HasObjectItem(game_over_msg, KEY_CODE))
                cJSON_DeleteItemFromObject(game_over_msg, KEY_CODE);
            cJSON_AddNumberToObject(game_over_msg, KEY_CODE, 2);
        }

        if (is_success == 0)
        {
            char        szT[64] = "";
            get_timestamp(szT, true);
            if (cJSON_HasObjectItem(game_over_msg, KEY_TIMESTAMP))
                cJSON_DeleteItemFromObject(game_over_msg, KEY_TIMESTAMP);
            cJSON_AddStringToObject(game_over_msg, KEY_TIMESTAMP, szT);
            if (cJSON_HasObjectItem(game_over_msg, KEY_EVENT))
                cJSON_DeleteItemFromObject(game_over_msg, KEY_EVENT);
            cJSON_AddNumberToObject(game_over_msg, KEY_EVENT, EVENT_GAMEOVER);
            if (cJSON_HasObjectItem(game_over_msg, KEY_ACTION))
                cJSON_DeleteItemFromObject(game_over_msg, KEY_ACTION);
            cJSON_AddNumberToObject(game_over_msg, KEY_ACTION, ACTION_ROOM_RET);

            char* message = cJSON_Print(game_over_msg);
            endpoint->send_message(message, (const char*)"User WithDraw", true);

            cJSON_Delete(s_users_info[iPosition -1]);
            s_users_info[iPosition -1] = NULL;

            s_user_in_room[iPosition-1] = 0;
            s_count_down[iPosition-1] = 0;
            save_users_info(iPosition);
        }
        cJSON_Delete(game_over_msg);
        s_with_drawing[iPosition-1] = 0;

        if (is_failed_snap)
        {

        }
    }
    return NULL;
}

static void user_with_draw(int iPosition)
{
    pthread_t pid;
    int* withdraw_ptr = (int*)malloc(sizeof(int));
    *withdraw_ptr = iPosition;
    pthread_create(&pid, NULL, pthread_with_draw, withdraw_ptr);
    pthread_detach(pid);
}

static void* pthread_auto_shoot_pk(void *in)
{
    LOGD ("[%s] [Auto Shoot for PK] In Thread", log_Time());

    int iPositions[4] = {0, 0, 0, 0};
    int j = 0;
    for (int i = 0; i < 4; i ++)
    {
        if(s_pk_enable_pos[i] == 1)
        {
            LOGD ("[%s] [Auto Shoot for PK] iPositions: i = %d, j = %d", log_Time(), i, j);
            iPositions[j] = i+1;
            j ++;
        }
    }
    while(s_pk_started == 1)
    {
        controller_shoots(iPositions, j);
        usleep(100000);
    }

    LOGD ("[%s] [Auto Shoot for PK] Thread End", log_Time());

    return NULL;
}

static void* pthread_auto_shoot(void *inpos_ptr)
{
    int* pos_ptr = (int*)inpos_ptr;
    if (pos_ptr == NULL)
    {
        LOGD ("[%s] [Auto Shoot] No user.", log_Time());
        return NULL;
    }

    int iPosition = *pos_ptr;
    free(pos_ptr);
    pos_ptr = NULL;

    LOGD ("[%s] [Auto Shoot] In Thread, Position: %d", log_Time(), iPosition);

    while(s_user_in_room[iPosition-1] == 1)
    {
        while (s_is_auto_shoot[iPosition-1] == true)
        {
            if (s_reset_countdown[iPosition-1] == true)
                s_count_down[iPosition-1] = s_game_time[iPosition-1];
            if (s_black_hole_user[iPosition-1] == 1)
            {
                if (s_operate_count[iPosition-1] > 0)
                {
                    if (s_operate_count[iPosition-1]%13 == 0)
                    {
                        controller_user_option_click(iPosition, OPTION_GAME_LEFT);
                    }
                    else if (s_operate_count[iPosition-1]%23 == 0)
                    {
                        controller_user_option_click(iPosition, OPTION_GAME_RIGHT);
                    }
                    else if (s_operate_count[iPosition-1]%5 == 0 || s_operate_count[iPosition-1]%5 == 1)
                    {
                    }
                    else
                    {
                        controller_user_shoot(iPosition);
                    }
                }
                else
                {
                    controller_user_shoot(iPosition);
                }
            }
            else
            {
                controller_user_shoot(iPosition);
            }
            s_operate_count[iPosition-1] += 1;
            s_shoot_count[iPosition-1] += 1;
            usleep(s_shoot_interval[iPosition-1]);
            if (s_reset_countdown[iPosition-1] == true)
            {
                if (s_resend_countdown[iPosition-1] == true)
                {
                    send_countdown(iPosition, s_count_down[iPosition-1], 0);
                    s_resend_countdown[iPosition-1] = false;
                }
                else
                {
                    send_countdown(iPosition, s_count_down[iPosition-1], 1);
                }
            }

        }
    }

    s_auto_shoot_pid[iPosition-1] = 0;

    LOGD ("[%s] [Auto Shoot] Thread End, Position: %d", log_Time(), iPosition);

    return NULL;
}

static void* pthread_auto_shoot_strength(void *inpos_ptr)
{
    int* pos_ptr = (int*)inpos_ptr;
    if (pos_ptr == NULL)
    {
        LOGD ("[%s] [Auto Shoot Strength] No user.", log_Time());
        return NULL;
    }

    int iPosition = *pos_ptr;
    free(pos_ptr);
    pos_ptr = NULL;

    while (s_is_auto_shoot_strength[iPosition-1] == true)
    {
//        if (s_reset_countdown[iPosition-1] == true)
//            s_count_down[iPosition-1] = s_game_time[iPosition-1];
        controller_user_shoot_strength(iPosition);
        s_operate_count[iPosition-1] += 1;
        usleep(100000);
//        if (s_reset_countdown[iPosition-1] == true)
//            send_countdown(iPosition, s_count_down[iPosition-1], 1);
    }

    if (room_info_ptr->iSupportMonster == 1)
    {
        long long ll_change_time = get_timestamp(NULL, 1);
        usleep(900000);
        int strength = 0;
        Mat source_image = get_frame_image();
        if (!source_image.empty())
        {
            strength = get_strength_value(source_image, iPosition);
            source_image.release();
        }
        if (strength > 0)
        {
            STRENGTH_DATA* strength_data = (STRENGTH_DATA*)malloc(sizeof(STRENGTH_DATA));
            strength_data->ll_time = ll_change_time;
            strength_data->strength_value = strength;
            s_strength_list[iPosition-1].push_back(strength_data);
        }
        while (s_strength_list[iPosition-1].size() > 1)
        {
            long long current_time = get_timestamp(NULL, 1);
            if ((current_time-s_strength_list[iPosition-1][0]->ll_time) > 60000 && (current_time-s_strength_list[iPosition-1][1]->ll_time) > 60000)
            {
                const vector<STRENGTH_DATA*>::iterator it = s_strength_list[iPosition-1].begin();
                if (it != s_strength_list[iPosition-1].end())
                {
                    STRENGTH_DATA* s_data = *it;
                    s_strength_list[iPosition-1].erase(it);
                    if (s_data != NULL)
                        free(s_data);
                }
            }
            else
                break;
        }
    }
    return NULL;
}

static void* pthread_add_strength_data(void *inpos_ptr)
{
    int* pos_ptr = (int*)inpos_ptr;
    if (pos_ptr == NULL)
    {
        LOGD ("[%s] [pthread_add_strength_data] No user.", log_Time());
        return NULL;
    }

    int iPosition = *pos_ptr;
    free(pos_ptr);

    long long ll_change_time = get_timestamp(NULL, 1);
    usleep(1000000);
    int strength = 0;
    Mat source_image = get_frame_image();
    if (!source_image.empty())
    {
        strength = get_strength_value(source_image, iPosition);
        source_image.release();
    }

    if (strength > 0)
    {
//        LOGD ("[%s] [Change Strength] Position: %ld, Strength: %ld", log_Time(), iPosition, strength);
//
//        for (int i = 0; i < s_strength_list[iPosition-1].size(); i++)
//        {
//            long long ll_time = s_strength_list[iPosition-1][i]->ll_time;
//            int i_strength = s_strength_list[iPosition-1][i]->strength_value;
//            LOGD ("[%s] [Change Strength] Original Strength List %d: %ld, Position is: %ld, Change Time: %lld", log_Time(), i, i_strength, iPosition, ll_time);
//        }

        bool can_add = true;
        if (s_strength_list[iPosition-1].size() > 0)
        {
            STRENGTH_DATA* last_strength_data = s_strength_list[iPosition-1][s_strength_list[iPosition-1].size()-1];
            if (last_strength_data->strength_value == strength)
            {
                if ((ll_change_time - last_strength_data->ll_time) <= 1000)
                {
                    if ((ll_change_time - last_strength_data->ll_time) > 0)
                        last_strength_data->ll_time = ll_change_time;
                    can_add = false;
                }
            }
        }
        if (can_add)
        {
            STRENGTH_DATA* strength_data = (STRENGTH_DATA*)malloc(sizeof(STRENGTH_DATA));
            strength_data->ll_time = ll_change_time;
            strength_data->strength_value = strength;
            s_strength_list[iPosition-1].push_back(strength_data);
        }
    }

//    for (int i = 0; i < s_strength_list[iPosition-1].size(); i++)
//    {
//        long long ll_time = s_strength_list[iPosition-1][i]->ll_time;
//        int i_strength = s_strength_list[iPosition-1][i]->strength_value;
//        LOGD ("[%s] [Change Strength] After Add Strength List %d: %ld, Position is: %ld, Change Time: %lld", log_Time(), i, i_strength, iPosition, ll_time);
//    }

    while (s_strength_list[iPosition-1].size() > 1)
    {
        long long current_time = get_timestamp(NULL, 1);
        if ((current_time-s_strength_list[iPosition-1][0]->ll_time) > 60000 && (current_time-s_strength_list[iPosition-1][1]->ll_time) > 60000)
        {
            const vector<STRENGTH_DATA*>::iterator it = s_strength_list[iPosition-1].begin();
            if (it != s_strength_list[iPosition-1].end())
            {
                STRENGTH_DATA* s_data = *it;
                s_strength_list[iPosition-1].erase(it);
                if (s_data != NULL)
                    free(s_data);
            }
        }
        else
            break;
    }
//    for (int i = 0; i < s_strength_list[iPosition-1].size(); i++)
//    {
//        long long ll_time = s_strength_list[iPosition-1][i]->ll_time;
//        int i_strength = s_strength_list[iPosition-1][i]->strength_value;
//        LOGD ("[%s] [Change Strength] After Erase Strength List %d: %ld, Position is: %ld, Change Time: %lld", log_Time(), i, i_strength, iPosition, ll_time);
//    }

    return NULL;
}

static void* pthread_user_options(void* in)
{
    LOGD ("[%s] [pthread_user_options] In", log_Time());

    while(1)
    {
        long long current_time = get_timestamp(NULL, 1);
        while(s_push_option == 1)
        {
            long long tmp_time = get_timestamp(NULL, 1);
            if ((tmp_time - current_time) > 100)
            {
                s_push_option = 0;
                break;
            }
        }
        s_pop_option = 1;
        if (s_users_options.size() > 0)
        {
            const vector<cJSON*>::iterator it = s_users_options.begin();
            if (it != s_users_options.end())
            {
                cJSON* optJson = *it;
                s_users_options.erase(it);
                s_pop_option = 0;
                if (optJson != NULL)
                {
//                    s_operation_count ++;
//                    char* message = cJSON_Print(optJson);
//                    char* map_str = get_value_map(message);
//                    delete_char(map_str, '\n');
//                    LOGD ("[%s] [pthread_user_options] s_operation_count: %d, %s", log_Time(), s_operation_count, map_str);
//                    free(message);

                    int iPosition = cJSON_GetObjectItemCaseSensitive(optJson, KEY_GAME_POS)->valueint;
                    if (iPosition >= 1 && iPosition <= room_info_ptr->iPlayerCount && s_user_in_room[iPosition-1] == 1)
                    {
                        int iEvent = cJSON_GetObjectItemCaseSensitive(optJson, KEY_EVENT)->valueint;
                        int iOption = cJSON_GetObjectItemCaseSensitive(optJson, KEY_OPTION)->valueint;
                        int iAutoMode = 0;
                        cJSON* modeJson = cJSON_GetObjectItemCaseSensitive(optJson, KEY_AUTO_MODE);
                        if (modeJson != NULL)
                            iAutoMode = modeJson->valueint;

                        if (iEvent == EVENT_USER_PRESS_CLICKED)
                        {
                            if (iOption == OPTION_GAME_AB)
                            {
                                controller_user_option_ab(iPosition);
                            }
                            else
                            {
                                if (iOption == OPTION_GAME_A && s_is_auto_shoot[iPosition-1] == 1)
                                    s_is_auto_shoot[iPosition-1] = 0;
                                if (iOption == OPTION_GAME_B && s_is_auto_shoot_strength[iPosition-1] == 1)
                                    s_is_auto_shoot_strength[iPosition-1] = 0;

                                if (s_black_hole_user[iPosition-1] == 1 && iOption == OPTION_GAME_A)
                                {
                                    if (s_operate_count[iPosition-1] > 0)
                                    {
                                        if (s_operate_count[iPosition-1]%13 == 0)
                                        {
                                            controller_user_option_click(iPosition, OPTION_GAME_LEFT);
                                        }
                                        else if (s_operate_count[iPosition-1]%23 == 0)
                                        {
                                            controller_user_option_click(iPosition, OPTION_GAME_RIGHT);
                                        }
                                        else if (s_operate_count[iPosition-1]%5 == 0)
                                        {
                                        }
                                        else
                                        {
                                            controller_user_option_click(iPosition, iOption);
                                        }
                                    }
                                    else
                                    {
                                        controller_user_option_click(iPosition, iOption);
                                    }
                                }
                                else {
                                    controller_user_option_click(iPosition, iOption);
                                }
                                s_operate_count[iPosition-1] += 1;

                                if (iOption == OPTION_GAME_A)
                                {
                                    s_shoot_count[iPosition-1] += 1;
                                }

                                if (iOption == OPTION_GAME_B && room_info_ptr->iSupportMonster ==1)
                                {
                                    pthread_t pid;
                                    int* pos_ptr = (int*)malloc(sizeof(int));
                                    *pos_ptr = iPosition;
                                    pthread_create(&pid, NULL, pthread_add_strength_data, pos_ptr);
                                    pthread_detach(pid);
                                }
                            }
                        }
                        else if (iEvent == EVENT_USER_PRESS_DOWN)
                        {
                            if (s_black_hole_user[iPosition-1] == 1 && s_direction_count[iPosition-1] > 0 && (iOption == OPTION_GAME_LEFT || iOption == OPTION_GAME_RIGHT))
                            {
                                if (s_direction_count[iPosition-1]%5 == 0 || s_direction_count[iPosition-1]%5 == 1)
                                {
                                }
                                else
                                {
                                    controller_user_option_down(iPosition, iOption);
                                }
                            }
                            else
                            {
                                controller_user_option_down(iPosition, iOption);
                            }
                        }
                        else if (iEvent == EVENT_USER_PRESS_UP)
                        {
                            if (s_black_hole_user[iPosition-1] == 1 && s_direction_count[iPosition-1] > 0 && (iOption == OPTION_GAME_LEFT || iOption == OPTION_GAME_RIGHT))
                            {
                                if (s_direction_count[iPosition-1]%5 == 0 || s_direction_count[iPosition-1]%5 == 1)
                                {
                                }
                                else
                                {
                                    controller_user_option_up(iPosition, iOption);
                                }
                            }
                            else
                            {
                                controller_user_option_up(iPosition, iOption);
                            }
                            s_direction_count[iPosition-1] += 1;
                            if (s_direction_count[iPosition-1] > 10000)
                                s_direction_count[iPosition-1] = 0;
                        }
                        else if (iEvent == EVENT_USER_PRESS_LONG)
                        {
                            if (iOption == OPTION_GAME_A)
                            {
                                if (iAutoMode == 1)
                                {
                                    s_is_auto_shoot[iPosition-1] = 1;
                                    pthread_t pid;
                                    int* autoshoot_ptr = (int*)malloc(sizeof(int));
                                    *autoshoot_ptr = iPosition;

                                    if (s_auto_shoot_pid[iPosition-1] == 0)
                                    {
                                        pthread_create(&s_auto_shoot_pid[iPosition-1], NULL, pthread_auto_shoot, autoshoot_ptr);
                                        pthread_detach(s_auto_shoot_pid[iPosition-1]);
                                    }
                                }
                                else
                                    s_is_auto_shoot[iPosition-1] = 0;
                            }
                            else if (iOption == OPTION_GAME_B)
                            {
                                if (iAutoMode == 1)
                                {
                                    s_is_auto_shoot_strength[iPosition-1] = 1;
                                    pthread_t pid;
                                    int* autoshoot_ptr = (int*)malloc(sizeof(int));
                                    *autoshoot_ptr = iPosition;
                                    pthread_create(&pid, NULL, pthread_auto_shoot_strength, autoshoot_ptr);
                                    pthread_detach(pid);
                                }
                                else
                                    s_is_auto_shoot_strength[iPosition-1] = 0;
                            }
                        }

                        if (iOption == OPTION_GAME_A)// || iOption == OPTION_GAME_B)
                        {
                            if (s_reset_countdown[iPosition-1] == true)
                            {
                                s_count_down[iPosition-1] = s_game_time[iPosition-1];
                                if (s_resend_countdown[iPosition-1] == true)
                                {
                                    send_countdown(iPosition, s_count_down[iPosition-1], 0);
                                    s_resend_countdown[iPosition-1] = false;
                                }
                                else
                                    send_countdown(iPosition, s_count_down[iPosition-1], 1);
                            }
                            else {
                                send_countdown(iPosition, s_count_down[iPosition-1], 0, 1);
                            }
                        }
                    }
                    cJSON_Delete(optJson);
                    optJson = NULL;
                }
            }
        }
        s_pop_option = 0;

        int i = 0;
        for(i = 0; i < room_info_ptr->iPlayerCount; i ++)
        {
            if (s_user_in_room[i] == 1)
                break;
        }

        if (i >= room_info_ptr->iPlayerCount)
        {
            while (s_users_options.size() > 0)
            {
                const vector<cJSON*>::iterator it = s_users_options.begin();
                if (it != s_users_options.end())
                {
                    cJSON* optJson = *it;
                    s_users_options.erase(it);
                    cJSON_Delete(optJson);
                    optJson = NULL;
                }
            }
            break;
        }
        usleep(5000);
    }

    LOGD ("[%s] [pthread_user_options] End.", log_Time());

    s_user_options_pid = 0;
    return NULL;
}

static void* pthread_user_options_pk(void* in)
{
    LOGD ("[%s] [pthread_user_options_pk] In", log_Time());

    while(1)
    {
        long long current_time = get_timestamp(NULL, 1);
        while(s_push_option == 1)
        {
            long long tmp_time = get_timestamp(NULL, 1);
            if ((tmp_time - current_time) > 100)
            {
                s_push_option = 0;
                break;
            }
        }
        s_pop_option = 1;
        if (s_users_options.size() > 0)
        {
            const vector<cJSON*>::iterator it = s_users_options.begin();
            if (it != s_users_options.end())
            {
                cJSON* optJson = *it;
                s_users_options.erase(it);
                s_pop_option = 0;
                if (optJson != NULL)
                {
                    int iPosition = cJSON_GetObjectItemCaseSensitive(optJson, KEY_GAME_POS)->valueint;
                    if (iPosition >= 1 && iPosition <= room_info_ptr->iPlayerCount)
                    {
                        int iEvent = cJSON_GetObjectItemCaseSensitive(optJson, KEY_EVENT)->valueint;
                        int iOption = cJSON_GetObjectItemCaseSensitive(optJson, KEY_OPTION)->valueint;
                        if (iEvent == EVENT_USER_PRESS_DOWN)
                        {
                            controller_user_option_down(iPosition, iOption);
                        }
                        else if (iEvent == EVENT_USER_PRESS_UP)
                        {
                            controller_user_option_up(iPosition, iOption);
                        }
                    }
                    cJSON_Delete(optJson);
                    optJson = NULL;
                }
            }
        }
        s_pop_option = 0;

        if (s_pk_started == 0)
        {
            while (s_users_options.size() > 0)
            {
                const vector<cJSON*>::iterator it = s_users_options.begin();
                if (it != s_users_options.end())
                {
                    cJSON* optJson = *it;
                    s_users_options.erase(it);
                    cJSON_Delete(optJson);
                    optJson = NULL;
                }
            }
            break;
        }
        usleep(5000);
    }

    LOGD ("[%s] [pthread_user_options_pk] End.", log_Time());

    s_user_options_pid = 0;
    return NULL;
}


static void* pthread_cast_magic(void* in)
{
    LOGD ("[%s] [pthread_cast_magic] In", log_Time());

    static long long s_end_time[8] = {0};
    while(1)
    {
        if (s_cast_magic_message.size() > 0)
        {
            const vector<cJSON*>::iterator it = s_cast_magic_message.begin();
            if (it != s_cast_magic_message.end())
            {
                cJSON* magicJson = *it;
                s_cast_magic_message.erase(it);
                if (magicJson != NULL)
                {
                    int iPosition = cJSON_GetObjectItemCaseSensitive(magicJson, KEY_GAME_POS)->valueint;
                    //LOGD ("[%s] [pthread_cast_magic] iPosition: %d", log_Time(), iPosition);
                    if (iPosition >= 1 && iPosition <= room_info_ptr->iPlayerCount && s_user_in_room[iPosition-1] == 1)
                    {
                        int iMagicType = cJSON_GetObjectItemCaseSensitive(magicJson, KEY_MAGIC_TYPE)->valueint;
                        long long  iDuration = cJSON_GetObjectItemCaseSensitive(magicJson, KEY_DURATION)->valueint;
                        //LOGD ("[%s] [pthread_cast_magic] iMagicType: %d, iDuration: %lld", log_Time(), iMagicType, iDuration);
                        if (iMagicType == 3)
                        {
                        }
                        else if (iMagicType == 4)   //极速卡
                        {
                            int is_on = cJSON_GetObjectItemCaseSensitive(magicJson, KEY_ON)->valueint;
                            if (is_on == 1)
                            {
                                s_end_time[iPosition-1] = iDuration;
                                s_shoot_interval[iPosition-1] = 50000;
                            }
                            else
                            {
                                s_end_time[iPosition-1] = 0;
                                s_shoot_interval[iPosition-1] = 100000;
                            }
                        }

                    }
                    cJSON_Delete(magicJson);
                    magicJson = NULL;
                }
            }
        }

        for (int index = 0; index < room_info_ptr->iPlayerCount; index ++)
        {
            if (s_end_time[index] > 0)
            {
                long long ll_cur_time = get_timestamp(NULL, 1);
                if (ll_cur_time >= s_end_time[index])
                {
                    s_end_time[index] = 0;
                    s_shoot_interval[index] = 100000;
                }
            }
        }

        int i = 0;
        for(i = 0; i < room_info_ptr->iPlayerCount; i ++)
        {
            if (s_user_in_room[i] == 1)
                break;
        }

        if (i >= room_info_ptr->iPlayerCount)
        {
            while (s_cast_magic_message.size() > 0)
            {
                const vector<cJSON*>::iterator it = s_cast_magic_message.begin();
                if (it != s_cast_magic_message.end())
                {
                    cJSON* magicJson = *it;
                    s_cast_magic_message.erase(it);
                    cJSON_Delete(magicJson);
                    magicJson = NULL;
                }
            }

            break;
        }
        usleep(1000);
    }

    LOGD ("[%s] [pthread_cast_magic] End.", log_Time());

    s_cast_magic_pid = 0;
    return NULL;
}

static void* pthread_all_users_withdraw(void* inJson)
{
    cJSON* optJson = (cJSON*)inJson;
    for (int i = 0; i < room_info_ptr->iPlayerCount; i ++)
    {
        if (s_is_auto_shoot_strength[i] == 1)
            s_is_auto_shoot_strength[i] = 0;
        if (s_is_auto_shoot[i] == 1)
            s_is_auto_shoot[i] = 0;

        char sz_user_info_file[512];
        sprintf(sz_user_info_file, "%sWaController/user_info_%d.ini", WORK_FOLDER, i+1);
        if (s_user_in_room[i] == 1)
        {
            int check_count = 0;
            int iScore = 0;
            while(1)
            {
                iScore = get_score(i+1, 0);
                if (iScore > 0 || (iScore == 0 && check_count >= 1))
                    break;
                check_count += 1;
                LOGD ("[%s] [ALL Users WithDraw] Position: %d, check user score count: %d", log_Time(), i+1, check_count);
                if (check_count > 5)
                    break;
                sleep(1);
            }
            LOGD ("[%s] [ALL Users WithDraw] Position: %d, user has score: %d", log_Time(), i+1, iScore);
            if (iScore > 0)
            {
                cJSON* msg = cJSON_Duplicate(s_users_info[i], 1);
                char        szT[64] = "";
                get_timestamp(szT, true);
                if (cJSON_HasObjectItem(msg, KEY_TIMESTAMP))
                    cJSON_DeleteItemFromObject(msg, KEY_TIMESTAMP);
                cJSON_AddStringToObject(msg, KEY_TIMESTAMP, szT);
                if (cJSON_HasObjectItem(msg, KEY_ACTION))
                    cJSON_DeleteItemFromObject(msg, KEY_ACTION);
                cJSON_AddNumberToObject(msg, KEY_ACTION, ACTION_ROOM_RET);
                if (cJSON_HasObjectItem(msg, KEY_EVENT))
                    cJSON_DeleteItemFromObject(msg, KEY_EVENT);
                cJSON_AddNumberToObject(msg, KEY_EVENT, EVENT_USERSCORE);
                if (cJSON_HasObjectItem(msg, KEY_SCORE))
                    cJSON_DeleteItemFromObject(msg, KEY_SCORE);
                cJSON_AddNumberToObject(msg, KEY_SCORE, iScore);
                if (cJSON_HasObjectItem(msg, KEY_TIMEOUT))
                    cJSON_DeleteItemFromObject(msg, KEY_TIMEOUT);
                cJSON_AddNumberToObject(msg, KEY_TIMEOUT, 10);

                if (cJSON_GetObjectItemCaseSensitive(optJson, KEY_PASSIVE) != NULL)
                    cJSON_AddNumberToObject(msg, KEY_PASSIVE, 1);

                char* message = cJSON_Print(msg);
                endpoint->send_message(message, (const char*)"ALL USER WITHDRAW", true);
                cJSON_Delete(msg);
            }
        }
        if (access(sz_user_info_file, 0) == 0)
            remove(sz_user_info_file);
    }
    int* pos_ptr = (int*)malloc(sizeof(int)*room_info_ptr->iPlayerCount);
    for(int i = 0; i < room_info_ptr->iPlayerCount; i ++)
        pos_ptr[i] = i+1;

    controller_admin_clear_scores(pos_ptr, room_info_ptr->iPlayerCount);
    free(pos_ptr);

    if (optJson != NULL)
    {
        cJSON_Delete(optJson);
        optJson = NULL;
    }

    return NULL;
}

static void* pthread_auto_play1(void* in)
{
    LOGD ("[%s] [Auto Play1] Enter", log_Time());

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);
    sleep(60);
    while(1)
    {
        int j = 0;
        while(j < 5)
        {
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
            controller_user_drop(1);
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
            pthread_testcancel();
            usleep(20000);
            pthread_testcancel();
            j++;
        }

        j = 0;
        while(j <= 200)
        {
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
            controller_user_shoot(1);
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
            pthread_testcancel();
            usleep(20000);
            pthread_testcancel();
            j ++;
        }
    }
    LOGD ("[%s] [Auto Play1] End", log_Time());
    return NULL;
}

static void* pthread_auto_play2(void* in)
{
    LOGD ("[%s] [Auto Play2] Enter", log_Time());

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);
    sleep(60);
    while(1)
    {
        int j = 0;
        while(j < 5)
        {
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
            controller_user_drop(2);
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
            pthread_testcancel();
            usleep(20000);
            pthread_testcancel();
            j++;
        }

        j = 0;
        while(j <= 200)
        {
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
            controller_user_shoot(2);
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
            pthread_testcancel();
            usleep(20000);
            pthread_testcancel();
            j ++;
        }
    }
    LOGD ("[%s] [Auto Play2] End", log_Time());
    return NULL;
}

static void* pthread_auto_play3(void* in)
{
    LOGD ("[%s] [Auto Play3] Enter", log_Time());

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);
    sleep(60);
    while(1)
    {
        int j = 0;
        while(j < 5)
        {
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
            controller_user_drop(3);
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
            pthread_testcancel();
            usleep(20000);
            pthread_testcancel();
            j++;
        }

        j = 0;
        while(j <= 200)
        {
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
            controller_user_shoot(3);
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
            pthread_testcancel();
            usleep(20000);
            pthread_testcancel();
            j ++;
        }
    }
    LOGD ("[%s] [Auto Play3] End", log_Time());
    return NULL;
}

static void* pthread_auto_play4(void* in)
{
    LOGD ("[%s] [Auto Play4] Enter", log_Time());

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);
    sleep(60);
    while(1)
    {
        int j = 0;
        while(j < 5)
        {
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
            controller_user_drop(4);
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
            pthread_testcancel();
            usleep(20000);
            pthread_testcancel();
            j++;
        }

        j = 0;
        while(j <= 200)
        {
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
            controller_user_shoot(4);
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
            pthread_testcancel();
            usleep(20000);
            pthread_testcancel();
            j ++;
        }
    }
    LOGD ("[%s] [Auto Play4] End", log_Time());
    return NULL;
}

static void* pthread_auto_rock(void* in)
{
    LOGD ("[%s] [Auto Rock] Enter", log_Time());

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);
    sleep(60);
    int option = OPTION_GAME_LEFT;
    while(1)
    {
        for (int i = 1; i <= 4; i++)
        {
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
            controller_user_option_down(i, option);
            usleep(300000);
            controller_user_option_up(i, option);
            usleep(10000);
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
            pthread_testcancel();
        }
        if (option == OPTION_GAME_LEFT)
            option = OPTION_GAME_RIGHT;
        else if (option == OPTION_GAME_RIGHT)
            option = OPTION_GAME_LEFT;
        usleep(200000);
        pthread_testcancel();
    }
    LOGD ("[%s] [Auto Rock] End", log_Time());
    return NULL;
}

static int get_monster_map_type(int monster_type)
{
    return  monster_type+TYPE_DEATH;
}

static int get_weapon_map_type(int weapon_type)
{
    return  weapon_type+TYPE_MSD;
}

static int get_little_monster_map_type(int little_monster_type)
{
    return  little_monster_type+TYPE_PUMPKIN_SMALL;
}

static void* pthread_detection(void* in)
{
    LOGD ("[%s] [pthread_detection] Enter", log_Time());
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);

    if (s_detect_log == 1)
    {
        LOGD ("[Detect Log], 检测时间, 检测物体, 用户ID, 机位, 房间号, 倍率");
    }
    else if (s_auto_debug == 1)
    {
        LOGD ("[Object Detected], 检测时间, 检测物体, 机位, 房间号");
    }

    while(1)
    {
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
        Mat source_image = get_frame_image();
        if (!source_image.empty())
        {
            if (check_board_error(source_image))
            {
                if (s_board_error == 0)
                {
                    s_board_error = 1;
                    long long ll_cur_time = get_timestamp(NULL, 1);
                    if ((ll_cur_time - s_ll_error_time) > 30*60*1000)
                    {
                        s_ll_error_time = ll_cur_time;
                        if (s_auto_debug == 1)
                        {
                            LOGD ("[Object Detected], %s, 控制板故障, 0, %s", log_Time(), room_info_ptr->szName);
                        }
                        else
                        {
                            cJSON *msg = cJSON_CreateObject();
                            cJSON_AddNumberToObject(msg, KEY_ACTION, ACTION_ROOM_OUT);
                            cJSON_AddNumberToObject(msg, KEY_EVENT, EVENT_ROOM_MAINTAINED);
                            cJSON_AddStringToObject(msg, KEY_REASON, "控制板故障");
                            cJSON_AddStringToObject(msg, KEY_ROOM_ID, room_info_ptr->szRoomId);
                            char* message = cJSON_Print(msg);
                            endpoint->send_message(message, (char*)"Board Error", true);
                            cJSON_Delete(msg);
                        }
                    }
                }
            }
            else
            {
                s_board_error = 0;

                if (room_info_ptr->iSubType == 1)
                {
                    int strength_position = 1;
                    int strength_array[4] = {0, 0, 0, 0};
                    long long find_time = get_timestamp(NULL, 1);
                    while (strength_position <= room_info_ptr->iPlayerCount)
                    {
                        if (s_user_in_room[strength_position-1] == 1 && s_end_play[strength_position-1] == 0 && s_with_drawing[strength_position-1] == 0)
                        {
                            int strength = get_strength_value(source_image, strength_position);
//                            LOGD ("[%s] [Detection Strength] Current Strength: %ld, Position is: %ld", log_Time(), strength, strength_position);
                            if (s_strength_list[strength_position-1].size() > 0)
                            {
                                int tmp_strength = 0;
                                long used_time = 0;
                                for (int i = s_strength_list[strength_position-1].size()-1; i >= 0; i --)
                                {
                                    long long ll_time = s_strength_list[strength_position-1][i]->ll_time;
                                    int i_strength = s_strength_list[strength_position-1][i]->strength_value;
//                                    LOGD ("[%s] [Detection Strength] Strength List %d: %ld, Position is: %ld, Change Time: %lld", log_Time(), i, i_strength, strength_position, ll_time);
                                    if ((find_time-ll_time) < 60000)
                                    {
                                        tmp_strength += (i_strength*(find_time-ll_time-used_time));
                                        used_time = find_time-ll_time;
                                    }
                                    else {
                                        tmp_strength += (i_strength*(60000-used_time));
                                        used_time = 60000;
                                        break;
                                    }
                                }
                                strength = tmp_strength/used_time;
//                                LOGD ("[%s] [Detection Strength] Average Strength: %ld, Position is: %ld, Used Time: %ld", log_Time(), strength, strength_position, used_time);
                            }
                            strength_array[strength_position-1] = strength;
                        }
                        strength_position += 1;
                    }

                    int check_position = 1;
                    while (check_position <= room_info_ptr->iPlayerCount)
                    {
                        if (s_auto_debug == 1 || (s_user_in_room[check_position-1] == 1 && s_with_drawing[check_position-1] == 0))
                        {
    //                        int littlemonster_type = get_littlemonster_type(source_image, check_position);
    //                        if (littlemonster_type >= 0)
    //                        {
    //                            Detect_Data detect_data = s_l_monster_detect[check_position-1][littlemonster_type];
    //                            long long ll_cur_time = get_timestamp(NULL, 1);
    //                            if ((ll_cur_time-detect_data.ll_time) > detect_data.life_cycle)
    //                            {
    //                                detect_data.ll_time = ll_cur_time;
    //                                s_l_monster_detect[check_position-1][littlemonster_type] = detect_data;
    //
    //                                char* little_monster_name = get_littlemonster_name(littlemonster_type);
    //
    //                                if (s_auto_debug == 1)
    //                                {
    //                                    LOGD ("[%s] [Object Detected] %s, {Player: %d, ID: %d, LC: 2000}", log_Time(), little_monster_name, check_position, littlemonster_type+21);
    //                                }
    //                                else
    //                                {
    //                                    cJSON *msg = cJSON_CreateObject();
    //                                    cJSON_AddNumberToObject(msg, KEY_ACTION, ACTION_ROOM_OPT);
    //                                    cJSON_AddNumberToObject(msg, KEY_EVENT, EVENT_MONSTER_DETECT);
    //                                    cJSON_AddNumberToObject(msg, KEY_TYPE, get_little_monster_map_type(littlemonster_type));
    //                                    cJSON_AddNumberToObject(msg, KEY_GAME_POS, check_position);
    //                                    cJSON_AddNumberToObject(msg, KEY_ONLY_SERVER, 1);
    //                                    cJSON* itemPtr = cJSON_GetObjectItemCaseSensitive(s_users_info[check_position-1], KEY_ROOM_ID);
    //                                    if (itemPtr != NULL)
    //                                        cJSON_AddStringToObject(msg, KEY_ROOM_ID, itemPtr->valuestring);
    //
    //                                    itemPtr = cJSON_GetObjectItemCaseSensitive(s_users_info[check_position-1], KEY_USER_ID);
    //                                    if (itemPtr != NULL)
    //                                        cJSON_AddStringToObject(msg, KEY_USER_ID, itemPtr->valuestring);
    //
    //                                    cJSON * array_json = cJSON_CreateIntArray(strength_array, 4);
    //                                    cJSON_AddItemToObject(msg, KEY_GAME_BETS, array_json);
    //
    //                                    char* message = cJSON_Print(msg);
    //                                    endpoint->send_message(message, (char*)"Find Little Monster", true);
    //                                    cJSON_Delete(msg);
    //                                }
    //                            }
    //                        }
    //                        else {
    //                            for (int i = 0; i < 9; i ++)
    //                            {
    //                                Detect_Data detect_data = s_l_monster_detect[check_position-1][i];
    //                                long long ll_cur_time = get_timestamp(NULL, 1);
    //                                if ((ll_cur_time-detect_data.ll_time) > detect_data.life_cycle)
    //                                {
    //                                    detect_data.ll_time = 0;
    //                                    s_l_monster_detect[check_position-1][i] = detect_data;
    //                                }
    //                            }
    //                        }

                            int weapon_type = get_weapon_type(source_image, check_position);
                            if (weapon_type >= 0)
                            {
                                Detect_Data detect_data = s_weapon_detect[check_position-1][weapon_type];
                                long long ll_cur_time = get_timestamp(NULL, 1);
                                if ((ll_cur_time-detect_data.ll_time) > detect_data.life_cycle)
                                {
                                    detect_data.ll_time = ll_cur_time;
                                    s_weapon_detect[check_position-1][weapon_type] = detect_data;

                                    char* weapon_name = get_weapon_name(weapon_type);
                                    if (s_auto_debug == 1)
                                    {
                                        //LOGD ("[%s] [Object Detected] %s, {Player: %d, ID: %d, LC: 2000}", log_Time(), weapon_name, check_position, weapon_type+31);
                                        LOGD ("[Object Detected], %s, %s, %d, %s", log_Time(), weapon_name, check_position, room_info_ptr->szName);
                                    }
                                    else if (s_user_in_room[check_position-1] == 1 && s_end_play[check_position-1] == 0 && s_with_drawing[check_position-1] == 0)
                                    {
                                        cJSON *msg = cJSON_CreateObject();
                                        cJSON_AddNumberToObject(msg, KEY_ACTION, ACTION_ROOM_OPT);
                                        cJSON_AddNumberToObject(msg, KEY_EVENT, EVENT_MONSTER_DETECT);
                                        cJSON_AddNumberToObject(msg, KEY_TYPE, get_weapon_map_type(weapon_type));
                                        cJSON_AddNumberToObject(msg, KEY_GAME_POS, check_position);
                                        cJSON_AddNumberToObject(msg, KEY_ONLY_SERVER, 1);
                                        cJSON* itemPtr = cJSON_GetObjectItemCaseSensitive(s_users_info[check_position-1], KEY_ROOM_ID);
                                        if (itemPtr != NULL)
                                            cJSON_AddStringToObject(msg, KEY_ROOM_ID, itemPtr->valuestring);

                                        itemPtr = cJSON_GetObjectItemCaseSensitive(s_users_info[check_position-1], KEY_USER_ID);
                                        if (itemPtr != NULL)
                                            cJSON_AddStringToObject(msg, KEY_USER_ID, itemPtr->valuestring);

                                        cJSON * array_json = cJSON_CreateIntArray(strength_array, 4);
                                        cJSON_AddItemToObject(msg, KEY_GAME_BETS, array_json);

                                        char* message = cJSON_Print(msg);
                                        endpoint->send_message(message, (char*)"Find Weapon", true);
                                        cJSON_Delete(msg);
                                        if (s_detect_log == 1)
                                        {
                                            cJSON* useridPtr = cJSON_GetObjectItemCaseSensitive(s_users_info[check_position-1], KEY_USER_ID);
                                            LOGD ("[Detect Log], %s, %s, %s, %d, %s, %d", log_Time(), weapon_name, useridPtr->valuestring, check_position, room_info_ptr->szName, strength_array[check_position-1]);
                                        }
                                    }
                                }
                            }
                            else {
                                for (int i = 0; i < 6; i ++)
                                {
                                    Detect_Data detect_data = s_weapon_detect[check_position-1][i];
                                    long long ll_cur_time = get_timestamp(NULL, 1);
                                    if (detect_data.ll_time > 0 && ((ll_cur_time-detect_data.ll_time) > detect_data.life_cycle))
                                    {
                                        detect_data.ll_time = 0;
                                        s_weapon_detect[check_position-1][i] = detect_data;
//                                        LOGD ("[%s] [Detect Log] s_weapon_detect reset Position=%d, weapon type=%d.", log_Time(), check_position, i);
                                    }
                                }
                            }

                            int monster_type = get_monster_type(source_image, check_position);
                            if (s_auto_monster == 1 && s_auto_start_time[check_position-1] == 0)
                            {
                                s_auto_start_time[check_position-1] = get_timestamp(NULL, 1);
                                LOGD ("[%s] [Detect Log] test detect monster begin time %lld, ", log_Time(), s_auto_start_time[check_position-1]);
                            }
                            long long ll_cur_auto_time = get_timestamp(NULL, 1);
                            if (monster_type >= 0 || (s_auto_monster && (ll_cur_auto_time-s_auto_start_time[check_position-1]) >= 120000))
                            {
                                if (s_auto_monster && (ll_cur_auto_time-s_auto_start_time[check_position-1]) >= 120000)
                                {
                                    s_auto_start_time[check_position-1] = get_timestamp(NULL, 1);

                                    srand((unsigned)time(NULL));
                                    monster_type = rand() % 6;
                                    LOGD ("[%s] [Detect Log] test detect monster type: %d, ", log_Time(), monster_type);
                                }
                                Detect_Data detect_data = s_monster_detect[check_position-1][monster_type];
                                long long ll_cur_time = get_timestamp(NULL, 1);
                                if ((ll_cur_time-detect_data.ll_time) > detect_data.life_cycle)
                                {
                                    detect_data.ll_time = ll_cur_time;
                                    s_monster_detect[check_position-1][monster_type] = detect_data;
                                    char* monster_name = get_monster_name(monster_type);

                                    if (s_auto_debug == 1)
                                    {
                                        //LOGD ("[%s] [Object Detected] %s, {Player: %d, ID: %d, LC: 2000}", log_Time(), monster_name, check_position, monster_type+11);
                                        LOGD ("[Object Detected], %s, %s, %d, %s", log_Time(), monster_name, check_position, room_info_ptr->szName);
                                    }
                                    else if (s_user_in_room[check_position-1] == 1 && s_end_play[check_position-1] == 0 && s_with_drawing[check_position-1] == 0)
                                    {
                                        cJSON *msg = cJSON_CreateObject();
                                        cJSON_AddNumberToObject(msg, KEY_ACTION, ACTION_ROOM_OPT);
                                        cJSON_AddNumberToObject(msg, KEY_EVENT, EVENT_MONSTER_DETECT);
                                        cJSON_AddNumberToObject(msg, KEY_TYPE, get_monster_map_type(monster_type));
                                        cJSON_AddNumberToObject(msg, KEY_GAME_POS, check_position);
                                        cJSON_AddNumberToObject(msg, KEY_ONLY_SERVER, 1);
                                        cJSON* itemPtr = cJSON_GetObjectItemCaseSensitive(s_users_info[check_position-1], KEY_ROOM_ID);
                                        if (itemPtr != NULL)
                                            cJSON_AddStringToObject(msg, KEY_ROOM_ID, itemPtr->valuestring);

                                        itemPtr = cJSON_GetObjectItemCaseSensitive(s_users_info[check_position-1], KEY_USER_ID);
                                        if (itemPtr != NULL)
                                            cJSON_AddStringToObject(msg, KEY_USER_ID, itemPtr->valuestring);

                                        cJSON * array_json = cJSON_CreateIntArray(strength_array, 4);
                                        cJSON_AddItemToObject(msg, KEY_GAME_BETS, array_json);

                                        char* message = cJSON_Print(msg);
                                        endpoint->send_message(message, (char*)"Find Monster", true);
                                        cJSON_Delete(msg);

                                        if (s_detect_log == 1)
                                        {
                                            cJSON* useridPtr = cJSON_GetObjectItemCaseSensitive(s_users_info[check_position-1], KEY_USER_ID);
                                            LOGD ("[Detect Log], %s, %s, %s, %d, %s, %d", log_Time(), monster_name, useridPtr->valuestring, check_position, room_info_ptr->szName, strength_array[check_position-1]);
                                        }
                                    }
                                }
                            }
                            else {
                                for (int i = 0; i < 6; i ++)
                                {
                                    Detect_Data detect_data = s_monster_detect[check_position-1][i];
                                    long long ll_cur_time = get_timestamp(NULL, 1);
                                    if (detect_data.ll_time > 0 && ((ll_cur_time-detect_data.ll_time) > detect_data.life_cycle))
                                    {
                                        detect_data.ll_time = 0;
                                        s_monster_detect[check_position-1][i] = detect_data;
//                                        LOGD ("[%s] [Detect Log] s_monster_detect reset Position=%d, monster type=%d.", log_Time(), check_position, i);
                                    }
                                }
                            }

                            if (s_check_baoji_status[check_position-1] == 0)
                            {
                                if (s_auto_baoji == 1 && s_auto_baoji_starttime[check_position-1] == 0)
                                {
                                    s_auto_baoji_starttime[check_position-1] = get_timestamp(NULL, 1);
                                    LOGD ("[%s] [Detect Log] test detect baoji begin time %lld, ", log_Time(), s_auto_baoji_starttime[check_position-1]);
                                }
                                long long ll_cur_auto_baoji_time = get_timestamp(NULL, 1);
                                if (check_baoji_status(source_image, check_position) || (s_auto_baoji && (ll_cur_auto_baoji_time-s_auto_baoji_starttime[check_position-1]) >= 120000))
                                {
                                    s_check_baoji_status[check_position-1] = 1;
                                    if (s_auto_debug == 1)
                                    {
                                        //LOGD ("[%s] [Object Detected] 爆机, {Player: %d, ID: 1, LC: 2000}", log_Time(), check_position);
                                        LOGD ("[Object Detected], %s, 系统爆机, %d, %s", log_Time(), check_position, room_info_ptr->szName);
                                    }
                                    else if (s_user_in_room[check_position-1] == 1 && s_end_play[check_position-1] == 0 && s_with_drawing[check_position-1] == 0)
                                    {
                                        cJSON *msg = cJSON_CreateObject();
                                        cJSON_AddNumberToObject(msg, KEY_ACTION, ACTION_ROOM_OPT);
                                        cJSON_AddNumberToObject(msg, KEY_EVENT, EVENT_MONSTER_DETECT);
                                        cJSON_AddNumberToObject(msg, KEY_TYPE, TYPE_XTBJ);
                                        cJSON_AddNumberToObject(msg, KEY_GAME_POS, check_position);
                                        cJSON_AddNumberToObject(msg, KEY_ONLY_SERVER, 1);
                                        cJSON* itemPtr = cJSON_GetObjectItemCaseSensitive(s_users_info[check_position-1], KEY_ROOM_ID);
                                        if (itemPtr != NULL)
                                            cJSON_AddStringToObject(msg, KEY_ROOM_ID, itemPtr->valuestring);

                                        itemPtr = cJSON_GetObjectItemCaseSensitive(s_users_info[check_position-1], KEY_USER_ID);
                                        if (itemPtr != NULL)
                                            cJSON_AddStringToObject(msg, KEY_USER_ID, itemPtr->valuestring);

                                        cJSON * array_json = cJSON_CreateIntArray(strength_array, 4);
                                        cJSON_AddItemToObject(msg, KEY_GAME_BETS, array_json);

                                        char* message = cJSON_Print(msg);
                                        endpoint->send_message(message, (char*)"System BaoJi", true);
                                        cJSON_Delete(msg);

                                        if (s_detect_log == 1)
                                        {
                                            cJSON* useridPtr = cJSON_GetObjectItemCaseSensitive(s_users_info[check_position-1], KEY_USER_ID);
                                            LOGD ("[Detect Log], %s, 系统爆机, %s, %d, %s, %d", log_Time(), useridPtr->valuestring, check_position, room_info_ptr->szName, strength_array[check_position-1]);
                                        }
                                    }
                                }
                            }
                        }
                        check_position += 1;
                    }
                }
            }
            source_image.release();
        }
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
        pthread_testcancel();
        usleep(10000);
        pthread_testcancel();
    }
    LOGD ("[%s] [pthread_detection] End", log_Time());
    return NULL;
}

static void* pthread_handle_exception(void* in)
{
    LOGD ("[%s] [Handle Exception]", log_Time());

    for (int i = 0; i < room_info_ptr->iPlayerCount; i ++)
    {
        if (s_is_auto_shoot_strength[i] == 1)
            s_is_auto_shoot_strength[i] = 0;
        if (s_is_auto_shoot[i] == 1)
            s_is_auto_shoot[i] = 0;

        char sz_user_info_file[512];
        sprintf(sz_user_info_file, "%sWaController/user_info_%d.ini", WORK_FOLDER, i+1);
        if (access(sz_user_info_file, 0) == 0)
        {
            if (s_with_drawing[i] == 1)
                continue;

            char section[32];
            sprintf(section, "position%d", i+1);
            char* user_info = GetIniKeyString(section, (char*)"UserInfo", sz_user_info_file);
            if (user_info != NULL)
            {
                s_with_drawing[i] = 1;
                pthread_t pid;
                int* withdraw_pos_ptr = (int*)malloc(sizeof(int));
                *withdraw_pos_ptr = i+1;
                pthread_create(&pid, NULL, pthread_withdraw_lock, withdraw_pos_ptr);
                pthread_detach(pid);

                int check_count = 0;
                int iScore = 0;
                while(1)
                {
                    iScore = get_score(i+1, 0);
                    if (iScore > 0 || (iScore == 0 && check_count >= 1))
                        break;
                    check_count += 1;
                    LOGD ("[%s] [Handle Exception] Position: %d, check user score count: %d", log_Time(), i+1, check_count);
                    if (check_count > 5)
                        break;
                    sleep(1);
                }
                LOGD ("[%s] [Handle Exception] Position: %d, user has score: %d", log_Time(), i+1, iScore);
                if (iScore > 0)
                {
                    LOGD ("[%s] [Handle Exception] Position: %d, UserInfo: %s", log_Time(), i+1, user_info);
                    cJSON* msg = cJSON_Parse(user_info);
                    char        szT[64] = "";
                    get_timestamp(szT, true);
                    cJSON_AddStringToObject(msg, KEY_TIMESTAMP, szT);
                    cJSON_AddNumberToObject(msg, KEY_ACTION, ACTION_ROOM_RET);
                    cJSON_AddNumberToObject(msg, KEY_EVENT, EVENT_USERSCORE);
                    cJSON_AddNumberToObject(msg, KEY_SCORE, iScore);
                    cJSON_AddNumberToObject(msg, KEY_TIMEOUT, 10);

                    char* message = cJSON_Print(msg);
                    endpoint->send_message(message, (const char*)"Handle Exception", true);
                    cJSON_Delete(msg);
                }

                //free(user_info);
            }
            remove(sz_user_info_file);
            s_with_drawing[i] = 0;
        }
    }

    int* pos_ptr = (int*)malloc(sizeof(int)*room_info_ptr->iPlayerCount);
    for(int i = 0; i < room_info_ptr->iPlayerCount; i ++)
        pos_ptr[i] = i+1;

    controller_admin_clear_scores(pos_ptr, room_info_ptr->iPlayerCount);
    free(pos_ptr);

    return NULL;
}

static void* pthread_stream_off(void* inJson)
{
    s_in_streamoff_thread = 1;
    while (s_stream_on == 0)
    {
        long long current_time = get_timestamp(NULL, 1);
        if ((current_time-s_streamoff_time) > 10000)
        {
            std::string rtmpIp(room_info_ptr->szRTMPIp);
            std::string quotation("\"");
            std::string url_command = "http://"+rtmpIp+"/set_output?venc_framerate=1&venc_bitrate=32?user=admin&pass=admin";
            std::string sys_command = "curl " + quotation + url_command + quotation;
            LOGD ("[%s] [Stream Off] %s ", log_Time(), sys_command.c_str());
            system(sys_command.c_str());
            s_streamoff = 1;
            break;
        }
        sleep(1);
    }
    s_in_streamoff_thread = 0;
    return NULL;
}

static void* pthread_add_scores(void* inJson)
{
    cJSON* operatePtr = (cJSON*)inJson;
    if (operatePtr == NULL)
    {
        LOGD ("[%s] [pthread_add_scores] Operate Error", log_Time());
        return NULL;
    }

    int status = 1;
    if (s_pk_mode == 1)
    {
        int check_count = 2;
        int cc = 0;
        cJSON* checkCountPtr = cJSON_GetObjectItemCaseSensitive(operatePtr, KEY_COUNT);
        if (checkCountPtr != NULL)
            check_count = checkCountPtr->valueint;

        while(cc < check_count)
        {
            status = 1;
            sleep(5);
            int* pos_ptr = (int*)malloc(sizeof(int)*room_info_ptr->iPlayerCount);
            for(int i = 0; i < room_info_ptr->iPlayerCount; i ++)
                pos_ptr[i] = i+1;

            controller_admin_clear_scores(pos_ptr, room_info_ptr->iPlayerCount);
            free(pos_ptr);

            cJSON* countPtr = cJSON_GetObjectItemCaseSensitive(operatePtr, KEY_PK_COINCOUNT);
            if (countPtr != NULL)
            {
                int count = countPtr->valueint;
                LOGD ("[%s] [pthread_add_scores] count = %ld", log_Time(), count);
                for (int i = 0;i < count; i ++)
                {
                    int iPositions[4] = {1, 2, 3, 4};
                    controller_add_scores(iPositions, room_info_ptr->iPlayerCount);
                    usleep(35000);
                }

                sleep(1);

                int cc_score = 0;
                while(cc_score < 5)
                {
                    Mat source_image;
                    int check_position = 1;
                    int check_score = count * 1000;
                    status = 1;
                    LOGD ("[%s] [pthread_add_scores] check_score = %ld", log_Time(), check_score);
                    while (check_position <= room_info_ptr->iPlayerCount)
                    {
                        if (source_image.empty())
                        {
                            source_image = get_frame_image();
                        }
                        if (!source_image.empty())
                        {
                            int iScore = read_score_by_image(source_image, check_position);
                            LOGD ("[%s] [pthread_add_scores] score = %ld in Position = %d, cc_score = %d", log_Time(), iScore, check_position, cc_score);
                            if (iScore == -1)
                            {
                                cc_score += 1;
                                status = 0;
                                break;
                            }
                            else if (iScore != check_score)
                            {
                                status = 0;
                                cc_score = 0;
                                break;
                            }
                            else
                                cc_score = 0;
                        }
                        check_position += 1;
                    }

                    if (!source_image.empty())
                    {
                        source_image.release();
                    }
                    if (cc_score > 0)
                    {
                        usleep(500000);
                        continue;
                    }
                    if (status == 1 || cc_score == 0)
                    {
                        break;
                    }
                }

                if (status == 0)
                {
                    cc += 1;
                    continue;
                }

                break;
            }
        }
    }
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, KEY_ROOM_ID, room_info_ptr->szRoomId);
    cJSON_AddNumberToObject(root, KEY_ACTION, ACTION_PK_OPERATE);
    cJSON_AddNumberToObject(root, KEY_STATUS, status);

    char* message = cJSON_Print(root);
    endpoint->send_message(message, (const char*)"PK OPERATE", true);
    cJSON_Delete(root);

    cJSON_Delete(operatePtr);
    operatePtr = NULL;

    return NULL;
}

static char *str_replace(const char *string, const char *substr, const char *replacement )
{
    char *tok    = NULL;
    char *newstr = NULL;
    char *oldstr = NULL;

    /* if either substr or replacement is NULL, duplicate string a let caller handle it */
    if ( substr == NULL || replacement == NULL )
        return strdup (string);

    newstr = strdup (string);
    while ( (tok = strstr( newstr, substr)))
    {
        oldstr = newstr;
        newstr = (char*)malloc (strlen ( oldstr ) - strlen ( substr ) + strlen ( replacement ) + 1 );
        /*failed to alloc mem, free old string and return NULL */
        if (newstr == NULL)
        {
            free (oldstr);
            return NULL;
        }
        memcpy ( newstr, oldstr, tok - oldstr );
        memcpy ( newstr + (tok - oldstr), replacement, strlen ( replacement ) );
        memcpy ( newstr + (tok - oldstr) + strlen( replacement ), tok + strlen ( substr ), strlen ( oldstr ) - strlen ( substr ) - ( tok - oldstr ) );
        memset ( newstr + strlen ( oldstr ) - strlen ( substr ) + strlen ( replacement ) , 0, 1 );

        free(oldstr);
    }

    return newstr;
}

static void change_server(char* user_id)
{
    LOGD ("[%s] [Change Server] ", log_Time());

    bool is_changed = false;

    char sz_file_name[512];
    sprintf(sz_file_name, "%sWaController/server.ini", WORK_FOLDER);
    FILE *pFile = fopen(sz_file_name, "r");
    if (pFile != NULL)
    {
        char	szLine[512] = "";
        Vector* vertorLines = vector_new(512);
        while(!feof(pFile))
        {
            strcpy(szLine, "");
            fgets(szLine, 512, pFile);
            if (strlen(szLine) <= 0)
                continue;
            if (strstr(szLine, "https://r.zhuagewawa.com") != NULL)
            {
                strcpy(szLine, str_replace(szLine, "https://r.zhuagewawa.com", "https://t.zhuagewawa.com"));
                LOGD ("[%s] [Change Server] change: %s ", log_Time(), szLine);
                is_changed = true;
            }
            else if (strstr(szLine, "https://t.zhuagewawa.com") != NULL)
            {
                strcpy(szLine, str_replace(szLine, "https://t.zhuagewawa.com", "https://r.zhuagewawa.com"));
                LOGD ("[%s] [Change Server] change: %s ", log_Time(), szLine);
                is_changed = true;
            }
            vector_append(vertorLines, szLine);
        }
        fclose(pFile);

        if (is_changed)
        {
            pFile = fopen(sz_file_name, "w");
		    if (pFile != NULL)
		    {
		        for (int i = 0; i < vector_length(vertorLines); i ++)
                {
                    vector_get(vertorLines, i, szLine);
                    fputs(szLine, pFile);
                }
                fclose(pFile);

                sleep(5);
                FILE * p_file = popen("sudo reboot", "r");
                if (!p_file) {
                    LOGD ("[%s] [Change Server] Reboot: Error", log_Time());
                }
                char	szLine[1024] = "";
                while(!feof(p_file))
                {
                    fgets(szLine, 1024, p_file);
                    LOGD ("[%s] [Change Server] fgets: %s", log_Time(), szLine);
                }
                pclose(p_file);
		    }
        }
		vector_free(vertorLines);
    }
    if (!is_changed)
    {
        cJSON* msg = cJSON_CreateObject();

        cJSON_AddStringToObject(msg, KEY_ROOM_ID, room_info_ptr->szRoomId);
        cJSON_AddNumberToObject(msg, KEY_ACTION, ACTION_ADMIN_SET);
        cJSON_AddNumberToObject(msg, KEY_EVENT, EVENT_CHANGE_SERVER);
        cJSON_AddStringToObject(msg, KEY_USER_ID, user_id);
        cJSON_AddNumberToObject(msg, KEY_RESULT, 0);

        char* message = cJSON_Print(msg);
        endpoint->send_message(message, (const char*)"Change Server", true);
        cJSON_Delete(msg);
    }
}

static void restart_frp()
{
    cJSON* json = get_frp_info();

    bool bUpdate = false;
    if ( NULL != json )
    {
        cJSON * pItem = cJSON_GetObjectItem(json, "status");
        if ( NULL != pItem && strcmp(pItem->valuestring, "ok") == 0)
        {
            cJSON * pRoomItem = cJSON_GetObjectItem(json, "room");
            if ( NULL != pRoomItem )
            {
                pItem = cJSON_GetObjectItem(pRoomItem, "frpport");
                if ( NULL != pItem )
                {
                    strcpy(room_info_ptr->szFrpPort, pItem->valuestring);
                }
                pItem = cJSON_GetObjectItem(pRoomItem, "frps");
                if ( NULL != pItem )
                {
                    strcpy(room_info_ptr->szFrps, pItem->valuestring);
                    bUpdate = true;
                }
            }
        }
        cJSON_Delete(json);
    }

    if (bUpdate)
    {
        char    szFrp_addr[64] = "";
        char    szFrp_port[32] = "";
        if (strlen(room_info_ptr->szFrps) > 0)
        {
            char* pszTemp = strchr(room_info_ptr->szFrps, ':');
            strncpy(szFrp_addr, room_info_ptr->szFrps, strlen(room_info_ptr->szFrps)-strlen(pszTemp));
            strcpy(szFrp_port, pszTemp+1);
        }

        char sz_file_name[512];
        sprintf(sz_file_name, "%sfrp_0.13.0_linux_arm/frpc.ini", WORK_FOLDER);

        FILE *pFile = fopen(sz_file_name, "r");
        if (pFile != NULL)
        {
            char	szLine[512] = "";
            Vector* vertor = vector_new(512);
            while(!feof(pFile))
            {
                strcpy(szLine, "");
                fgets(szLine, 512, pFile);
                //printf("Line frpc: %s", szLine);
                if (strlen(szLine) <= 0)
                    continue;

                if (strlen(szFrp_addr) > 0 && strstr(szLine, "server_addr") != NULL)
                {
                    char szTmp[256];
                    sprintf(szTmp, "server_addr=%s\n", szFrp_addr);
                    vector_append(vertor, szTmp);
                }
                else if (strlen(szFrp_port) > 0 && strstr(szLine, "server_port") != NULL)
                {
                    char szTmp[256];
                    sprintf(szTmp, "server_port=%s\n", szFrp_port);
                    vector_append(vertor, szTmp);
                }
                else
                    vector_append(vertor, szLine);
                if (strstr(szLine, "log_max_days") != NULL)
                {
                    vector_append(vertor, (char*)"\n");
                    break;
                }

            }
            char szTmp[256];
            sprintf(szTmp, "[%s]\n", room_info_ptr->szFrpPort);
            vector_append(vertor, szTmp);
            sprintf(szTmp, "type = tcp\n");
            vector_append(vertor, szTmp);
            sprintf(szTmp, "local_ip = 127.0.0.1\n");
            vector_append(vertor, szTmp);
            sprintf(szTmp, "local_port = 22\n");
            vector_append(vertor, szTmp);
            sprintf(szTmp, "remote_port = %s\n", room_info_ptr->szFrpPort);
            vector_append(vertor, szTmp);

            fclose(pFile);

            pFile = fopen(sz_file_name, "w");
            if (pFile != NULL)
            {
                uint i = 0;

                for (i = 0; i < vector_length(vertor); i ++ )
                {
                    vector_get(vertor, i, szLine);
                    //printf("Line_Write frpc: %s", szLine);
                    fputs(szLine, pFile);
                }
                fclose(pFile);
            }
            vector_free(vertor);
        }
        sleep(5);
        FILE * p_file = popen("sudo supervisorctl restart frp", "r");
        if (!p_file) {
            LOGD ("[%s] [restart_frp] Restart: Error", log_Time());
        }
        char	szLine[1024] = "";
        while(!feof(p_file))
        {
            fgets(szLine, 1024, p_file);
            LOGD ("[%s] [restart_frp] fgets: %s", log_Time(), szLine);
        }
        pclose(p_file);
        LOGD ("[%s] [restart_frp] Restart Success", log_Time());
    }
}

perftest::perftest (std::string uri)
{
    m_uri = uri;
    m_ping_time = 0;
    m_reconnect_c = 0;
    m_reconnect_delay = 0;
    m_status = STATUS_DISCONNECTED;
    m_wss_support = false;
    if(strstr(m_uri.c_str(), "wss:") != NULL)
        m_wss_support = true;

    if(m_wss_support)
    {
        m_endpoint_wss.set_access_channels(websocketpp::log::alevel::none);
        m_endpoint_wss.set_error_channels(websocketpp::log::elevel::none);

        // Initialize ASIO
        m_endpoint_wss.init_asio();

        // Register our handlers
        m_endpoint_wss.set_message_handler(bind(&type::on_message,this,::_1,::_2));
        m_endpoint_wss.set_open_handler(bind(&type::on_open,this,::_1));
        m_endpoint_wss.set_close_handler(bind(&type::on_close,this,::_1));
        m_endpoint_wss.set_fail_handler(bind(&type::on_fail,this,::_1));
        m_endpoint_wss.set_ping_handler(bind(&type::on_ping,this,::_1,::_2));
        m_endpoint_wss.set_tls_init_handler([this](websocketpp::connection_hdl){
            return websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12);
        });
    }
    else
    {
        m_endpoint_ws.set_access_channels(websocketpp::log::alevel::none);
        m_endpoint_ws.set_error_channels(websocketpp::log::elevel::none);

        // Initialize ASIO
        m_endpoint_ws.init_asio();

        // Register our handlers
        m_endpoint_ws.set_message_handler(bind(&type::on_message,this,::_1,::_2));
        m_endpoint_ws.set_open_handler(bind(&type::on_open,this,::_1));
        m_endpoint_ws.set_close_handler(bind(&type::on_close,this,::_1));
        m_endpoint_ws.set_fail_handler(bind(&type::on_fail,this,::_1));
        m_endpoint_ws.set_ping_handler(bind(&type::on_ping,this,::_1,::_2));
    }
}

perftest::~perftest()
{
}

void perftest::start()
{
    while (true)
    {
        if (m_status == STATUS_CONNECTING || m_status == STATUS_DISCONNECTING)
            continue;

        long long ll_cur_time = get_timestamp(NULL, 1);
        if (m_ping_time > 0 && (ll_cur_time-m_ping_time) >= 30000 && m_status == STATUS_CONNECTED)
        {
            LOGD ("[%s] [WebSocket] ping timeout", log_Time());
            m_status = STATUS_DISCONNECTING;
            m_status = STATUS_DISCONNECTED;
            m_ping_time = 0;
            close_socket("ping timeout", m_hdl);
            continue;
        }

        if (m_reconnect_delay > 0)
            sleep(m_reconnect_delay);

        if (m_status == STATUS_DISCONNECTED)
        {
            m_status = STATUS_CONNECTING;
            try
            {
                websocketpp::lib::error_code ec;
                if(m_wss_support)
                {
                    client_wss::connection_ptr con = m_endpoint_wss.get_connection(m_uri, ec);
                    if (ec)
                    {
                        LOGD ("[%s] [WebSocket] get_connection error: %s", log_Time(), ec.message());
                        m_endpoint_wss.get_alog().write(websocketpp::log::alevel::app,ec.message());
                        return;
                    }
                    m_con_wss = con;
                    m_endpoint_wss.connect(con);
                    con->set_termination_handler(bind(&type::on_termination_handler,this,::_1));

                    m_endpoint_wss.run();
                    m_endpoint_wss.reset();
                }
                else
                {
                    client_ws::connection_ptr con = m_endpoint_ws.get_connection(m_uri, ec);
                    if (ec)
                    {
                        LOGD ("[%s] [WebSocket] get_connection error: %s", log_Time(), ec.message());
                        m_endpoint_ws.get_alog().write(websocketpp::log::alevel::app,ec.message());
                        return;
                    }
                    m_con_ws = con;
                    m_endpoint_ws.connect(con);
                    con->set_termination_handler(bind(&type::on_termination_handler,this,::_1));

                    m_endpoint_ws.run();
                    m_endpoint_ws.reset();
                }
            }
            catch (websocketpp::exception const & e) {
		        std::cout << e.what() << std::endl;
            }

            m_status = STATUS_DISCONNECTED;
            m_reconnect_c += 1;
        }
    }
}

void perftest::on_termination_handler(websocketpp::connection_hdl hdl)
{
    //LOGD ("[%s] [WebSocket] termination", log_Time());
	m_status = STATUS_DISCONNECTED;
    m_ping_time = 0;
    close_socket("termination", hdl);
}

int perftest::close_socket(std::string str_reason, websocketpp::connection_hdl hdl)
{
	int nRet = 0;
	try {
	    if(m_wss_support)
	    {
	        client_wss::connection_ptr con = m_endpoint_wss.get_con_from_hdl(hdl);
            if (con->get_state() == websocketpp::session::state::value::open)
            {
                LOGD ("[%s] [WebSocket] close_socket", log_Time());
                websocketpp::close::status::value cvValue = 0;
                websocketpp::lib::error_code ec;
                m_endpoint_wss.close(m_hdl, websocketpp::close::status::normal, str_reason, ec);
            }
	    }
        else
        {
            client_ws::connection_ptr con = m_endpoint_ws.get_con_from_hdl(hdl);
            if (con->get_state() == websocketpp::session::state::value::open)
            {
                LOGD ("[%s] [WebSocket] close_socket", log_Time());
                websocketpp::close::status::value cvValue = 0;
                websocketpp::lib::error_code ec;
                m_endpoint_ws.close(m_hdl, websocketpp::close::status::normal, str_reason, ec);
            }
        }
	}
	catch (websocketpp::exception const & e)
	{
		nRet = -1;
	}
	return nRet;
}

void perftest::on_open(websocketpp::connection_hdl hdl)
{
    m_hdl = hdl;
    m_status = STATUS_CONNECTED;
    m_reconnect_c = 0;
    m_reconnect_delay = 0;
    if (room_info_ptr != NULL)
    {
        int i = 0;
        char        szT[64] = "";
        get_timestamp(szT, 1);

        char szMac[64] = "";
        get_mac(szMac);

        MD5_CTX md5;
        MD5Init(&md5);
        unsigned char encrypt[256] = "";
        unsigned char decrypt[32];
        char hexdecrypt[64] = "";
        memcpy(encrypt, room_info_ptr->szRoomId, strlen(room_info_ptr->szRoomId));
        memcpy(encrypt+strlen(room_info_ptr->szRoomId), szMac, strlen(szMac));
        memcpy(encrypt+strlen(szMac)+strlen(room_info_ptr->szRoomId), szT, strlen(szT));
        MD5Update(&md5,encrypt,strlen(room_info_ptr->szRoomId)+strlen(szMac)+strlen(szT));
        MD5Final(&md5,decrypt);
        for(i=0;i<16;i++)
        {
            sprintf(hexdecrypt, "%s%02x", hexdecrypt, decrypt[i]);
        }

        cJSON* root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, KEY_TIMESTAMP, szT);
        cJSON_AddStringToObject(root, KEY_ROOM_ID, room_info_ptr->szRoomId);
        cJSON_AddStringToObject(root, KEY_SIGN, hexdecrypt);
        cJSON_AddNumberToObject(root, KEY_ACTION, ACTION_ROOM_IN);

        char* message = cJSON_Print(root);
        send_message(message, (const char*)"ON OPEN", true);
        cJSON_Delete(root);
    }
}

void perftest::on_message(websocketpp::connection_hdl hdl, message_ptr msg)
{
    try {
        pthread_t pid;
        char* map_str = get_value_map((const char*)msg->get_payload().c_str());
        delete_char(map_str, '\n');

        cJSON *jsonPtr = cJSON_Parse(msg->get_payload().c_str());
        if ( NULL != jsonPtr )
        {
            cJSON * actionPtr = cJSON_GetObjectItemCaseSensitive(jsonPtr, KEY_ACTION);
            if (actionPtr != NULL)
            {
                switch (actionPtr->valueint)
                {
                    case ACTION_CAST_MAGIC:
                    {
                        LOGD ("[%s] [WebSocket] Client recvived: %s", log_Time(), map_str);
                        cJSON* gamePosPtr = cJSON_GetObjectItemCaseSensitive(jsonPtr, KEY_GAME_POS);
                        if (gamePosPtr != NULL && gamePosPtr->valueint >= 1 && gamePosPtr->valueint <= room_info_ptr->iPlayerCount)
                        {
                            if (s_cast_magic_pid == 0)
                            {
                                pthread_create(&s_cast_magic_pid, NULL, pthread_cast_magic, NULL);
                                pthread_detach(s_cast_magic_pid);
                            }

                            cJSON* magicJson = cJSON_Duplicate(jsonPtr, 1);
                            s_cast_magic_message.push_back(magicJson);
                        }
                        break;
                    }
                    case ACTION_USER_OPT:
                    {
                        cJSON* gamePosPtr = cJSON_GetObjectItemCaseSensitive(jsonPtr, KEY_GAME_POS);
                        if (gamePosPtr != NULL && gamePosPtr->valueint >= 1 && gamePosPtr->valueint <= room_info_ptr->iPlayerCount)
                        {
                            if (s_user_in_room[gamePosPtr->valueint-1] == 1)
                            {
                                cJSON* eventPtr = cJSON_GetObjectItemCaseSensitive(jsonPtr, KEY_EVENT);
                                if (eventPtr != NULL && eventPtr->valueint == EVENT_MONSTER && room_info_ptr->iSupportMonster == 1)
                                {
                                    int* pos_ptr = (int*)malloc(sizeof(int));
                                    *pos_ptr = gamePosPtr->valueint;
                                    pthread_create(&pid, NULL, pthread_find_monster, pos_ptr);
                                    pthread_detach(pid);
                                }
                                else if (eventPtr != NULL && eventPtr->valueint == EVENT_USERSCORE)
                                {
                                    LOGD ("[%s] [WebSocket] Client recvived: %s", log_Time(), map_str);
                                    if (s_count_down[gamePosPtr->valueint-1] > 0)
                                        user_with_draw(gamePosPtr->valueint);
                                }
                                else if (eventPtr != NULL && eventPtr->valueint == EVENT_GAMEOVER)
                                {
                                    user_out_manually(gamePosPtr->valueint);
                                }
                                else
                                {
                                    cJSON* optionPtr = cJSON_GetObjectItemCaseSensitive(jsonPtr, KEY_OPTION);
                                    if (optionPtr != NULL && optionPtr->valueint != OPTION_GAME_D && optionPtr->valueint != OPTION_GAME_C)
                                    {
                                        if (s_user_options_pid == 0)
                                        {
                                            pthread_create(&s_user_options_pid, NULL, pthread_user_options, NULL);
                                            pthread_detach(s_user_options_pid);
                                        }

                                        int iPosition = cJSON_GetObjectItemCaseSensitive(jsonPtr, KEY_GAME_POS)->valueint;
                                        if (iPosition >= 1 && iPosition <= room_info_ptr->iPlayerCount)
                                        {
//                                            s_option_count ++;
//                                            LOGD ("[%s] [WebSocket] Option Count: %d, ACTION_USER_OPT: %s", log_Time(), s_option_count, map_str);
                                            cJSON* optJson = cJSON_Duplicate(jsonPtr, 1);

                                            long long current_time = get_timestamp(NULL, 1);
                                            while(s_pop_option == 1)
                                            {
                                                long long tmp_time = get_timestamp(NULL, 1);
                                                if ((tmp_time - current_time) > 100)
                                                {
                                                    s_pop_option = 0;
                                                    break;
                                                }
                                            }
                                            s_push_option = 1;
                                            s_users_options.push_back(optJson);
                                            s_push_option = 0;
                                        }
                                    }
                                }
                            }
                        }
                        break;
                    }
                    case ACTION_USER_DROP_COIN:
                    {
                        LOGD ("[%s] [WebSocket] Client recvived: %s", log_Time(), map_str);
                        user_drop(jsonPtr);
                        break;
                    }
                    case ACTION_ROOM_IN:
                    {
                        LOGD ("[%s] [WebSocket] Client recvived: %s", log_Time(), map_str);
                        m_ping_time = get_timestamp(NULL, 1);

                        if (s_check_websocket_pid != 0)
                            pthread_cancel(s_check_websocket_pid);

                        if (s_detection_pid != 0)
                            pthread_cancel(s_detection_pid);

                        pthread_create(&s_check_websocket_pid, NULL, pthread_check_websocket, NULL);
                        pthread_detach(s_check_websocket_pid);

                        pthread_create(&pid, NULL, pthread_handle_exception, NULL);
                        pthread_detach(pid);

                        pthread_create(&s_detection_pid, NULL, pthread_detection, NULL);
                        pthread_detach(s_detection_pid);

                        cJSON* pkModePtr = cJSON_GetObjectItemCaseSensitive(jsonPtr, KEY_PK_MODE);
                        if(pkModePtr != NULL && pkModePtr->valueint == 1)
                        {
                            s_pk_mode = 1;
                            s_pk_close = 0;
                        }
                        else
                        {
                            s_pk_mode = 0;
                            if (s_auto_play_pid1 != 0)
                                pthread_cancel(s_auto_play_pid1);
                            if (s_auto_rock_pid != 0)
                                pthread_cancel(s_auto_rock_pid);
                            if (s_auto_play_pid2 != 0)
                                pthread_cancel(s_auto_play_pid2);
                            if (s_auto_play_pid3 != 0)
                                pthread_cancel(s_auto_play_pid3);
                            if (s_auto_play_pid4 != 0)
                                pthread_cancel(s_auto_play_pid4);

                            if (s_check_empty_cannon_pid != 0)
                                pthread_cancel(s_check_empty_cannon_pid);

                            if (room_info_ptr->iSubType == 1 || room_info_ptr->iSubType == 2 || room_info_ptr->iSubType == 4)
                            {
                                pthread_create(&s_check_empty_cannon_pid, NULL, pthread_check_empty_cannon, NULL);
                                pthread_detach(s_check_empty_cannon_pid);
                            }

                            if (s_auto_debug == 1)
                            {
                                pthread_create(&s_auto_play_pid1, NULL, pthread_auto_play1, NULL);
                                pthread_detach(s_auto_play_pid1);
                                pthread_create(&s_auto_play_pid2, NULL, pthread_auto_play2, NULL);
                                pthread_detach(s_auto_play_pid2);
                                pthread_create(&s_auto_play_pid3, NULL, pthread_auto_play3, NULL);
                                pthread_detach(s_auto_play_pid3);
                                pthread_create(&s_auto_play_pid4, NULL, pthread_auto_play4, NULL);
                                pthread_detach(s_auto_play_pid4);

                                pthread_create(&s_auto_rock_pid, NULL, pthread_auto_rock, NULL);
                                pthread_detach(s_auto_rock_pid);
                            }
                        }
                        break;
                    }

                    case ACTION_PK_OPERATE:
                    {
                        LOGD ("[%s] [WebSocket] Client recvived: %s", log_Time(), map_str);

                        cJSON* operatePtr = cJSON_Duplicate(jsonPtr, 1);
                        pthread_t pid;
                        pthread_create(&pid, NULL, pthread_add_scores, operatePtr);
                        pthread_detach(pid);

                        break;
                    }
                    case ACTION_PK_START:
                    {
                        LOGD ("[%s] [WebSocket] Client recvived: %s", log_Time(), map_str);

                        s_pk_close = 0;

                        for (int i = 0; i < 4; i ++)
                        {
                            s_pk_enable_pos[i] = 0;
                        }

                        cJSON* startTimePtr = cJSON_GetObjectItemCaseSensitive(jsonPtr, KEY_START_TIME);
                        cJSON* endTimePtr = cJSON_GetObjectItemCaseSensitive(jsonPtr, KEY_END_TIME);
                        cJSON* pkIdPtr = cJSON_GetObjectItemCaseSensitive(jsonPtr, KEY_PK_ID);
                        cJSON* pkPosArrayPtr = cJSON_GetObjectItemCaseSensitive(jsonPtr, KEY_PK_POS);

                        if (s_pk_mode == 1)
                        {
                            long long start_time = startTimePtr->valueint;
                            long long end_time = endTimePtr->valueint;
                            s_pk_start_time = get_timestamp(NULL, 1);
                            s_pk_end_time = s_pk_start_time + (end_time - start_time);
                            strcpy(s_pk_id, pkIdPtr->valuestring);

                            int array_size = cJSON_GetArraySize(pkPosArrayPtr);
                            for (int i = 0; i < array_size; i ++)
                            {
                                cJSON * subPtr = cJSON_GetArrayItem(pkPosArrayPtr, i);
                                if (subPtr != NULL)
                                {
                                    int pos = subPtr->valueint;
                                    LOGD ("[%s] [WebSocket] ACTION_PK_START: valueint = %ld", log_Time(), pos);
                                    s_pk_enable_pos[pos-1] = 1;
                                }
                            }
                            if (array_size > 0)
                            {
                                s_pk_started = 1;
                                pthread_t pid;
                                pthread_create(&pid, NULL, pthread_auto_shoot_pk, NULL);
                                pthread_detach(pid);

                                pthread_t pid1;
                                pthread_create(&pid1, NULL, pthread_check_all_scores, NULL);
                                pthread_detach(pid1);

                                pthread_t pid2;
                                pthread_create(&pid2, NULL, pthread_countdown_pk, NULL);
                                pthread_detach(pid2);
                            }
                        }

                        cJSON* root = cJSON_CreateObject();
                        cJSON_AddStringToObject(root, KEY_ROOM_ID, room_info_ptr->szRoomId);
                        cJSON_AddNumberToObject(root, KEY_ACTION, ACTION_PK_START);

                        cJSON* posArrayPtr1 = cJSON_Duplicate(pkPosArrayPtr, 1);
                        cJSON_AddItemToObject(root, KEY_PK_POS, posArrayPtr1);

                        cJSON* endTimePtr1 = cJSON_Duplicate(endTimePtr, 1);
                        cJSON_AddItemToObject(root, KEY_END_TIME, endTimePtr1);

                        cJSON* startTimePtr1 = cJSON_Duplicate(startTimePtr, 1);
                        cJSON_AddItemToObject(root, KEY_START_TIME, startTimePtr1);

                        char* message = cJSON_Print(root);
                        send_message(message, (const char*)"PK START", true);
                        cJSON_Delete(root);

                        break;
                    }
                    case ACTION_PK_CONTROL:
                    {
                        if (s_pk_started == 1)
                        {
                            cJSON* roomIdPtr = cJSON_GetObjectItemCaseSensitive(jsonPtr, KEY_ROOM_ID);
                            cJSON* pkIdPtr = cJSON_GetObjectItemCaseSensitive(jsonPtr, KEY_PK_ID);
                            if (roomIdPtr != NULL && pkIdPtr != NULL && strcmp(roomIdPtr->valuestring, room_info_ptr->szRoomId) == 0 && strcmp(pkIdPtr->valuestring, s_pk_id) == 0)
                            {
                                cJSON* eventPtr = cJSON_GetObjectItemCaseSensitive(jsonPtr, KEY_EVENT);
                                if (eventPtr != NULL && (eventPtr->valueint == EVENT_USER_PRESS_UP || eventPtr->valueint == EVENT_USER_PRESS_DOWN))
                                {
                                    cJSON* optionPtr = cJSON_GetObjectItemCaseSensitive(jsonPtr, KEY_OPTION);
                                    if (optionPtr != NULL &&
                                        (optionPtr->valueint == OPTION_GAME_DOWN || optionPtr->valueint == OPTION_GAME_RIGHT
                                        || optionPtr->valueint == OPTION_GAME_UP || optionPtr->valueint == OPTION_GAME_LEFT))
                                    {
                                        if (s_user_options_pid == 0)
                                        {
                                            pthread_create(&s_user_options_pid, NULL, pthread_user_options_pk, NULL);
                                            pthread_detach(s_user_options_pid);
                                        }

                                        int iPosition = cJSON_GetObjectItemCaseSensitive(jsonPtr, KEY_GAME_POS)->valueint;
                                        if (iPosition >= 1 && iPosition <= room_info_ptr->iPlayerCount && s_pk_enable_pos[iPosition-1] == 1)
                                        {
                                            cJSON* optJson = cJSON_Duplicate(jsonPtr, 1);

                                            long long current_time = get_timestamp(NULL, 1);
                                            while(s_pop_option == 1)
                                            {
                                                long long tmp_time = get_timestamp(NULL, 1);
                                                if ((tmp_time - current_time) > 100)
                                                {
                                                    s_pop_option = 0;
                                                    break;
                                                }
                                            }
                                            s_push_option = 1;
                                            s_users_options.push_back(optJson);
                                            s_push_option = 0;
                                        }
                                    }
                                }
                            }
                        }
                        break;
                    }
                    case ACTION_ADMIN_OPT:
                    {
                        LOGD ("[%s] [WebSocket] Client recvived: %s", log_Time(), map_str);
                        cJSON* eventPtr = cJSON_GetObjectItemCaseSensitive(jsonPtr, KEY_EVENT);
                        if (eventPtr != NULL && eventPtr->valueint == EVENT_USERSCORE_ALL)
                        {
                            cJSON* optJson = cJSON_Duplicate(jsonPtr, 1);
                            pthread_create(&pid, NULL, pthread_all_users_withdraw, optJson);
                            pthread_detach(pid);
                        }
                        else if (eventPtr != NULL && eventPtr->valueint == EVENT_STREAM_SWITCH)
                        {
                            if (s_auto_debug == 0)
                            {
                                cJSON* keyPtr = cJSON_GetObjectItemCaseSensitive(jsonPtr, KEY_ON);
                                s_stream_on = keyPtr->valueint;
                                if (s_stream_on == 0)
                                {
                                    if (s_in_streamoff_thread == 0 && s_streamoff == 0)
                                    {
                                        s_streamoff_time = get_timestamp(NULL, 1);
                                        pthread_create(&pid, NULL, pthread_stream_off, NULL);
                                        pthread_detach(pid);
                                    }
                                }
                                else if (s_stream_on == 1)
                                {
                                    s_streamoff_time = 0;
                                    if (s_streamoff == 1)
                                    {
                                        std::string rtmpIp(room_info_ptr->szRTMPIp);
                                        std::string quotation("\"");
                                        std::string url_command = "http://"+rtmpIp+"/set_output?venc_framerate=50&venc_bitrate=4096?user=admin&pass=admin";
                                        std::string sys_command = "curl " + quotation + url_command + quotation;
                                        LOGD ("[%s] [Stream Off] %s ", log_Time(), sys_command.c_str());
                                        system(sys_command.c_str());
                                        s_streamoff = 0;
                                    }
                                }
                            }
                            else
                            {
                                std::string rtmpIp(room_info_ptr->szRTMPIp);
                                std::string quotation("\"");
                                std::string url_command = "http://"+rtmpIp+"/set_output?venc_framerate=50&venc_bitrate=4096?user=admin&pass=admin";
                                std::string sys_command = "curl " + quotation + url_command + quotation;
                                LOGD ("[%s] [Stream Off] %s ", log_Time(), sys_command.c_str());
                                system(sys_command.c_str());
                            }
                        }
                        else if (eventPtr != NULL && eventPtr->valueint == EVENT_PK_CLOSE)
                        {
                            if (s_pk_mode == 1)
                            {
                                int return_update = 0;
                                s_pk_close = 1;
                                if (s_pk_started == 1)
                                {
                                    return_update = 1;
                                }
                                s_pk_end_time = 0;
                                s_pk_start_time = 0;
                                s_pk_started = 0;

                                for (int i = 0; i < 4; i ++)
                                {
                                    s_pk_enable_pos[i] = 0;
                                }

                                if (return_update == 1)
                                {
                                    cJSON* root = cJSON_CreateObject();
                                    cJSON_AddNumberToObject(root, KEY_PK_STATUS, 1);
                                    cJSON_AddStringToObject(root, KEY_ROOM_ID, room_info_ptr->szRoomId);
                                    cJSON_AddStringToObject(root, KEY_PK_ID, s_pk_id);
                                    cJSON_AddNumberToObject(root, KEY_ACTION, ACTION_PK_RET);
                                    cJSON_AddNumberToObject(root, KEY_EVENT, EVENT_PK_UPDATE);

                                    Mat source_image;
                                    int check_position = 1;
                                    int iScoreArray[room_info_ptr->iPlayerCount] = {0};
                                    while (check_position <= room_info_ptr->iPlayerCount)
                                    {
                                        if (source_image.empty())
                                        {
                                            source_image = get_frame_image();
                                        }
                                        if (!source_image.empty())
                                        {
                                            int iScore = read_score_by_image(source_image, check_position);
                                            iScoreArray[check_position-1] = iScore;
                                        }
                                        check_position += 1;
                                    }

                                    if (!source_image.empty())
                                    {
                                        source_image.release();
                                    }

                                    cJSON* scorePtr = cJSON_CreateIntArray(iScoreArray, room_info_ptr->iPlayerCount);
                                    cJSON_AddItemToObject(root, KEY_PK_SCORE, scorePtr);

                                    char* message = cJSON_Print(root);
                                    endpoint->send_message(message, (char*)"Return PK Scores", true);
                                    cJSON_Delete(root);
                                }

                                int* pos_ptr = (int*)malloc(sizeof(int)*room_info_ptr->iPlayerCount);
                                for(int i = 0; i < room_info_ptr->iPlayerCount; i ++)
                                    pos_ptr[i] = i+1;

                                controller_admin_clear_scores(pos_ptr, room_info_ptr->iPlayerCount);
                                free(pos_ptr);
                            }
                        }
                        break;
                    }
                    case ACTION_ADMIN_SET:
                    {
                        LOGD ("[%s] [WebSocket] Client recvived: %s", log_Time(), map_str);
                        cJSON* eventPtr = cJSON_GetObjectItemCaseSensitive(jsonPtr, KEY_EVENT);
                        if (eventPtr->valueint == EVENT_OPENSETTINGS)
                        {
                            controller_admin_open_settings();
                            s_streamoff_time = 0;
                            s_stream_on = 1;
                            if (s_streamoff == 1)
                            {
                                std::string rtmpIp(room_info_ptr->szRTMPIp);
                                std::string quotation("\"");
                                std::string url_command = "http://"+rtmpIp+"/set_output?venc_framerate=50&venc_bitrate=4096?user=admin&pass=admin";
                                std::string sys_command = "curl " + quotation + url_command + quotation;
                                LOGD ("[%s] [Stream Off] %s ", log_Time(), sys_command.c_str());
                                system(sys_command.c_str());
                                s_streamoff = 0;
                            }
                        }
                        else if (eventPtr->valueint == EVENT_SETTINGCONTROLL)
                        {
                            cJSON* optionPtr = cJSON_GetObjectItemCaseSensitive(jsonPtr, KEY_OPTION);
                            controller_admin_options(optionPtr->valueint);
                        }
                        else if (eventPtr->valueint == EVENT_REBOOT_SYSTEM)
                        {
                            FILE * p_file = popen("sudo reboot", "r");
                            if (!p_file) {
                                LOGD ("[%s] [WebSocket] EVENT_REBOOT_SYSTEM: Error", log_Time());
                            }
                            char	szLine[1024] = "";
                            while(!feof(p_file))
                            {
                                fgets(szLine, 1024, p_file);
                                LOGD ("[%s] [WebSocket] fgets: %s", log_Time(), szLine);
                            }
                            pclose(p_file);
                        }
                        else if (eventPtr->valueint == EVENT_CHANGE_SERVER)
                        {
                            cJSON* userIdPtr = cJSON_GetObjectItemCaseSensitive(jsonPtr, KEY_USER_ID);
                            change_server(userIdPtr->valuestring);
                        }
                        else if (eventPtr->valueint == EVENT_SCORE_RELEASE)
                        {
                            cJSON* positionsPtr = cJSON_GetObjectItemCaseSensitive(jsonPtr, "game_positions");
                            if (positionsPtr != NULL && cJSON_IsArray(positionsPtr))
                            {
                                int arraySize = cJSON_GetArraySize(positionsPtr);
                                int* pos_ptr = (int*)malloc(sizeof(int)*arraySize);
                                for(int i = 0; i < arraySize; i ++)
                                {
                                    cJSON* itemPtr = cJSON_GetArrayItem(positionsPtr, i);
                                    pos_ptr[i] = itemPtr->valueint;
                                }
                                controller_admin_clear_scores(pos_ptr, arraySize);
                                free(pos_ptr);
                            }
                        }
                        else if (eventPtr->valueint == EVENT_RESTART_FRP)
                        {
                            restart_frp();
                        }
                        break;
                    }
                }
            }
            cJSON_Delete(jsonPtr);
            jsonPtr = NULL;
        }
    } catch (websocketpp::exception const & e) {
        LOGD ("[%s] [Main-on_message] websocketpp::exception: %s", log_Time(), e.what());
    } catch (std::exception const & e) {
        LOGD ("[%s] [Main-on_message] std::exception: %s", log_Time(), e.what());
    } catch (...) {
        LOGD ("[%s] [Main-on_message] other exception", log_Time());
    }
}

bool perftest::on_ping(websocketpp::connection_hdl hdl, std::string str)
{
    LOGD ("[%s] [WebSocket] on_ping: %s", log_Time(), str.c_str());
    m_ping_time = get_timestamp(NULL, 1);
    return true;
}

void perftest::on_fail(websocketpp::connection_hdl hdl)
{
    try {
        if(m_wss_support)
        {
            client_wss::connection_ptr con = m_endpoint_wss.get_con_from_hdl(hdl);
            LOGE ("[%s] [WebSocket] Connect with server error, local reason: %d, %s, remote reason: %d, %s, message: %s", log_Time(), con->get_local_close_code(), con->get_local_close_reason().c_str(), \
                con->get_remote_close_code(), con->get_remote_close_reason().c_str(), con->get_ec().message().c_str());
        }
        else
        {
            client_ws::connection_ptr con = m_endpoint_ws.get_con_from_hdl(hdl);
            LOGE ("[%s] [WebSocket] Connect with server error, local reason: %d, %s, remote reason: %d, %s, message: %s", log_Time(), con->get_local_close_code(), con->get_local_close_reason().c_str(), \
                con->get_remote_close_code(), con->get_remote_close_reason().c_str(), con->get_ec().message().c_str());
        }

        m_status = STATUS_DISCONNECTED;
        m_ping_time = 0;
    } catch (websocketpp::exception const & e) {
        LOGD ("[%s] [WebSocket] on_fail websocketpp::exception: %s", log_Time(), e.what());
    } catch (std::exception const & e) {
        LOGD ("[%s] [WebSocket] on_fail std::exception: %s", log_Time(), e.what());
    } catch (...) {
        LOGD ("[%s] [WebSocket] on_fail other exception", log_Time());
    }
}

void perftest::on_close(websocketpp::connection_hdl hdl)
{
    try {
        if(m_wss_support)
        {
            client_wss::connection_ptr con = m_endpoint_wss.get_con_from_hdl(hdl);
            if (con->get_local_close_code() == websocketpp::close::status::abnormal_close)
                m_reconnect_delay = 1;
            else
                m_reconnect_delay = 0;

            LOGE ("[%s] [WebSocket] on_close, local reason: %d, %s, remote reason: %d, %s, message: %s", log_Time(), con->get_local_close_code(), con->get_local_close_reason().c_str(), \
                con->get_remote_close_code(), con->get_remote_close_reason().c_str(), con->get_ec().message().c_str());
        }
        else
        {
            client_ws::connection_ptr con = m_endpoint_ws.get_con_from_hdl(hdl);
            if (con->get_local_close_code() == websocketpp::close::status::abnormal_close)
                m_reconnect_delay = 1;
            else
                m_reconnect_delay = 0;

            LOGE ("[%s] [WebSocket] on_close, local reason: %d, %s, remote reason: %d, %s, message: %s", log_Time(), con->get_local_close_code(), con->get_local_close_reason().c_str(), \
                con->get_remote_close_code(), con->get_remote_close_reason().c_str(), con->get_ec().message().c_str());
        }
        m_status = STATUS_DISCONNECTED;
        m_ping_time = 0;
        close_socket("on_close", hdl);
    } catch (websocketpp::exception const & e) {
        LOGD ("[%s] [WebSocket] on_close websocketpp::exception: %s", log_Time(), e.what());
    } catch (std::exception const & e) {
        LOGD ("[%s] [WebSocket] on_close std::exception: %s", log_Time(), e.what());
    } catch (...) {
        LOGD ("[%s] [WebSocket] on_close other exception", log_Time());
    }
}

void perftest::send_message(char* message, const char* title, bool is_log)
{
    try {
        if(m_wss_support)
            m_endpoint_wss.send(m_hdl, message, websocketpp::frame::opcode::TEXT);
        else
            m_endpoint_ws.send(m_hdl, message, websocketpp::frame::opcode::TEXT);
        if (is_log)
        {
            char* map_str = get_value_map(message);
            delete_char(map_str, '\n');
            if (title != NULL && strlen(title) > 0)
                LOGD ("[%s] [WebSocket SEND: %s] %s", log_Time(), title, map_str);
            else
                LOGD ("[%s] [WebSocket SEND] %s", log_Time(), map_str);
        }
        free(message);

    } catch (websocketpp::exception const & e) {
        LOGD ("[%s] [WebSocket] send_message websocketpp::exception: %s", log_Time(), e.what());
    } catch (std::exception const & e) {
        LOGD ("[%s] [WebSocket] send_message std::exception: %s", log_Time(), e.what());
    } catch (...) {
        LOGD ("[%s] [WebSocket] send_message other exception", log_Time());
    }
}

void perftest::check_websocet()
{
    long long ll_cur_time = get_timestamp(NULL, 1);
    if (m_ping_time > 0 && (ll_cur_time-m_ping_time) >= 30000 && m_status == STATUS_CONNECTED)
    {
        LOGD ("[%s] [WebSocket] ping timeout", log_Time());
        m_status = STATUS_DISCONNECTING;
        close_socket("ping timeout", m_hdl);
    }
}

int main ()
{
    room_info_ptr = loto_room_init();
    if (room_info_ptr == NULL)
    {
        LOGE ("[%s] can't get room info", log_Time());
        return 0;
    }

    char sz_server_file[512];
    sprintf(sz_server_file, "%sWaController/server.ini", WORK_FOLDER);
    s_auto_debug = GetIniKeyInt((char*)"server", (char*)"auto_debug", sz_server_file);
    s_detect_log = GetIniKeyInt((char*)"server", (char*)"detect_log", sz_server_file);
    s_auto_monster = GetIniKeyInt((char*)"server", (char*)"auto_monster", sz_server_file);
    s_auto_baoji = GetIniKeyInt((char*)"server", (char*)"auto_baoji", sz_server_file);
    generate_maps();
    s_strength_list.resize(room_info_ptr->iPlayerCount);
    LOGD ("[%s] [Main] room subtype: %d", log_Time(), room_info_ptr->iSubType);
    controller_init(room_info_ptr->iSubType);
    init_detection(room_info_ptr->iSubType, room_info_ptr->iPlayerCount);

    try {
        endpoint = new perftest(room_info_ptr->szWebsocket);
        endpoint->start();
    } catch (websocketpp::exception const & e) {
        LOGD ("[%s] [Main] websocketpp::exception: %s", log_Time(), e.what());
    } catch (std::exception const & e) {
        LOGD ("[%s] [Main] std::exception: %s", log_Time(), e.what());
    } catch (...) {
        LOGD ("[%s] [Main] other exception", log_Time());
    }

    if (endpoint != NULL)
    {
        delete endpoint;
        endpoint = NULL;
    }

    free(room_info_ptr);
    room_info_ptr = NULL;

    uninit_detection();
    if (key_maps != NULL)
        cJSON_Delete(key_maps);
    if (action_maps != NULL)
        cJSON_Delete(action_maps);
    if (event_maps != NULL)
        cJSON_Delete(event_maps);
    if (option_maps != NULL)
        cJSON_Delete(option_maps);
    return 0;
}