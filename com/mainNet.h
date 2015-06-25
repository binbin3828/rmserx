#ifndef __MAINNET_H__
#define __MAINNET_H__


/*JackFrost 2013.10.31 网络模块对外接口，网络模块是核心模块，必不可少模块 */
/*
#include "../../com/safeList.h"
#include "netCom/netSocketRecv.h"
#include "netCom/netSocketSend.h"
#include "../../com/safeHash.h"
*/

#include "safeList.h"
#include "netSocketRecv.h"
#include "netSocketSend.h"
#include "safeHash.h"

#define NET_SOCKET_USER_MAX	20000	//最多支持多少用户
/***********************网络模块对外结构(结构放这里方便外部模块查看)  ************************/

#define NET_HEAD_SIZE	100
typedef struct tagNetRecBuf
{
	unsigned int hadReadSize;
	unsigned int ibodySize;
	char headBuf[NET_HEAD_SIZE];
	char *pIns;
}NET_REC_BUF_T;

/*用户socket信息父类，其他模块用户信息不同继承就行*/
typedef struct tagSocketInfo
{
	int nfd;					//ePoll
	int nSocketID;					//socket ID
	int nRoomID;					//用户所在房间(用户进入房间后赋值)
	int nUserID;					//用户注册后赋值
	int nPort;					//连接端口
	int nSendClose;					//关闭
	NET_REC_BUF_T  insBuf;
	int nRecFireWallMark;				//是否接收过防火墙标识
	CNetSocketRecv *listRecv;			//接收数组
	CNetSocketSend *listSend;			//发送数组
    long lUserIp;                       //用户IP
}SOCKET_INFO_T;

#define PACKKET_MARK 0xA35CD932
#define MAX_PACKET_SIZE	1024*1024*20
#define PACK_ALIGN	__attribute__((packed))
typedef struct tagPacketHead
{
	unsigned int nMark;				//识别
	unsigned short nLen;			//消息长度
	unsigned int nMsgID;			//消息ID
	unsigned int nOrder;			//包序号
	unsigned int nSourceID;			//用户ID
	unsigned int nAimID;			//目标ID
	short nAck;						//执行结果
	unsigned char szPackType;		//包类型 0 为一个完整包 1为开始包 2为中间包 3为结束包
	unsigned char szPackTotal;		//总共有多少个包
	unsigned char szPackOrder;		//包顺序
	unsigned char szType;			//类型
	char szReserve[8];				//保留
}PACK_ALIGN PACKET_HEAD_T;
#undef PACK_ALIGN

int PutDataToPacketHead(char *pBuf, int bufLen,unsigned int nMsgID,unsigned short nLen,short nAck,unsigned int nOrder,
						unsigned int nSourceID,unsigned int nAimID,unsigned char szPackType = 0,unsigned char szPackTotal = 0,unsigned char szPackOrder = 0,
						unsigned char szType = 0);

typedef struct tagSocketUserClose
{
    int nUserID;
    int nRoomID;
    int nType;
}SOCKET_USER_CLOSE_T;
/**********************************************************网络模块对外接口  **********************************************/
/*添加用户回调函数样式*/
typedef int NetAddSocketMallocBack(int nfd,int nSocketID,int nPort,CNetSocketRecv *pRecv,CNetSocketSend *pSend);

/*消息回调函数样式*/
/*nMsgID:消息号*/
/*pBuf:收到数据*/
/*pBackUserBuf:发回自己数据，如果需要群发，调用NetSendmsgToSocket,使用malloc申请，不要调用free*/
/*nBackBufLen 发回自己数据长度，小于等于0为不发回自己*/
typedef int NetRegisterMsgBack(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen);

/*关闭某个socket回调函数结构*/
typedef int NetCloseSocketBack(int nSocket,void *pUser,int nType);

/*设置端口,要最先调用*/
int ServiceNetSetPort(int nListerPort);

/*开启网络模块*/
int ServiceNetStart();

/*注册添加Socket用户回调函数*/
/*pCallBack 添加用户网络模块回调函数（每个模块用户结构不同）。*/
/*pCallBack 游戏模块用户要继承 tagSocketInfo,调用 */
int NetRegisterAddSocketBack(NetAddSocketMallocBack *pCallBack,int nPort);

/*socket 关闭回调函数*/
int NetRegisterCloseSocketBack(NetCloseSocketBack *pCloseSocketBack,int nPort);

/*注册释放用户内存回调*/
int NetResgisterFreeSocketBack(HashRealFreeDataBack *pFreeDataBack,int nPort);

/*注册消息 */
int NetRegisterNormalMsg(int nMsgID,NetRegisterMsgBack *pBack);

/*关闭网络模块*/
int ServiceNetStop();

/*获得模块运行状态，是否有运行（以后增加支持暂停等状态）*/
/*0 为没有运行  1 为正常运行*/
#define NET_STATUS_NO_INIT 	0		/*网络模块未初始化*/
#define NET_STATUS_RUNNING 	1		/*网络模块正在运行*/
#define NET_STATUS_STOP 	2		/*网络模块已经关闭*/
int GetNetStatus();		

/*系统是否运行，内部调用*/
int IsSystemRunning();

/*发送消息给socket*/
/*pBuf:数据，使用malloc申请，成功不要free,返回值小于0需要自己free*/
int NetSendMsgToSocket(int nSocketID,char *pBuf,int nLen);

/*pSocketInfo:用户信息或者socket信息都可以，因为两个保存的是同一指针*/
int NetSendMsgToSocketP(void *pSocket,char *pBuf,int nLen);

/**************************************  用户操作(因为不同游戏用户信息不同，这里使用回调函数，实现接口也会比较多)***********/

/*<<<<<<<< socketUser >>>>>>>>>>>*/
/*用户结构需要调用此函数分配内存,不要释放*/
void * NetSocketUserMalloc(int nSize);

/*说明:socketUser不需要外部调用关闭，添加只能在NetAddSocketMallocBack 回调函数添加,也不需要显示调用删除，因为socket断开会自动删除并将nSocket置为-1*/
int NetAddUserToSocket(int nSocketID,void *pSocketUser,int nLen);


/*根据socketID获得用户*/
/*说明：SocketUser为已经连接的socket，NetUser为已经注册用户，为同一结构指针，存在不同哈希*/
void *NetGetSocketUser(int nSocketID);

/*释放取得的SocketUser指针*/
int NetFreeSocketUser(void *pSocketUser);

/*主动关闭socket*/
int NetCloseSocketUser(int nSocketID,int nCloseUser = 1);

/*获得当前有多少个连接*/
int NetGetSocketUserCount();

/*<<<<<<<< NetUser >>>>>>>>>>>*/
/*添加用户信息到哈希,用户信息为通过NetGetSocketUser获得的指针*/
/*添加之前需要判断原先是否有，如果原先已经有，说明用户出在短线状态保留，或者重登录，需要把原来删除，如果nSocket跟要添加不同，还需关闭 */
int NetAddUserToNet(int nUserID,void * pUser,int nLen);

/*根据用户ID获得用户信息*/
void *NetGetNetUser(int nUserID);

/*释放取得的NetUser指针*/
int NetFreeNetUser(void *pNetUser);

/*删除注册用户*/
/*nCloseSocket:是否关闭其关联socket. 0为不关闭，1为关闭 */
int NetRemoveNetUser(int nUserID,int nCloseSocket = 1);

/*删除用户*/
int NetRemoveNetUserP(SOCKET_INFO_T * pSocketInfo,int nUserID,int nCloseSocket = 1);

/*获得当前有多少个注册用户*/
int NetGetNetUserCount();

//获得所有注册用户
SAFE_HASH_LIST_T *NetGetListUser();

//获得所有用户需要调用此接口释放
int NetFreeListUser(SAFE_HASH_LIST_T * pList);

/*获得毫秒时间*/
int GetTickTime();


#endif //__MAINNET_H__
