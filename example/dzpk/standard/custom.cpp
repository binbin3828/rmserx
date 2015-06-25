/*
**************************************************************
** 版权：深圳市华诚亿科技有限公司
** 创建时间：2015.5.2
** 版本：1.0
** 说明：德州扑克标准流程活动
**************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <string>
#include <vector>
#include "custom.h"
#include "../include/json/json.h"
#include <openssl/md5.h>
#include "../../../core/loger/log.h"
#include "dzpk.h"
#include "../Start.h"
#include "../../../com/common.h"
#include "../db/dzpkDb.h"
#include "dzpkCustom.h"
#include "dzpkTask.h"
#include "dzpkActivity.h"

using namespace  std;

/*
* 字符串倒转函数
*/
void strrever(char *s)
{
    int c;
    char *t;

    for (t = s + (strlen(s) - 1); s < t; s++, t-- )
    {
        c = *s;
        *s = *t;
        *t = c;
    }
}


/*
*
*
*/
char * convert(int value, char* buffer)
{
    sprintf(buffer, "%d", value);
    return buffer;
}

string GetString(int n)
{
    std::stringstream str;
    str<<n;
    return str.str();
}

string GetlongString(long n)
{
    std::stringstream str;
    str<<n;
    return str.str();
}

int GetInt(string s)
{
    int n = atoi(s.c_str());
    return n;
}


/*
* 转换整型变量到字符串型
*/
char* itoa(int i, int radix)
{
    // 考虑了32位的二进制
    static char local[33];
    char *p = &local[32];
    int sign = 0;
    unsigned int tmp;
    static unsigned char table[] = "0123456789abcdef";

    if ( radix < 2 || radix > 16 )
    {
        *p = '\0';
        return p;
    }

    // 十进制才有"负数"之说
    if (i < 0 && radix == 10)
    {
        i = -i;
        sign = 1;
    }

    // 其它进制，强制转换成无符号类型
    tmp =  i;

    // 逆序保存
    *p-- = '\0';
    do
    {
        *p-- = table[tmp % radix];
        tmp /= radix;
    }
    while (tmp > 0);

    if (sign)
    {
        *p-- = '-';
    }
    return p + 1;
}




/*
* 分割字符串
*/
void split(const string& src, const string& separator, vector<string>& dest)
{
    string str = src;
    string substring;
    string::size_type start = 0, index;

    do
    {
        index = str.find_first_of(separator,start);
        if (index != string::npos)
        {
            substring = str.substr(start,index-start);
            dest.push_back(substring);
            start = str.find_first_not_of(separator,index);
            if (start == string::npos) return;
        }
    }
    while(index != string::npos);

    substring = str.substr(start);
    dest.push_back(substring);
}


/*
*  设置SOCKET句柄为非阻塞模式
*/
int setnonblocking(int sockfd)
{
    if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)|O_NONBLOCK) == -1)
    {
        return -1;
    }
    return 0;
}


/*
* 获取当前高精度时间精确到毫秒
*
*/
long  get_current_time()
{
    struct timeval  tv;
    gettimeofday(&tv,NULL);
    return  tv.tv_sec *1000 + tv.tv_usec /1000;
}

/*获得随机数*/
int DzpkGetRandom(int nMax)
{
    int nRet = 0;
    static int nIndex = 2;
    nIndex++;
    if(nIndex > 100000000)
    {
        nIndex = 2;
    }

    struct timeval  tv;
    gettimeofday(&tv,NULL);

    unsigned int nBase =   tv.tv_sec + tv.tv_usec;
    unsigned int nBaseChild = (tv.tv_usec)*nIndex;
    unsigned int nBaseSecond = (tv.tv_sec)*nIndex + 1;
    unsigned int nSrand = nBase + nBaseChild + nBaseSecond;

    srand(nSrand);
    nRet = random()%nMax;

    return nRet;
}


/*
*   计算经验值和等级
*
*/
long computer_exp_level( long& experience, int& level,vector<level_system>  level_list)
{
    long rest_exp=0;
    for(unsigned int i=0; i< level_list.size(); i++)
    {
        if( experience > (level_list.at(i).exp_total -  level_list.at(i).exp_upgrade)  &&  experience <= level_list.at(i).exp_total )
        {
            rest_exp = experience - (level_list.at(i).exp_total -  level_list.at(i).exp_upgrade);
            level = level_list.at(i).level;
        }
    }

    return rest_exp;
}



/*
*   获取该等级升级需要的经验值
*
*/
long get_total_exp_level( int& level,vector<level_system>  level_list)
{
    long upgrade_exp=0;
    for(unsigned int i=0; i< level_list.size(); i++)
    {
        if( level == level_list.at(i).level )
        {
            upgrade_exp =  level_list.at(i).exp_upgrade;
        }
    }

    return upgrade_exp;
}


/*
*更新玩家最大的赢牌记录
*
*/
void update_user_best_win_card(vector<dealer *>&  dealers_vector,int room_number,ConnPool *connpool)
{

    Connection *con = connpool->GetConnection();
    if(con)
    {
        string sShow;
        try
        {
            Statement *state =con->createStatement();//获取连接状态
            state->execute("use dzpk");//使用指定的数据库

            for(unsigned int i=0; i< dealers_vector.at(room_number)->players_list.size(); i++)
            {
                string user_account = dealers_vector.at(room_number)->players_list.at(i).user_account;
                vector<int> win_cards = dealers_vector.at(room_number)->players_list.at(i).win_cards;

                string query_sql="select * from `tbl_user_mst` t1 left join `tbl_userinfo_mst` t2 on  t1.user_id = t2.user_id  where user_account='"+user_account+"'";
                sShow = query_sql;
                ResultSet *result = state->executeQuery(query_sql);
                string user_id="";
                string best_win_card="";
                while (result->next())
                {
                    user_id =result->getString("user_id");
                    best_win_card = result->getString("best_win_card");
                }
                if(user_id =="")
                {
                    WriteLog(40058,"user id null sql[%s]\n",query_sql.c_str());
                    if(result)
                    {
                        delete result;
                        result = NULL;
                    }
                    break;
                    /*
                    delete state;//删除连接状态
                    connpool->ReleaseConnection(con);//释放当前连接
                    return;
                    */
                }
                vector<string> db_win_cards;
                split(best_win_card,"_",db_win_cards);
                if( db_win_cards.size() < 6 )
                {
                    best_win_card = "302_203_304_206_207_107";
                    split(best_win_card,"_",db_win_cards);
                }

                if(win_cards.size() ==6)
                {
                    if( win_cards.at(5)  >  atoi(db_win_cards.at(5).c_str()) )
                    {
                        string up_win_card = itoa(win_cards.at(0),10);
                        up_win_card += "_";
                        up_win_card +=  itoa(win_cards.at(1),10);
                        up_win_card += "_";
                        up_win_card +=  itoa(win_cards.at(2),10);
                        up_win_card += "_";
                        up_win_card +=  itoa(win_cards.at(3),10);
                        up_win_card += "_";
                        up_win_card +=  itoa(win_cards.at(4),10);
                        up_win_card += "_";
                        up_win_card +=  itoa(win_cards.at(5),10);
                        string  sql="update tbl_userinfo_mst set best_win_card = '"+ up_win_card;
                        sql=sql+"' where user_id = " + user_id;
                        sShow = sql;
                        // UPDATE
                        state->execute(sql);

                    }
                }

                if(result)
                {
                    delete result;
                    result = NULL;
                }
            }
            delete state;//删除连接状态
        }
        catch(sql::SQLException&e)
        {
            WriteLog(HIGHEST_LEVEL_ERROR,"sql error[%s] sql [%s] \n",e.what(),sShow.c_str());
        }
        catch(...)
        {
            WriteLog(HIGHEST_LEVEL_ERROR,"error \n");
        }
        connpool->ReleaseConnection(con);//释放当前连接
    }
}

/*
*更新玩家的等级经验
*
*/
void update_user_exp_level(string user_account,int dzpk_level,long dzpk_experience,ConnPool *connpool)
{
    //更新

    string sdzpk_level = itoa(dzpk_level,10);
    if(dzpk_experience < 0 )
    {
        dzpk_experience = 0;
    }
    string sdzpk_experience = itoa(dzpk_experience,10);
    Connection *con = connpool->GetConnection();
    if(con)
    {
        string sShow;
        try
        {
            Statement *state =con->createStatement();//获取连接状态
            state->execute("use dzpk");//使用指定的数据库
            string query_sql="select * from `tbl_user_mst` where user_account='"+user_account+"'";
            sShow = query_sql;
            ResultSet *result = state->executeQuery(query_sql);
            string user_id="";
            while (result->next())
            {
                user_id =result->getString("user_id");
            }

            if( user_id !="")
            {
                string  sql="update tbl_userinfo_mst set dzpk_level = "+ sdzpk_level +",dzpk_experience= "+sdzpk_experience;
                sql=sql+" where user_id = " + user_id;
                sShow = sql;
                // INSERT
                state->execute(sql);
            }

            if(result)
            {
                delete result;
                result = NULL;
            }

            delete state;//删除连接状态
        }
        catch(sql::SQLException&e)
        {
            WriteLog(HIGHEST_LEVEL_ERROR,"sql error[%s] sql [%s] \n",e.what(),sShow.c_str());
        }
        catch(...)
        {
            WriteLog(HIGHEST_LEVEL_ERROR,"error \n");
        }
        connpool->ReleaseConnection(con);//释放当前连接
    }

}
/*
*更新玩家session中跟踪的房间位置
*
*/
void update_user_persition(string user_account,int room_num,ConnPool *connpool)
{

    //更新
    Connection *con = connpool->GetConnection();
    if(con)
    {
        string sShow;
        try
        {
            Statement *state =con->createStatement();//获取连接状态
            state->execute("use dzpk");//使用指定的数据库

            string query_sql="select * from `tbl_user_mst` where user_account='"+user_account+"'";
            sShow = query_sql;
            ResultSet *result = state->executeQuery(query_sql);
            string user_id="";
            while (result->next())
            {
                user_id =result->getString("user_id");
            }
            string  sroom_num = itoa(room_num,10);
            if( user_id!="")
            {
                string  sql="update tbl_session_mst set roomnum = "+sroom_num;
                sql=sql+" where user_id = " + user_id  ;
                sShow = sql;
                WriteLog(4000,"update_user_persition1:%s \n",sql.c_str());
                // INSERT
                state->execute(sql);
            }

            if(result)
            {
                delete result;
                result = NULL;
            }
            delete state;//删除连接状态
        }
        catch(sql::SQLException&e)
        {
            WriteLog(HIGHEST_LEVEL_ERROR,"sql error[%s] sql [%s] \n",e.what(),sShow.c_str());
        }
        catch(...)
        {
            WriteLog(HIGHEST_LEVEL_ERROR,"error \n");
        }
        connpool->ReleaseConnection(con);//释放当前连接
    }
}

/*
*更新玩家session中跟踪的房间位置
*
*/
void update_session_time(string user_account,ConnPool *connpool)
{
    //更新
    Connection *con = connpool->GetConnection();
    if(con)
    {
        string sShow;
        try
        {
            Statement *state =con->createStatement();//获取连接状态
            state->execute("use dzpk");//使用指定的数据库

            string query_sql="select * from `tbl_user_mst` where user_account='"+user_account+"'";
            sShow = query_sql;
            ResultSet *result = state->executeQuery(query_sql);
            string user_id="";
            while (result->next())
            {
                user_id =result->getString("user_id");
            }

            if( user_id !="" )
            {
                string  sql="update tbl_session_mst set start_time = now() ";
                sql=sql+" where user_id = " + user_id  ;
                sShow = sql;
                // INSERT
                state->execute(sql);
            }

            if(result)
            {
                delete result;
                result = NULL;
            }

            delete state;//删除连接状态
        }
        catch(sql::SQLException&e)
        {
            WriteLog(HIGHEST_LEVEL_ERROR,"sql error[%s] sql [%s] \n",e.what(),sShow.c_str());
        }
        catch(...)
        {
            WriteLog(HIGHEST_LEVEL_ERROR,"error \n");
        }
        connpool->ReleaseConnection(con);//释放当前连接
    }
}

/*
* 更新玩家的筹码账号
*
*/
#if 0
void update_user_chip_account(string user_account,long chip_account,ConnPool *connpool)
{
    //更新,已经没有调用此函数

    Connection *con = connpool->GetConnection();
    Statement *state =con->createStatement();//获取连接状态
    state->execute("use dzpk");//使用指定的数据库

    string query_sql="select * from `tbl_user_mst` t1 left join `tbl_userinfo_mst` t2 on t1.user_id = t2.user_id where user_account='"+user_account+"'";
    ResultSet *result = state->executeQuery(query_sql);
    string user_id="";
    string db_chip_account="";
    string   bigest_win_chip="";
    string   bigest_chip="";

    while (result->next())
    {
        user_id =result->getString("user_id");
        db_chip_account = result->getString("chip_account");
        bigest_win_chip = result->getString("bigest_win_chip");
        bigest_chip = result->getString("bigest_chip");
    }

    if(user_id !="")
    {
        if ( chip_account > atol(db_chip_account.c_str())*8 || chip_account< 0)
        {

            if(result)
            {
                delete result;
                result = NULL;
            }
            delete state;//删除连接状态
            connpool->ReleaseConnection(con);//释放当前连接
            return;
        }

        //是否需要更新赢得最大的筹码数
        if( (chip_account -  atol(db_chip_account.c_str())) > atol(bigest_win_chip.c_str()) && user_id !="" )
        {
            string  bigest_chip_acnt = itoa( (chip_account -  atol(db_chip_account.c_str())) ,10);
            string  sql="update tbl_userinfo_mst set bigest_win_chip = "+bigest_chip_acnt;
            sql=sql+" where user_id = " + user_id;
            // UPDATE
            state->execute(sql);
        }

        //赢牌次数加一
        if( (chip_account -  atol(db_chip_account.c_str())) >= 0 && user_id !="" )
        {
            string  sql="update tbl_userinfo_mst set win_per = win_per +1  where user_id =  "+user_id;
            // UPDATE
            state->execute(sql);
        }

        //我筹码达到最大记录
        if( chip_account > atol(bigest_chip.c_str()) && user_id !=""  )
        {
            string  bigest_acnt = itoa( chip_account,10);
            string  sql="update tbl_userinfo_mst set bigest_chip = "+bigest_acnt;
            sql=sql+" where user_id = " + user_id;
            // UPDATE
            state->execute(sql);
        }

        //更新筹码
        string  chip_acnt = itoa(chip_account,10);
        string  sql="update tbl_user_mst set chip_account = "+chip_acnt;
        sql=sql+" where user_id = " + user_id;
        // UPDATE
        state->execute(sql);

        //更新玩了多少次
        string sql_a ="update tbl_userinfo_mst set total_time = total_time +1  where user_id ="+user_id;
        // UPDATE
        state->execute(sql_a);

    }

    if(result)
    {
        delete result;
        result = NULL;
    }
    delete state;//删除连接状态
    connpool->ReleaseConnection(con);//释放当前连接

}
#endif

/*
*更新房间内玩家坐下及旁观人数
*
*/
void DzpkUpdateRoomPerson(int nRoomID,ConnPool *connpool)
{
    /*更新房间坐下人数和旁观人数*/
    int nSeatPersion = 0,nSidePersion = 0;
    dealer *pRoom = DzpkGetRoomInfo(nRoomID);
    if(pRoom)
    {
    }
    else
    {
        return;
    }

    /*计算房间人数*/
    for(unsigned int i=0; i<pRoom->players_list.size(); i++)
    {
        if(pRoom->players_list[i].seat_num > 0)
        {
            nSeatPersion++;
        }
        else
        {
            nSidePersion++;
        }
    }

    /*更新*/
    Connection *con = connpool->GetConnection();
    if(con)
    {
        string sShow;
        try
        {
            Statement *state =con->createStatement();//获取连接状态
            state->execute("use dzpk");//使用指定的数据库
            string  sql="update tbl_room_mst set current_seat_person = " + GetString(nSeatPersion);
            sql = sql +",current_sideline_person =" + GetString(nSidePersion);
            sql=sql+" where room_id = " + GetString(nRoomID);
            sShow = sql;

            WriteLog(400000,"DzpkUpdateRoomPerson sql:[%s] \n",sql.c_str());


            // INSERT
            state->execute(sql);

            delete state;//删除连接状态
        }
        catch(sql::SQLException&e)
        {
            WriteLog(HIGHEST_LEVEL_ERROR,"sql error[%s] sql [%s] \n",e.what(),sShow.c_str());
        }
        catch(...)
        {
            WriteLog(HIGHEST_LEVEL_ERROR,"error \n");
        }
        connpool->ReleaseConnection(con);//释放当前连接
    }
}



/*
*  获取下一个玩家的座位号
*/
int next_player(int current_seat, vector<dealer *>&  dealers_vector,int room_number,int nSeatCount)
{
    int next_seat=0;//下一个玩家的座位号
    for (int j=current_seat; j<current_seat +(nSeatCount -1) ; j++)
    {
        for(unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++)
        {
            //查找下一家的范围包含 加注、跟注、看牌的人，弃牌，全下的人不在范围内
            if( (((dealers_vector.at(room_number))->players_list).at(i)).user_staus == 2 || (((dealers_vector.at(room_number))->players_list).at(i)).user_staus == 3 || (((dealers_vector.at(room_number))->players_list).at(i)).user_staus == 4 )
            {
                if( (j+1) <= nSeatCount)
                {
                    if( (((dealers_vector.at(room_number))->players_list).at(i)).seat_num == (j+1)  )
                    {
                        next_seat =(j+1) ;
                    }
                }
                else
                {
                    if( (((dealers_vector.at(room_number))->players_list).at(i)).seat_num == (j+1)%nSeatCount)
                    {
                        next_seat = (j+1)%nSeatCount ;
                    }
                }

            }

        }
        if(next_seat !=0)
        {
            break;
        }
    }

    //假如找到的下家仍然是当前玩家，则下一个玩家为0
    if( next_seat == current_seat)
    {
        next_seat =0;
    }

    //假如找到的下家，可以进行的最大加注是0
    for(unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++)
    {
        if(((dealers_vector.at(room_number))->players_list).at(i).seat_num == next_seat )
        {
            int max_follow_chip=0;
            for(unsigned int m=0; m<((dealers_vector.at(room_number))->players_list).size(); m++)
            {
                if( (((dealers_vector.at(room_number))->players_list).at(m)).nUserID != (((dealers_vector.at(room_number))->players_list).at(i)).nUserID)
                {
                    //除自己之外
                    if( (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 2 || (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 3 || (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 4 || (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 6  )
                    {
                        //仍然可以下注的玩家
                        if( max_follow_chip < (((dealers_vector.at(room_number))->players_list).at(m)).hand_chips + (((dealers_vector.at(room_number))->players_list).at(m)).desk_chip )
                        {
                            max_follow_chip = (((dealers_vector.at(room_number))->players_list).at(m)).hand_chips + (((dealers_vector.at(room_number))->players_list).at(m)).desk_chip ;
                        }
                    }
                }
            }


            //除自己外，剩余玩家的手上筹码最多的一位，若大于自己手上筹码时，最大跟注为手上的总筹码数
            if( max_follow_chip >= (((dealers_vector.at(room_number))->players_list).at(i)).hand_chips + (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip )
            {
                max_follow_chip = (((dealers_vector.at(room_number))->players_list).at(i)).hand_chips;
            }
            else
            {
                max_follow_chip = max_follow_chip - (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip;
            }

            if( max_follow_chip <= 0)
            {
                next_seat =0;
            }
        }
    }
    return next_seat;
}


/*
*  异常边池计算逻辑
*/
void  edge_pool_special(vector<dealer *>&  dealers_vector,int room_number,vector<edge_pool>& edge_pool_temp )
{
    while(1)
    {

        //先找到边池的最底部,即有全下的玩家且桌面注金少于最小跟注，产生边池。
        int bottom = (dealers_vector.at(room_number))->follow_chip;
        for(unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++)
        {
            if( (((dealers_vector.at(room_number))->players_list).at(i)).user_staus == 2 || (((dealers_vector.at(room_number))->players_list).at(i)).user_staus == 3 || (((dealers_vector.at(room_number))->players_list).at(i)).user_staus == 4 || (((dealers_vector.at(room_number))->players_list).at(i)).user_staus == 5 || (((dealers_vector.at(room_number))->players_list).at(i)).user_staus == 6   )
            {
                if(  (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip < bottom )
                {
                    if( (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip > 0 )
                    {
                        bottom  =  (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip;
                    }
                }
            }
        }

        for(unsigned int i=0; i<((dealers_vector.at(room_number))->leave_players_list).size(); i++)
        {
            if( (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus == 2 || (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus == 3 || (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus == 4 || (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus == 5 || (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus == 6   )
            {
                if(  (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip < bottom )
                {
                    if( (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip > 0 )
                    {
                        bottom  =  (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip;
                    }
                }
            }
        }

        if(bottom <= 0)
        {
            break;
        }

        edge_pool  ed_pool;
        ed_pool.edge_height = bottom;
        ed_pool.pool_type = 1;

        for(unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++)
        {
            if( (((dealers_vector.at(room_number))->players_list).at(i)).user_staus == 6  &&  (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip == 0  )
            {
                ed_pool.pool_type = 0;
            }
        }
        if( ed_pool.pool_type == 1)
        {
            for(unsigned int i=0; i<((dealers_vector.at(room_number))->leave_players_list).size(); i++)
            {
                if( (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus == 6  &&  (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip == 0  )
                {
                    ed_pool.pool_type = 0;
                }
            }
        }

        int max_desk_chip=0;
        for(unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++)
        {
            if( (((dealers_vector.at(room_number))->players_list).at(i)).user_staus  >=2 )
            {

                if( ed_pool.pool_type != 1 && (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip <=0)
                {
                    continue;
                }

                if( (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip > max_desk_chip )
                {
                    max_desk_chip = (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip;
                }

            }

        }

        //已经离开或站起的玩家同样在本局有效
        for(unsigned int i=0; i<((dealers_vector.at(room_number))->leave_players_list).size(); i++)
        {
            if( (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus  >=2 )
            {


                if( ed_pool.pool_type != 1 && (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip <=0)
                {
                    continue;
                }

                if( (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip > max_desk_chip )
                {
                    max_desk_chip = (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip;
                }
            }
        }

        if( max_desk_chip <= 0 )
        {
            break;
        }


        max_desk_chip=0;


        int dirr_temp=0;

        for(unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++)
        {
            if( (((dealers_vector.at(room_number))->players_list).at(i)).user_staus  >=2 )
            {

                if(  (((dealers_vector.at(room_number))->players_list).at(i)).user_staus ==2  || (((dealers_vector.at(room_number))->players_list).at(i)).user_staus ==3 || (((dealers_vector.at(room_number))->players_list).at(i)).user_staus ==4 )
                {
                }
                else
                {
                    if( ed_pool.pool_type != 1 && (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip <=0)
                    {
                        continue;
                    }
                }


                if(  (((dealers_vector.at(room_number))->players_list).at(i)).user_staus ==2  || (((dealers_vector.at(room_number))->players_list).at(i)).user_staus ==3 || (((dealers_vector.at(room_number))->players_list).at(i)).user_staus ==4 || (((dealers_vector.at(room_number))->players_list).at(i)).user_staus ==6 ) //从桌面注金中减去底池
                {
                    if( (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip  >=  bottom) //已经弃牌的玩家桌面注金如大于底池，参与分享该层边池的玩家数量增加一位，但座位号设为0
                    {
                        (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip -= bottom;
                    }
                    else if (  (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip  <  bottom)  //已经弃牌的玩家桌面注金小于底池，也增加一个座位，但座位号为负值（底池的差额）
                    {
                        dirr_temp += (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip  - bottom;
                        (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip = 0;
                    }
                    ed_pool.share_seat.push_back(  (((dealers_vector.at(room_number))->players_list).at(i)).seat_num);

                }
                else if ( (((dealers_vector.at(room_number))->players_list).at(i)).user_staus ==5 )  //弃牌玩家的边池计算
                {

                    if( (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip  >=  bottom) //已经弃牌的玩家桌面注金如大于底池，参与分享该层边池的玩家数量增加一位，但座位号设为0
                    {
                        (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip -= bottom;
                        ed_pool.share_seat.push_back(0);
                    }
                    else if (  (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip  <  bottom)  //已经弃牌的玩家桌面注金小于底池，也增加一个座位，但座位号为负值（底池的差额）
                    {

                        ed_pool.share_seat.push_back( (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip  - bottom  );
                        (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip = 0;
                    }
                }

                if( (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip > max_desk_chip )
                {
                    max_desk_chip = (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip;
                }

            }

        }

        //已经离开或站起的玩家同样在本局有效
        for(unsigned int i=0; i<((dealers_vector.at(room_number))->leave_players_list).size(); i++)
        {
            if( (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus  >=2 )
            {
                if(  (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus ==2  || (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus ==3 || (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus ==4 )
                {
                }
                else
                {
                    if( ed_pool.pool_type != 1 && (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip <=0)
                    {
                        continue;
                    }
                }


                if(  (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus ==2  || (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus ==3 || (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus ==4 || (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus ==6 ) //从桌面注金中减去底池
                {
                    if( (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip  >=  bottom) //已经弃牌的玩家桌面注金如大于底池，参与分享该层边池的玩家数量增加一位，但座位号设为0
                    {
                        (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip -= bottom;
                    }
                    else if (  (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip  <  bottom)  //已经弃牌的玩家桌面注金小于底池，也增加一个座位，但座位号为负值（底池的差额）
                    {
                        dirr_temp += (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip  - bottom;
                        (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip = 0;
                    }
                    ed_pool.share_seat.push_back(  (((dealers_vector.at(room_number))->leave_players_list).at(i)).seat_num);
                }
                else if (    (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus ==5 )  //弃牌玩家的边池计算
                {

                    if( (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip  >=  bottom) //已经弃牌的玩家桌面注金如大于底池，参与分享该层边池的玩家数量增加一位，但座位号设为0
                    {
                        (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip -= bottom;
                        ed_pool.share_seat.push_back(0);
                    }
                    else if (  (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip  <  bottom)  //已经弃牌的玩家桌面注金小于底池，也增加一个座位，但座位号为负值（底池的差额）
                    {
                        ed_pool.share_seat.push_back( (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip  - bottom  );
                        (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip = 0;
                    }
                }

                if( (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip > max_desk_chip )
                {
                    max_desk_chip = (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip;
                }
            }
        }

        //对主池、边池重新检查，如果分享人数与第一个主池的分享人数不相等的，判定为边池。
        if( ed_pool.pool_type == 1 )
        {
            if( (dealers_vector.at(room_number))->edge_pool_list.size() > 0 )
            {
                if( ed_pool.share_seat.size() != (dealers_vector.at(room_number))->edge_pool_list.at(0).share_seat.size()   )
                {
                    ed_pool.pool_type = 0;
                }
            }
        }




        if(ed_pool.share_seat.size() >=1) //边池如有玩家参与分享，将该边池存入 边池队列容器
        {

            if( dirr_temp < 0 )
            {

                for(unsigned int h=0; h < ed_pool.share_seat.size(); h++)
                {
                    if( ed_pool.share_seat.at(h) <= 0 )
                    {
                        ed_pool.share_seat.at(h)  += dirr_temp;
                        h = ed_pool.share_seat.size() -1;
                    }
                }
            }

            //计算池子的pool_id    0为主池的id  大于0为各边池的id
            if(  (dealers_vector.at(room_number))->edge_pool_list.size() == 0 )
            {
                ed_pool.pool_id = 0;
                (dealers_vector.at(room_number))->edge_pool_list.push_back(ed_pool);
                edge_pool_temp.push_back(ed_pool);
            }
            else
            {
                //检查池子的种类，分享人数相同的 为需要合并的池子，pool_id相同
                int find_same_pool = 0;
                for(unsigned int r=0; r< (dealers_vector.at(room_number))->edge_pool_list.size(); r++  )
                {

                    int sizea = ed_pool.share_seat.size();
                    int sizeb = (dealers_vector.at(room_number))->edge_pool_list.at(r).share_seat.size();

                    if(  sizea  == sizeb  )
                    {
                        ed_pool.pool_id = (dealers_vector.at(room_number))->edge_pool_list.at(r).pool_id;

                        (dealers_vector.at(room_number))->edge_pool_list.at(r).edge_height +=ed_pool.edge_height;

                        edge_pool ed_pool_tmp;
                        ed_pool_tmp = ed_pool;

                        int temp_f = 0;
                        for(unsigned int t=0; t < (dealers_vector.at(room_number))->edge_pool_list.at(r).share_seat.size(); t++)
                        {
                            if( (dealers_vector.at(room_number))->edge_pool_list.at(r).share_seat.at(t) < 0 )
                            {
                                temp_f += (dealers_vector.at(room_number))->edge_pool_list.at(r).share_seat.at(t);
                            }
                        }

                        for(unsigned int h=0; h < ed_pool_tmp.share_seat.size(); h++)
                        {
                            if( ed_pool_tmp.share_seat.at(h) < 0 )
                            {
                                ed_pool_tmp.share_seat.at(h)  += temp_f;
                                h = ed_pool_tmp.share_seat.size() -1;
                            }
                        }

                        (dealers_vector.at(room_number))->edge_pool_list.at(r).share_seat = ed_pool_tmp.share_seat;

                        find_same_pool ++ ;
                        r =  (dealers_vector.at(room_number))->edge_pool_list.size()-1;
                    }

                }

                if( find_same_pool == 0)  //没有找到的话
                {
                    int k = (dealers_vector.at(room_number))->edge_pool_list.size();
                    ed_pool.pool_id = (dealers_vector.at(room_number))->edge_pool_list.at(k-1).pool_id +1;
                    (dealers_vector.at(room_number))->edge_pool_list.push_back(ed_pool);

                }
                edge_pool_temp.push_back(ed_pool);
            }


            for(unsigned int i =0; i< ed_pool.share_seat.size(); i++)
            {
            }

        }

        if( max_desk_chip <= 0 )
        {
            break;
        }
    }


}


/*
*  正常边池计算逻辑
*/
void  edge_pool_normal(vector<dealer*>&  dealers_vector,int room_number,vector<edge_pool>& edge_pool_temp )
{
    char szLog[WRITE_LOG_LEN],szLogTemp[200];
    memset(szLog,0,WRITE_LOG_LEN);

    sprintf(szLogTemp,"step[%d] follow chip[%d] :",dealers_vector[room_number]->step,(dealers_vector.at(room_number))->follow_chip);
    memcpy(szLog+strlen(szLog),szLogTemp,strlen(szLogTemp));

    while(1)
    {
        int nCanCompose = 0;
        //先找到边池的最底部,即有全下的玩家且桌面注金少于最小跟注，产生边池。
        int bottom = (dealers_vector.at(room_number))->follow_chip;
        for(unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++)
        {
            if( (((dealers_vector.at(room_number))->players_list).at(i)).user_staus == 2 || (((dealers_vector.at(room_number))->players_list).at(i)).user_staus == 3 || (((dealers_vector.at(room_number))->players_list).at(i)).user_staus == 4 || (((dealers_vector.at(room_number))->players_list).at(i)).user_staus == 6   )
            {
                if((((dealers_vector.at(room_number))->players_list).at(i)).desk_chip > 0 )
                {
                    if((((dealers_vector.at(room_number))->players_list).at(i)).desk_chip < bottom )
                    {
                        bottom  =  (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip;
                    }
                    sprintf(szLogTemp,"user[%d]chip[%d],",(((dealers_vector.at(room_number))->players_list).at(i)).nUserID,(((dealers_vector.at(room_number))->players_list).at(i)).desk_chip);
                    memcpy(szLog+strlen(szLog),szLogTemp,strlen(szLogTemp));
                }
            }
        }

        for(unsigned int i=0; i<((dealers_vector.at(room_number))->leave_players_list).size(); i++)
        {
            if( (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus == 2 || (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus == 3 || (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus == 4 || (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus == 5 || (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus == 6   )
            {
                if( (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip > 0 )
                {
                    if((((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip < bottom )
                    {
                        bottom  =  (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip;
                    }
                    sprintf(szLogTemp,"user[%d]chip[%d],",(((dealers_vector.at(room_number))->leave_players_list).at(i)).nUserID,(((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip);
                    memcpy(szLog+strlen(szLog),szLogTemp,strlen(szLogTemp));
                }
            }
        }

        if(bottom <= 0)
        {
            WriteRoomLog(4001,dealers_vector[room_number]->dealer_id,0,"%s\n",szLog);
            break;
        }

        edge_pool  ed_pool;
        ed_pool.edge_height = bottom;
        ed_pool.pool_type = 1;

        //如果有全下并且筹码为0，一定是边池
        for(unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++)
        {
            if((((dealers_vector.at(room_number))->players_list).at(i)).user_staus == 6  &&  (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip == 0  )
            {
                ed_pool.pool_type = 0;
            }
        }
        if( ed_pool.pool_type == 1)
        {
            for(unsigned int i=0; i<((dealers_vector.at(room_number))->leave_players_list).size(); i++)
            {
                if( (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus == 6  &&  (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip == 0  )
                {
                    ed_pool.pool_type = 0;
                }
            }
        }

        //边池子是否能合并
        nCanCompose = 1;
        for(unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++)
        {
            if((((dealers_vector.at(room_number))->players_list).at(i)).user_staus == 6  &&  (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip > 0  )
            {
                nCanCompose = 0;
                break;
            }
        }
        if( nCanCompose  == 1)
        {
            for(unsigned int i=0; i<((dealers_vector.at(room_number))->leave_players_list).size(); i++)
            {
                if( (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus == 6  &&  (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip > 0  )
                {
                    nCanCompose = 0;
                    break;
                }
            }
        }

        int max_desk_chip=0;
        for(unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++)
        {
            if( (((dealers_vector.at(room_number))->players_list).at(i)).user_staus  >=2 )
            {
                if( ed_pool.pool_type != 1 && (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip <=0)
                {
                    continue;
                }
                if( (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip > max_desk_chip )
                {
                    max_desk_chip = (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip;
                }
            }
        }

        //已经离开或站起的玩家同样在本局有效
        for(unsigned int i=0; i<((dealers_vector.at(room_number))->leave_players_list).size(); i++)
        {
            if( (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus  >=2 )
            {
                if( ed_pool.pool_type != 1 && (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip <=0)
                {
                    continue;
                }

                if( (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip > max_desk_chip )
                {
                    max_desk_chip = (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip;
                }
            }
        }

        //如果最大筹码为0，不会产生池子
        if( max_desk_chip <= 0 )
        {
            WriteRoomLog(4001,dealers_vector[room_number]->dealer_id,0,"%s\n",szLog);
            break;
        }


        max_desk_chip=0;

        for(unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++)
        {
            if( (((dealers_vector.at(room_number))->players_list).at(i)).user_staus  >=2 )
            {
                if( ed_pool.pool_type != 1 && (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip <=0)
                {
                    continue;
                }

                if(  (((dealers_vector.at(room_number))->players_list).at(i)).user_staus ==2  || (((dealers_vector.at(room_number))->players_list).at(i)).user_staus ==3 || (((dealers_vector.at(room_number))->players_list).at(i)).user_staus ==4 || (((dealers_vector.at(room_number))->players_list).at(i)).user_staus ==6 )
                {
                    //从桌面注金中减去底池
                    (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip -= bottom;
                    ed_pool.share_seat.push_back(  (((dealers_vector.at(room_number))->players_list).at(i)).seat_num);
                }
                else if ( (((dealers_vector.at(room_number))->players_list).at(i)).user_staus ==5 )
                {
                    //弃牌玩家的边池计算
                    if( (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip  >=  bottom)
                    {
                        //已经弃牌的玩家桌面注金如大于底池，参与分享该层边池的玩家数量增加一位，但座位号设为0
                        (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip -= bottom;
                        ed_pool.share_seat.push_back(0);
                    }
                    else if (  (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip  <  bottom)
                    {
                        //已经弃牌的玩家桌面注金小于底池，也增加一个座位，但座位号为负值（底池的差额）
                        ed_pool.share_seat.push_back( (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip  - bottom  );
                        (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip = 0;
                    }
                }

                if( (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip > max_desk_chip )
                {
                    max_desk_chip = (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip;
                }
            }
        }

        //已经离开或站起的玩家同样在本局有效
        for(unsigned int i=0; i<((dealers_vector.at(room_number))->leave_players_list).size(); i++)
        {
            if( (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus  >=2 )
            {
                if( ed_pool.pool_type != 1 && (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip <=0)
                {
                    continue;
                }

                if(  (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus ==2  || (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus ==3 || (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus ==4 || (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus ==6 )
                {
                    //从桌面注金中减去底池
                    (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip -= bottom;
                    ed_pool.share_seat.push_back(  (((dealers_vector.at(room_number))->leave_players_list).at(i)).seat_num);
                }
                else if (    (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus ==5 )
                {
                    //弃牌玩家的边池计算
                    if( (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip  >=  bottom)
                    {
                        //已经弃牌的玩家桌面注金如大于底池，参与分享该层边池的玩家数量增加一位，但座位号设为0
                        (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip -= bottom;
                        ed_pool.share_seat.push_back(0);
                    }
                    else if (  (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip  <  bottom)
                    {
                        //已经弃牌的玩家桌面注金小于底池，也增加一个座位，但座位号为负值（底池的差额）
                        ed_pool.share_seat.push_back( (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip  - bottom  );
                        (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip = 0;
                    }
                }

                if( (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip > max_desk_chip )
                {
                    max_desk_chip = (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip;
                }
            }
        }

        //对主池、边池重新检查，如果分享人数与第一个主池的分享人数不相等的，判定为边池。
        if( ed_pool.pool_type == 1 )
        {
            if( (dealers_vector.at(room_number))->edge_pool_list.size() > 0 )
            {
                if( ed_pool.share_seat.size() != (dealers_vector.at(room_number))->edge_pool_list.at(0).share_seat.size()   )
                {
                    ed_pool.pool_type = 0;
                }
            }
        }

        if(ed_pool.pool_type == 0)
        {
            ed_pool.nCanCompose = nCanCompose;
        }

        if(ed_pool.share_seat.size() >=1)
        {
            //边池如有玩家参与分享，将该边池存入 边池队列容器
            //计算池子的pool_id    0为主池的id  大于0为各边池的id
            if(  (dealers_vector.at(room_number))->edge_pool_list.size() <= 0 )
            {
                ed_pool.pool_id = 0;
                (dealers_vector.at(room_number))->edge_pool_list.push_back(ed_pool);
                edge_pool_temp.push_back(ed_pool);
            }
            else
            {
                //检查池子的种类，分享人数相同的 为需要合并的池子，pool_id相同
                int find_same_pool = 0;
                for(unsigned int r=0; r< (dealers_vector.at(room_number))->edge_pool_list.size(); r++  )
                {
                    int sizea = ed_pool.share_seat.size();
                    int sizeb = (dealers_vector.at(room_number))->edge_pool_list.at(r).share_seat.size();

                    if(  sizea  == sizeb  )
                    {
                        (dealers_vector.at(room_number))->edge_pool_list.at(r).nCanCompose = ed_pool.nCanCompose;
                        ed_pool.pool_id = (dealers_vector.at(room_number))->edge_pool_list.at(r).pool_id;

                        (dealers_vector.at(room_number))->edge_pool_list.at(r).edge_height +=ed_pool.edge_height;

                        edge_pool ed_pool_tmp;
                        ed_pool_tmp = ed_pool;

                        int temp_f = 0;
                        for(unsigned int t=0; t < (dealers_vector.at(room_number))->edge_pool_list.at(r).share_seat.size(); t++)
                        {
                            if( (dealers_vector.at(room_number))->edge_pool_list.at(r).share_seat.at(t) < 0 )
                            {
                                temp_f += (dealers_vector.at(room_number))->edge_pool_list.at(r).share_seat.at(t);
                            }
                        }

                        for(unsigned int h=0; h < ed_pool_tmp.share_seat.size(); h++)
                        {
                            if( ed_pool_tmp.share_seat.at(h) < 0 )
                            {
                                ed_pool_tmp.share_seat.at(h)  += temp_f;
                                h = ed_pool_tmp.share_seat.size() -1;
                            }
                        }

                        (dealers_vector.at(room_number))->edge_pool_list.at(r).share_seat = ed_pool_tmp.share_seat;

                        find_same_pool ++ ;
                        r =  (dealers_vector.at(room_number))->edge_pool_list.size()-1;
                    }

                }

                //如果是边池，看一下是否需要合并
                if(ed_pool.pool_type == 0 && find_same_pool == 0)
                {
                    //if(ed_pool.nCanCompose == 1)
                    if(1)
                    {
                        int r = (dealers_vector.at(room_number))->edge_pool_list.size() - 1;
                        if((dealers_vector.at(room_number))->edge_pool_list[r].pool_type == 0  && (dealers_vector.at(room_number))->edge_pool_list[r].nCanCompose == 1)
                        {
                            int sizea = ed_pool.share_seat.size();
                            int sizeb = (dealers_vector.at(room_number))->edge_pool_list.at(r).share_seat.size();
                            if(sizeb > sizea)
                            {
                                for(int k=0; k<sizeb- sizea; k++)
                                {
                                    ed_pool.share_seat.push_back(-ed_pool.edge_height);
                                }
                            }
                            sizea = ed_pool.share_seat.size();
                            if(  sizea  == sizeb  )
                            {
                                (dealers_vector.at(room_number))->edge_pool_list.at(r).nCanCompose = ed_pool.nCanCompose;
                                ed_pool.pool_id = (dealers_vector.at(room_number))->edge_pool_list.at(r).pool_id;

                                (dealers_vector.at(room_number))->edge_pool_list.at(r).edge_height +=ed_pool.edge_height;

                                edge_pool ed_pool_tmp;
                                ed_pool_tmp = ed_pool;

                                int temp_f = 0;
                                for(unsigned int t=0; t < (dealers_vector.at(room_number))->edge_pool_list.at(r).share_seat.size(); t++)
                                {
                                    if( (dealers_vector.at(room_number))->edge_pool_list.at(r).share_seat.at(t) < 0 )
                                    {
                                        temp_f += (dealers_vector.at(room_number))->edge_pool_list.at(r).share_seat.at(t);
                                    }
                                }

                                for(unsigned int h=0; h < ed_pool_tmp.share_seat.size(); h++)
                                {
                                    if( ed_pool_tmp.share_seat.at(h) < 0 )
                                    {
                                        ed_pool_tmp.share_seat.at(h)  += temp_f;
                                        h = ed_pool_tmp.share_seat.size() -1;
                                    }
                                }

                                (dealers_vector.at(room_number))->edge_pool_list.at(r).share_seat = ed_pool_tmp.share_seat;

                                find_same_pool ++ ;
                                r =  (dealers_vector.at(room_number))->edge_pool_list.size()-1;
                            }
                        }
                    }
                }

                if( find_same_pool == 0)
                {
                    //没有找到的话
                    int k = (dealers_vector.at(room_number))->edge_pool_list.size();
                    ed_pool.pool_id = (dealers_vector.at(room_number))->edge_pool_list.at(k-1).pool_id +1;
                    (dealers_vector.at(room_number))->edge_pool_list.push_back(ed_pool);

                }
                edge_pool_temp.push_back(ed_pool);
            }

            sprintf(szLogTemp,":: edge_poll type[%d] height[%d] ",ed_pool.pool_type,ed_pool.edge_height);
            memcpy(szLog+strlen(szLog),szLogTemp,strlen(szLogTemp));
            for(unsigned int i =0; i< ed_pool.share_seat.size(); i++)
            {
                sprintf(szLogTemp,"mem[%d]",ed_pool.share_seat.at(i));
                memcpy(szLog+strlen(szLog),szLogTemp,strlen(szLogTemp));
            }
            sprintf(szLogTemp,"size[%d] :: ",(int)ed_pool.share_seat.size());
            memcpy(szLog+strlen(szLog),szLogTemp,strlen(szLogTemp));
        }

        if( max_desk_chip <= 0 )
        {
            WriteRoomLog(4001,dealers_vector[room_number]->dealer_id,0,"%s\n",szLog);
            break;
        }
    }
}


/*
*  边池计算器
*/
void  edge_pool_computer(vector<dealer *>&  dealers_vector,int room_number,vector<edge_pool>& edge_pool_temp )
{
    edge_pool_temp.clear();
    //当玩家丢在桌面上的筹码最大时，异常弃牌
    int max_desk_chip =0 ;
    int user_status=0;

    for(unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++)
    {
        if( (((dealers_vector.at(room_number))->players_list).at(i)).user_staus >= 2  )  //在牌局中的玩家
        {
            if(  (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip > max_desk_chip )
            {
                max_desk_chip  =  (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip;
                user_status = (((dealers_vector.at(room_number))->players_list).at(i)).user_staus;
            }
        }
    }

    for(unsigned int i=0; i<((dealers_vector.at(room_number))->leave_players_list).size(); i++)
    {
        if( (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus >= 2  )  //在牌局中的玩家
        {
            if(  (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip > max_desk_chip )
            {
                max_desk_chip  =  (((dealers_vector.at(room_number))->leave_players_list).at(i)).desk_chip;
                user_status = (((dealers_vector.at(room_number))->leave_players_list).at(i)).user_staus;
            }
        }
    }

    if( max_desk_chip >0  &&  user_status == 5 )
    {
        edge_pool_special( dealers_vector,room_number,edge_pool_temp );


    }
    else
    {
        edge_pool_normal( dealers_vector,room_number,edge_pool_temp );
    }
}



/*
*
* 手牌排序
*/
//从小到大排序
void sort_cards(vector<int>& card_temp)
{
    int temp = 0,size = card_temp.size();
    for(int i = 0; i < size; ++i)
    {
        for(int j = i+1; j < size; ++j)
        {
            if(card_temp.at(i)%100 > card_temp.at(j)%100)  //去掉花色比大小
            {
                temp = card_temp.at(j);
                card_temp.at(j) = card_temp.at(i);
                card_temp.at(i) = temp;
            }
        }
    }
}

//从大到小排序
void sort_cardsMax(vector<int>& card_temp )
{
    int temp = 0,size = card_temp.size();
    for(int i = 0; i < size; ++i)
    {
        for(int j = i+1; j < size; ++j)
        {
            if(card_temp.at(i)%100 < card_temp.at(j)%100)  //去掉花色比大小
            {
                temp = card_temp.at(j);
                card_temp.at(j) = card_temp.at(i);
                card_temp.at(i) = temp;
            }
        }
    }
}

/*
*同花判断，放入win_cards
* return 同花的花色  1 2 3 4
*
*/
int judge_flush( vector<int>& card_temp,vector<int>& win_cards )
{

    int color = 0,size = card_temp.size();
    //本次循环，只需要计算出那个花色
    for(int i=0; i<size; i++ )
    {
        int count =0;
        for(int j=0; j<size; j++  )
        {
            if( (card_temp.at(j)/100) == (card_temp.at(i)/100) )
            {
                count++;
            }
        }

        if( count >= 5)
        {
            color =card_temp.at(i)/100;
            break;
        }
    }

    //将同花的牌都存入win_cards
    if( color !=0)
    {
        for(int i=0; i<size; i++ )
        {
            if( card_temp.at(i)/100 == color)
            {
                win_cards.push_back( card_temp.at(i) );
            }
        }
    }
    return color;
}

/**
 * 判断传入的vector是否是顺子,如果是顺子返回非0;如果不是顺子，返回为0
 **/
int judge_straight_by3(vector<int>& card_temp,vector<int>& win_cards)
{
    unsigned int v_size = card_temp.size();
    //已经从小到大排好序,只要判断前三张是否有顺子
    for(unsigned int i = 0; i<3; i++)
    {
        int totalNum = 1;
        int firstNum = card_temp.at(i)%100;
        vector<int> shunzi;
        shunzi.push_back(card_temp.at(i));
        for(unsigned int j = i+1; j<v_size; j++)
        {
            if(card_temp.at(j)%100 == firstNum+totalNum)
            {
                totalNum++;
                shunzi.push_back(card_temp.at(j));
            }
        }
        //如果等于4位且第一位是2，看看有没有A  增加(A 2 3 4 5也是顺子)
        if( shunzi.size() == 4 && shunzi.at(0)%100 == 2 )
        {
            for(unsigned int t=0; t< v_size; t++)
            {
                if( card_temp.at(t)%100  == 14 )
                {
                    shunzi.push_back( card_temp.at(t) );
                    int tempc = shunzi.at(4);
                    shunzi.at(4) = shunzi.at(3);
                    shunzi.at(3) = shunzi.at(2);
                    shunzi.at(2) = shunzi.at(1);
                    shunzi.at(1) = shunzi.at(0);
                    shunzi.at(0) =tempc;
                    //如果有值，再进行清除，添加
                    win_cards.clear();
                    win_cards = shunzi;
                    return 1;
                }
            }
        }
        else if(shunzi.size() > 4)
        {
            if(shunzi.size() == 7)
            {
                shunzi.erase(shunzi.begin());
                shunzi.erase(shunzi.begin());
            }
            else if(shunzi.size() == 6)
            {
                shunzi.erase(shunzi.begin());
            }
            //如果有值，再进行清除，添加
            win_cards.clear();
            win_cards = shunzi;
            return 1;
        }
    }
    return 0;
}

/*
*顺子判断，放入win_cards
*return  非0:是顺子  0:不是顺子
*
*/
int judge_straight( vector<int>& card_temp,vector<int>& win_cards )
{
    //同花当中可能有5、6、7张牌，各自依据条件判断
    if(win_cards.size() > 4)
    {
        // 同花的5张中判断顺子
        vector<int> win_cardsTemp;
        win_cardsTemp = win_cards;
        return judge_straight_by3(win_cardsTemp,win_cards);
    }
    else
    {
        return judge_straight_by3(card_temp,win_cards);
    }

    return 0;
}

/*
*四条判断，放入win_cards
*return  非0:是四条(四条的牌值)  0:不是四条
*
*/
int judge_4ofking( vector<int>& card_temp,vector<int>& win_cards )
{
    int fours=0;
    for(unsigned int i=0; i<card_temp.size(); i++)
    {
        int cont =1;
        for(unsigned int j=i+1; j<card_temp.size(); j++)
        {
            if( card_temp.at(i)%100 == card_temp.at(j)%100 )
            {
                cont++;
            }
        }
        if(cont == 4)
        {
            //放入之前，首先清空
            win_cards.clear();
            fours = card_temp.at(i)%100;
            win_cards.push_back(100+fours);
            win_cards.push_back(200+fours);
            win_cards.push_back(300+fours);
            win_cards.push_back(400+fours);

            if( card_temp.at(6)%100 != fours)
            {
                win_cards.push_back(card_temp.at(6));
            }
            else if ( card_temp.at(6)%100 == fours)
            {
                //当6位子有相同的4张牌的时候，第0，1肯定是不想同的
                win_cards.push_back(card_temp.at(2));
            }
            //跳出循环
            i = 100;
        }
    }
    return fours;
}


/*
*三条判断，放入win_cards
* return  100:三条 200:葫芦(三条加一对)  0:没有三条
*
*/
int judge_3ofking( vector<int>& card_temp,vector<int>& win_cards )
{
    int ret = 0,three1 = 0,three2 = 0,size = card_temp.size();
    //判断是否有3条//始终从当前位置向后查找
    for(int i = 0; i < size; ++i)
    {
        int cont = 1,value = card_temp.at(i)%100;
        for(int j = i+1; j< size; ++j)
        {
            if( value == card_temp.at(j)%100 )
            {
                ++cont;
            }
        }
        //压入3条的牌值信息
        if(3 == cont)
        {
            win_cards.push_back(card_temp.at(i));
            if(0 == three1)
            {
                three1 = value;
                ret = 100;
            }
            else
            {
                //2个3条
                three2 = value;
                ret = 200;
            }
            for(int j = i+1; j < size; ++j)
            {
                if( value == card_temp.at(j)%100 )
                    win_cards.push_back(card_temp.at(j));
            }
        }
    }
    //没有3条
    if(0 == ret)
        return 0;
    else if(200 == ret)//2个3条
    {
        //把从小到大排序改成从大到小排序
        win_cards.push_back(win_cards.at(0));
        win_cards.push_back(win_cards.at(1));
        win_cards.erase(win_cards.begin());
        win_cards.erase(win_cards.begin());
        win_cards.erase(win_cards.begin());
        return 200;
    }
    //判断是否有对子//始终从当前位置向后查找
    for(int i = 0; i < size; ++i)
    {
        int cont = 1,j = i+1,value = card_temp.at(i)%100;
        //对子的牌值不与3条相同
        if(value == three1 || value == three2)
            continue;
        for(; j < size; ++j)
        {
            if( card_temp.at(j)%100 == value )
            {
                ++cont;
                break;
            }
        }
        //压入对子的牌值信息
        if(2 == cont)
        {
            win_cards.push_back(card_temp.at(i));
            win_cards.push_back(card_temp.at(j));
            ret = 200;
        }
    }
    //有对子
    if(200 == ret)
    {
        if(win_cards.size() == 7)
        {
            //移除小对子
            win_cards.erase(win_cards.begin()+3);
            win_cards.erase(win_cards.begin()+3);
        }
        return 200;
    }
    //没有对子
    if(three1 != card_temp.at(5)%100)
    {
        //第5张牌不等的话,后续牌肯定也不等
        win_cards.push_back(card_temp.at(6));
        win_cards.push_back(card_temp.at(5));
    }
    else
    {
        if(three1 != card_temp.at(6)%100)
            win_cards.push_back(card_temp.at(6));
        else
            win_cards.push_back(card_temp.at(3));
        win_cards.push_back(card_temp.at(2));
    }
    return 100;
}

/*
*两对判断，放入win_cards
* return  非0:两对  0:不是两对
*
*/
int judge_2ofdouble( vector<int>& card_temp,vector<int>& win_cards )
{
    int lpPair[3],pairs = 0,size = card_temp.size();
    //对子判断//始终从当前位置向后查找
    for(int i = 0; i < size; ++i)
    {
        int j = i+1,value = card_temp.at(i)%100;
        for(; j < size; ++j)
        {
            if(value == card_temp.at(j)%100)
            {
                lpPair[pairs] = value;
                ++pairs;
                win_cards.push_back(card_temp.at(i));
                win_cards.push_back(card_temp.at(j));
                break;
            }
        }
    }
    //少于两对
    if(pairs < 2)
    {
        win_cards.clear();
        return 0;
    }
    //查找最大单牌
    int single = 0,value = 0;
    for(int i = 0; i < size; ++i)
    {
        value = card_temp.at(i)%100;
        if(value == lpPair[0] || value == lpPair[1] || value == lpPair[2])
            continue;
        single = card_temp.at(i);
    }
    //有三对
    if(win_cards.size() == 6)
    {
        if(single%100 < win_cards.at(0)%100)
            single = win_cards.at(0);
        //三对//把从小到大排序改成从大到小排序
        win_cards.erase(win_cards.begin());
        win_cards.erase(win_cards.begin());
        win_cards.push_back(win_cards.at(0));
        win_cards.push_back(win_cards.at(1));
        win_cards.erase(win_cards.begin());
        win_cards.erase(win_cards.begin());
    }
    else
    {
        //两对//把从小到大排序改成从大到小排序
        win_cards.push_back(win_cards.at(0));
        win_cards.push_back(win_cards.at(1));
        win_cards.erase(win_cards.begin());
        win_cards.erase(win_cards.begin());
    }
    //加入单牌
    win_cards.push_back(single);
    return 1;
}

/*
*一对判断，放入win_cards
* return  非0:对子  0:不是一对
*
*/
int judge_1ofdouble( vector<int>& card_temp,vector<int>& win_cards )
{
    int pair = 0,value = 0,size = card_temp.size();
    //判断对子
    for(int i = 0; i < size; ++i)
    {
        value = card_temp.at(i)%100;
        for(int j = i+1; j < size; ++j)
        {
            if(value == card_temp.at(j)%100)
            {
                pair = 1;
                win_cards.push_back(card_temp.at(i));
                win_cards.push_back(card_temp.at(j));
                break;
            }
        }
        if(1 == pair)
            break;
    }
    if(0 == pair)
        return 0;
    //单牌
    for(int i = size-1; i >= 0; --i)
    {
        if(value != card_temp.at(i)%100)
        {
            win_cards.push_back(card_temp.at(i));
            if(5 == win_cards.size())
                break;
        }
    }
    return 1;
}

/*
*   计算手牌的最大虚拟值
*   desk_card 公共牌
*   hand_cards 手牌
*   win_cards  最大的组合
*   返回  虚拟后的牌值  同花顺 四条 葫芦 ..等等每一个牌型代表的一级数值
*/
void virtual_card_value( int desk_card[5], vector<int>& hand_cards,vector<int>& win_cards)
{
    //合并公共牌和手牌
    vector<int> card_temp;
    for( unsigned int i=0; i<5; i++)
    {
        card_temp.push_back(desk_card[i]);
    }
    card_temp.push_back(hand_cards.at(0));
    card_temp.push_back(hand_cards.at(1));

    //测试打印原始牌的信息
    WriteLog(55555,"virtual_card_value init card_temp:[%d] [%d] [%d] [%d] [%d] [%d] [%d] \n",card_temp[0],card_temp[1],card_temp[2],card_temp[3],card_temp[4],card_temp[5],card_temp[6]);


    //从小到大排序
    sort_cards(card_temp);

    //测试打印原始牌从小到大排序后的信息
    WriteLog(55555,"virtual_card_value sort_cards card_temp:[%d] [%d] [%d] [%d] [%d] [%d] [%d] \n",card_temp[0],card_temp[1],card_temp[2],card_temp[3],card_temp[4],card_temp[5],card_temp[6]);

    vector<int> card_tempLog = card_temp;
    //先判断是否是同花
    if( judge_flush( card_temp,win_cards ) >0 )
    {
        //判断是否是顺子
        if( judge_straight( card_temp,win_cards ) >0 )
        {
            //从大到小排序
            sort_cardsMax(win_cards);
            //如果顺子是A2345，将第二位作为最大值
            if( win_cards.at(0)%100 == 14 && win_cards.at(1)%100 == 5 && win_cards.at(2)%100 == 4)
            {
                win_cards.push_back( win_cards.at(1)%100 + 900 );

            }
            else
            {
                //同花顺 皇家同花顺914 //比牌时只需比较最大值就可确定大小和相等
                win_cards.push_back( win_cards.at(0)%100 + 900 );
            }

        }
        else
        {
            //同花
            if(win_cards.size() == 6)
            {
                win_cards.erase(win_cards.begin());
            }
            else if( win_cards.size() == 7)
            {
                win_cards.erase(win_cards.begin());
                win_cards.erase(win_cards.begin());
            }
            //从大到小排序
            sort_cardsMax(win_cards);
            win_cards.push_back(win_cards.at(0)%100+600);
        }
    }
    else
    {
        //非同花
        //判断是否是顺子
        if( judge_straight( card_temp,win_cards ) >0 )
        {
            //从大到小排序
            sort_cardsMax(win_cards);
            //如果顺子是A2345，将第二位作为最大值
            if( win_cards.at(0)%100 == 14 && win_cards.at(1)%100 == 5 && win_cards.at(2)%100 == 4)
            {
                win_cards.push_back( win_cards.at(1)%100 + 500 );

            }
            else
            {
                //顺子 //比牌时只需比较最大值就可确定大小和相等
                win_cards.push_back( win_cards.at(0)%100 + 500 );
            }
        }
        else
        {
            if( judge_4ofking( card_temp,win_cards ) >0 )
            {
                //四条
                win_cards.push_back( win_cards.at(0)%100 + 800 );
            }
            else
            {
                int judg3 = judge_3ofking( card_temp,win_cards );
                if(100 == judg3)
                {
                    //三条
                    win_cards.push_back( win_cards.at(0)%100 + 400 );
                }
                else if(200 == judg3)
                {
                    //葫芦(三条加一对)
                    win_cards.push_back( win_cards.at(0)%100 + 700 );
                }
                else
                {
                    if(judge_2ofdouble( card_temp,win_cards) > 0)
                    {
                        //两对
                        win_cards.push_back( win_cards.at(0)%100 + 300 );
                    }
                    else if(judge_1ofdouble( card_temp,win_cards ) > 0)
                    {
                        //对子
                        win_cards.push_back( win_cards.at(0)%100 + 200 );
                    }
                    else
                    {
                        //高牌
                        win_cards = card_temp;
                        win_cards.erase(win_cards.begin());
                        win_cards.erase(win_cards.begin());
                        //从大到小排序
                        sort_cardsMax(win_cards);
                        win_cards.push_back( win_cards.at(0)%100 + 100 );
                    }
                }
            }
        }
    }

    //测试打印原始牌的牌型信息
    WriteLog(55555,"virtual_card_value win_cards:[%d] [%d] [%d] [%d] [%d] [%d] \n",win_cards[0],win_cards[1],win_cards[2],win_cards[3],win_cards[4],win_cards[5]);

    if(win_cards.size() < 6)
    {
        //牌个数错误
        WriteLog(40001,"win_cards size error:[%d]  \n",win_cards.size());
        for(int i=0; i<(int)win_cards.size(); i++)
        {
            WriteLog(40001,"win_cards:[%d]\n",win_cards[i]);
        }
        for(int i=0; i<(int)card_tempLog.size(); i++)
        {
            WriteLog(40001,"card_temp:[%d]\n",card_tempLog[i]);
        }

        for(int i=0; i<6; i++)
        {
            win_cards.push_back(0);
        }
    }
}

/*
* 计算牌面虚拟值   将每级牌型的基数加上
*/
void computer_card_value(vector<dealer *>&  dealers_vector,int room_number)
{
    for( unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++ )
    {
        //计算牌面虚拟值
        //只有弃牌的玩家不参与虚拟值计算
        user &userPlayer = (dealers_vector.at(room_number))->players_list.at(i);
        if( userPlayer.user_staus==2 || userPlayer.user_staus==3 || userPlayer.user_staus==4 || userPlayer.user_staus==6 )
        {

            userPlayer.win_cards.clear();
            virtual_card_value((dealers_vector.at(room_number))->desk_card,userPlayer.hand_cards,userPlayer.win_cards);

        }
        else if( userPlayer.user_staus==5 )
        {
            //弃牌的玩家虚拟值为零0 牌型为公共牌
            userPlayer.win_cards.clear();
            userPlayer.win_cards.push_back( (dealers_vector.at(room_number))->desk_card[0]);
            userPlayer.win_cards.push_back( (dealers_vector.at(room_number))->desk_card[1]);
            userPlayer.win_cards.push_back( (dealers_vector.at(room_number))->desk_card[2]);
            userPlayer.win_cards.push_back( (dealers_vector.at(room_number))->desk_card[3]);
            userPlayer.win_cards.push_back( (dealers_vector.at(room_number))->desk_card[4]);
            (dealers_vector.at(room_number))->players_list.at(i).win_cards.push_back(0);
        }
        else
        {
            userPlayer.win_cards.clear();
            userPlayer.win_cards.push_back( (dealers_vector.at(room_number))->desk_card[0]);
            userPlayer.win_cards.push_back( (dealers_vector.at(room_number))->desk_card[1]);
            userPlayer.win_cards.push_back( (dealers_vector.at(room_number))->desk_card[2]);
            userPlayer.win_cards.push_back( (dealers_vector.at(room_number))->desk_card[3]);
            userPlayer.win_cards.push_back( (dealers_vector.at(room_number))->desk_card[4]);
            (dealers_vector.at(room_number))->players_list.at(i).win_cards.push_back(0);
        }

    }
}



/*
* 计算赢家赢回的注金、及牌型
*/
void computer_winer_card(vector<dealer *>&  dealers_vector,int room_number,vector<winer_pool>&  winder_list)
{

    //合并边池列表中的主池
    vector<edge_pool>::iterator iter;
    //计算所有主池
    unsigned int pool_area=0;
    for(iter= (dealers_vector.at(room_number))->edge_pool_list.begin(); iter!= (dealers_vector.at(room_number))->edge_pool_list.end(); iter++  )
    {
        edge_pool  pool_m = (*iter);
        if(pool_m.pool_type ==1 )
        {
            pool_area = pool_area+pool_m.edge_height * pool_m.share_seat.size() ;
        }
    }

    edge_pool  pool_tmp;

    //将高度累计到last主池上
    for(unsigned int i= 0; i< (dealers_vector.at(room_number))->edge_pool_list.size(); i++ )
    {
        if( (dealers_vector.at(room_number))->edge_pool_list.at(i).pool_type ==1 )
        {
            pool_tmp=(dealers_vector.at(room_number))->edge_pool_list.at(i);
        }
    }


    if(pool_tmp.share_seat.size()!=0)
    {
        pool_tmp.edge_height =  pool_area / pool_tmp.share_seat.size();
    }
    else
    {
        pool_tmp.edge_height = 0;
    }


    //删除掉主池
    for(iter= (dealers_vector.at(room_number))->edge_pool_list.begin(); iter!= (dealers_vector.at(room_number))->edge_pool_list.end(); iter++  )
    {
        edge_pool  pool_m = (*iter);
        if(pool_m.pool_type ==1)
        {
            (dealers_vector.at(room_number))->edge_pool_list.erase(iter);
            iter--;
        }
    }


    (dealers_vector.at(room_number))->edge_pool_list.push_back(pool_tmp);

    edge_pool edge;
    vector<winer_pool> winder_temp;
    for( unsigned int j=0; j<((dealers_vector.at(room_number))->edge_pool_list).size(); j++  )
    {
        edge = ((dealers_vector.at(room_number))->edge_pool_list).at(j);//取第一个边池

        int max_value=0;
        int seat_number=0; //旧的最大座位号
        int seat_old=0;//预找的虚拟值最大的座位号
        vector<int> seat_same;
        //先找到虚拟值最大的座位号
        for( unsigned int i=0; i<edge.share_seat.size(); i++  )
        {
            if( edge.share_seat.at(i) >0)
            {
                for( unsigned int k=0; k<((dealers_vector.at(room_number))->players_list).size(); k++ )
                {
                    if( ((dealers_vector.at(room_number))->players_list).at(k).seat_num == edge.share_seat.at(i) )
                    {
                        if(  ((dealers_vector.at(room_number))->players_list).at(k).win_cards.at(5) > max_value )
                        {
                            max_value = ((dealers_vector.at(room_number))->players_list).at(k).win_cards.at(5);
                            seat_number=((dealers_vector.at(room_number))->players_list).at(k).seat_num;
                        }
                    }
                }
            }
        }

        seat_old = seat_number;


        for( unsigned int i=0; i<edge.share_seat.size(); i++  )
        {
            //再找到虚拟值相同的 进行深度比较大小
            if( edge.share_seat.at(i) >0 && edge.share_seat.at(i) != seat_number )
            {
                vector<int> win_cards_temp;
                vector<int> win_cards_temp2;
                for( unsigned int k=0; k<((dealers_vector.at(room_number))->players_list).size(); k++ )
                {
                    if( ((dealers_vector.at(room_number))->players_list).at(k).seat_num == edge.share_seat.at(i) )
                    {
                        if(  ((dealers_vector.at(room_number))->players_list).at(k).win_cards.at(5) == max_value )
                        {
                            //虚拟值相等
                            win_cards_temp = ((dealers_vector.at(room_number))->players_list).at(k).win_cards;
                            //深度比较
                            for( unsigned int n=0; n<((dealers_vector.at(room_number))->players_list).size(); n++ )
                            {
                                if( ((dealers_vector.at(room_number))->players_list).at(n).seat_num == seat_number )
                                {
                                    win_cards_temp2 = ((dealers_vector.at(room_number))->players_list).at(n).win_cards;
                                    int equal_count=0;

#if 1
                                    /*
                                    for(unsigned int m=1;m<5;m++)
                                    {
                                        WriteLog(4008," %d<>%d |  %d<>%d  \n",win_cards_temp.at(m),win_cards_temp2.at(m),win_cards_temp.at(m)%100,win_cards_temp2.at(m)%100);
                                    }
                                    */
                                    for(unsigned int m=1; m<5; m++)
                                    {
                                        if( win_cards_temp.at(m)%100 > win_cards_temp2.at(m)%100 )
                                        {
                                            seat_number= ((dealers_vector.at(room_number))->players_list).at(k).seat_num;
                                            seat_same.clear();
                                            m = 4;
                                            n= ((dealers_vector.at(room_number))->players_list).size() -1;
                                        }
                                        else  if( win_cards_temp.at(m)%100 == win_cards_temp2.at(m)%100  )  //两个牌的大小相等
                                        {
                                            equal_count++;
                                        }
                                        else  if(  win_cards_temp.at(m)%100 < win_cards_temp2.at(m)%100 )
                                        {
                                            //seat_same.clear();
                                            m = 4;
                                            n= ((dealers_vector.at(room_number))->players_list).size() -1;
                                        }
                                    }
#else
#endif


                                    if( equal_count ==4  ) //两个人牌的大小完全相等
                                    {
                                        seat_same.push_back( ((dealers_vector.at(room_number))->players_list).at(k).seat_num );
                                    }

                                }
                            }
                        }
                    }
                }
            }
        }
        winer_pool  winer;
        if(seat_number != seat_old )
        {
            if(seat_same.size() !=0)
            {
                for(unsigned int i=0; i<seat_same.size(); i++)
                {
                    winer.pool_id_list.clear();
                    winer.persent_list.clear();
                    winer.seat_number=seat_same.at(i);
                    winer.persent_list.push_back( seat_same.size() +1 );
                    (winer.pool_id_list).push_back(j);
                    winder_temp.push_back(winer);
                }
            }
            winer.pool_id_list.clear();
            winer.persent_list.clear();
            winer.seat_number=seat_number;
            winer.persent_list.push_back( seat_same.size() +1 );
            (winer.pool_id_list).push_back(edge.pool_id);
            winder_temp.push_back(winer);
        }
        else if(seat_number == seat_old)
        {
            if(seat_same.size() !=0)
            {
                for(unsigned int i=0; i<seat_same.size(); i++)
                {
                    winer.pool_id_list.clear();
                    winer.persent_list.clear();
                    winer.seat_number=seat_same.at(i);
                    winer.persent_list.push_back( seat_same.size() +1 );
                    (winer.pool_id_list).push_back(edge.pool_id);
                    winder_temp.push_back(winer);
                }
            }
            winer.pool_id_list.clear();
            winer.persent_list.clear();
            winer.seat_number=seat_number;
            winer.persent_list.push_back( seat_same.size() +1 );
            (winer.pool_id_list).push_back(edge.pool_id);
            winder_temp.push_back(winer);
        }
    }

    //合并同一个玩家的所有边池
    for(unsigned int m=0; m<winder_temp.size(); m++)
    {
        winer_pool winer = winder_temp.at(m);

        int cout_same_seat=0;
        for(unsigned int i=0; i<winder_list.size(); i++)
        {
            if( winder_list.at(i).seat_number ==winer.seat_number )
            {
                cout_same_seat++;
            }
        }

        if( cout_same_seat >0)
        {
            for(unsigned int i=0; i<winder_list.size(); i++)
            {
                if( winder_list.at(i).seat_number ==winer.seat_number )
                {
                    winder_list.at(i).pool_id_list.push_back((winer.pool_id_list).at(0));
                    winder_list.at(i).persent_list.push_back((winer.persent_list).at(0));
                }
            }
        }
        else if (cout_same_seat == 0)
        {
            winder_list.push_back(winer);
        }
    }


    //计算玩家赢得边池的具体筹码，更新手上的筹码数
    for(unsigned int i=0; i<winder_list.size(); i++)
    {
        int win_chips=0;
        for(unsigned int k=0; k< winder_list.at(i).pool_id_list.size(); k++)
        {
            int pool_id = (winder_list.at(i).pool_id_list).at(k);//取到第一个边池号
            int persent = (winder_list.at(i).persent_list).at(k);
            int pool_total=0;
            for( unsigned int p=0; p< (dealers_vector.at(room_number))->edge_pool_list.size(); p++ )
            {
                if(pool_id == (dealers_vector.at(room_number))->edge_pool_list.at(p).pool_id  )
                {
                    pool_total= ((dealers_vector.at(room_number))->edge_pool_list).at(p).share_seat.size() * ((dealers_vector.at(room_number))->edge_pool_list).at(p).edge_height;//边池高度 * 参与人数
                    for(unsigned int m=0; m< ((dealers_vector.at(room_number))->edge_pool_list).at(p).share_seat.size(); m++)
                    {
                        if( ((dealers_vector.at(room_number))->edge_pool_list).at(p).share_seat.at(m) <=0  )
                        {
                            pool_total = pool_total + ((dealers_vector.at(room_number))->edge_pool_list).at(p).share_seat.at(m);
                        }
                    }
                }
            }
            pool_total= pool_total / persent;
            win_chips= win_chips + pool_total;
        }

        //找到该玩家
        for(unsigned int j=0; j<((dealers_vector.at(room_number))->players_list).size(); j++)
        {
            if( ((dealers_vector.at(room_number))->players_list).at(j).seat_num == winder_list.at(i).seat_number )
            {
                ((dealers_vector.at(room_number))->players_list).at(j).hand_chips +=  win_chips;
            }
        }
    }

}



/*
*  牌面最大玩家的座位号
*/
int max_card_seat( vector<int>& share_seat,vector<dealer *>&  dealers_vector ,int room_number)
{
    int seat_num=0;
    for( unsigned int i=0; i<share_seat.size(); i++ )
    {
        for( unsigned int j=0; j<((dealers_vector.at(room_number))->players_list).size(); j++  )
        {
            if( share_seat.at(i) ==  ((dealers_vector.at(room_number))->players_list).at(j).seat_num )
            {


            }

        }

    }

    return seat_num;

}


/*
* 处理下一步
*
**/
void do_next_work( int nUserID,vector<dealer *>& dealers_vector,int room_number,string user_account,int chips_value,int seat_number,ConnPool *connpool,vector<level_system>  level_list ,DZPK_USER_T *pSocketUser,int nType)
{
    //查找下一个玩家的座位号
    int next_seat = next_player((dealers_vector.at(room_number))->turn,dealers_vector,room_number);

    //如果找到的下家加满注，且仅只有这一家，则直接比牌,或者强制游戏结束
    if( next_seat == 0 || nType == DEAL_RESULT_TYPE_FORCELEAVE)
    {
        //如果没有找到下一个玩家

        (dealers_vector.at(room_number))->look_flag = 0;
        int count=0; //剩余玩家人数
        for(unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++)
        {
            //计算剩余玩家人数不包含弃牌玩家
            if( ((dealers_vector.at(room_number))->players_list).at(i).user_staus == 2 || ((dealers_vector.at(room_number))->players_list).at(i).user_staus == 3 || ((dealers_vector.at(room_number))->players_list).at(i).user_staus == 4 || ((dealers_vector.at(room_number))->players_list).at(i).user_staus == 6  )
            {
                count++;
            }
        }
        if(count == 1) //只剩一个玩家，直接发赢牌消息
        {
            //隐藏光圈
            StdStopHideLightRing(dealers_vector[room_number]);

            //今天玩牌次数加一
            StdTodayPlayTimes(dealers_vector.at(room_number));

            //幸运手牌活动
            StdLuckyCardActivity(dealers_vector[room_number],2);

            edge_pool eg_pool;
            eg_pool.edge_height=0;
            vector<edge_pool> edge_pool_temp;
            edge_pool_temp.push_back(eg_pool);
            edge_pool_computer(dealers_vector,room_number,edge_pool_temp);

            //向房间内的所有玩家发送赢牌消息
            Json::FastWriter  writer;
            Json::Value  msg_body;
            Json::Value  edge_user_list;
            Json::Value  hand_cards_all;
            Json::Value  chip_account_list;
            Json::Value  experience_change_list;
            Json::Value  edge_desk_pool;
            Json::Value  pool_st;

            msg_body.clear();
            msg_body["msg_head"]="dzpk";
            msg_body["game_number"]="dzpk";
            msg_body["area_number"]="normal";
            msg_body["act_commond"]=CMD_WIN_LIST;
            msg_body["next_turn"]=0;
            //手牌-座位号对应列表
            for(  unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++ )
            {
                if( ((dealers_vector.at(room_number))->players_list).at(i).user_staus == 2 ||((dealers_vector.at(room_number))->players_list).at(i).user_staus == 3 || ((dealers_vector.at(room_number))->players_list).at(i).user_staus == 4 || ((dealers_vector.at(room_number))->players_list).at(i).user_staus == 6)
                {
                    Json::Value  hand_cards;
                    Json::Value  card;
                    hand_cards["seat_number"]= ((dealers_vector.at(room_number))->players_list).at(i).seat_num;
                    card.append( ((dealers_vector.at(room_number))->players_list).at(i).hand_cards.at(0) );
                    card.append( ((dealers_vector.at(room_number))->players_list).at(i).hand_cards.at(1) );
                    hand_cards["hand_card"]= card;
                    hand_cards_all.append(hand_cards);

                    g_pMemData->updateGameNum((dealers_vector.at(room_number))->dealer_id,
                                        (((dealers_vector.at(room_number))->players_list).at(i)).nUserID,
                                        (dealers_vector.at(room_number))->small_blind,
                                        (dealers_vector.at(room_number))->current_player_num, 1);
                }
            }

            msg_body["hand_cards_all"]=hand_cards_all;

            for(  unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++ )
            {
                //
                if( ((dealers_vector.at(room_number))->players_list).at(i).user_staus == 2 || ((dealers_vector.at(room_number))->players_list).at(i).user_staus == 3  || ((dealers_vector.at(room_number))->players_list).at(i).user_staus == 4 || ((dealers_vector.at(room_number))->players_list).at(i).user_staus == 6 )
                {
                    Json::Value  edge_number;
                    Json::Value  pool;
                    Json::Value  card_mate;

                    edge_number["user_id"] = ((dealers_vector.at(room_number))->players_list).at(i).user_account ;
                    edge_number["seat_num"] = ((dealers_vector.at(room_number))->players_list).at(i).seat_num ;
                    for( unsigned int j=0; j<((dealers_vector.at(room_number))->edge_pool_list).size(); j++ )
                    {
                        pool.append(j);
                    }
                    edge_number["pool_id"] = pool;//赢得的边池号
                    card_mate.append(5);
                    card_mate.append(6);
                    edge_number["card_mate"] = card_mate;//手牌与公共牌的组合模型，只发[5,6]表示不需要组合
                    edge_number["card_type"] = 100;//赢牌的牌型 全弃牌的默认为高牌，不计算牌型
                    edge_user_list.append(edge_number);
                }
            }


            for(unsigned int i=0; i<edge_pool_temp.size(); i++)
            {
                int pool=edge_pool_temp.at(i).edge_height *  edge_pool_temp.at(i).share_seat.size();
                for(unsigned int j=0; j<edge_pool_temp.at(i).share_seat.size(); j++)
                {
                    if(  edge_pool_temp.at(i).share_seat.at(j) <=0 )
                    {
                        pool+= edge_pool_temp.at(i).share_seat.at(j);
                    }
                }
                pool_st["pool"]=pool;//池子里总的注金数
                pool_st["type"]=(edge_pool_temp.at(i)).pool_type;//池子的种类：1: 主池 0:边池
                pool_st["id"]=(edge_pool_temp.at(i)).pool_id;//池子的id：0: 主池 >1:边池
                edge_desk_pool.append(pool_st);

            }
            msg_body["desk_pool"]=edge_desk_pool;

            //计算每个玩家增加的经验
            Json::Value  exp_add;
            if(DzpkIfCpsOfType(dealers_vector[room_number]->room_type) == 0)
            {
            }
            else
            {
                for( unsigned int j=0; j<((dealers_vector.at(room_number))->players_list).size(); j++ )
                {
                    if( ((dealers_vector.at(room_number))->players_list).at(j).user_staus > 1  && ((dealers_vector.at(room_number))->players_list).at(j).user_staus < 7)
                    {
                        if( ((dealers_vector.at(room_number))->players_list).at(j).hand_chips >= ((dealers_vector.at(room_number))->players_list).at(j).last_chips && ((dealers_vector.at(room_number))->players_list).at(j).user_staus != 5 )
                        {
                            exp_add["user_id"] = ((dealers_vector.at(room_number))->players_list).at(j).user_account ;
                            exp_add["seat_num"]= ((dealers_vector.at(room_number))->players_list).at(j).seat_num;
                            if( (dealers_vector.at(room_number))->small_blind >= 2000 )
                            {
                                //盲注大于2000的房间，经验值*2
                                int nAddExperience = 0;
                                if( ((dealers_vector.at(room_number))->players_list).at(j).win_cards.size()==6 )
                                {
                                    int card_type_fen=0;
                                    if( ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) == 914  )
                                    {
                                        card_type_fen = 20;
                                    }
                                    else if (  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) > 900 &&  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) < 914 )
                                    {
                                        card_type_fen = 10;
                                    }
                                    else if (  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) > 800 &&  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) <= 814 )
                                    {
                                        card_type_fen = 6;
                                    }
                                    else if (  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) > 700 &&  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) <= 714 )
                                    {
                                        card_type_fen = 4;
                                    }
                                    else if (  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) > 400 &&  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) <= 614 )
                                    {
                                        card_type_fen = 1;
                                    }
                                    //exp_add["exp_add"]= ((dealers_vector.at(room_number))->current_player_num-1+ card_type_fen)*2 ;
                                    //((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += card_type_fen*2;
                                    nAddExperience = ((dealers_vector.at(room_number))->current_player_num-1+ card_type_fen)*2 ;
                                }

                                nAddExperience += ((dealers_vector.at(room_number))->current_player_num-1)*2 ;
                                nAddExperience = StdGetAddExperience(&((dealers_vector.at(room_number))->players_list).at(j),nAddExperience);
                                exp_add["exp_add"]= nAddExperience;
                                ((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += nAddExperience;

                                //exp_add["exp_add"]= ((dealers_vector.at(room_number))->current_player_num-1)*2 ;
                                //((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += ((dealers_vector.at(room_number))->current_player_num-1)*2;
                            }
                            else
                            {
                                int nAddExperience = ((dealers_vector.at(room_number))->current_player_num-1);;
                                nAddExperience = StdGetAddExperience(&((dealers_vector.at(room_number))->players_list).at(j),nAddExperience);
                                exp_add["exp_add"]= nAddExperience;
                                ((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += nAddExperience;
                                //exp_add["exp_add"]= ((dealers_vector.at(room_number))->current_player_num-1);
                                //((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += ((dealers_vector.at(room_number))->current_player_num-1);

                            }
                            exp_add["exp_rest"] = itoa(computer_exp_level(((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience,((dealers_vector.at(room_number))->players_list).at(j).dzpk_level,level_list),10);
                            exp_add["exp_upgrade"] = itoa(get_total_exp_level(((dealers_vector.at(room_number))->players_list).at(j).dzpk_level,level_list),10);

                            exp_add["dzpk_level"]= ((dealers_vector.at(room_number))->players_list).at(j).dzpk_level;
                            update_user_exp_level(((dealers_vector.at(room_number))->players_list).at(j).user_account,((dealers_vector.at(room_number))->players_list).at(j).dzpk_level,((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience,connpool);
                            experience_change_list.append(exp_add);
                        }
                        else if( ((dealers_vector.at(room_number))->players_list).at(j).hand_chips < ((dealers_vector.at(room_number))->players_list).at(j).last_chips || ( ((dealers_vector.at(room_number))->players_list).at(j).hand_chips == ((dealers_vector.at(room_number))->players_list).at(j).last_chips && ((dealers_vector.at(room_number))->players_list).at(j).user_staus == 5 ) )
                        {

                            exp_add["user_id"] = ((dealers_vector.at(room_number))->players_list).at(j).user_account ;
                            exp_add["seat_num"]= ((dealers_vector.at(room_number))->players_list).at(j).seat_num;
                            int nAddExperience = 0;
                            if(  (dealers_vector.at(room_number))->small_blind >= 2000 )
                            {
                                if( ((dealers_vector.at(room_number))->players_list).at(j).user_staus == 5 )
                                {
                                    //exp_add["exp_add"]= 1;
                                    //((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += 1;
                                    nAddExperience = 1;

                                }
                                else
                                {
                                    if( ((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience >=2)
                                    {
                                        //exp_add["exp_add"]= -2;
                                        //((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += -2;
                                        nAddExperience = -2;
                                    }
                                    else
                                    {
                                        //exp_add["exp_add"]= 0;
                                        nAddExperience = 0;
                                    }
                                }

                            }
                            else
                            {

                                if( ((dealers_vector.at(room_number))->players_list).at(j).user_staus == 5 )
                                {
                                    //exp_add["exp_add"]= 0;
                                    nAddExperience = 0;
                                }
                                else
                                {
                                    if( ((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience >=1)
                                    {
                                        //exp_add["exp_add"]= -1;
                                        //((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += -1;
                                        nAddExperience = -1;
                                    }
                                    else
                                    {
                                        //exp_add["exp_add"]= 0;
                                        nAddExperience = 0;
                                    }
                                }
                            }

                            nAddExperience = StdGetAddExperience(&((dealers_vector.at(room_number))->players_list).at(j),nAddExperience);
                            exp_add["exp_add"]= nAddExperience;
                            ((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += nAddExperience;

                            exp_add["exp_rest"]=itoa( computer_exp_level(((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience,((dealers_vector.at(room_number))->players_list).at(j).dzpk_level,level_list),10);
                            exp_add["exp_upgrade"] = itoa(get_total_exp_level(((dealers_vector.at(room_number))->players_list).at(j).dzpk_level,level_list),10);
                            exp_add["dzpk_level"]= ((dealers_vector.at(room_number))->players_list).at(j).dzpk_level;
                            update_user_exp_level(((dealers_vector.at(room_number))->players_list).at(j).user_account,((dealers_vector.at(room_number))->players_list).at(j).dzpk_level,((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience,connpool);

                            experience_change_list.append(exp_add);
                        }
                    }
                }
            }
            msg_body["exp_update"]=experience_change_list;

            //计算每个玩家的筹码增减
            Json::Value  chip_account;
            for( unsigned int j=0; j<((dealers_vector.at(room_number))->players_list).size(); j++ )
            {
                if( ((dealers_vector.at(room_number))->players_list).at(j).hand_chips != ((dealers_vector.at(room_number))->players_list).at(j).last_chips )
                {
                    chip_account["user_id"] = ((dealers_vector.at(room_number))->players_list).at(j).user_account ;
                    chip_account["seat_num"]= ((dealers_vector.at(room_number))->players_list).at(j).seat_num;
                    if(DzpkIfCpsOfType(dealers_vector[room_number]->room_type) == 0)
                    {
                        chip_account["chips_add"]= 0;
                    }
                    else
                    {
                        chip_account["chips_add"]= ((dealers_vector.at(room_number))->players_list).at(j).hand_chips - ((dealers_vector.at(room_number))->players_list).at(j).last_chips ;
                        ((dealers_vector.at(room_number))->players_list).at(j).chip_account += ((dealers_vector.at(room_number))->players_list).at(j).hand_chips - ((dealers_vector.at(room_number))->players_list).at(j).last_chips ;
                        dzpkDbUpdateUserChip(((dealers_vector.at(room_number))->players_list).at(j).nUserID,((dealers_vector.at(room_number))->players_list).at(j).hand_chips - ((dealers_vector.at(room_number))->players_list).at(j).last_chips,
                                             ((dealers_vector.at(room_number))->players_list).at(j).chip_account,0,&((dealers_vector.at(room_number))->players_list).at(j));
                        //插入玩家牌局信息
                        dzpkDbInsertPlayCard(&((dealers_vector.at(room_number))->players_list).at(j),dealers_vector.at(room_number),((dealers_vector.at(room_number))->players_list).at(j).hand_chips - ((dealers_vector.at(room_number))->players_list).at(j).last_chips);
                        //赢牌任务
                        dzpkCheckTaskWin(((dealers_vector.at(room_number))->players_list).at(j).hand_chips - ((dealers_vector.at(room_number))->players_list).at(j).last_chips,&((dealers_vector.at(room_number))->players_list).at(j));
                    }
                    ((dealers_vector.at(room_number))->players_list).at(j).last_chips = ((dealers_vector.at(room_number))->players_list).at(j).hand_chips ;
                    //update_user_chip_account(((dealers_vector.at(room_number))->players_list).at(j).user_account,((dealers_vector.at(room_number))->players_list).at(j).chip_account,connpool);
                    chip_account_list.append(chip_account);
                }
            }


            msg_body["update_chip_account"]=chip_account_list;

            msg_body["win_list"]=edge_user_list;

            for(unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++)
            {

                if( (((dealers_vector.at(room_number))->players_list).at(i)).user_staus >= 2)  //
                {
                    dztimer timer;
                    //账号和当前时间一起用MD5加密

                    timer.nTimerID = DzpkGetTimerID();
                    msg_body["timer_id"] = GetString(timer.nTimerID);
                    timer.nUserID = atoi((((dealers_vector.at(room_number))->players_list).at(i)).user_id.c_str());

                    timer.socket_fd =  (((dealers_vector.at(room_number))->players_list).at(i)).socket_fd;
                    timer.seat_num = (((dealers_vector.at(room_number))->players_list).at(i)).seat_num;
                    memcpy(timer.szUserAccount,(((dealers_vector.at(room_number))->players_list).at(i)).user_account.c_str(),strlen((((dealers_vector.at(room_number))->players_list).at(i)).user_account.c_str()));
                    timer.room_num = room_number;
                    timer.start_time = time(NULL);
                    timer.timer_type = 2;
                    timer.time_out = ((dealers_vector.at(room_number))->edge_pool_list).size()*3+3;
                    g_dzpkTimer.Insert(timer.nTimerID,(void*)&timer,sizeof(dztimer));
                    /*一局完成*/
                    WriteRoomLog(4000,dealers_vector[room_number]->dealer_id,timer.nUserID," game over play over flash(do next work) set timer type 2 (%d) \n",timer.nTimerID);

                }

                string   str_msg =   writer.write(msg_body);
                msg_body.removeMember("timer_id");

                DzpkSend( (((dealers_vector.at(room_number))->players_list).at(i)).socket_fd,CMD_WIN_LIST,(char *)str_msg.c_str(),strlen(str_msg.c_str()),dealers_vector[room_number]->dealer_id,atoi((((dealers_vector.at(room_number))->players_list).at(i)).user_id.c_str()));
            }

            //清除房间信息
            StdClearRoomInfo(dealers_vector.at(room_number),1);

            dealers_vector.at(room_number)->nActionUserID = -1;

        }
        else if(count > 1)   //剩余多个玩家，直接翻完剩余的牌，比较大小
        {

            //隐藏光圈
            StdStopHideLightRing(dealers_vector[room_number]);

            printf("-*333333333333333333333333333***--\n");
            //今天玩牌次数加一
            StdTodayPlayTimes(dealers_vector.at(room_number));

            //边池计算
            edge_pool eg_pool;
            eg_pool.edge_height=0;
            vector<edge_pool> edge_pool_temp;
            edge_pool_temp.push_back(eg_pool);
            edge_pool_computer(dealers_vector,room_number,edge_pool_temp);




            Json::FastWriter  writer;
            Json::Value  msg_body;
            Json::Value   open_card;
            Json::Value  pool_st;
            Json::Value  edge_desk_pool;

            int step_flg = 0;
            msg_body.clear();
            msg_body["msg_head"]="dzpk";
            msg_body["game_number"]="dzpk";
            msg_body["area_number"]="normal";
            msg_body["act_commond"]=CMD_DRAW_CARD; //翻牌


            if( (dealers_vector.at(room_number))->step == 1)
            {
                //第一轮的话，翻完5张
                open_card.append((dealers_vector.at(room_number))->desk_card[0]);
                open_card.append((dealers_vector.at(room_number))->desk_card[1]);
                open_card.append((dealers_vector.at(room_number))->desk_card[2]);
                open_card.append((dealers_vector.at(room_number))->desk_card[3]);
                open_card.append((dealers_vector.at(room_number))->desk_card[4]);
                msg_body["public_brand"]=open_card;


                for(unsigned int i=0; i<edge_pool_temp.size(); i++)
                {
                    int pool=edge_pool_temp.at(i).edge_height *  edge_pool_temp.at(i).share_seat.size();
                    for(unsigned int j=0; j<edge_pool_temp.at(i).share_seat.size(); j++)
                    {
                        if(  edge_pool_temp.at(i).share_seat.at(j) <=0 )
                        {
                            pool+= edge_pool_temp.at(i).share_seat.at(j);
                        }
                    }
                    pool_st["pool"]=pool;//池子里总的注金数
                    pool_st["type"]=(edge_pool_temp.at(i)).pool_type;//池子的种类：1: 主池 0:边池
                    pool_st["id"]=(edge_pool_temp.at(i)).pool_id;//池子的id：0: 主池 >1:边池
                    edge_desk_pool.append(pool_st);

                }

                msg_body["desk_pool"]=edge_desk_pool;//本轮产生的主池和边池的筹码数序列
                msg_body["next_turn"]=0;
                msg_body["max_follow_chip"]=0;
                (dealers_vector.at(room_number))->nTurnMaxChip = 0;
                msg_body["step"]=4;

                for(unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++)
                {

                    string   str_msg =   writer.write(msg_body);
                    DzpkSend( (((dealers_vector.at(room_number))->players_list).at(i)).socket_fd,CMD_DRAW_CARD,(char *)str_msg.c_str(),strlen(str_msg.c_str()),dealers_vector[room_number]->dealer_id,atoi(dealers_vector[room_number]->players_list[i].user_id.c_str()));
                }
                (dealers_vector.at(room_number))->step = 4;

            }
            else if( (dealers_vector.at(room_number))->step == 2)   //第二轮的话，翻完2张
            {
                // 翻牌预操作
                open_card.append((dealers_vector.at(room_number))->desk_card[3]);
                open_card.append((dealers_vector.at(room_number))->desk_card[4]);
                msg_body["public_brand"]=open_card;


                for(unsigned int i=0; i<edge_pool_temp.size(); i++)
                {
                    int pool=edge_pool_temp.at(i).edge_height *  edge_pool_temp.at(i).share_seat.size();
                    for(unsigned int j=0; j<edge_pool_temp.at(i).share_seat.size(); j++)
                    {
                        if(  edge_pool_temp.at(i).share_seat.at(j) <=0 )
                        {
                            pool+= edge_pool_temp.at(i).share_seat.at(j);
                        }
                    }
                    pool_st["pool"]=pool;//池子里总的注金数
                    pool_st["type"]=(edge_pool_temp.at(i)).pool_type;//池子的种类：1: 主池 0:边池
                    pool_st["id"]=(edge_pool_temp.at(i)).pool_id;//池子的id：0: 主池 >1:边池
                    edge_desk_pool.append(pool_st);
                }



                msg_body["desk_pool"]=edge_desk_pool;//本轮产生的主池和边池的筹码数序列
                msg_body["next_turn"]=0;
                msg_body["max_follow_chip"]=0;
                (dealers_vector.at(room_number))->nTurnMaxChip = 0;
                msg_body["step"]=4;

                for(unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++)
                {
                    string   str_msg =   writer.write(msg_body);
                    DzpkSend( (((dealers_vector.at(room_number))->players_list).at(i)).socket_fd,CMD_DRAW_CARD,(char *)str_msg.c_str(),strlen(str_msg.c_str()),dealers_vector[room_number]->dealer_id,atoi(dealers_vector[room_number]->players_list[i].user_id.c_str()));
                }
                (dealers_vector.at(room_number))->step = 4;

            }
            else if( (dealers_vector.at(room_number))->step == 3)   //第三轮的话，翻完1张
            {
                // 翻牌预操作
                open_card.append((dealers_vector.at(room_number))->desk_card[4]);
                msg_body["public_brand"]=open_card;


                for(unsigned int i=0; i<edge_pool_temp.size(); i++)
                {
                    int pool=edge_pool_temp.at(i).edge_height *  edge_pool_temp.at(i).share_seat.size();
                    for(unsigned int j=0; j<edge_pool_temp.at(i).share_seat.size(); j++)
                    {
                        if(  edge_pool_temp.at(i).share_seat.at(j) <=0 )
                        {
                            pool+= edge_pool_temp.at(i).share_seat.at(j);
                        }
                    }
                    pool_st["pool"]=pool;//池子里总的注金数
                    pool_st["type"]=(edge_pool_temp.at(i)).pool_type;//池子的种类：1: 主池 0:边池
                    pool_st["id"]=(edge_pool_temp.at(i)).pool_id;//池子的id：0: 主池 >1:边池
                    edge_desk_pool.append(pool_st);

                }

                msg_body["desk_pool"]=edge_desk_pool;//本轮产生的主池和边池的筹码数序列
                msg_body["next_turn"]=0;
                msg_body["max_follow_chip"]=0;
                (dealers_vector.at(room_number))->nTurnMaxChip = 0;
                msg_body["step"]=4;
                for(unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++)
                {
                    string   str_msg =   writer.write(msg_body);
                    DzpkSend( (((dealers_vector.at(room_number))->players_list).at(i)).socket_fd,CMD_DRAW_CARD,(char *)str_msg.c_str(),strlen(str_msg.c_str()),dealers_vector[room_number]->dealer_id,atoi(dealers_vector[room_number]->players_list[i].user_id.c_str()));
                }
                (dealers_vector.at(room_number))->step = 4;

            }
            else if( (dealers_vector.at(room_number))->step == 4)   //第三轮的话，翻完1张
            {
                step_flg =4;
            }


            (dealers_vector.at(room_number))->nStepOver = 1;//最后一轮已经跟满注，状态改为1

            //牌面虚拟值计算
            computer_card_value(dealers_vector,room_number);

            //幸运手牌活动
            StdLuckyCardActivity(dealers_vector[room_number],1);


            //赢牌计算
            vector<winer_pool>  winder_list;
            computer_winer_card(dealers_vector,room_number,winder_list);


            update_user_best_win_card(dealers_vector,room_number,connpool);



            //向房间内的所有玩家发送赢牌消息
            Json::Value  edge_user_list;
            Json::Value  hand_cards_all;
            Json::Value  chip_account_list;
            Json::Value  experience_change_list;
            edge_desk_pool.clear();
            msg_body.clear();
            msg_body["msg_head"]="dzpk";
            msg_body["game_number"]="dzpk";
            msg_body["area_number"]="normal";
            msg_body["act_commond"]=CMD_WIN_LIST;
            msg_body["next_turn"]=0;

            //手牌-座位号对应列表
            StdPutResultShowHandCards(dealers_vector[room_number],hand_cards_all,winder_list,(dealers_vector.at(room_number))->edge_pool_list);



            msg_body["hand_cards_all"]=hand_cards_all;

            if( step_flg== 4)
            {

                for(unsigned int i=0; i<edge_pool_temp.size(); i++)
                {
                    int pool=edge_pool_temp.at(i).edge_height *  edge_pool_temp.at(i).share_seat.size();
                    for(unsigned int j=0; j<edge_pool_temp.at(i).share_seat.size(); j++)
                    {
                        if(  edge_pool_temp.at(i).share_seat.at(j) <=0 )
                        {
                            pool+= edge_pool_temp.at(i).share_seat.at(j);
                        }
                    }
                    pool_st["pool"]=pool;//池子里总的注金数
                    pool_st["type"]=(edge_pool_temp.at(i)).pool_type;//池子的种类：1: 主池 0:边池
                    pool_st["id"]=(edge_pool_temp.at(i)).pool_id;//池子的id：0: 主池 >1:边池
                    edge_desk_pool.append(pool_st);

                }

                msg_body["desk_pool"]=edge_desk_pool;//本轮产生的主池和边池的筹码数序列
            }
            for(  unsigned int i=0; i<winder_list.size(); i++ )
            {
                Json::Value  edge_number;
                Json::Value  pool;
                Json::Value  card_mate;
                int seat_number = winder_list.at(i).seat_number;
                for( unsigned int j=0; j<((dealers_vector.at(room_number))->players_list).size(); j++ )
                {
                    if( ((dealers_vector.at(room_number))->players_list).at(j).seat_num == seat_number)
                    {
                        edge_number["user_id"] = ((dealers_vector.at(room_number))->players_list).at(j).user_account ;
                        edge_number["seat_num"] = ((dealers_vector.at(room_number))->players_list).at(j).seat_num ;
                        if( ((dealers_vector.at(room_number))->players_list).at(j).win_cards.size() == 6)
                        {
                            edge_number["card_type"] = ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5);
                            for(unsigned int x=0; x<5; x++ )
                            {
                                int card_va = ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(x);

                                //先到公共牌中找
                                for(unsigned int y=0; y< 5; y++  )
                                {
                                    if( card_va == (dealers_vector.at(room_number))->desk_card[y] )
                                    {
                                        card_mate.append(y);
                                    }
                                }
                                //再到手牌中找
                                for(unsigned int z=0; z< ((dealers_vector.at(room_number))->players_list).at(j).hand_cards.size(); z++  )
                                {
                                    if( card_va == ((dealers_vector.at(room_number))->players_list).at(j).hand_cards.at(z) )
                                    {
                                        card_mate.append(5+z);
                                    }
                                }
                            }
                        }
                        else
                        {
                            edge_number["card_type"] = 100;
                        }

                        g_pMemData->updateGameNum((dealers_vector.at(room_number))->dealer_id,
                                        (((dealers_vector.at(room_number))->players_list).at(j)).nUserID,
                                        (dealers_vector.at(room_number))->small_blind,
                                        (dealers_vector.at(room_number))->current_player_num, 1);

                    }
                }

                for( unsigned int k=0; k<winder_list.at(i).pool_id_list.size(); k++ )
                {
                    pool.append( winder_list.at(i).pool_id_list.at(k));
                }
                edge_number["pool_id"] = pool;//赢得的边池号
                edge_number["card_mate"] = card_mate;//手牌-公共牌匹配位置
                edge_user_list.append(edge_number);
            }


            //计算每个玩家增加的经验
            Json::Value  exp_add;
            if(DzpkIfCpsOfType(dealers_vector[room_number]->room_type) == 0)
            {
            }
            else
            {
                for( unsigned int j=0; j<((dealers_vector.at(room_number))->players_list).size(); j++ )
                {
                    if( ((dealers_vector.at(room_number))->players_list).at(j).user_staus > 1  && ((dealers_vector.at(room_number))->players_list).at(j).user_staus < 7)
                    {
                        if( ((dealers_vector.at(room_number))->players_list).at(j).hand_chips >= ((dealers_vector.at(room_number))->players_list).at(j).last_chips && ((dealers_vector.at(room_number))->players_list).at(j).user_staus != 5 )
                        {
                            exp_add["user_id"] = ((dealers_vector.at(room_number))->players_list).at(j).user_account ;
                            exp_add["seat_num"]= ((dealers_vector.at(room_number))->players_list).at(j).seat_num;
                            if( (dealers_vector.at(room_number))->small_blind >= 2000 )
                            {
                                int nAddExperience = 0;
                                //盲注大于2000的房间，经验值*2
                                if( ((dealers_vector.at(room_number))->players_list).at(j).win_cards.size()==6 )
                                {
                                    int card_type_fen=0;
                                    if( ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) == 914  )
                                    {
                                        card_type_fen = 20;
                                    }
                                    else if (  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) > 900 &&  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) < 914 )
                                    {
                                        card_type_fen = 10;
                                    }
                                    else if (  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) > 800 &&  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) <= 814 )
                                    {
                                        card_type_fen = 6;
                                    }
                                    else if (  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) > 700 &&  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) <= 714 )
                                    {
                                        card_type_fen = 4;
                                    }
                                    else if (  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) > 400 &&  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) <= 614 )
                                    {
                                        card_type_fen = 1;
                                    }
                                    nAddExperience = ((dealers_vector.at(room_number))->current_player_num-1+ card_type_fen)*2 ;
                                    //exp_add["exp_add"]= ((dealers_vector.at(room_number))->current_player_num-1+ card_type_fen)*2 ;
                                    // ((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += card_type_fen*2;

                                }

                                nAddExperience += ((dealers_vector.at(room_number))->current_player_num-1)*2 ;
                                nAddExperience = StdGetAddExperience(&((dealers_vector.at(room_number))->players_list).at(j),nAddExperience);
                                exp_add["exp_add"]= nAddExperience;
                                ((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += nAddExperience;

                                //exp_add["exp_add"]= ((dealers_vector.at(room_number))->current_player_num-1)*2 ;
                                //((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += ((dealers_vector.at(room_number))->current_player_num-1)*2;
                            }
                            else
                            {
                                int nAddExperience = ((dealers_vector.at(room_number))->current_player_num-1);
                                nAddExperience = StdGetAddExperience(&((dealers_vector.at(room_number))->players_list).at(j),nAddExperience);
                                exp_add["exp_add"]= nAddExperience;
                                ((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += nAddExperience;

                                //exp_add["exp_add"]= ((dealers_vector.at(room_number))->current_player_num-1);
                                //((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += ((dealers_vector.at(room_number))->current_player_num-1);

                            }
                            exp_add["exp_rest"] = itoa(computer_exp_level(((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience,((dealers_vector.at(room_number))->players_list).at(j).dzpk_level,level_list),10);
                            exp_add["exp_upgrade"] = itoa(get_total_exp_level(((dealers_vector.at(room_number))->players_list).at(j).dzpk_level,level_list),10);

                            exp_add["dzpk_level"]= ((dealers_vector.at(room_number))->players_list).at(j).dzpk_level;
                            update_user_exp_level(((dealers_vector.at(room_number))->players_list).at(j).user_account,((dealers_vector.at(room_number))->players_list).at(j).dzpk_level,((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience,connpool);
                            experience_change_list.append(exp_add);
                        }
                        else if( ((dealers_vector.at(room_number))->players_list).at(j).hand_chips < ((dealers_vector.at(room_number))->players_list).at(j).last_chips || ( ((dealers_vector.at(room_number))->players_list).at(j).hand_chips == ((dealers_vector.at(room_number))->players_list).at(j).last_chips && ((dealers_vector.at(room_number))->players_list).at(j).user_staus == 5 ) )
                        {

                            exp_add["user_id"] = ((dealers_vector.at(room_number))->players_list).at(j).user_account ;
                            exp_add["seat_num"]= ((dealers_vector.at(room_number))->players_list).at(j).seat_num;

                            int nAddExperience = 0;
                            if(  (dealers_vector.at(room_number))->small_blind >= 2000 )
                            {
                                if( ((dealers_vector.at(room_number))->players_list).at(j).user_staus == 5 )
                                {
                                    //exp_add["exp_add"]= 1;
                                    // ((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += 1;
                                    nAddExperience = 1;

                                }
                                else
                                {
                                    if( ((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience >=2)
                                    {
                                        //exp_add["exp_add"]= -2;
                                        //((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += -2;
                                        nAddExperience = -2;
                                    }
                                    else
                                    {
                                        nAddExperience = 0;
                                        //exp_add["exp_add"]= 0;
                                    }
                                }
                            }
                            else
                            {
                                if( ((dealers_vector.at(room_number))->players_list).at(j).user_staus == 5 )
                                {
                                    nAddExperience = 0;
                                    // exp_add["exp_add"]= 0;
                                }
                                else
                                {
                                    if( ((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience >=1)
                                    {
                                        //exp_add["exp_add"]= -1;
                                        //((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += -1;
                                        nAddExperience = -1;
                                    }
                                    else
                                    {
                                        // exp_add["exp_add"]= 0;
                                        nAddExperience = 0;
                                    }
                                }
                            }

                            nAddExperience = StdGetAddExperience(&((dealers_vector.at(room_number))->players_list).at(j),nAddExperience);
                            exp_add["exp_add"]= nAddExperience;
                            ((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += nAddExperience;

                            exp_add["exp_rest"]=itoa( computer_exp_level(((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience,((dealers_vector.at(room_number))->players_list).at(j).dzpk_level,level_list),10);
                            exp_add["exp_upgrade"] = itoa(get_total_exp_level(((dealers_vector.at(room_number))->players_list).at(j).dzpk_level,level_list),10);
                            exp_add["dzpk_level"]= ((dealers_vector.at(room_number))->players_list).at(j).dzpk_level;
                            update_user_exp_level(((dealers_vector.at(room_number))->players_list).at(j).user_account,((dealers_vector.at(room_number))->players_list).at(j).dzpk_level,((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience,connpool);
                            experience_change_list.append(exp_add);
                        }
                    }
                }
            }
            msg_body["exp_update"]=experience_change_list;


            //计算每个玩家的筹码增减
            Json::Value  chip_account;
            for( unsigned int j=0; j<((dealers_vector.at(room_number))->players_list).size(); j++ )
            {
                if( ((dealers_vector.at(room_number))->players_list).at(j).hand_chips !=  ((dealers_vector.at(room_number))->players_list).at(j).last_chips )
                {
                    chip_account["user_id"] = ((dealers_vector.at(room_number))->players_list).at(j).user_account ;
                    chip_account["seat_num"]= ((dealers_vector.at(room_number))->players_list).at(j).seat_num;
                    if(DzpkIfCpsOfType(dealers_vector[room_number]->room_type) == 0)
                    {
                        chip_account["chips_add"]= 0;
                    }
                    else
                    {
                        chip_account["chips_add"]= ((dealers_vector.at(room_number))->players_list).at(j).hand_chips -  ((dealers_vector.at(room_number))->players_list).at(j).last_chips ;
                        ((dealers_vector.at(room_number))->players_list).at(j).chip_account += ((dealers_vector.at(room_number))->players_list).at(j).hand_chips - ((dealers_vector.at(room_number))->players_list).at(j).last_chips ;
                        dzpkDbUpdateUserChip(((dealers_vector.at(room_number))->players_list).at(j).nUserID,((dealers_vector.at(room_number))->players_list).at(j).hand_chips - ((dealers_vector.at(room_number))->players_list).at(j).last_chips,
                                             ((dealers_vector.at(room_number))->players_list).at(j).chip_account,0,&((dealers_vector.at(room_number))->players_list).at(j));
                        //插入玩家牌局信息
                        dzpkDbInsertPlayCard(&((dealers_vector.at(room_number))->players_list).at(j),dealers_vector.at(room_number),((dealers_vector.at(room_number))->players_list).at(j).hand_chips - ((dealers_vector.at(room_number))->players_list).at(j).last_chips);

                        //赢牌任务
                        dzpkCheckTaskWin(((dealers_vector.at(room_number))->players_list).at(j).hand_chips - ((dealers_vector.at(room_number))->players_list).at(j).last_chips,&((dealers_vector.at(room_number))->players_list).at(j));
                    }
                    ((dealers_vector.at(room_number))->players_list).at(j).last_chips = ((dealers_vector.at(room_number))->players_list).at(j).hand_chips ;
                    //update_user_chip_account(((dealers_vector.at(room_number))->players_list).at(j).user_account,((dealers_vector.at(room_number))->players_list).at(j).chip_account,connpool);
                    chip_account_list.append(chip_account);
                }
                else
                {
                    chip_account["user_id"] = ((dealers_vector.at(room_number))->players_list).at(j).user_account ;
                    chip_account["seat_num"]= ((dealers_vector.at(room_number))->players_list).at(j).seat_num;
                    chip_account["chips_add"]= 0;
                }
            }
            msg_body["update_chip_account"]=chip_account_list;

            msg_body["win_list"]=edge_user_list;

            for(unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++)
            {

                if( (((dealers_vector.at(room_number))->players_list).at(i)).user_staus >= 2)  //
                {
                    dztimer timer;

                    //账号和当前时间一起用MD5加密

                    timer.nTimerID = DzpkGetTimerID();
                    msg_body["timer_id"] = GetString(timer.nTimerID);
                    timer.nUserID =  atoi(((dealers_vector.at(room_number))->players_list).at(i).user_id.c_str());

                    timer.socket_fd =  (((dealers_vector.at(room_number))->players_list).at(i)).socket_fd;
                    timer.seat_num = (((dealers_vector.at(room_number))->players_list).at(i)).seat_num;
                    memcpy(timer.szUserAccount,((dealers_vector.at(room_number))->players_list).at(i).user_account.c_str(),strlen(((dealers_vector.at(room_number))->players_list).at(i).user_account.c_str()));
                    timer.room_num = room_number;
                    timer.start_time = time(NULL);
                    timer.timer_type = 2;
                    timer.time_out = ((dealers_vector.at(room_number))->edge_pool_list).size()*3+3;
                    g_dzpkTimer.Insert(timer.nTimerID,(void*)&timer,sizeof(dztimer));
                    //正常结束
                    WriteRoomLog(4000,dealers_vector[room_number]->dealer_id ,timer.nUserID,"game normal over play flash ( do next work) set timer type 2 (%d)  \n",timer.nTimerID);
                }
                string str_msg =   writer.write(msg_body);
                msg_body.removeMember("timer_id");

                DzpkSend( (((dealers_vector.at(room_number))->players_list).at(i)).socket_fd,CMD_WIN_LIST,(char *)str_msg.c_str(),strlen(str_msg.c_str()),dealers_vector[room_number]->dealer_id,atoi(((dealers_vector.at(room_number))->players_list).at(i).user_id.c_str()));
            }
            //清除房间信息
            StdClearRoomInfo(dealers_vector.at(room_number),1);

            dealers_vector.at(room_number)->nActionUserID = -1;
        }
    }
    else
    {
        //找到下家
        for(unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++)
        {
            if( (((dealers_vector.at(room_number))->players_list).at(i)).seat_num == next_seat  )
            {
                //查找当前用户状态
                int current_status=0;
                for(unsigned int m=0; m<((dealers_vector.at(room_number))->players_list).size(); m++)
                {
                    if( (((dealers_vector.at(room_number))->players_list).at(m)).seat_num == (dealers_vector.at(room_number))->turn   )
                    {
                        current_status = (((dealers_vector.at(room_number))->players_list).at(m)).user_staus;
                        break;
                    }
                }

                int kan_all =0;
                //满注判断
                if(((((dealers_vector.at(room_number))->players_list).at(i)).desk_chip < (dealers_vector.at(room_number))->follow_chip) )
                {
                    //没有跟满注，且不是全部看牌，轮到下一家
                    if( current_status == 4 && (((dealers_vector.at(room_number))->players_list).at(i)).user_staus == 4  )
                    {
                        kan_all = 1;
                    }
                    else
                    {
                        //操作定时器
                        dztimer  timer;
                        Json::Value  pre_operation;
                        Json::Value  pre_chip;
                        Json::FastWriter  writer;
                        Json::Value  msg_body;
                        msg_body.clear();
                        msg_body["msg_head"]="dzpk";
                        msg_body["game_number"]="dzpk";
                        msg_body["area_number"]="normal";
                        msg_body["act_commond"]=CMD_NEXT_TURN;

                        //下一个加注的人就是玩家状态为打牌中 顺次的座位号没有坐人，则累加座位号，直到找到位置
                        unsigned int next_p = next_player((dealers_vector.at(room_number))->turn,dealers_vector,room_number);
                        msg_body["next_turn"] = next_p;
                        (dealers_vector.at(room_number))->turn = next_p;


                        timer.socket_fd = (((dealers_vector.at(room_number))->players_list).at(i)).socket_fd;
                        timer.seat_num = next_p;
                        memcpy(timer.szUserAccount,((dealers_vector.at(room_number))->players_list).at(i).user_account.c_str(),strlen(((dealers_vector.at(room_number))->players_list).at(i).user_account.c_str()));
                        //账号和当前时间一起用MD5加密
                        MD5_CTX ctx;
                        char* buf=(char*) ((((dealers_vector.at(room_number))->players_list).at(i)).user_account+itoa(get_current_time(),10)).c_str();
                        unsigned char md[10];
                        MD5_Init(&ctx);
                        MD5_Update(&ctx,buf,strlen(buf));
                        MD5_Final(md,&ctx);
                        char aaa[10];
                        for(unsigned int x=0; x<10; x++)
                        {
                            sprintf(aaa+x,"%x",md[x]);
                        }

                        timer.nTimerID = DzpkGetTimerID();
                        msg_body["timer_id"]= GetString(timer.nTimerID);
                        timer.nUserID = ((dealers_vector.at(room_number))->players_list).at(i).nUserID;


                        int max_follow_chip=0;
                        for(unsigned int m=0; m<((dealers_vector.at(room_number))->players_list).size(); m++)
                        {
                            if(  (((dealers_vector.at(room_number))->players_list).at(m)).nUserID != (((dealers_vector.at(room_number))->players_list).at(i)).nUserID) //除自己之外
                            {
                                if(  (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 2 || (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 3 || (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 4 || (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 6 ) //仍然可以下注的玩家
                                {
                                    if( max_follow_chip < (((dealers_vector.at(room_number))->players_list).at(m)).hand_chips  + (((dealers_vector.at(room_number))->players_list).at(m)).desk_chip )
                                    {
                                        max_follow_chip = (((dealers_vector.at(room_number))->players_list).at(m)).hand_chips + (((dealers_vector.at(room_number))->players_list).at(m)).desk_chip;
                                    }
                                }
                            }
                        }

                        //除自己外，剩余玩家的手上筹码最多的一位，若大于自己手上筹码时，最大跟注为手上的总筹码数
                        if( max_follow_chip >= (((dealers_vector.at(room_number))->players_list).at(i)).hand_chips + (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip )
                        {
                            max_follow_chip = (((dealers_vector.at(room_number))->players_list).at(i)).hand_chips;
                        }
                        else
                        {
                            max_follow_chip = max_follow_chip - (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip;
                        }
                        msg_body["max_follow_chip"] = max_follow_chip;
                        (dealers_vector.at(room_number))->nTurnMaxChip = max_follow_chip;


                        //判断最小跟注= 当前最小跟注 - 已加的桌面注金
                        int follow_chipNow = (dealers_vector.at(room_number))->follow_chip - (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip;
                        if(follow_chipNow >= max_follow_chip)
                        {
                            follow_chipNow = max_follow_chip;
                        }
                        msg_body["follow_chip"]=follow_chipNow;

                        //将当前操作区间保存
                        (dealers_vector.at(room_number))->nTurnMinChip = follow_chipNow;



                        int add_chipNow=0;
                        if ( (dealers_vector.at(room_number))->add_chip_manual_flg==0 )
                        {
                            if ( (chips_value == 0 && (dealers_vector.at(room_number))->look_flag == 0 )  )  //看牌
                            {
                                add_chipNow = 2 * (dealers_vector.at(room_number))->small_blind - (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip ;
                            }
                            else
                            {
                                add_chipNow = (dealers_vector.at(room_number))->follow_chip + 2 * (dealers_vector.at(room_number))->small_blind - (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip ;
                            }
                        }
                        else if ( (dealers_vector.at(room_number))->add_chip_manual_flg==1)
                        {
                            add_chipNow = ((dealers_vector.at(room_number))->follow_chip - (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip) * 2 ;
                        }



                        if(add_chipNow >= max_follow_chip)
                        {
                            add_chipNow = max_follow_chip;
                        }

                        msg_body["default_add_chip"]=add_chipNow;



                        //计算操作权限
                        Json::Value  rights;
                        rights.append(0);

                        if ( (chips_value == 0 && (dealers_vector.at(room_number))->look_flag == 0 )  )
                        {
                            //上一个玩家看牌，则下一家也可以看牌
                            rights.append(2);// 0:加注 1：跟注  2：看牌  3：弃牌
                            (dealers_vector.at(room_number))->nTurnMinChip = 0;
                        }
                        else
                        {
                            rights.append(1);
                        }

                        rights.append(3);
                        msg_body["ctl_rights"]=rights;


                        //预操作
                        pre_operation.clear();
                        for(unsigned int n=0; n<((dealers_vector.at(room_number))->players_list).size(); n++)
                        {
                            if( ((dealers_vector.at(room_number))->players_list).at(n).user_staus == 2 || ((dealers_vector.at(room_number))->players_list).at(n).user_staus == 3 || ((dealers_vector.at(room_number))->players_list).at(n).user_staus == 4)
                            {
                                if( ((dealers_vector.at(room_number))->players_list).at(n).desk_chip < (dealers_vector.at(room_number))->follow_chip )
                                {
                                    if( ((dealers_vector.at(room_number))->players_list).at(n).hand_chips >= (dealers_vector.at(room_number))->follow_chip - ((dealers_vector.at(room_number))->players_list).at(n).desk_chip )
                                    {
                                        pre_chip["seat_num"]= ((dealers_vector.at(room_number))->players_list).at(n).seat_num;
                                        pre_chip["follow_chip"] = (dealers_vector.at(room_number))->follow_chip - ((dealers_vector.at(room_number))->players_list).at(n).desk_chip;
                                    }
                                    else
                                    {
                                        pre_chip["seat_num"]= ((dealers_vector.at(room_number))->players_list).at(n).seat_num;
                                        pre_chip["follow_chip"] = ((dealers_vector.at(room_number))->players_list).at(n).hand_chips;
                                    }
                                    pre_operation.append(pre_chip);
                                }
                                else
                                {
                                    pre_chip["seat_num"]= ((dealers_vector.at(room_number))->players_list).at(n).seat_num;
                                    pre_chip["follow_chip"] = 0;
                                    pre_operation.append(pre_chip);
                                }
                            }
                        }

                        msg_body["pre_operation"]=pre_operation;

                        string str_msg =   writer.write(msg_body);

                        //向房间内的所有玩家发送消息周知
                        for(unsigned int k=0; k<((dealers_vector.at(room_number))->players_list).size(); k++)
                        {
                            DzpkSend((((dealers_vector.at(room_number))->players_list).at(k)).socket_fd,CMD_NEXT_TURN,(char *)str_msg.c_str(),strlen(str_msg.c_str()),dealers_vector[room_number]->dealer_id,atoi(((dealers_vector.at(room_number))->players_list).at(k).user_id.c_str()));
                        }
                        //等待操作定时器

                        timer.room_num = room_number;
                        timer.timer_type = 1;  //弃牌定时器
                        timer.start_time = time(NULL);
                        timer.time_out = (dealers_vector.at(room_number))->bet_time;//根据房间是快速房 还是普通房来决定超时时间
                        msg_body.removeMember("public_brand");
                        str_msg =   writer.write(msg_body);
                        memcpy(timer.szMsg,str_msg.c_str(),strlen(str_msg.c_str()));
                        timer.nMsgLen = strlen(str_msg.c_str());
                        g_dzpkTimer.Insert(timer.nTimerID,(void*)&timer,sizeof(dztimer));
                        dealers_vector.at(room_number)->nActionUserID = timer.nUserID;
                        WriteRoomLog(500,dealers_vector[room_number]->dealer_id ,timer.nUserID," do next work set timer (%d) \n",timer.nTimerID);
                    }
                }

                //玩家已跟满注 OR 全部看牌  /*翻公共牌*/
                if( ((((dealers_vector.at(room_number))->players_list).at(i)).desk_chip == (dealers_vector.at(room_number))->follow_chip ) || kan_all ==1  )
                {
                    //根据当前牌局的进行的阶段翻公共牌
                    if( (dealers_vector.at(room_number))->step == 1)
                    {
                        if( ((dealers_vector.at(room_number))->small_blind * 2+(dealers_vector.at(room_number))->before_chip)
                                == (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip
                                && (((dealers_vector.at(room_number))->players_list).at(i)).first_turn_blind == 1 )
                        {
                            //操作定时器
                            dztimer  timer;
                            Json::Value  pre_operation;
                            Json::Value  pre_chip;
                            Json::FastWriter  writer;
                            Json::Value  msg_body;
                            msg_body.clear();
                            msg_body["msg_head"]="dzpk";
                            msg_body["game_number"]="dzpk";
                            msg_body["area_number"]="normal";
                            msg_body["act_commond"]=CMD_NEXT_TURN;

                            //下一个加注的人就是玩家状态为打牌中 顺次的座位号没有坐人，则累加座位号，直到找到位置
                            unsigned int next_p = next_player((dealers_vector.at(room_number))->turn,dealers_vector,room_number);
                            msg_body["next_turn"] = next_p;
                            (dealers_vector.at(room_number))->turn = next_p;


                            timer.socket_fd = (((dealers_vector.at(room_number))->players_list).at(i)).socket_fd;
                            timer.seat_num = next_p;
                            memcpy(timer.szUserAccount,((dealers_vector.at(room_number))->players_list).at(i).user_account.c_str(),strlen(((dealers_vector.at(room_number))->players_list).at(i).user_account.c_str()));
                            //账号和当前时间一起用MD5加密
                            MD5_CTX ctx;
                            char* buf=(char*) ((((dealers_vector.at(room_number))->players_list).at(i)).user_account+itoa(get_current_time(),10)).c_str();
                            unsigned char md[10];
                            MD5_Init(&ctx);
                            MD5_Update(&ctx,buf,strlen(buf));
                            MD5_Final(md,&ctx);
                            char aaa[10];
                            for(unsigned int x=0; x<10; x++)
                            {
                                sprintf(aaa+x,"%x",md[x]);
                            }

                            timer.nTimerID = DzpkGetTimerID();
                            msg_body["timer_id"]= GetString(timer.nTimerID);
                            timer.nUserID = ((dealers_vector.at(room_number))->players_list).at(i).nUserID;

                            int max_follow_chip=0;
                            for(unsigned int m=0; m<((dealers_vector.at(room_number))->players_list).size(); m++)
                            {
                                if(  (((dealers_vector.at(room_number))->players_list).at(m)).nUserID != (((dealers_vector.at(room_number))->players_list).at(i)).nUserID  ) //除自己之外
                                {
                                    if(  (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 2 || (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 3 || (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 4 || (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 6 ) //仍然可以下注的玩家
                                    {
                                        if( max_follow_chip < (((dealers_vector.at(room_number))->players_list).at(m)).hand_chips  + (((dealers_vector.at(room_number))->players_list).at(m)).desk_chip )
                                        {
                                            max_follow_chip = (((dealers_vector.at(room_number))->players_list).at(m)).hand_chips + (((dealers_vector.at(room_number))->players_list).at(m)).desk_chip;
                                        }
                                    }
                                }
                            }

                            //除自己外，剩余玩家的手上筹码最多的一位，若大于自己手上筹码时，最大跟注为手上的总筹码数
                            if( max_follow_chip >= (((dealers_vector.at(room_number))->players_list).at(i)).hand_chips + (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip )
                            {
                                max_follow_chip = (((dealers_vector.at(room_number))->players_list).at(i)).hand_chips;
                            }
                            else
                            {
                                max_follow_chip = max_follow_chip - (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip;
                            }
                            msg_body["max_follow_chip"] = max_follow_chip;
                            (dealers_vector.at(room_number))->nTurnMaxChip = max_follow_chip;

                            //判断最小跟注= 当前最小跟注 - 已加的桌面注金
                            msg_body["follow_chip"]=0;
                            (dealers_vector.at(room_number))->nTurnMinChip = 0;

                            int add_chipNow=0;
                            if ( (dealers_vector.at(room_number))->add_chip_manual_flg==0 )
                            {
                                add_chipNow = (dealers_vector.at(room_number))->follow_chip + 2 * (dealers_vector.at(room_number))->small_blind - (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip ;
                            }
                            else if ( (dealers_vector.at(room_number))->add_chip_manual_flg==1)
                            {
                                add_chipNow = ((dealers_vector.at(room_number))->follow_chip - (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip) * 2 ;
                            }

                            if(add_chipNow >= max_follow_chip)
                            {
                                add_chipNow = max_follow_chip;
                            }
                            msg_body["default_add_chip"]=add_chipNow;

                            //计算操作权限
                            Json::Value  rights;
                            rights.append(0);
                            rights.append(2);// 0:加注 1：跟注  2：看牌  3：弃牌
                            rights.append(3);
                            msg_body["ctl_rights"]=rights;
                            (((dealers_vector.at(room_number))->players_list).at(i)).first_turn_blind = 0;

                            //预操作
                            pre_operation.clear();
                            for(unsigned int n=0; n<((dealers_vector.at(room_number))->players_list).size(); n++)
                            {
                                if( ((dealers_vector.at(room_number))->players_list).at(n).user_staus == 2 || ((dealers_vector.at(room_number))->players_list).at(n).user_staus == 3 || ((dealers_vector.at(room_number))->players_list).at(n).user_staus == 4)
                                {
                                    if( ((dealers_vector.at(room_number))->players_list).at(n).desk_chip < (dealers_vector.at(room_number))->follow_chip )
                                    {
                                        if( ((dealers_vector.at(room_number))->players_list).at(n).hand_chips >= (dealers_vector.at(room_number))->follow_chip - ((dealers_vector.at(room_number))->players_list).at(n).desk_chip )
                                        {
                                            pre_chip["seat_num"]= ((dealers_vector.at(room_number))->players_list).at(n).seat_num;
                                            pre_chip["follow_chip"] = (dealers_vector.at(room_number))->follow_chip - ((dealers_vector.at(room_number))->players_list).at(n).desk_chip;
                                        }
                                        else
                                        {
                                            pre_chip["seat_num"]= ((dealers_vector.at(room_number))->players_list).at(n).seat_num;
                                            pre_chip["follow_chip"] = ((dealers_vector.at(room_number))->players_list).at(n).hand_chips;
                                        }
                                        pre_operation.append(pre_chip);
                                    }
                                    else
                                    {
                                        pre_chip["seat_num"]= ((dealers_vector.at(room_number))->players_list).at(n).seat_num;
                                        pre_chip["follow_chip"] = 0;
                                        pre_operation.append(pre_chip);
                                    }

                                }
                            }
                            msg_body["pre_operation"]=pre_operation;

                            string str_msg = writer.write(msg_body);

                            //向房间内的所有玩家发送消息周知
                            for(unsigned int k=0; k<((dealers_vector.at(room_number))->players_list).size(); k++)
                            {
                                DzpkSend((((dealers_vector.at(room_number))->players_list).at(k)).socket_fd,CMD_NEXT_TURN,(char *)str_msg.c_str(),strlen(str_msg.c_str()),dealers_vector[room_number]->dealer_id,atoi(((dealers_vector.at(room_number))->players_list).at(k).user_id.c_str()));
                            }

                            //等待操作定时器
                            timer.room_num = room_number;
                            timer.timer_type = 1;  //弃牌定时器
                            timer.start_time = time(NULL);
                            timer.time_out = (dealers_vector.at(room_number))->bet_time;//根据房间是快速房 还是普通房来决定超时时间
                            msg_body.removeMember("public_brand");
                            str_msg =   writer.write(msg_body);
                            memcpy(timer.szMsg,str_msg.c_str(),strlen(str_msg.c_str()));
                            timer.nMsgLen = strlen(str_msg.c_str());
                            g_dzpkTimer.Insert(timer.nTimerID,(void*)&timer,sizeof(dztimer));
                            dealers_vector.at(room_number)->nActionUserID = timer.nUserID;
                            WriteRoomLog(500,dealers_vector[room_number]->dealer_id,timer.nUserID," do next work set timer (%d) \n",timer.nTimerID);
                        }
                        else
                        {
                            //一轮完

                            //边池计算
                            edge_pool eg_pool;
                            eg_pool.edge_height=0;
                            vector<edge_pool> edge_pool_temp;
                            edge_pool_temp.push_back(eg_pool);
                            edge_pool_computer(dealers_vector,room_number,edge_pool_temp);

                            //清空所有参与玩家桌面注金
                            for(unsigned int m=0; m<((dealers_vector.at(room_number))->players_list).size(); m++)
                            {
                                if( (((dealers_vector.at(room_number))->players_list).at(m)).user_staus >= 2 )
                                {
                                    (((dealers_vector.at(room_number))->players_list).at(m)).desk_chip=0;
                                }
                            }
                            (dealers_vector.at(room_number))->follow_chip = (dealers_vector.at(room_number))->small_blind*2;//翻牌之后最小跟注
                            (dealers_vector.at(room_number))->look_flag = 0;//翻牌后，看牌的标识改为0

                            dztimer timer;
                            Json::FastWriter  writer;
                            Json::Value  msg_body;
                            Json::Value   open_card;
                            Json::Value   pool_st;
                            Json::Value  edge_desk_pool;
                            msg_body["msg_head"]="dzpk";
                            msg_body["game_number"]="dzpk";
                            msg_body["area_number"]="normal";
                            msg_body["act_commond"]=CMD_DRAW_CARD; //翻牌


                            open_card.append((dealers_vector.at(room_number))->desk_card[0]);
                            open_card.append((dealers_vector.at(room_number))->desk_card[1]);
                            open_card.append((dealers_vector.at(room_number))->desk_card[2]);
                            msg_body["public_brand"]=open_card;
                            for(unsigned int l=0; l<edge_pool_temp.size(); l++)
                            {
                                int pool=edge_pool_temp.at(l).edge_height *  edge_pool_temp.at(l).share_seat.size();
                                for(unsigned int j=0; j<edge_pool_temp.at(l).share_seat.size(); j++)
                                {
                                    if(  edge_pool_temp.at(l).share_seat.at(j) <=0 )
                                    {
                                        pool+= edge_pool_temp.at(l).share_seat.at(j);
                                    }
                                }
                                pool_st["pool"]=pool;//池子里总的注金数
                                pool_st["type"]=(edge_pool_temp.at(l)).pool_type;//池子的种类：1: 主池 0:边池
                                pool_st["id"]=(edge_pool_temp.at(l)).pool_id;//池子的id：0: 主池 >1:边池
                                edge_desk_pool.append(pool_st);

                            }
                            msg_body["desk_pool"]=edge_desk_pool;//本轮产生的主池和边池的筹码数序列
                            msg_body["step"]=2;


                            //如果小盲注没有站起、弃牌的话，翻牌后轮到他先下注（站起来之后，荷官上记录的小盲注位置更新）
                            for( unsigned int y=0; y<((dealers_vector.at(room_number))->players_list).size(); y++ )
                            {
                                if( (((dealers_vector.at(room_number))->players_list).at(y)).seat_num == (dealers_vector.at(room_number))->seat_small_blind  )
                                {
                                    if( (((dealers_vector.at(room_number))->players_list).at(y)).user_staus !=5 && (((dealers_vector.at(room_number))->players_list).at(y)).user_staus !=6  )
                                    {
                                        msg_body["next_turn"] = (dealers_vector.at(room_number))->seat_small_blind;

                                        (dealers_vector.at(room_number))->turn = (dealers_vector.at(room_number))->seat_small_blind;
                                        timer.socket_fd = (((dealers_vector.at(room_number))->players_list).at(y)).socket_fd;
                                        timer.seat_num =  (((dealers_vector.at(room_number))->players_list).at(y)).seat_num;
                                        memcpy(timer.szUserAccount,(((dealers_vector.at(room_number))->players_list).at(y)).user_account.c_str(),strlen((((dealers_vector.at(room_number))->players_list).at(y)).user_account.c_str()));
                                        //账号和当前时间一起用MD5加密
                                        MD5_CTX ctx;
                                        char* buf=(char*) ((((dealers_vector.at(room_number))->players_list).at(y)).user_account+itoa(get_current_time(),10)).c_str();
                                        unsigned char md[10];
                                        MD5_Init(&ctx);
                                        MD5_Update(&ctx,buf,strlen(buf));
                                        MD5_Final(md,&ctx);
                                        char aaa[10];
                                        for(unsigned int x=0; x<10; x++)
                                        {
                                            sprintf(aaa+x,"%x",md[x]);
                                        }

                                        timer.nTimerID = DzpkGetTimerID();
                                        msg_body["timer_id"]=GetString(timer.nTimerID);
                                        timer.nUserID = ((dealers_vector.at(room_number))->players_list).at(y).nUserID;



                                        int max_follow_chip=0;
                                        for(unsigned int m=0; m<((dealers_vector.at(room_number))->players_list).size(); m++)
                                        {
                                            if(  (((dealers_vector.at(room_number))->players_list).at(m)).nUserID != (((dealers_vector.at(room_number))->players_list).at(y)).nUserID) //除自己之外
                                            {
                                                if(  (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 2 || (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 3 || (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 4 || (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 6 ) //仍然可以下注的玩家
                                                {
                                                    if( max_follow_chip < (((dealers_vector.at(room_number))->players_list).at(m)).hand_chips + (((dealers_vector.at(room_number))->players_list).at(m)).desk_chip )
                                                    {
                                                        max_follow_chip = (((dealers_vector.at(room_number))->players_list).at(m)).hand_chips + (((dealers_vector.at(room_number))->players_list).at(m)).desk_chip ;
                                                    }
                                                }
                                            }
                                        }



                                        //除自己外，剩余玩家的手上筹码最多的一位，若大于自己手上筹码时，最大跟注为手上的总筹码数
                                        if( max_follow_chip >= (((dealers_vector.at(room_number))->players_list).at(y)).hand_chips + (((dealers_vector.at(room_number))->players_list).at(y)).desk_chip )
                                        {
                                            max_follow_chip = (((dealers_vector.at(room_number))->players_list).at(y)).hand_chips;
                                            msg_body["max_follow_chip"] = max_follow_chip;
                                            (dealers_vector.at(room_number))->nTurnMaxChip = max_follow_chip;
                                        }
                                        else
                                        {
                                            max_follow_chip = max_follow_chip - (((dealers_vector.at(room_number))->players_list).at(y)).desk_chip;
                                            msg_body["max_follow_chip"] = max_follow_chip;
                                            (dealers_vector.at(room_number))->nTurnMaxChip = max_follow_chip;
                                        }

                                        (dealers_vector.at(room_number))->add_chip_manual_flg = 0;
                                        int add_chipNow = 2*(dealers_vector.at(room_number))->small_blind;
                                        if(add_chipNow >= max_follow_chip)
                                        {
                                            add_chipNow = max_follow_chip;
                                        }
                                        msg_body["default_add_chip"]=add_chipNow;

                                    }
                                    else
                                    {
                                        unsigned int next_p = next_player((dealers_vector.at(room_number))->seat_small_blind,dealers_vector,room_number);
                                        msg_body["next_turn"] = next_p;
                                        (dealers_vector.at(room_number))->turn = next_p;
                                        for( unsigned int j=0; j<((dealers_vector.at(room_number))->players_list).size(); j++ )
                                        {
                                            if( ((dealers_vector.at(room_number))->players_list).at(j).seat_num == (dealers_vector.at(room_number))->turn  )
                                            {
                                                timer.socket_fd = ((dealers_vector.at(room_number))->players_list).at(j).socket_fd;
                                                timer.seat_num = ((dealers_vector.at(room_number))->players_list).at(j).seat_num;
                                                memcpy(timer.szUserAccount,((dealers_vector.at(room_number))->players_list).at(j).user_account.c_str(),strlen(((dealers_vector.at(room_number))->players_list).at(j).user_account.c_str()));
                                                //账号和当前时间一起用MD5加密
                                                MD5_CTX ctx;
                                                char* buf=(char*) (((dealers_vector.at(room_number))->players_list).at(j).user_account+itoa(get_current_time(),10)).c_str();
                                                unsigned char md[10];
                                                MD5_Init(&ctx);
                                                MD5_Update(&ctx,buf,strlen(buf));
                                                MD5_Final(md,&ctx);
                                                char aaa[10];
                                                for(unsigned int x=0; x<10; x++)
                                                {
                                                    sprintf(aaa+x,"%x",md[x]);
                                                }

                                                timer.nTimerID = DzpkGetTimerID();
                                                msg_body["timer_id"] = GetString(timer.nTimerID);
                                                timer.nUserID = ((dealers_vector.at(room_number))->players_list).at(j).nUserID;

                                                int max_follow_chip=0;
                                                for(unsigned int m=0; m<((dealers_vector.at(room_number))->players_list).size(); m++)
                                                {
                                                    if(  (((dealers_vector.at(room_number))->players_list).at(m)).nUserID != (((dealers_vector.at(room_number))->players_list).at(y)).nUserID) //除自己之外
                                                    {
                                                        if(  (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 2 || (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 3 || (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 4 ||  (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 6 ) //仍然可以下注的玩家
                                                        {
                                                            if( max_follow_chip < (((dealers_vector.at(room_number))->players_list).at(m)).hand_chips + (((dealers_vector.at(room_number))->players_list).at(m)).desk_chip )
                                                            {
                                                                max_follow_chip = (((dealers_vector.at(room_number))->players_list).at(m)).hand_chips  + (((dealers_vector.at(room_number))->players_list).at(m)).desk_chip;
                                                            }
                                                        }
                                                    }
                                                }


                                                //除自己外，剩余玩家的手上筹码最多的一位，若大于自己手上筹码时，最大跟注为手上的总筹码数
                                                if( max_follow_chip >= ((dealers_vector.at(room_number))->players_list).at(j).hand_chips + ((dealers_vector.at(room_number))->players_list).at(j).desk_chip )
                                                {
                                                    max_follow_chip = ((dealers_vector.at(room_number))->players_list).at(j).hand_chips;
                                                    msg_body["max_follow_chip"] = max_follow_chip;
                                                    (dealers_vector.at(room_number))->nTurnMaxChip = max_follow_chip;
                                                }
                                                else
                                                {
                                                    max_follow_chip = max_follow_chip - ((dealers_vector.at(room_number))->players_list).at(j).desk_chip;
                                                    msg_body["max_follow_chip"] = max_follow_chip;
                                                    (dealers_vector.at(room_number))->nTurnMaxChip = max_follow_chip;
                                                }

                                                (dealers_vector.at(room_number))->add_chip_manual_flg = 0;
                                                int add_chipNow = 2*(dealers_vector.at(room_number))->small_blind;

                                                if(add_chipNow >= max_follow_chip)
                                                {
                                                    add_chipNow = max_follow_chip;
                                                }
                                                msg_body["default_add_chip"]=add_chipNow;

                                            }
                                        }
                                    }
                                }
                            }

                            //判断最小跟注= 当前最小跟注
                            msg_body["follow_chip"]=0;
                            (dealers_vector.at(room_number))->nTurnMinChip = 0;

                            //计算操作权限
                            Json::Value  rights;
                            rights.append(0);
                            rights.append(2);
                            rights.append(3);// 0:加注 1：跟注  2：看牌  3：弃牌

                            msg_body["ctl_rights"]=rights;

                            for(unsigned int k=0; k<((dealers_vector.at(room_number))->players_list).size(); k++)
                            {
                                string str_msg = writer.write(msg_body);

                                DzpkSend( (((dealers_vector.at(room_number))->players_list).at(k)).socket_fd,CMD_DRAW_CARD,(char *)str_msg.c_str(),strlen(str_msg.c_str()),dealers_vector[room_number]->dealer_id,atoi(((dealers_vector.at(room_number))->players_list).at(k).user_id.c_str()));
                            }
                            (dealers_vector.at(room_number))->step =2;

                            //等待操作定时器
                            timer.room_num = room_number;
                            timer.timer_type = 1;  //弃牌定时器
                            timer.start_time = time(NULL);
                            timer.time_out = (dealers_vector.at(room_number))->bet_time;//根据房间是快速房 还是普通房来决定超时时间
                            msg_body.removeMember("public_brand");
                            string   str_msg =   writer.write(msg_body);
                            memcpy(timer.szMsg,str_msg.c_str(),strlen(str_msg.c_str()));
                            timer.nMsgLen = strlen(str_msg.c_str());
                            g_dzpkTimer.Insert(timer.nTimerID,(void*)&timer,sizeof(dztimer));
                            dealers_vector.at(room_number)->nActionUserID = timer.nUserID;
                            WriteRoomLog(500,dealers_vector[room_number]->dealer_id ,timer.nUserID," do next work set timer (%d) (%d) \n",timer.nTimerID,timer.nUserID);
                        }

                    }
                    else if ( (dealers_vector.at(room_number))->step == 2)
                    {
                        //边池计算
                        edge_pool eg_pool;
                        eg_pool.edge_height=0;
                        vector<edge_pool> edge_pool_temp;
                        edge_pool_temp.push_back(eg_pool);
                        edge_pool_computer(dealers_vector,room_number,edge_pool_temp);

                        //清空所有参与玩家桌面注金
                        for(unsigned int m=0; m<((dealers_vector.at(room_number))->players_list).size(); m++)
                        {
                            if( (((dealers_vector.at(room_number))->players_list).at(m)).user_staus >= 2 )
                            {
                                (((dealers_vector.at(room_number))->players_list).at(m)).desk_chip=0;
                            }
                        }
                        (dealers_vector.at(room_number))->follow_chip = (dealers_vector.at(room_number))->small_blind*2;//翻牌之后最小跟注
                        (dealers_vector.at(room_number))->look_flag = 0;//翻牌后，看牌的标识改为0

                        dztimer  timer;
                        Json::FastWriter  writer;
                        Json::Value  msg_body;
                        Json::Value  open_card;
                        Json::Value  pool_st;
                        Json::Value  edge_desk_pool;
                        msg_body.clear();
                        msg_body["msg_head"]="dzpk";
                        msg_body["game_number"]="dzpk";
                        msg_body["area_number"]="normal";
                        msg_body["act_commond"]=CMD_DRAW_CARD; //翻牌

                        open_card.append((dealers_vector.at(room_number))->desk_card[3]);
                        msg_body["public_brand"]=open_card;
                        msg_body["step"]=3;

                        for(unsigned int n=0; n<edge_pool_temp.size(); n++)
                        {
                            int pool=edge_pool_temp.at(n).edge_height *  edge_pool_temp.at(n).share_seat.size();
                            for(unsigned int j=0; j<edge_pool_temp.at(n).share_seat.size(); j++)
                            {
                                if(  edge_pool_temp.at(n).share_seat.at(j) <=0 )
                                {
                                    pool+= edge_pool_temp.at(n).share_seat.at(j);
                                }
                            }

                            pool_st["pool"]=pool;//池子里总的注金数
                            pool_st["type"]=(edge_pool_temp.at(n)).pool_type;//池子的种类：1: 主池 0:边池
                            pool_st["id"]=(edge_pool_temp.at(n)).pool_id;//池子的id：0: 主池 >1:边池
                            edge_desk_pool.append(pool_st);

                        }


                        msg_body["desk_pool"]=edge_desk_pool;//本轮产生的主池和边池的筹码数序列

                        //如果小盲注没有弃牌的话，翻牌后轮到他先下注
                        for( unsigned int y=0; y<((dealers_vector.at(room_number))->players_list).size(); y++ )
                        {
                            if( (((dealers_vector.at(room_number))->players_list).at(y)).seat_num == (dealers_vector.at(room_number))->seat_small_blind  )
                            {
                                if( (((dealers_vector.at(room_number))->players_list).at(y)).user_staus !=5 && (((dealers_vector.at(room_number))->players_list).at(y)).user_staus !=6  )
                                {
                                    msg_body["next_turn"] = (dealers_vector.at(room_number))->seat_small_blind;
                                    (dealers_vector.at(room_number))->turn = (dealers_vector.at(room_number))->seat_small_blind;
                                    timer.socket_fd = (((dealers_vector.at(room_number))->players_list).at(y)).socket_fd;
                                    timer.seat_num = (((dealers_vector.at(room_number))->players_list).at(y)).seat_num;
                                    memcpy(timer.szUserAccount,(((dealers_vector.at(room_number))->players_list).at(y)).user_account.c_str(),strlen((((dealers_vector.at(room_number))->players_list).at(y)).user_account.c_str()));
                                    //账号和当前时间一起用MD5加密
                                    MD5_CTX ctx;
                                    char* buf=(char*) ((((dealers_vector.at(room_number))->players_list).at(y)).user_account+itoa(get_current_time(),10)).c_str();
                                    unsigned char md[10];
                                    MD5_Init(&ctx);
                                    MD5_Update(&ctx,buf,strlen(buf));
                                    MD5_Final(md,&ctx);
                                    char aaa[10];
                                    for(unsigned int x=0; x<10; x++)
                                    {
                                        sprintf(aaa+x,"%x",md[x]);
                                    }

                                    timer.nTimerID = DzpkGetTimerID();
                                    msg_body["timer_id"]=GetString(timer.nTimerID);
                                    timer.nUserID = (((dealers_vector.at(room_number))->players_list).at(y)).nUserID;



                                    int max_follow_chip=0;
                                    for(unsigned int m=0; m<((dealers_vector.at(room_number))->players_list).size(); m++)
                                    {
                                        if(  (((dealers_vector.at(room_number))->players_list).at(m)).nUserID != (((dealers_vector.at(room_number))->players_list).at(y)).nUserID) //除自己之外
                                        {
                                            if(  (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 2 || (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 3 || (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 4 || (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 6) //仍然可以下注的玩家
                                            {
                                                if( max_follow_chip <  (((dealers_vector.at(room_number))->players_list).at(m)).hand_chips + (((dealers_vector.at(room_number))->players_list).at(m)).desk_chip )
                                                {
                                                    max_follow_chip =   (((dealers_vector.at(room_number))->players_list).at(m)).hand_chips + (((dealers_vector.at(room_number))->players_list).at(m)).desk_chip ;
                                                }
                                            }
                                        }
                                    }

                                    //除自己外，剩余玩家的手上筹码最多的一位，若大于自己手上筹码时，最大跟注为手上的总筹码数
                                    if( max_follow_chip >= (((dealers_vector.at(room_number))->players_list).at(y)).hand_chips + (((dealers_vector.at(room_number))->players_list).at(y)).desk_chip )
                                    {
                                        max_follow_chip = (((dealers_vector.at(room_number))->players_list).at(y)).hand_chips;
                                        msg_body["max_follow_chip"] = max_follow_chip;
                                        (dealers_vector.at(room_number))->nTurnMaxChip = max_follow_chip;
                                    }
                                    else
                                    {
                                        max_follow_chip = max_follow_chip - (((dealers_vector.at(room_number))->players_list).at(y)).desk_chip;
                                        msg_body["max_follow_chip"] = max_follow_chip;
                                        (dealers_vector.at(room_number))->nTurnMaxChip = max_follow_chip;
                                    }

                                    (dealers_vector.at(room_number))->add_chip_manual_flg = 0;
                                    int add_chipNow = 2*(dealers_vector.at(room_number))->small_blind;

                                    if(add_chipNow >= max_follow_chip)
                                    {
                                        add_chipNow = max_follow_chip;
                                    }
                                    msg_body["default_add_chip"]=add_chipNow;


                                }
                                else
                                {
                                    unsigned int next_pl = next_player((dealers_vector.at(room_number))->seat_small_blind,dealers_vector,room_number);
                                    msg_body["next_turn"] = next_pl;
                                    (dealers_vector.at(room_number))->turn = next_pl;
                                    for( unsigned int j=0; j<((dealers_vector.at(room_number))->players_list).size(); j++ )
                                    {
                                        if( (((dealers_vector.at(room_number))->players_list).at(j)).seat_num == (dealers_vector.at(room_number))->turn  )
                                        {
                                            timer.socket_fd = (((dealers_vector.at(room_number))->players_list).at(j)).socket_fd;
                                            timer.seat_num = (((dealers_vector.at(room_number))->players_list).at(j)).seat_num;
                                            memcpy(timer.szUserAccount,(((dealers_vector.at(room_number))->players_list).at(j)).user_account.c_str(),strlen((((dealers_vector.at(room_number))->players_list).at(j)).user_account.c_str()));
                                            //账号和当前时间一起用MD5加密
                                            MD5_CTX ctx;
                                            char* buf=(char*) ((((dealers_vector.at(room_number))->players_list).at(j)).user_account+itoa(get_current_time(),10)).c_str();
                                            unsigned char md[10];
                                            MD5_Init(&ctx);
                                            MD5_Update(&ctx,buf,strlen(buf));
                                            MD5_Final(md,&ctx);
                                            char aaa[10];
                                            for(unsigned int x=0; x<10; x++)
                                            {
                                                sprintf(aaa+x,"%x",md[x]);
                                            }

                                            timer.nTimerID = DzpkGetTimerID();
                                            msg_body["timer_id"]=GetString(timer.nTimerID);
                                            timer.nUserID = (((dealers_vector.at(room_number))->players_list).at(j)).nUserID;


                                            int max_follow_chip=0;
                                            for(unsigned int m=0; m<((dealers_vector.at(room_number))->players_list).size(); m++)
                                            {
                                                if(  (((dealers_vector.at(room_number))->players_list).at(m)).user_id != (((dealers_vector.at(room_number))->players_list).at(j)).user_id  ) //除自己之外
                                                {
                                                    if(  (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 2 || (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 3 || (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 4  || (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 6 ) //仍然在牌局当中的玩家
                                                    {
                                                        if( max_follow_chip <  (((dealers_vector.at(room_number))->players_list).at(m)).hand_chips + (((dealers_vector.at(room_number))->players_list).at(m)).desk_chip )
                                                        {
                                                            max_follow_chip =   (((dealers_vector.at(room_number))->players_list).at(m)).hand_chips + (((dealers_vector.at(room_number))->players_list).at(m)).desk_chip;
                                                        }
                                                    }
                                                }
                                            }

                                            //除自己外，剩余玩家的手上筹码最多的一位，若大于自己手上筹码时，最大跟注为手上的总筹码数
                                            if( max_follow_chip >= (((dealers_vector.at(room_number))->players_list).at(j)).hand_chips + (((dealers_vector.at(room_number))->players_list).at(j)).desk_chip )
                                            {
                                                max_follow_chip = (((dealers_vector.at(room_number))->players_list).at(j)).hand_chips;
                                                msg_body["max_follow_chip"] = max_follow_chip;
                                                (dealers_vector.at(room_number))->nTurnMaxChip = max_follow_chip;
                                            }
                                            else
                                            {
                                                max_follow_chip = max_follow_chip - (((dealers_vector.at(room_number))->players_list).at(j)).desk_chip;
                                                msg_body["max_follow_chip"] = max_follow_chip;
                                                (dealers_vector.at(room_number))->nTurnMaxChip = max_follow_chip;
                                            }

                                            (dealers_vector.at(room_number))->add_chip_manual_flg = 0;
                                            int add_chipNow = 2*(dealers_vector.at(room_number))->small_blind;

                                            if(add_chipNow >= max_follow_chip)
                                            {
                                                add_chipNow = max_follow_chip;
                                            }
                                            msg_body["default_add_chip"]=add_chipNow;

                                        }
                                    }
                                }
                            }

                        }

                        //判断最小跟注= 当前最小跟注
                        msg_body["follow_chip"]=0;
                        (dealers_vector.at(room_number))->nTurnMinChip = 0;

                        //计算操作权限
                        Json::Value  rights;
                        rights.append(0);
                        rights.append(2);
                        rights.append(3);// 0:加注 1：跟注  2：看牌  3：弃牌
                        msg_body["ctl_rights"]=rights;

                        for(unsigned int k=0; k<((dealers_vector.at(room_number))->players_list).size(); k++)
                        {
                            string   str_msg =   writer.write(msg_body);
                            DzpkSend( (((dealers_vector.at(room_number))->players_list).at(k)).socket_fd,CMD_DRAW_CARD,(char *)str_msg.c_str(),strlen(str_msg.c_str()),dealers_vector[room_number]->dealer_id,atoi(((dealers_vector.at(room_number))->players_list).at(k).user_id.c_str()));
                        }
                        (dealers_vector.at(room_number))->step =3;

                        //将看牌的玩家状态改为跟注
                        for(unsigned int m=0; m<((dealers_vector.at(room_number))->players_list).size(); m++)
                        {
                            if( (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 4)
                            {
                                (((dealers_vector.at(room_number))->players_list).at(m)).user_staus= 3;
                            }
                        }

                        //等待操作定时器
                        timer.room_num = room_number;
                        timer.timer_type = 1;  //弃牌定时器
                        timer.start_time = time(NULL);
                        timer.time_out = (dealers_vector.at(room_number))->bet_time;//根据房间是快速房 还是普通房来决定超时时间
                        msg_body.removeMember("public_brand");
                        string   str_msg =   writer.write(msg_body);
                        memcpy(timer.szMsg,str_msg.c_str(),strlen(str_msg.c_str()));
                        timer.nMsgLen = strlen(str_msg.c_str());
                        g_dzpkTimer.Insert(timer.nTimerID,(void*)&timer,sizeof(dztimer));
                        dealers_vector.at(room_number)->nActionUserID = timer.nUserID;
                        WriteRoomLog(500,dealers_vector[room_number]->dealer_id ,timer.nUserID," do next work set timer (%d)  \n",timer.nTimerID);
                    }
                    else if ( (dealers_vector.at(room_number))->step == 3)
                    {
                        //边池计算
                        edge_pool eg_pool;
                        eg_pool.edge_height=0;
                        vector<edge_pool> edge_pool_temp;
                        edge_pool_temp.push_back(eg_pool);
                        edge_pool_computer(dealers_vector,room_number,edge_pool_temp);

                        //清空所有参与玩家桌面注金
                        for(unsigned int m=0; m<((dealers_vector.at(room_number))->players_list).size(); m++)
                        {
                            if( (((dealers_vector.at(room_number))->players_list).at(m)).user_staus >= 2 )
                            {
                                (((dealers_vector.at(room_number))->players_list).at(m)).desk_chip=0;
                            }
                        }
                        (dealers_vector.at(room_number))->follow_chip = (dealers_vector.at(room_number))->small_blind*2;//翻牌之后最小跟注
                        (dealers_vector.at(room_number))->look_flag = 0;//翻牌后，看牌的标识改为0

                        dztimer  timer;
                        Json::FastWriter  writer;
                        Json::Value  msg_body;
                        Json::Value  open_card;
                        Json::Value  pool_st;
                        Json::Value  edge_desk_pool;
                        msg_body.clear();
                        msg_body["msg_head"]="dzpk";
                        msg_body["game_number"]="dzpk";
                        msg_body["area_number"]="normal";
                        msg_body["act_commond"]=CMD_DRAW_CARD; //翻牌

                        open_card.append((dealers_vector.at(room_number))->desk_card[4]);
                        msg_body["public_brand"]=open_card;
                        msg_body["step"]=4;

                        for(unsigned int n=0; n<edge_pool_temp.size(); n++)
                        {
                            int pool=edge_pool_temp.at(n).edge_height *  edge_pool_temp.at(n).share_seat.size();
                            for(unsigned int j=0; j<edge_pool_temp.at(n).share_seat.size(); j++)
                            {
                                if(  edge_pool_temp.at(n).share_seat.at(j) <=0 )
                                {
                                    pool+= edge_pool_temp.at(n).share_seat.at(j);
                                }
                            }

                            pool_st["pool"]=pool;//池子里总的注金数
                            pool_st["type"]=(edge_pool_temp.at(n)).pool_type;//池子的种类：1: 主池 0:边池
                            pool_st["id"]=(edge_pool_temp.at(n)).pool_id;//池子的id：0: 主池 >1:边池
                            edge_desk_pool.append(pool_st);

                        }


                        msg_body["desk_pool"]=edge_desk_pool;//本轮产生的主池和边池的筹码数序列

                        //如果小盲注没有弃牌的话，翻牌后轮到他先下注
                        for( unsigned int y=0; y<((dealers_vector.at(room_number))->players_list).size(); y++ )
                        {
                            if( (((dealers_vector.at(room_number))->players_list).at(y)).seat_num == (dealers_vector.at(room_number))->seat_small_blind  )
                            {
                                if( (((dealers_vector.at(room_number))->players_list).at(y)).user_staus !=5 && (((dealers_vector.at(room_number))->players_list).at(y)).user_staus !=6  )
                                {
                                    msg_body["next_turn"] = (dealers_vector.at(room_number))->seat_small_blind;
                                    (dealers_vector.at(room_number))->turn = (dealers_vector.at(room_number))->seat_small_blind;
                                    timer.socket_fd = (((dealers_vector.at(room_number))->players_list).at(y)).socket_fd;
                                    timer.seat_num = (((dealers_vector.at(room_number))->players_list).at(y)).seat_num;
                                    memcpy(timer.szUserAccount,(((dealers_vector.at(room_number))->players_list).at(y)).user_account.c_str(),strlen((((dealers_vector.at(room_number))->players_list).at(y)).user_account.c_str()));
                                    //账号和当前时间一起用MD5加密
                                    MD5_CTX ctx;
                                    char* buf=(char*) ((((dealers_vector.at(room_number))->players_list).at(y)).user_account+itoa(get_current_time(),10)).c_str();
                                    unsigned char md[10];
                                    MD5_Init(&ctx);
                                    MD5_Update(&ctx,buf,strlen(buf));
                                    MD5_Final(md,&ctx);
                                    char aaa[10];
                                    for(unsigned int x=0; x<10; x++)
                                    {
                                        sprintf(aaa+x,"%x",md[x]);
                                    }

                                    timer.nTimerID = DzpkGetTimerID();
                                    msg_body["timer_id"]=GetString(timer.nTimerID);
                                    timer.nUserID = (((dealers_vector.at(room_number))->players_list).at(y)).nUserID;



                                    int max_follow_chip=0;
                                    for(unsigned int m=0; m<((dealers_vector.at(room_number))->players_list).size(); m++)
                                    {
                                        if(  (((dealers_vector.at(room_number))->players_list).at(m)).nUserID != (((dealers_vector.at(room_number))->players_list).at(y)).nUserID  ) //除自己之外
                                        {
                                            if(  (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 2 || (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 3 || (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 4 || (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 6) //仍然可以下注的玩家
                                            {
                                                if( max_follow_chip <  (((dealers_vector.at(room_number))->players_list).at(m)).hand_chips + (((dealers_vector.at(room_number))->players_list).at(m)).desk_chip )
                                                {
                                                    max_follow_chip =   (((dealers_vector.at(room_number))->players_list).at(m)).hand_chips + (((dealers_vector.at(room_number))->players_list).at(m)).desk_chip ;
                                                }
                                            }
                                        }
                                    }

                                    //除自己外，剩余玩家的手上筹码最多的一位，若大于自己手上筹码时，最大跟注为手上的总筹码数
                                    if( max_follow_chip >= (((dealers_vector.at(room_number))->players_list).at(y)).hand_chips + (((dealers_vector.at(room_number))->players_list).at(y)).desk_chip )
                                    {
                                        max_follow_chip = (((dealers_vector.at(room_number))->players_list).at(y)).hand_chips;
                                        msg_body["max_follow_chip"] = max_follow_chip;
                                        (dealers_vector.at(room_number))->nTurnMaxChip = max_follow_chip;
                                    }
                                    else
                                    {
                                        max_follow_chip = max_follow_chip - (((dealers_vector.at(room_number))->players_list).at(y)).desk_chip;
                                        msg_body["max_follow_chip"] = max_follow_chip;
                                        (dealers_vector.at(room_number))->nTurnMaxChip = max_follow_chip;
                                    }

                                    (dealers_vector.at(room_number))->add_chip_manual_flg = 0;
                                    int add_chipNow = 2*(dealers_vector.at(room_number))->small_blind;

                                    if(add_chipNow >= max_follow_chip)
                                    {
                                        add_chipNow = max_follow_chip;
                                    }
                                    msg_body["default_add_chip"]=add_chipNow;


                                }
                                else
                                {
                                    unsigned int next_pl = next_player((dealers_vector.at(room_number))->seat_small_blind,dealers_vector,room_number);
                                    msg_body["next_turn"] = next_pl;
                                    (dealers_vector.at(room_number))->turn = next_pl;
                                    for( unsigned int j=0; j<((dealers_vector.at(room_number))->players_list).size(); j++ )
                                    {
                                        if( (((dealers_vector.at(room_number))->players_list).at(j)).seat_num == (dealers_vector.at(room_number))->turn  )
                                        {
                                            timer.socket_fd = (((dealers_vector.at(room_number))->players_list).at(j)).socket_fd;
                                            timer.seat_num = (((dealers_vector.at(room_number))->players_list).at(j)).seat_num;
                                            memcpy(timer.szUserAccount, (((dealers_vector.at(room_number))->players_list).at(j)).user_account.c_str(),strlen((((dealers_vector.at(room_number))->players_list).at(j)).user_account.c_str()));
                                            //账号和当前时间一起用MD5加密
                                            MD5_CTX ctx;
                                            char* buf=(char*) ((((dealers_vector.at(room_number))->players_list).at(j)).user_account+itoa(get_current_time(),10)).c_str();
                                            unsigned char md[10];
                                            MD5_Init(&ctx);
                                            MD5_Update(&ctx,buf,strlen(buf));
                                            MD5_Final(md,&ctx);
                                            char aaa[10];
                                            for(unsigned int x=0; x<10; x++)
                                            {
                                                sprintf(aaa+x,"%x",md[x]);
                                            }

                                            timer.nTimerID = DzpkGetTimerID();
                                            msg_body["timer_id"]=GetString(timer.nTimerID);
                                            timer.nUserID = (((dealers_vector.at(room_number))->players_list).at(j)).nUserID;


                                            int max_follow_chip=0;
                                            for(unsigned int m=0; m<((dealers_vector.at(room_number))->players_list).size(); m++)
                                            {
                                                if(  (((dealers_vector.at(room_number))->players_list).at(m)).user_id != (((dealers_vector.at(room_number))->players_list).at(j)).user_id  ) //除自己之外
                                                {
                                                    if(  (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 2 || (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 3 || (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 4 || (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 6 ) //仍然在牌局当中的玩家
                                                    {
                                                        if( max_follow_chip <  (((dealers_vector.at(room_number))->players_list).at(m)).hand_chips + (((dealers_vector.at(room_number))->players_list).at(m)).desk_chip )
                                                        {
                                                            max_follow_chip =   (((dealers_vector.at(room_number))->players_list).at(m)).hand_chips + (((dealers_vector.at(room_number))->players_list).at(m)).desk_chip;
                                                        }
                                                    }
                                                }
                                            }

                                            //除自己外，剩余玩家的手上筹码最多的一位，若大于自己手上筹码时，最大跟注为手上的总筹码数
                                            if( max_follow_chip >= (((dealers_vector.at(room_number))->players_list).at(j)).hand_chips + (((dealers_vector.at(room_number))->players_list).at(j)).desk_chip )
                                            {
                                                max_follow_chip = (((dealers_vector.at(room_number))->players_list).at(j)).hand_chips;
                                                msg_body["max_follow_chip"] = max_follow_chip;
                                                (dealers_vector.at(room_number))->nTurnMaxChip = max_follow_chip;
                                            }
                                            else
                                            {
                                                max_follow_chip = max_follow_chip - (((dealers_vector.at(room_number))->players_list).at(j)).desk_chip;
                                                msg_body["max_follow_chip"] = max_follow_chip;
                                                (dealers_vector.at(room_number))->nTurnMaxChip = max_follow_chip;
                                            }

                                            (dealers_vector.at(room_number))->add_chip_manual_flg = 0;
                                            int add_chipNow = 2*(dealers_vector.at(room_number))->small_blind;

                                            if(add_chipNow >= max_follow_chip)
                                            {
                                                add_chipNow = max_follow_chip;
                                            }
                                            msg_body["default_add_chip"]=add_chipNow;

                                        }
                                    }
                                }
                            }

                        }

                        //判断最小跟注= 当前最小跟注
                        msg_body["follow_chip"]=0;
                        (dealers_vector.at(room_number))->nTurnMinChip = 0;

                        //计算操作权限
                        Json::Value  rights;
                        rights.append(0);
                        rights.append(2);
                        rights.append(3);// 0:加注 1：跟注  2：看牌  3：弃牌
                        msg_body["ctl_rights"]=rights;


                        for(unsigned int k=0; k<((dealers_vector.at(room_number))->players_list).size(); k++)
                        {
                            string   str_msg =   writer.write(msg_body);
                            DzpkSend( (((dealers_vector.at(room_number))->players_list).at(k)).socket_fd,CMD_DRAW_CARD,(char *)str_msg.c_str(),strlen(str_msg.c_str()),dealers_vector[room_number]->dealer_id,atoi(((dealers_vector.at(room_number))->players_list).at(k).user_id.c_str()));
                        }
                        (dealers_vector.at(room_number))->step =4;

                        //将看牌的玩家状态改为跟注
                        for(unsigned int m=0; m<((dealers_vector.at(room_number))->players_list).size(); m++)
                        {
                            if( (((dealers_vector.at(room_number))->players_list).at(m)).user_staus == 4)
                            {
                                (((dealers_vector.at(room_number))->players_list).at(m)).user_staus= 3;
                            }
                        }

                        //等待操作定时器
                        timer.room_num = room_number;
                        timer.timer_type = 1;  //弃牌定时器
                        timer.start_time = time(NULL);
                        timer.time_out = (dealers_vector.at(room_number))->bet_time;//根据房间是快速房 还是普通房来决定超时时间
                        msg_body.removeMember("public_brand");
                        string   str_msg =   writer.write(msg_body);
                        memcpy(timer.szMsg,str_msg.c_str(),strlen(str_msg.c_str()));
                        timer.nMsgLen = strlen(str_msg.c_str());
                        g_dzpkTimer.Insert(timer.nTimerID,(void*)&timer,sizeof(dztimer));
                        dealers_vector.at(room_number)->nActionUserID = timer.nUserID;
                        WriteRoomLog(500,dealers_vector[room_number]->dealer_id ,timer.nUserID," do next work set timer (%d)  \n",timer.nTimerID);
                    }
                    else if (  (dealers_vector.at(room_number))->step == 4 )
                    {
                        //隐藏光圈
                        StdStopHideLightRing(dealers_vector[room_number]);

                        //今天玩牌次数加一
                        StdTodayPlayTimes(dealers_vector.at(room_number));

                        //边池计算
                        edge_pool eg_pool;
                        eg_pool.edge_height=0;
                        vector<edge_pool> edge_pool_temp;
                        edge_pool_temp.push_back(eg_pool);
                        edge_pool_computer(dealers_vector,room_number,edge_pool_temp);


                        //清空所有参与玩家桌面注金
                        for(unsigned int m=0; m<((dealers_vector.at(room_number))->players_list).size(); m++)
                        {
                            if( (((dealers_vector.at(room_number))->players_list).at(m)).user_staus >= 2 )
                            {
                                (((dealers_vector.at(room_number))->players_list).at(m)).desk_chip=0;
                            }
                        }
                        (dealers_vector.at(room_number))->follow_chip = (dealers_vector.at(room_number))->small_blind*2;//翻牌之后最小跟注
                        (dealers_vector.at(room_number))->look_flag = 0;//翻牌后，看牌的标识改为0
                        (dealers_vector.at(room_number))->nStepOver = 1;//最后一轮已经跟满注，状态改为1


                        //牌面虚拟值计算
                        computer_card_value(dealers_vector,room_number);

                        //幸运手牌活动
                        StdLuckyCardActivity(dealers_vector[room_number],1);

                        //赢牌计算
                        vector<winer_pool>  winder_list;
                        computer_winer_card(dealers_vector,room_number,winder_list);
                        update_user_best_win_card(dealers_vector,room_number,connpool);

                        //向房间内的所有玩家发送赢牌消息
                        Json::FastWriter  writer;
                        Json::Value  msg_body;
                        Json::Value  pool_st;
                        Json::Value  edge_desk_pool;
                        Json::Value  edge_user_list;
                        Json::Value  hand_cards_all;
                        Json::Value  chip_account_list;
                        Json::Value  experience_change_list;


                        msg_body.clear();
                        msg_body["msg_head"]="dzpk";
                        msg_body["game_number"]="dzpk";
                        msg_body["area_number"]="normal";
                        msg_body["act_commond"]=CMD_WIN_LIST;

                        //手牌-座位号对应列表
                        StdPutResultShowHandCards(dealers_vector[room_number],hand_cards_all,winder_list,(dealers_vector.at(room_number))->edge_pool_list);


                        msg_body["hand_cards_all"]=hand_cards_all;

                        for(unsigned int i=0; i<edge_pool_temp.size(); i++)
                        {
                            int pool=edge_pool_temp.at(i).edge_height *  edge_pool_temp.at(i).share_seat.size();
                            for(unsigned int j=0; j<edge_pool_temp.at(i).share_seat.size(); j++)
                            {
                                if(  edge_pool_temp.at(i).share_seat.at(j) <=0 )
                                {
                                    pool+= edge_pool_temp.at(i).share_seat.at(j);
                                }
                            }
                            pool_st["pool"]=pool;//池子里总的注金数
                            pool_st["type"]=(edge_pool_temp.at(i)).pool_type;//池子的种类：1: 主池 0:边池
                            pool_st["id"]=(edge_pool_temp.at(i)).pool_id;//池子的id：0: 主池 >1:边池
                            edge_desk_pool.append(pool_st);

                        }
                        msg_body["desk_pool"]=edge_desk_pool;//本轮产生的主池和边池的筹码数序列
                        for(  unsigned int n=0; n<winder_list.size(); n++ )
                        {
                            Json::Value  edge_number;
                            Json::Value  pool;
                            Json::Value  card_mate;
                            int seat_number = winder_list.at(n).seat_number;
                            for( unsigned int j=0; j<((dealers_vector.at(room_number))->players_list).size(); j++ )
                            {
                                if( ((dealers_vector.at(room_number))->players_list).at(j).seat_num == seat_number)
                                {
                                    edge_number["user_id"] = ((dealers_vector.at(room_number))->players_list).at(j).user_account ;
                                    edge_number["seat_num"] = ((dealers_vector.at(room_number))->players_list).at(j).seat_num ;

                                    if( ((dealers_vector.at(room_number))->players_list).at(j).win_cards.size() == 6)
                                    {
                                        edge_number["card_type"] = ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5);
                                        for(unsigned int x=0; x<5; x++ )
                                        {
                                            int card_va = ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(x);

                                            //先到公共牌中找
                                            for(unsigned int y=0; y< 5; y++  )
                                            {
                                                if( card_va == (dealers_vector.at(room_number))->desk_card[y] )
                                                {
                                                    card_mate.append(y);
                                                }
                                            }
                                            //再到手牌中找
                                            for(unsigned int z=0; z< ((dealers_vector.at(room_number))->players_list).at(j).hand_cards.size(); z++  )
                                            {
                                                if( card_va == ((dealers_vector.at(room_number))->players_list).at(j).hand_cards.at(z) )
                                                {
                                                    card_mate.append(5+z);
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        edge_number["card_type"] = 100;
                                    }

                                    g_pMemData->updateGameNum((dealers_vector.at(room_number))->dealer_id,
                                        (((dealers_vector.at(room_number))->players_list).at(j)).nUserID,
                                        (dealers_vector.at(room_number))->small_blind,
                                        (dealers_vector.at(room_number))->current_player_num, 1);

                                }
                            }

                            for( unsigned int k=0; k<winder_list.at(n).pool_id_list.size(); k++ )
                            {
                                pool.append( winder_list.at(n).pool_id_list.at(k) );
                            }
                            edge_number["pool_id"] = pool;//赢得的边池号
                            edge_number["card_mate"] = card_mate;//手牌-公共牌匹配位置
                            edge_user_list.append(edge_number);
                        }


                        //计算每个玩家增加的经验
                        Json::Value  exp_add;
                        if(DzpkIfCpsOfType(dealers_vector[room_number]->room_type) == 0)
                        {
                        }
                        else
                        {
                            for( unsigned int j=0; j<((dealers_vector.at(room_number))->players_list).size(); j++ )
                            {
                                if( ((dealers_vector.at(room_number))->players_list).at(j).user_staus > 1  && ((dealers_vector.at(room_number))->players_list).at(j).user_staus < 7)
                                {
                                    if( ((dealers_vector.at(room_number))->players_list).at(j).hand_chips >= ((dealers_vector.at(room_number))->players_list).at(j).last_chips && ((dealers_vector.at(room_number))->players_list).at(j).user_staus != 5 )
                                    {
                                        exp_add["user_id"] = ((dealers_vector.at(room_number))->players_list).at(j).user_account ;
                                        exp_add["seat_num"]= ((dealers_vector.at(room_number))->players_list).at(j).seat_num;
                                        if( (dealers_vector.at(room_number))->small_blind >= 2000 )
                                        {
                                            //盲注大于2000的房间，经验值*2
                                            int nAddExperience = 0;
                                            if( ((dealers_vector.at(room_number))->players_list).at(j).win_cards.size()==6 )
                                            {
                                                int card_type_fen=0;
                                                if( ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) == 914  )
                                                {
                                                    card_type_fen = 20;
                                                }
                                                else if (  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) > 900 &&  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) < 914 )
                                                {
                                                    card_type_fen = 10;
                                                }
                                                else if (  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) > 800 &&  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) <= 814 )
                                                {
                                                    card_type_fen = 6;
                                                }
                                                else if (  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) > 700 &&  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) <= 714 )
                                                {
                                                    card_type_fen = 4;
                                                }
                                                else if (  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) > 400 &&  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) <= 614 )
                                                {
                                                    card_type_fen = 1;
                                                }
                                                nAddExperience = ((dealers_vector.at(room_number))->current_player_num-1+ card_type_fen)*2 ;
                                                //exp_add["exp_add"]= ((dealers_vector.at(room_number))->current_player_num-1+ card_type_fen)*2 ;
                                                // ((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += card_type_fen*2;
                                            }

                                            nAddExperience += ((dealers_vector.at(room_number))->current_player_num-1)*2 ;
                                            nAddExperience = StdGetAddExperience(&((dealers_vector.at(room_number))->players_list).at(j),nAddExperience);
                                            exp_add["exp_add"]= nAddExperience;
                                            ((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += nAddExperience;


                                        }
                                        else
                                        {
                                            int nAddExperience = ((dealers_vector.at(room_number))->current_player_num-1);
                                            nAddExperience = StdGetAddExperience(&((dealers_vector.at(room_number))->players_list).at(j),nAddExperience);
                                            exp_add["exp_add"]= nAddExperience;
                                            ((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += nAddExperience;


                                        }
                                        exp_add["exp_rest"] = itoa(computer_exp_level(((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience,((dealers_vector.at(room_number))->players_list).at(j).dzpk_level,level_list),10);
                                        exp_add["exp_upgrade"] = itoa(get_total_exp_level(((dealers_vector.at(room_number))->players_list).at(j).dzpk_level,level_list),10);

                                        exp_add["dzpk_level"]= ((dealers_vector.at(room_number))->players_list).at(j).dzpk_level;
                                        update_user_exp_level(((dealers_vector.at(room_number))->players_list).at(j).user_account,((dealers_vector.at(room_number))->players_list).at(j).dzpk_level,((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience,connpool);
                                        experience_change_list.append(exp_add);
                                    }
                                    else if( ((dealers_vector.at(room_number))->players_list).at(j).hand_chips < ((dealers_vector.at(room_number))->players_list).at(j).last_chips || ( ((dealers_vector.at(room_number))->players_list).at(j).hand_chips == ((dealers_vector.at(room_number))->players_list).at(j).last_chips && ((dealers_vector.at(room_number))->players_list).at(j).user_staus == 5 ) )
                                    {

                                        exp_add["user_id"] = ((dealers_vector.at(room_number))->players_list).at(j).user_account ;
                                        exp_add["seat_num"]= ((dealers_vector.at(room_number))->players_list).at(j).seat_num;

                                        int nAddExperience = 0;

                                        if(  (dealers_vector.at(room_number))->small_blind >= 2000 )
                                        {
                                            if( ((dealers_vector.at(room_number))->players_list).at(j).user_staus == 5 )
                                            {
                                                //exp_add["exp_add"]= 1;
                                                //((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += 1;
                                                nAddExperience  = 1;

                                            }
                                            else
                                            {
                                                if( ((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience >=2)
                                                {
                                                    //exp_add["exp_add"]= -2;
                                                    //((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += -2;
                                                    nAddExperience  = -2;
                                                }
                                                else
                                                {
                                                    //exp_add["exp_add"]= 0;
                                                    nAddExperience  = 0;
                                                }
                                            }

                                        }
                                        else
                                        {

                                            if( ((dealers_vector.at(room_number))->players_list).at(j).user_staus == 5 )
                                            {
                                                //exp_add["exp_add"]= 0;
                                                nAddExperience  = 0;
                                            }
                                            else
                                            {
                                                if( ((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience >=1)
                                                {
                                                    //exp_add["exp_add"]= -1;
                                                    //((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += -1;
                                                    nAddExperience  = -1;
                                                }
                                                else
                                                {
                                                    nAddExperience  = 0;
                                                    // exp_add["exp_add"]= 0;
                                                }
                                            }
                                        }
                                        nAddExperience = StdGetAddExperience(&((dealers_vector.at(room_number))->players_list).at(j),nAddExperience);
                                        exp_add["exp_add"]= nAddExperience;
                                        ((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += nAddExperience;

                                        exp_add["exp_rest"]=itoa( computer_exp_level(((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience,((dealers_vector.at(room_number))->players_list).at(j).dzpk_level,level_list),10);
                                        exp_add["exp_upgrade"] = itoa(get_total_exp_level(((dealers_vector.at(room_number))->players_list).at(j).dzpk_level,level_list),10);
                                        exp_add["dzpk_level"]= ((dealers_vector.at(room_number))->players_list).at(j).dzpk_level;
                                        update_user_exp_level(((dealers_vector.at(room_number))->players_list).at(j).user_account,((dealers_vector.at(room_number))->players_list).at(j).dzpk_level,((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience,connpool);
                                        experience_change_list.append(exp_add);
                                    }
                                }
                            }
                        }
                        msg_body["exp_update"]=experience_change_list;
                        //计算每个玩家的筹码增减
                        Json::Value  chip_account;
                        for( unsigned int j=0; j<((dealers_vector.at(room_number))->players_list).size(); j++ )
                        {
                            if( ((dealers_vector.at(room_number))->players_list).at(j).hand_chips !=  ((dealers_vector.at(room_number))->players_list).at(j).last_chips )
                            {
                                chip_account["user_id"] = ((dealers_vector.at(room_number))->players_list).at(j).user_account ;
                                chip_account["seat_num"]= ((dealers_vector.at(room_number))->players_list).at(j).seat_num;
                                if(DzpkIfCpsOfType(dealers_vector[room_number]->room_type) == 0)
                                {
                                    chip_account["chips_add"]= 0;
                                }
                                else
                                {
                                    chip_account["chips_add"]= ((dealers_vector.at(room_number))->players_list).at(j).hand_chips - ((dealers_vector.at(room_number))->players_list).at(j).last_chips ;
                                    ((dealers_vector.at(room_number))->players_list).at(j).chip_account += ((dealers_vector.at(room_number))->players_list).at(j).hand_chips - ((dealers_vector.at(room_number))->players_list).at(j).last_chips ;
                                    dzpkDbUpdateUserChip(((dealers_vector.at(room_number))->players_list).at(j).nUserID,((dealers_vector.at(room_number))->players_list).at(j).hand_chips - ((dealers_vector.at(room_number))->players_list).at(j).last_chips,
                                                         ((dealers_vector.at(room_number))->players_list).at(j).chip_account,0,&((dealers_vector.at(room_number))->players_list).at(j));
                                    //插入玩家牌局信息
                                    dzpkDbInsertPlayCard(&((dealers_vector.at(room_number))->players_list).at(j),dealers_vector.at(room_number),((dealers_vector.at(room_number))->players_list).at(j).hand_chips - ((dealers_vector.at(room_number))->players_list).at(j).last_chips);
                                    //赢牌任务
                                    dzpkCheckTaskWin(((dealers_vector.at(room_number))->players_list).at(j).hand_chips - ((dealers_vector.at(room_number))->players_list).at(j).last_chips,&((dealers_vector.at(room_number))->players_list).at(j));
                                }
                                ((dealers_vector.at(room_number))->players_list).at(j).last_chips = ((dealers_vector.at(room_number))->players_list).at(j).hand_chips ;
                                //update_user_chip_account(((dealers_vector.at(room_number))->players_list).at(j).user_account,((dealers_vector.at(room_number))->players_list).at(j).chip_account,connpool);
                                chip_account_list.append(chip_account);
                            }
                        }
                        msg_body["update_chip_account"]=chip_account_list;

                        msg_body["win_list"]=edge_user_list;

                        for(unsigned int k=0; k<((dealers_vector.at(room_number))->players_list).size(); k++)
                        {
                            if( (((dealers_vector.at(room_number))->players_list).at(k)).user_staus >= 2)  //
                            {
                                dztimer timer;
                                //账号和当前时间一起用MD5加密
                                MD5_CTX ctx;
                                char* buf=(char*) ((((dealers_vector.at(room_number))->players_list).at(k)).user_account+itoa(get_current_time(),10)).c_str();
                                unsigned char md[10];
                                MD5_Init(&ctx);
                                MD5_Update(&ctx,buf,strlen(buf));
                                MD5_Final(md,&ctx);
                                char aaa[10];
                                for(unsigned int x=0; x<10; x++)
                                {
                                    sprintf(aaa+x,"%x",md[x]);
                                }

                                timer.nTimerID = DzpkGetTimerID();
                                msg_body["timer_id"]=GetString(timer.nTimerID);
                                timer.nUserID =  atoi((((dealers_vector.at(room_number))->players_list).at(k)).user_id.c_str());

                                timer.socket_fd =  (((dealers_vector.at(room_number))->players_list).at(k)).socket_fd;
                                timer.seat_num =  (((dealers_vector.at(room_number))->players_list).at(k)).seat_num;
                                memcpy(timer.szUserAccount,(((dealers_vector.at(room_number))->players_list).at(k)).user_account.c_str(),strlen((((dealers_vector.at(room_number))->players_list).at(k)).user_account.c_str()));;
                                timer.room_num =room_number;
                                timer.start_time = time(NULL);
                                timer.timer_type = 2;
                                timer.time_out = ((dealers_vector.at(room_number))->edge_pool_list).size()*3+3;
                                g_dzpkTimer.Insert(timer.nTimerID,(void*)&timer,sizeof(dztimer));
                                WriteRoomLog(4000,dealers_vector[room_number]->dealer_id ,timer.nUserID," do next work set timer type 2 (%d) \n",timer.nTimerID);
                            }
                            string str_msg =   writer.write(msg_body);
                            msg_body.removeMember("timer_id");
                            DzpkSend( (((dealers_vector.at(room_number))->players_list).at(k)).socket_fd,CMD_WIN_LIST,(char *)str_msg.c_str(),strlen(str_msg.c_str()),dealers_vector[room_number]->dealer_id,atoi(((dealers_vector.at(room_number))->players_list).at(k).user_id.c_str()));

                        }
                        //清除房间信息
                        StdClearRoomInfo(dealers_vector.at(room_number),1);
                    }
                }
            }

        }
    }
}

/*
* 加注逻辑处理分支
**/
void add_chip_branch( int nUserID,vector<dealer *>& dealers_vector,int room_number,string user_account,int chips_value,int seat_number,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,int nType)
{
    Json::FastWriter  writer;
    Json::Value  msg_body;
    Json::Value  pre_operation;
    Json::Value  pre_chip;
    //桌面注金增加，手上注金减少
    for(unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++)
    {
        if(atoi(((dealers_vector.at(room_number))->players_list).at(i).user_id.c_str()) == nUserID)
        {
            if( ((dealers_vector.at(room_number))->players_list).at(i).hand_chips >= chips_value )
            {

                WriteRoomLog(4000,(dealers_vector.at(room_number))->dealer_id,nUserID,"chouma chip[%d] chips_value[%d] nick[%s]  account[%s] seat_number[%d]  \n",((dealers_vector.at(room_number))->players_list).at(i).hand_chips,
                             chips_value,
                             ((dealers_vector.at(room_number))->players_list).at(i).nick_name.c_str(),
                             ((dealers_vector.at(room_number))->players_list).at(i).user_account.c_str(),
                             ((dealers_vector.at(room_number))->players_list).at(i).seat_num);

                ((dealers_vector.at(room_number))->players_list).at(i).hand_chips -= chips_value;
                ((dealers_vector.at(room_number))->players_list).at(i).desk_chip += chips_value;

                //更新当前最小跟注 (桌面已经加的注 与 现在加的注的总和 如果超过 当前最小跟注金额
                if((dealers_vector.at(room_number))->follow_chip < ((dealers_vector.at(room_number))->players_list).at(i).desk_chip)
                {
                    //说明是加注，看牌、跟注无需修改当前最小跟注
                    if( (dealers_vector.at(room_number))->follow_chip + 2 * (dealers_vector.at(room_number))->small_blind  == ((dealers_vector.at(room_number))->players_list).at(i).desk_chip)
                    {
                        (dealers_vector.at(room_number))->add_chip_manual_flg = 0;
                    }
                    else if ( (dealers_vector.at(room_number))->follow_chip + 2 * (dealers_vector.at(room_number))->small_blind  < ((dealers_vector.at(room_number))->players_list).at(i).desk_chip)
                    {
                        (dealers_vector.at(room_number))->add_chip_manual_flg = 1;
                    }

                    (dealers_vector.at(room_number))->follow_chip=((dealers_vector.at(room_number))->players_list).at(i).desk_chip;
                    ((dealers_vector.at(room_number))->players_list).at(i).user_staus = 2;
                    (dealers_vector.at(room_number))->look_flag = 1;
                    if( ((dealers_vector.at(room_number))->players_list).at(i).hand_chips ==0)
                    {
                        //全下
                        ((dealers_vector.at(room_number))->players_list).at(i).user_staus = 6;
                    }

                }
                else if (  (dealers_vector.at(room_number))->follow_chip >= ((dealers_vector.at(room_number))->players_list).at(i).desk_chip   &&   ((dealers_vector.at(room_number))->players_list).at(i).hand_chips ==0 )
                {
                    //玩家手上注金不够，全下产生边池
                    ((dealers_vector.at(room_number))->players_list).at(i).user_staus = 6;
                    (dealers_vector.at(room_number))->look_flag = 1;

                }
                else if (  (dealers_vector.at(room_number))->follow_chip == ((dealers_vector.at(room_number))->players_list).at(i).desk_chip   &&   ((dealers_vector.at(room_number))->players_list).at(i).hand_chips >0 )  //相等 表示跟注
                {
                    ((dealers_vector.at(room_number))->players_list).at(i).user_staus = 3;
                    (dealers_vector.at(room_number))->look_flag = 1;
                }
                else if ( chips_value == 0 )  //加注金为零 表示看牌
                {
                    ((dealers_vector.at(room_number))->players_list).at(i).user_staus = 4;
                }
                msg_body["user_status"]=((dealers_vector.at(room_number))->players_list).at(i).user_staus;
            }
            else
            {
                //

                WriteRoomLog(4000,(dealers_vector.at(room_number))->dealer_id,nUserID,"chouma all in chip[%d] chips_value[%d]  nick[%s]  account[%s] seat_number[%d]  \n",((dealers_vector.at(room_number))->players_list).at(i).hand_chips,
                             chips_value,
                             ((dealers_vector.at(room_number))->players_list).at(i).nick_name.c_str(),
                             ((dealers_vector.at(room_number))->players_list).at(i).user_account.c_str(),
                             ((dealers_vector.at(room_number))->players_list).at(i).seat_num);

                /*加注比手上的少，加注金为手上筹码*/
                chips_value = ((dealers_vector.at(room_number))->players_list).at(i).hand_chips;

                ((dealers_vector.at(room_number))->players_list).at(i).desk_chip += ((dealers_vector.at(room_number))->players_list).at(i).hand_chips;
                ((dealers_vector.at(room_number))->players_list).at(i).hand_chips = 0;


                //更新当前最小跟注 (桌面已经加的注 与 现在加的注的总和 如果超过 当前最小跟注金额
                if((dealers_vector.at(room_number))->follow_chip < ((dealers_vector.at(room_number))->players_list).at(i).desk_chip   ) //说明是加注，看牌、跟注无需修改当前最小跟注
                {

                    if( (dealers_vector.at(room_number))->follow_chip + 2 * (dealers_vector.at(room_number))->small_blind  == ((dealers_vector.at(room_number))->players_list).at(i).desk_chip)
                    {
                        (dealers_vector.at(room_number))->add_chip_manual_flg = 0;
                    }
                    else if ( (dealers_vector.at(room_number))->follow_chip + 2 * (dealers_vector.at(room_number))->small_blind  < ((dealers_vector.at(room_number))->players_list).at(i).desk_chip)
                    {
                        (dealers_vector.at(room_number))->add_chip_manual_flg = 1;
                    }

                    (dealers_vector.at(room_number))->follow_chip=((dealers_vector.at(room_number))->players_list).at(i).desk_chip;
                    ((dealers_vector.at(room_number))->players_list).at(i).user_staus = 2;
                    (dealers_vector.at(room_number))->look_flag = 1;
                    if( ((dealers_vector.at(room_number))->players_list).at(i).hand_chips ==0)  //全下
                    {
                        ((dealers_vector.at(room_number))->players_list).at(i).user_staus = 6;
                    }

                }
                else if (  (dealers_vector.at(room_number))->follow_chip >= ((dealers_vector.at(room_number))->players_list).at(i).desk_chip   &&   ((dealers_vector.at(room_number))->players_list).at(i).hand_chips ==0 )  //玩家手上注金不够，全下产生边池
                {
                    ((dealers_vector.at(room_number))->players_list).at(i).user_staus = 6;
                    (dealers_vector.at(room_number))->look_flag = 1;

                }
                else if (  (dealers_vector.at(room_number))->follow_chip == ((dealers_vector.at(room_number))->players_list).at(i).desk_chip   &&   ((dealers_vector.at(room_number))->players_list).at(i).hand_chips >0 )  //相等 表示跟注
                {
                    ((dealers_vector.at(room_number))->players_list).at(i).user_staus = 3;
                    (dealers_vector.at(room_number))->look_flag = 1;
                }
                else if ( chips_value == 0 )  //加注金为零 表示看牌
                {
                    ((dealers_vector.at(room_number))->players_list).at(i).user_staus = 4;
                }
                msg_body["user_status"]=((dealers_vector.at(room_number))->players_list).at(i).user_staus;
            }
        }
    }

    //桌面总筹码数增加
    (dealers_vector.at(room_number))->all_desk_chips+=chips_value;



    //向房间内的所有玩家发送消息周知

    msg_body["msg_head"]="dzpk";
    msg_body["game_number"]="dzpk";
    msg_body["area_number"]="normal";
    msg_body["act_user_id"]=user_account;

    for(unsigned int x=0; x<((dealers_vector.at(room_number))->players_list).size(); x++)
    {
        if( atoi(((dealers_vector.at(room_number))->players_list).at(x).user_id.c_str()) == nUserID )
        {
            msg_body["nick_name"]=((dealers_vector.at(room_number))->players_list).at(x).nick_name;
        }
    }
    msg_body["act_commond"]=CMD_ADD_CHIP;
    msg_body["seat_number"]=seat_number;
    msg_body["chips_value"]=chips_value;
    msg_body["ctl_type"]=1;

    //预操作
    pre_operation.clear();
    for(unsigned int n=0; n<((dealers_vector.at(room_number))->players_list).size(); n++)
    {
        if( ((dealers_vector.at(room_number))->players_list).at(n).user_staus == 2 || ((dealers_vector.at(room_number))->players_list).at(n).user_staus == 3 || ((dealers_vector.at(room_number))->players_list).at(n).user_staus == 4)
        {
            if( ((dealers_vector.at(room_number))->players_list).at(n).desk_chip < (dealers_vector.at(room_number))->follow_chip )
            {
                if( ((dealers_vector.at(room_number))->players_list).at(n).hand_chips >= (dealers_vector.at(room_number))->follow_chip - ((dealers_vector.at(room_number))->players_list).at(n).desk_chip )
                {
                    pre_chip["seat_num"]= ((dealers_vector.at(room_number))->players_list).at(n).seat_num;
                    pre_chip["follow_chip"] = (dealers_vector.at(room_number))->follow_chip - ((dealers_vector.at(room_number))->players_list).at(n).desk_chip;
                }
                else
                {
                    pre_chip["seat_num"]= ((dealers_vector.at(room_number))->players_list).at(n).seat_num;
                    pre_chip["follow_chip"] = ((dealers_vector.at(room_number))->players_list).at(n).hand_chips;
                }
                pre_operation.append(pre_chip);
            }
            else
            {
                pre_chip["seat_num"]= ((dealers_vector.at(room_number))->players_list).at(n).seat_num;
                pre_chip["follow_chip"] = 0;
                pre_operation.append(pre_chip);
            }
        }
    }
    msg_body["pre_operation"]=pre_operation;

    string   str_msg =   writer.write(msg_body);

    for(unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++)
    {
        DzpkSend( (((dealers_vector.at(room_number))->players_list).at(i)).socket_fd,CMD_ADD_CHIP,(char*)str_msg.c_str(),strlen(str_msg.c_str()),dealers_vector[room_number]->dealer_id,atoi(((dealers_vector.at(room_number))->players_list).at(i).user_id.c_str()));
    }

    //下一步工作 判断下一个玩家的座位及相关动作
    do_next_work( nUserID ,dealers_vector,room_number,user_account,chips_value,seat_number,connpool,level_list ,pSocketUser,nType);

}


/*
* 弃牌逻辑处理分支
**/
void give_up_branch( int nUserID,vector<dealer *>& dealers_vector,int room_number,string user_account,int chips_value,int seat_number,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,int nType)
{

    if(nType == DEAL_RESULT_TYPE_FORCELEAVE)
    {
    }
    else
    {
        for(unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++)
        {
            if( ((dealers_vector.at(room_number))->players_list).at(i).nUserID == nUserID )
            {
                ((dealers_vector.at(room_number))->players_list).at(i).user_staus = 5;//更新玩家状态

            }
        }

        //发送弃牌消息周知大家
        Json::FastWriter  writer;
        Json::Value  msg_body;
        msg_body["msg_head"]="dzpk";
        msg_body["game_number"]="dzpk";
        msg_body["area_number"]="normal";
        msg_body["act_user_id"]=user_account;

        for(unsigned int x=0; x<((dealers_vector.at(room_number))->players_list).size(); x++)
        {
            if( atoi(((dealers_vector.at(room_number))->players_list).at(x).user_id.c_str()) == nUserID )
            {
                msg_body["nick_name"]=((dealers_vector.at(room_number))->players_list).at(x).nick_name;
            }
        }

        msg_body["act_commond"]=CMD_ADD_CHIP;
        msg_body["seat_number"]=seat_number;
        msg_body["chips_value"]=chips_value;
        msg_body["user_status"]=5;
        msg_body["ctl_type"]=0;
        string   str_msg =   writer.write(msg_body);

        for(unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++)
        {
            DzpkSend( (((dealers_vector.at(room_number))->players_list).at(i)).socket_fd,CMD_ADD_CHIP,(char *)str_msg.c_str(),strlen(str_msg.c_str()),dealers_vector[room_number]->dealer_id,atoi(((dealers_vector.at(room_number))->players_list).at(i).user_id.c_str()));
        }
    }
    int count=0; //剩余玩家人数
    for(unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++)
    {
        if( ((dealers_vector.at(room_number))->players_list).at(i).user_staus == 2 ||((dealers_vector.at(room_number))->players_list).at(i).user_staus == 3 || ((dealers_vector.at(room_number))->players_list).at(i).user_staus == 4 ||((dealers_vector.at(room_number))->players_list).at(i).user_staus == 6 )
        {
            count++;
        }
    }

    if( count == 0)
    {
        //如果弃牌之后，所有玩家站起，避免牌局被卡住，牌局置回开始
        (dealers_vector.at(room_number))->step = 0;
    }
    else if(count == 1)
    {

        //隐藏光圈
        StdStopHideLightRing(dealers_vector[room_number]);

        //今天玩牌次数加一
        StdTodayPlayTimes(dealers_vector.at(room_number));

        //幸运手牌活动
        StdLuckyCardActivity(dealers_vector[room_number],2);

        //弃牌之后只剩一个玩家，判定该玩家赢
        edge_pool eg_pool;
        eg_pool.edge_height=0;
        vector<edge_pool> edge_pool_temp;
        edge_pool_temp.push_back(eg_pool);
        edge_pool_computer(dealers_vector,room_number,edge_pool_temp);//边池编号从集合底部单位开始，第0个默认为主池，依次代表边池的编号

        //向房间内的所有玩家发送赢牌消息
        Json::FastWriter  writer;
        Json::Value  msg_body;
        Json::Value  hand_cards_all;
        Json::Value  open_card;
        Json::Value  edge_user_list;
        Json::Value  chip_account_list;
        Json::Value  experience_change_list;
        Json::Value  edge_number;
        Json::Value  pool;
        Json::Value  edge_desk_pool;
        Json::Value  pool_st;
        msg_body.clear();
        msg_body["msg_head"]="dzpk";
        msg_body["game_number"]="dzpk";
        msg_body["area_number"]="normal";
        msg_body["act_commond"]=CMD_WIN_LIST;

        //手牌-座位号对应列表
        for(  unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++ )
        {
            if( ((dealers_vector.at(room_number))->players_list).at(i).user_staus == 2 ||((dealers_vector.at(room_number))->players_list).at(i).user_staus == 3 || ((dealers_vector.at(room_number))->players_list).at(i).user_staus == 4 || ((dealers_vector.at(room_number))->players_list).at(i).user_staus == 6)
            {
                Json::Value  hand_cards;
                Json::Value  card;
                hand_cards["seat_number"]= ((dealers_vector.at(room_number))->players_list).at(i).seat_num;
                card.append( ((dealers_vector.at(room_number))->players_list).at(i).hand_cards.at(0) );
                card.append( ((dealers_vector.at(room_number))->players_list).at(i).hand_cards.at(1) );
                hand_cards["hand_card"]= card;
                hand_cards_all.append(hand_cards);
                msg_body["lay_card_user"]=((dealers_vector.at(room_number))->players_list).at(i).user_id;//最后剩余的赢家，需要亮手牌
            }
        }
        msg_body["hand_cards_all"]=hand_cards_all;

        open_card.append((dealers_vector.at(room_number))->desk_card[0]);
        open_card.append((dealers_vector.at(room_number))->desk_card[1]);
        open_card.append((dealers_vector.at(room_number))->desk_card[2]);
        open_card.append((dealers_vector.at(room_number))->desk_card[3]);
        open_card.append((dealers_vector.at(room_number))->desk_card[4]);
        msg_body["public_brand"]=open_card;

        //合并边池列表中的主池
        vector<edge_pool>::iterator iter;
        //计算所有主池
        unsigned int pool_area=0;
        for(iter= (dealers_vector.at(room_number))->edge_pool_list.begin(); iter!= (dealers_vector.at(room_number))->edge_pool_list.end(); iter++  )
        {
            edge_pool  pool_m = (*iter);
            if(pool_m.pool_type ==1 )
            {
                pool_area = pool_area+pool_m.edge_height * pool_m.share_seat.size() ;
            }
        }

        edge_pool  pool_tmp;

        //合并池子
        for(unsigned int i= 0; i< (dealers_vector.at(room_number))->edge_pool_list.size(); i++ )
        {
            if( (dealers_vector.at(room_number))->edge_pool_list.at(i).pool_type ==1 )
            {
                pool_tmp=(dealers_vector.at(room_number))->edge_pool_list.at(i);
            }
        }

        if(pool_tmp.share_seat.size() > 0)
            pool_tmp.edge_height =  pool_area / pool_tmp.share_seat.size();

        //删除掉其他的主池
        for(iter= (dealers_vector.at(room_number))->edge_pool_list.begin(); iter!= (dealers_vector.at(room_number))->edge_pool_list.end(); iter++  )
        {
            edge_pool  pool_m = (*iter);
            if(pool_m.pool_type ==1)
            {
                (dealers_vector.at(room_number))->edge_pool_list.erase(iter);
                iter--;
            }
        }

        (dealers_vector.at(room_number))->edge_pool_list.push_back(pool_tmp);

        for(unsigned int i=0; i<edge_pool_temp.size(); i++)
        {
            int pool=edge_pool_temp.at(i).edge_height *  edge_pool_temp.at(i).share_seat.size();
            for(unsigned int j=0; j<edge_pool_temp.at(i).share_seat.size(); j++)
            {
                if(  edge_pool_temp.at(i).share_seat.at(j) <=0 )
                {
                    pool+= edge_pool_temp.at(i).share_seat.at(j);
                }
            }
            pool_st["pool"]=pool;//池子里总的注金数
            pool_st["type"]=(edge_pool_temp.at(i)).pool_type;//池子的种类：1: 主池 0:边池
            pool_st["id"]=(edge_pool_temp.at(i)).pool_id;//池子的id：0: 主池 >1:边池
            edge_desk_pool.append(pool_st);

        }
        msg_body["desk_pool"]=edge_desk_pool;

        for(  unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++ )
        {
            if( ((dealers_vector.at(room_number))->players_list).at(i).user_staus == 2 || ((dealers_vector.at(room_number))->players_list).at(i).user_staus == 3 || ((dealers_vector.at(room_number))->players_list).at(i).user_staus == 4 || ((dealers_vector.at(room_number))->players_list).at(i).user_staus == 6 )
            {
                edge_number.clear();
                edge_number["user_id"] = ((dealers_vector.at(room_number))->players_list).at(i).user_account ;
                edge_number["seat_num"] = ((dealers_vector.at(room_number))->players_list).at(i).seat_num ;
                pool.clear();

                for( unsigned int j=0; j<((dealers_vector.at(room_number))->edge_pool_list).size(); j++ )
                {
                    pool.append(j);
                }
                edge_number["pool_id"] = pool;
                edge_user_list.append(edge_number);

                g_pMemData->updateGameNum((dealers_vector.at(room_number))->dealer_id,
                                        (((dealers_vector.at(room_number))->players_list).at(i)).nUserID,
                                        (dealers_vector.at(room_number))->small_blind,
                                        (dealers_vector.at(room_number))->current_player_num, 1);
            }
        }

        //计算每个玩家的筹码增减，由于弃牌剩余一个玩家的情况没有经过【赢牌计算】这一步骤，直接在此计算并更新手上的筹码
        Json::Value  chip_account;
        for( unsigned int j=0; j<((dealers_vector.at(room_number))->players_list).size(); j++ )
        {
            if( ((dealers_vector.at(room_number))->players_list).at(j).hand_chips !=  ((dealers_vector.at(room_number))->players_list).at(j).last_chips )
            {

                chip_account["user_id"] = ((dealers_vector.at(room_number))->players_list).at(j).user_account ;
                chip_account["seat_num"]= ((dealers_vector.at(room_number))->players_list).at(j).seat_num;
                //如果是弃牌玩家，输掉已经投下去的筹码
                if( ((dealers_vector.at(room_number))->players_list).at(j).user_staus == 5  )
                {
                    if(DzpkIfCpsOfType(dealers_vector[room_number]->room_type) == 0)
                    {
                        chip_account["chips_add"]= 0;
                    }
                    else
                    {
                        chip_account["chips_add"]= ((dealers_vector.at(room_number))->players_list).at(j).hand_chips - ((dealers_vector.at(room_number))->players_list).at(j).last_chips ;
                        ((dealers_vector.at(room_number))->players_list).at(j).chip_account += ((dealers_vector.at(room_number))->players_list).at(j).hand_chips - ((dealers_vector.at(room_number))->players_list).at(j).last_chips ;
                        dzpkDbUpdateUserChip(((dealers_vector.at(room_number))->players_list).at(j).nUserID,((dealers_vector.at(room_number))->players_list).at(j).hand_chips - ((dealers_vector.at(room_number))->players_list).at(j).last_chips,
                                             ((dealers_vector.at(room_number))->players_list).at(j).chip_account,0,&((dealers_vector.at(room_number))->players_list).at(j));
                        //插入玩家牌局信息
                        dzpkDbInsertPlayCard(&((dealers_vector.at(room_number))->players_list).at(j),dealers_vector.at(room_number),((dealers_vector.at(room_number))->players_list).at(j).hand_chips - ((dealers_vector.at(room_number))->players_list).at(j).last_chips);

                        //赢牌任务
                        dzpkCheckTaskWin(((dealers_vector.at(room_number))->players_list).at(j).hand_chips - ((dealers_vector.at(room_number))->players_list).at(j).last_chips,&((dealers_vector.at(room_number))->players_list).at(j));
                    }

                }
                else  //如果是剩下的一个赢家，赢回所有的钱
                {

                    int total_pool=0;
                    if( ((dealers_vector.at(room_number))->players_list).at(j).user_staus >= 2  )
                    {
                        for( unsigned int m=0; m<((dealers_vector.at(room_number))->edge_pool_list).size(); m++ )
                        {
                            edge_pool pool =  ((dealers_vector.at(room_number))->edge_pool_list).at(m);
                            total_pool +=  pool.edge_height * pool.share_seat.size();
                            for(unsigned int n=0; n<pool.share_seat.size(); n++)
                            {
                                if( pool.share_seat.at(n) < 0)
                                {
                                    total_pool += pool.share_seat.at(n);
                                }
                            }
                        }
                    }

                    if(DzpkIfCpsOfType(dealers_vector[room_number]->room_type) == 0)
                    {
                        chip_account["chips_add"]= 0;
                    }
                    else
                    {
                        chip_account["chips_add"]= ((dealers_vector.at(room_number))->players_list).at(j).hand_chips + total_pool-((dealers_vector.at(room_number))->players_list).at(j).last_chips;
                        ((dealers_vector.at(room_number))->players_list).at(j).chip_account += ((dealers_vector.at(room_number))->players_list).at(j).hand_chips + total_pool - ((dealers_vector.at(room_number))->players_list).at(j).last_chips ;
                        dzpkDbUpdateUserChip(((dealers_vector.at(room_number))->players_list).at(j).nUserID,((dealers_vector.at(room_number))->players_list).at(j).hand_chips + total_pool - ((dealers_vector.at(room_number))->players_list).at(j).last_chips,
                                             ((dealers_vector.at(room_number))->players_list).at(j).chip_account,0,&((dealers_vector.at(room_number))->players_list).at(j));
                        //插入玩家牌局信息
                        dzpkDbInsertPlayCard(&((dealers_vector.at(room_number))->players_list).at(j),dealers_vector.at(room_number),((dealers_vector.at(room_number))->players_list).at(j).hand_chips + total_pool - ((dealers_vector.at(room_number))->players_list).at(j).last_chips);

                        //赢牌任务
                        dzpkCheckTaskWin(((dealers_vector.at(room_number))->players_list).at(j).hand_chips + total_pool - ((dealers_vector.at(room_number))->players_list).at(j).last_chips,&((dealers_vector.at(room_number))->players_list).at(j));
                    }
                    ((dealers_vector.at(room_number))->players_list).at(j).hand_chips += total_pool;
                }
                chip_account_list.append(chip_account);
            }
        }


        //计算每个玩家增加的经验
        Json::Value  exp_add;
        if(DzpkIfCpsOfType(dealers_vector[room_number]->room_type) == 0)
        {
        }
        else
        {
            for( unsigned int j=0; j<((dealers_vector.at(room_number))->players_list).size(); j++ )
            {
                if( ((dealers_vector.at(room_number))->players_list).at(j).user_staus > 1  && ((dealers_vector.at(room_number))->players_list).at(j).user_staus < 7)
                {
                    if( ((dealers_vector.at(room_number))->players_list).at(j).hand_chips >= ((dealers_vector.at(room_number))->players_list).at(j).last_chips && ((dealers_vector.at(room_number))->players_list).at(j).user_staus != 5 )
                    {
                        exp_add["user_id"] = ((dealers_vector.at(room_number))->players_list).at(j).user_account ;
                        exp_add["seat_num"]= ((dealers_vector.at(room_number))->players_list).at(j).seat_num;
                        if( (dealers_vector.at(room_number))->small_blind >= 2000 )
                        {
                            int nAddExperience = 0;
                            if( ((dealers_vector.at(room_number))->players_list).at(j).win_cards.size()==6 )
                            {
                                int card_type_fen=0;
                                if( ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) == 914  )
                                {
                                    card_type_fen = 20;
                                }
                                else if (  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) > 900 &&  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) < 914 )
                                {
                                    card_type_fen = 10;
                                }
                                else if (  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) > 800 &&  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) <= 814 )
                                {
                                    card_type_fen = 6;
                                }
                                else if (  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) > 700 &&  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) <= 714 )
                                {
                                    card_type_fen = 4;
                                }
                                else if (  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) > 400 &&  ((dealers_vector.at(room_number))->players_list).at(j).win_cards.at(5) <= 614 )
                                {
                                    card_type_fen = 1;
                                }
                                //exp_add["exp_add"]= ((dealers_vector.at(room_number))->current_player_num-1 + card_type_fen)*2;
                                //((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += card_type_fen*2;
                                nAddExperience = ((dealers_vector.at(room_number))->current_player_num-1 + card_type_fen)*2;
                            }
                            //盲注大于2000的房间，经验值*2
                            nAddExperience = nAddExperience + ((dealers_vector.at(room_number))->current_player_num-1)*2 ;
                            nAddExperience = StdGetAddExperience(&((dealers_vector.at(room_number))->players_list).at(j),nAddExperience);
                            exp_add["exp_add"]= nAddExperience ;
                            ((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += nAddExperience;
                        }
                        else
                        {
                            int nAddExperience = ((dealers_vector.at(room_number))->current_player_num-1);
                            nAddExperience = StdGetAddExperience(&((dealers_vector.at(room_number))->players_list).at(j),nAddExperience);
                            exp_add["exp_add"]= nAddExperience ;
                            ((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += nAddExperience;

                            //exp_add["exp_add"]= ((dealers_vector.at(room_number))->current_player_num-1);
                            //((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += ((dealers_vector.at(room_number))->current_player_num-1);

                        }
                        exp_add["exp_rest"] = itoa(computer_exp_level(((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience,((dealers_vector.at(room_number))->players_list).at(j).dzpk_level,level_list),10);
                        exp_add["exp_upgrade"] = itoa(get_total_exp_level(((dealers_vector.at(room_number))->players_list).at(j).dzpk_level,level_list),10);

                        exp_add["dzpk_level"]= ((dealers_vector.at(room_number))->players_list).at(j).dzpk_level;
                        update_user_exp_level(((dealers_vector.at(room_number))->players_list).at(j).user_account,((dealers_vector.at(room_number))->players_list).at(j).dzpk_level,((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience,connpool);
                        experience_change_list.append(exp_add);
                    }
                    else if( ((dealers_vector.at(room_number))->players_list).at(j).hand_chips < ((dealers_vector.at(room_number))->players_list).at(j).last_chips || ( ((dealers_vector.at(room_number))->players_list).at(j).hand_chips == ((dealers_vector.at(room_number))->players_list).at(j).last_chips && ((dealers_vector.at(room_number))->players_list).at(j).user_staus == 5 ) )
                    {

                        exp_add["user_id"] = ((dealers_vector.at(room_number))->players_list).at(j).user_account ;
                        exp_add["seat_num"]= ((dealers_vector.at(room_number))->players_list).at(j).seat_num;

                        int nAddExperience = 0;
                        if(  (dealers_vector.at(room_number))->small_blind >= 2000 )
                        {
                            if( ((dealers_vector.at(room_number))->players_list).at(j).user_staus == 5 )
                            {
                                nAddExperience = 1;
                                //exp_add["exp_add"]= 1;
                                // ((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += 1;

                            }
                            else
                            {
                                if( ((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience >=2)
                                {
                                    nAddExperience = -2;
                                    //exp_add["exp_add"]= -2;
                                    // ((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += -2;
                                }
                                else
                                {
                                    //exp_add["exp_add"]= 0;
                                    nAddExperience = 0;
                                }
                            }
                        }
                        else
                        {
                            if( ((dealers_vector.at(room_number))->players_list).at(j).user_staus == 5 )
                            {
                                //exp_add["exp_add"]= 0;
                                nAddExperience = 0;
                            }
                            else
                            {
                                if( ((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience >=1)
                                {
                                    nAddExperience = -1;
                                    //exp_add["exp_add"]= -1;
                                    // ((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += -1;
                                }
                                else
                                {
                                    //exp_add["exp_add"]= 0;
                                    nAddExperience = 0;
                                }
                            }
                        }

                        nAddExperience = StdGetAddExperience(&((dealers_vector.at(room_number))->players_list).at(j),nAddExperience);
                        exp_add["exp_add"]= nAddExperience ;
                        ((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience += nAddExperience;

                        exp_add["exp_rest"]=itoa( computer_exp_level(((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience,((dealers_vector.at(room_number))->players_list).at(j).dzpk_level,level_list),10);
                        exp_add["exp_upgrade"] = itoa(get_total_exp_level(((dealers_vector.at(room_number))->players_list).at(j).dzpk_level,level_list),10);
                        exp_add["dzpk_level"]= ((dealers_vector.at(room_number))->players_list).at(j).dzpk_level;
                        update_user_exp_level(((dealers_vector.at(room_number))->players_list).at(j).user_account,((dealers_vector.at(room_number))->players_list).at(j).dzpk_level,((dealers_vector.at(room_number))->players_list).at(j).dzpk_experience,connpool);

                        experience_change_list.append(exp_add);
                    }
                    //将手上的筹码更新到last_chips中
                    ((dealers_vector.at(room_number))->players_list).at(j).last_chips = ((dealers_vector.at(room_number))->players_list).at(j).hand_chips;
                }
            }
        }
        msg_body["exp_update"]=experience_change_list;
        msg_body["max_follow_chip"]=0;
        (dealers_vector.at(room_number))->nTurnMaxChip = 0;
        msg_body["next_turn"]=0;
        msg_body["step"]=4;
        msg_body["update_chip_account"]=chip_account_list;

        msg_body["win_list"]=edge_user_list;
        for(unsigned int i=0; i<((dealers_vector.at(room_number))->players_list).size(); i++)
        {
            if( (((dealers_vector.at(room_number))->players_list).at(i)).user_staus >= 2)
            {
                dztimer timer;

                //账号和当前时间一起用MD5加密

                timer.nTimerID = DzpkGetTimerID();
                msg_body["timer_id"] =GetString(timer.nTimerID);
                memcpy(timer.szUserAccount,(((dealers_vector.at(room_number))->players_list).at(i)).user_account.c_str(),strlen((((dealers_vector.at(room_number))->players_list).at(i)).user_account.c_str()));

                timer.socket_fd =  (((dealers_vector.at(room_number))->players_list).at(i)).socket_fd;
                timer.seat_num =  (((dealers_vector.at(room_number))->players_list).at(i)).seat_num;
                timer.room_num = room_number;
                timer.start_time = time(NULL);
                timer.timer_type = 2;
                timer.nUserID =  atoi((((dealers_vector.at(room_number))->players_list).at(i)).user_id.c_str());
                timer.time_out = ((dealers_vector.at(room_number))->edge_pool_list).size()*3+3;
                g_dzpkTimer.Insert(timer.nTimerID,(void*)&timer,sizeof(dztimer));
                WriteRoomLog(4000,dealers_vector[room_number]->dealer_id,timer.nUserID,"game over play over flash(give up) set timer type 2(%d) \n",timer.nTimerID);
            }
            string   str_msg =   writer.write(msg_body);
            msg_body.removeMember("timer_id");
            DzpkSend( (((dealers_vector.at(room_number))->players_list).at(i)).socket_fd,CMD_WIN_LIST,(char *)str_msg.c_str(),strlen(str_msg.c_str()),dealers_vector[room_number]->dealer_id,atoi((((dealers_vector.at(room_number))->players_list).at(i)).user_id.c_str()));
        }

        //清除房间信息
        StdClearRoomInfo(dealers_vector.at(room_number),1);

        dealers_vector.at(room_number)->nActionUserID = -1;
    }
    else if (count > 1)   //剩余多个玩家
    {

        /*异常弃牌不能调用do_next_work*/
        if( nType != DEAL_ADD_CHIP_TYPE_FORCELEAVE)
        {
            do_next_work(nUserID,dealers_vector,room_number,user_account,chips_value,seat_number,connpool,level_list,pSocketUser,nType);
        }
    }
}

/*需要显示手牌玩家*/
int StdPutResultShowHandCards(dealer *pRoom,Json::Value  &hand_cards_all,vector<winer_pool>  winder_list,vector<edge_pool> edge_pool_temp)
{
    int nRet = 0;
    if(pRoom)
    {
        char szLog[WRITE_LOG_LEN],szLogTemp[200];
        memset(szLog,0,WRITE_LOG_LEN);

        int nMainPoolID = -1,nMainSeatNum = -1,nMainNextSeatNum = -1;

        //获得主池ID
        for(int i=0; i<(int)edge_pool_temp.size(); i++)
        {
            if(edge_pool_temp[i].pool_type == 1)
            {
                nMainPoolID = edge_pool_temp[i].pool_id;
                sprintf(szLogTemp,"main pool[%d] :",nMainPoolID);
                memcpy(szLog+strlen(szLog),szLogTemp,strlen(szLogTemp));
                break;
            }
        }

        //赢得主池的位置
        for(int i=0; i<(int)winder_list.size(); i++)
        {
            sprintf(szLogTemp,": seat num [%d],",winder_list[i].seat_number);
            memcpy(szLog+strlen(szLog),szLogTemp,strlen(szLogTemp));

            for(int j=0; j<(int)winder_list[i].pool_id_list.size(); j++)
            {
                if(winder_list[i].pool_id_list[j] == nMainPoolID)
                {
                    nMainSeatNum = winder_list[i].seat_number;

                    if(1)
                    {
                        sprintf(szLogTemp," pool id [%d],",winder_list[i].pool_id_list[j]);
                        memcpy(szLog+strlen(szLog),szLogTemp,strlen(szLogTemp));
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }

        sprintf(szLogTemp,": MainSeatNum [%d] :",nMainSeatNum);
        memcpy(szLog+strlen(szLog),szLogTemp,strlen(szLogTemp));

        //赢得主池的下一个位置
        int nSeatCount = 9;
        if(nMainSeatNum > 0)
        {
            for (int j=nMainSeatNum; j< nMainSeatNum +(nSeatCount -1) ; j++)
            {
                for(unsigned int i=0; i<(pRoom->players_list).size(); i++)
                {
                    //查找下一家的范围包含 加注、跟注、看牌的人，弃牌，全下的人不在范围内
                    if( ((pRoom->players_list).at(i)).user_staus == 2 || ((pRoom->players_list).at(i)).user_staus == 3 || ((pRoom->players_list).at(i)).user_staus == 4 || ((pRoom->players_list).at(i)).user_staus == 6)
                    {
                        if( (j+1)  <=  nSeatCount)
                        {
                            if( ((pRoom->players_list).at(i)).seat_num == (j+1)  )
                            {
                                nMainNextSeatNum  =(j+1) ;
                            }
                        }
                        else
                        {
                            if( ((pRoom->players_list).at(i)).seat_num == (j+1)%nSeatCount)
                            {
                                nMainNextSeatNum  = (j+1)%nSeatCount ;
                            }
                        }
                    }
                }

                if(nMainNextSeatNum  > 0)
                {
                    break;
                }
            }
        }

        sprintf(szLogTemp,": next seat [%d] :",nMainNextSeatNum);
        memcpy(szLog+strlen(szLog),szLogTemp,strlen(szLogTemp));

        for(  unsigned int n=0; n<(pRoom->players_list).size(); n++ )
        {
            if( (pRoom->players_list).at(n).user_staus == 2 || (pRoom->players_list).at(n).user_staus == 3 || (pRoom->players_list).at(n).user_staus == 4 || (pRoom->players_list).at(n).user_staus == 6 )
            {
                sprintf(szLogTemp,"alive status[%d] num[%d],",(pRoom->players_list).at(n).user_staus,(pRoom->players_list).at(n).seat_num);
                memcpy(szLog+strlen(szLog),szLogTemp,strlen(szLogTemp));

                int nAdd = 0;

                if((pRoom->players_list).at(n).user_staus == 6 || (pRoom->players_list).at(n).seat_num == nMainNextSeatNum)
                {
                    nAdd = 1;
                }
                else
                {
                    for(int k=0; k<(int)winder_list.size(); k++)
                    {
                        if(winder_list[k].seat_number == (pRoom->players_list).at(n).seat_num)
                        {
                            nAdd = 1;
                            break;
                        }
                    }
                }

                if(nAdd == 1)
                {
                    Json::Value  hand_cards;
                    Json::Value  card;
                    hand_cards["seat_number"]= (pRoom->players_list).at(n).seat_num;
                    card.append( (pRoom->players_list).at(n).hand_cards.at(0) );
                    card.append( (pRoom->players_list).at(n).hand_cards.at(1) );
                    hand_cards["hand_card"]= card;
                    hand_cards_all.append(hand_cards);
                }
            }
        }

        WriteRoomLog(4008,pRoom->dealer_id,0,"%s\n",szLog);
    }

    return nRet;
}

/*隐藏光圈*/
int StdStopHideLightRing(dealer *pRoom)
{
    int nRet = 0;

    if(pRoom)
    {
        Json::Value  msg_body;
        Json::FastWriter  writer;
        msg_body["msg_head"]="dzpk";
        msg_body["game_number"]="dzpk";
        msg_body["area_number"]="normal";

        msg_body["act_commond"]=CMD_HIDE_LIGHT_RING;
        string   str_msg =   writer.write(msg_body);

        for(unsigned int i=0; i<(pRoom->players_list).size(); i++)
        {
            DzpkSend( ((pRoom->players_list).at(i)).socket_fd,CMD_HIDE_LIGHT_RING,(char *)str_msg.c_str(),strlen(str_msg.c_str()),pRoom->dealer_id,atoi((pRoom->players_list).at(i).user_id.c_str()));
        }
    }

    return nRet;
}




