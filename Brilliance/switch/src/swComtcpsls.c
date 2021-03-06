/****************************************************************/
/* 模块编号    ：swComtcpsls                                    */
/* 模块名称    : 通信例程-TCP同步长连接server                   */
/* 作    者    ：     �                                         */
/* 建立日期    ：2000/5/18                                      */
/* 最后修改日期：2001/9/18                                      */
/* 模块用途    ：                                               */
/* 本模块中包含如下函数及功能说明：                             */
/*                        (1)int swComtcpsls()                  */
/*                        (2)int swDoitsls()                    */
/****************************************************************/

#include "swapi.h"
#include "swCompub.h"
#include "swComtcp.h"
#include "swPubfun.h"

static long    igSockfd;
static long    igSocket;

void swQuit();
int swDoitsls();

int main( int argc , char **argv )
{
  int    ilRc;

  #ifdef OS_SCO
    int  ilAddrlen;
  #else
    socklen_t  ilAddrlen;
  #endif

  struct hostent *pslHp;
  struct linger slLinger;
  char   alCliname[80];
  char   alCliaddr[16];
  unsigned int ilQid,ilMsglen,ilPri,ilClass,ilType;
  char   alMsgbuf[iMSGMAXLEN];
  struct sockaddr_in slCli_addr;
  
  /* 取得版本号 */
  if(argc > 1)
    _swVersion("swComtcpsls Version 4.3.0",argv[1]);

  /* 设置调试程序名称 */
  sprintf( agDebugfile,"swComtcp%s.debug",argv[1]);

  if ( argc != 2 )
  {
    swVdebug(0,"S0010: [错误/其它] 传入参数数量不合法!");
    exit(1);
  }
  
  /* 处理信号 */
  swSiginit( );
  
  ilRc = swLoadcomcfg( argv[1] );
  if ( ilRc != 0 )
  {
    swVdebug(0,"S0020: [错误/函数调用] swLoadcomcfg()函数[%s]装入通讯配置参数出错,请检查输入的端口名,返回码=%d",argv[1],ilRc);
    exit( 1 );
  }
  
  swVdebug(2,"S0030: [%s]装入通讯配置参数成功",argv[1]);
    
  memset(alCliaddr,0x0,sizeof(alCliaddr));
  memset((char *)&slCli_addr, 0x00, sizeof(struct sockaddr_in) );

  igSocket=swTcpconnect(sgTcpcfg.iPartner_port);
  if (igSocket == -1)
  {
    swVdebug(0,"S0040: [错误/函数调用] swTcpconnect()ERROR!");
    exit( -1 );
  }
  swVdebug(2,"S0050: 建立连接成功");
   
  /*打开通讯邮箱*/
  ilRc = swMbopen( sgComcfg.iMb_comm_id ); 
  if ( ilRc )
  {
    swVdebug(0,"S0060: [错误/邮箱] 打开邮箱[%d]出错!",sgComcfg.iMb_comm_id );
    close( igSocket );
    exit( -1 );
  }
  swVdebug(2,"S0070: 打开邮箱成功");
  
  /*接收连接*/
  swVdebug(2,"S0080: 开始接收");
  ilAddrlen = sizeof(struct sockaddr_in);
  
  igSockfd  = accept(igSocket,(struct sockaddr*)&slCli_addr,&ilAddrlen);
  if ( igSockfd == -1)
  {
    swVdebug(0,"S0090: [错误/系统调用] 连接错误,errno=%d[%s]",errno,strerror(errno));
    close(igSocket);
    swMbclose();
    exit( -1 );
  }
  swVdebug(2,"S0100: accept() SUCCESS!");
 
  strcpy(alCliaddr,inet_ntoa(slCli_addr.sin_addr)); 
  
  swVdebug(2,"S0110: alCliaddr=%s",alCliaddr);
  
  /*获得客户端主机名*/
  pslHp = gethostbyaddr( (char *) &slCli_addr.sin_addr, sizeof(struct in_addr),
    slCli_addr.sin_family);
  if ( pslHp == NULL)
    strcpy( alCliname, inet_ntoa( slCli_addr.sin_addr ) );
  else
    strcpy( alCliname, pslHp->h_name );

  swVdebug(2,"S0120: gethostbyaddr SUCCESS");
    
  slLinger.l_onoff  =1;
  slLinger.l_linger =0;
  
  ilRc = setsockopt(igSockfd,SOL_SOCKET,SO_LINGER,&slLinger,
    sizeof(struct linger));
  if ( ilRc == -1 )
  {
    swVdebug(0,"S0130: [错误/系统调用] setsockopt(),Connect error!errno=[%d,%s]",errno,strerror(errno));
    close( igSocket );
    close( igSockfd );
    swMbclose();
    exit( -1 );
  }
  swVdebug(2,"S0140: setsockopt SUCCESS!");
 
  for(;;)
  {
    /*从客户端接收报文*/
    ilMsglen = sizeof( alMsgbuf );
  
    ilRc = swTcprcv(igSockfd, alMsgbuf, &ilMsglen );
    if ( ilRc )
    {
      swVdebug(1,"S0150: [错误/函数调用] swTcprcv()函数,从客户端{%s}接收报文出错,返回码=%d",alCliname,ilRc);
      close( igSocket );
      close( igSockfd );
      swMbclose();
      exit( -1 );
    }
    swVdebug(2,"S0160: 从客户端接收报文成功");
    if (cgDebug >= 2) 
    {
      swDebughex(alMsgbuf,ilMsglen);
    }

    /* next add by nh*/
    if (sgComcfg.iMsghead == 0)
    {
      /* 在报文前增加空报文头 */
      swComaddblankmsghead(alMsgbuf,&ilMsglen);
    }

    swVdebug(2,"S0170: 从客户端{%s}接收报文完毕!len=%d",alCliname,ilMsglen);
 
    ilPri = 0 ;
    ilClass = 0 ;
    ilType  = 0 ;
 
    ilRc = swSendpack(sgComcfg.iMb_fore_id, alMsgbuf, ilMsglen, ilPri, 
      ilClass, ilType);
    if (ilRc)
    {
      swVdebug(1,"S0180: [错误/函数调用] swSendpack()函数,写报文到邮箱[%d]出错,返回码=%d",ilQid,ilRc);
      close( igSocket );
      close( igSockfd );
      swMbclose();
      exit( -1 );
    }

      swVdebug(2,"S0190: 写报文到邮箱成功");
      swVdebug(2,"S0200: Writed to Qid=[%d],ilPri=[%d],Class=[%d],Type=[%d]",\
        ilQid, ilPri, ilClass, ilType );
        
    /*读通讯邮箱*/
    swVdebug(2,"S0210: 等待读通讯邮箱");
  
    ilMsglen = sizeof( alMsgbuf );
    ilPri = 0 ;
    ilClass = 0 ;
    ilType = 0 ;
 
    ilRc = swRecvpack( &ilQid, alMsgbuf, &ilMsglen, &ilPri, &ilClass,\
      &ilType , sgComcfg.iTime_out );
    if ( ilRc)
    {
      if ( ilRc == BMQ__TIMEOUT)
      {
        swVdebug(1,"S0220: [错误/函数调用] swRecvpack()函数,读邮箱[%d]超时,返回码=%d",ilQid,ilRc);
        continue;
      }
      swVdebug(1,"S0230: [错误/函数调用] swRecvpack()函数,读邮箱[%d]出错,返回码=%d",ilQid,ilRc);
      close( igSocket );
      close( igSockfd );
      swMbclose();
      exit(-1);
    }
    swVdebug(2,"S0240: swRecvpack() SUCCESS");
    if (cgDebug >= 2)
    {
      swDebughex(alMsgbuf,ilMsglen);
    }

    /*送回客户端 */
    /*判断有无报文头*/
    if(sgComcfg.iMsghead == 0)
    {
      /* 不带报文头往外发 */
      ilRc = swTcpsnd(igSockfd,alMsgbuf + sizeof(struct msghead),
        ilMsglen - sizeof(struct msghead));
    }
    else
    {
      ilRc = swTcpsnd(igSockfd,alMsgbuf,ilMsglen);
    }
    if ( ilRc )
    {
      swVdebug(1,"S0250: [错误/函数调用] swTcpsnd()函数 送回客户端出错,返回码=%d",ilRc );
      close( igSocket );
      close( igSockfd );
      swMbclose();
      exit( -1 );
    }
    swVdebug(2,"S0260: 已成功送回客户端!ilMsglen = %d",ilMsglen);
    if (cgDebug >= 2) 
    {
      swDebughex(alMsgbuf,ilMsglen);
    }
  }
}  


void swQuit(int sig)
{
  signal(SIGTERM,SIG_IGN);
  swMbclose();
  close(igSocket);
  close(igSockfd);
  exit(0);
}


