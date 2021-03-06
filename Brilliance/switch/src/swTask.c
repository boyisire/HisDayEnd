/****************************************************************/
/* 模块编号    ：TASK                                           */
/* 模块名称    ：任务管理                                       */
/* 版 本 号    ：V4.3.0                                         */
/* 作    者    ：                                               */
/* 建立日期    ：2001/7/18                                      */
/* 最后修改日期：2001/8/8                                       */
/* 模块用途    ：启动平台进程,并监控启动进程的状态,对异常作处理 */
/* 本模块中包含如下函数及功能说明：                             */
/*               (1)void main();                                */
/*               (3)int  swExecSubProcess(); 启动子进程         */
/*               (4)int  swChkMailbox;       检查邮箱           */
/*               (5)int  swTaskmanage;       控制任务多起       */
/*               (6)int  swTasktimer;        定时启动、关闭任务 */
/*               (7)int  swChkSigkill();     检查关闭信号       */ 
/*               (8)void swQuit();           关闭平台进程       */
/****************************************************************/
#define NOSQL

/* switch定义 */
#include "switch.h"
#include <setjmp.h>
#include "swNdbstruct.h"
#include "swShm.h"

int swChkSigkill();
int swTasktimer();
int swTaskmanage();
int swChkMailbox();
int swExecSubProcess(int alFlag,struct swt_sys_task sSwt_sys_task);
int Parse(char *buf,char args[][101]);

/* 常量定义 */
#define  aINSTANCENAME  "swTask"
#define SHMNOTFOUND	100     /* 没找到共享内存记录 */

/* 变量定义 */
/*char  *agargs[50];*/ /* deleted by fzj at 2002.03.04 */
char agargs[15][101];  /* added by fzj at 2002.03.04 */
char *execvpargs[10];  /* added by fzj at 2002.03.04 */

/* 函数原型定义 */
void swSigcld(int iSig);
void swQuit(); 
 
/**************************************************************
 ** 函数名      ： main
 ** 功  能      ：主函数
 ** 作  者      ：  
 ** 建立日期    ： 
 ** 最后修改日期：2001/09/10 
 ** 调用其它函数：
 ** 全局变量    ：
 ** 参数含义    ：
 ** 返回值      ： 无
***************************************************************/
int main(int argc,char *argv[])
{
  short ilRc;                                        /* 返回值 */   
  short ilCount;                                     /* 记录数 */
  FILE *plPopenfile;
  int  i,j;                                          /* 计数器 */
  struct swt_sys_task *pslSwt_sys_task;/* add by nh 20020807 */
  FILE *pp;                            /* add by nh 20020925 */
  char alName[20],alCmd[100];          /* add by nh 20020925 */


  /* 打印版本号 */
  if (argc > 1)
    _swVersion("swTask Version 4.3.0",argv[1]);

  /* LICENSE */
  if (_swLicense("swTask")) exit(FAIL);

  /* 设置调试程序名称 */
  memset(agDebugfile, 0x00, sizeof(agDebugfile));
  strncpy(agDebugfile, "swTask.debug", sizeof(agDebugfile));

  /* 取得DEBUG标志 */
  if ((cgDebug = _swDebugflag("swTask")) == FAIL)
  {
    fprintf(stderr,"无法取得DEBUG标志!\n");
    exit(FAIL);
  } 

  /* add by nh 20020925 */
  pp=popen("id -un","r");
  fscanf(pp,"%s",alName);
  pclose(pp);
  sprintf(alCmd,"%s%s%s","ps -u ",alName,"|grep swTask|wc -l");
  swVdebug(2,"S0010: command=[%s]",alCmd);
  /* end add */
  plPopenfile=popen(alCmd,"r");
  fscanf(plPopenfile,"%d",&i);  
  swVdebug(2,"S0020: 当前用户的swTask进程个数为[%d]",i);
  if (i>1)
  {
    swVdebug(1,"S0030: [错误/系统调用] popen()函数,errno=%d,swTask进程已存在!",errno);
      pclose(plPopenfile);
    exit(0);
  }
  else
    pclose(plPopenfile);

  ilRc = qattach(iMBTASK);
  if (ilRc)
  {
    swVdebug(1,"S0040: [错误/邮箱] qattack()函数,错误码=%d,初始化邮箱出错",ilRc);
    exit(FAIL);
  }
  /* 信号设置 */
  /* 
  signal(SIGTERM, swQuit);              
  signal(SIGCLD, SIG_IGN);
  signal(SIGINT, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGHUP, SIG_IGN);
  */
  for(j=1;j<=64;j++)
    signal(j, SIG_IGN);
  
  signal(SIGSEGV,SIG_DFL); 
  signal(SIGTERM, swQuit);

  /* 初始化共享内存指针 */
  ilRc = swShmcheck();  
  if ( ilRc != SUCCESS )
  {
    swVdebug(0,"S0050: [错误/共享内存] 初始化共享内存指针失败");
    exit(FAIL);
  }

  swVdebug(2,"S0060: 初始化共享内存指针成功");
 
  /* 从共享内存中取出所有任务记录 */
  /* modify by nh 20020807
  ilRc = swShmselect_swt_sys_task_all_p(pslSwt_sys_task, &ilCount);*/
  ilRc = swShmselect_swt_sys_task_all_p(&pslSwt_sys_task, &ilCount);
  if ( ilRc != SUCCESS )
  {
    swVdebug(1,"S0070: [错误/共享内存] 操作共享内存取任务表记录出错");
    exit(FAIL);
  } 
  
  /* 检查所有所有任务 */
  for ( i=0; i < ilCount; i++ )
  {
    memset(&sgSwt_sys_task, 0x00, sizeof(struct swt_sys_task));
    memcpy(&sgSwt_sys_task, &(pslSwt_sys_task[i]), sizeof(struct swt_sys_task));
    /* 判断任务是否该启 */
    if (sgSwt_sys_task.task_use[0] !=cTRUE ||\
        strlen(sgSwt_sys_task.task_timer) || \
        sgSwt_sys_task.task_flag !=1 )
    {
      sgSwt_sys_task.pid = -1;
      sgSwt_sys_task.restart_num = 0;
      sgSwt_sys_task.start_time = 0; 
      sgSwt_sys_task.task_status[0] = cTASKDOWN; 
    }
    else 
    {
      if ( ( sgSwt_sys_task.pid > 0 ) && ( kill(sgSwt_sys_task.pid,0) == 0 ) )
      {
	kill(sgSwt_sys_task.pid, 9); 
        swVdebug(2,"S0080: kill进程 [%s]",sgSwt_sys_task.task_name );
       }
      sgSwt_sys_task.task_status[0] = cTASKRUNING; 
      sgSwt_sys_task.restart_num = 0;
      sgSwt_sys_task.start_time = 0;
      sgSwt_sys_task.pid = -1;      
      if ( sgSwt_sys_task.task_flag != 1 )
      {
        sgSwt_sys_task.task_flag = 0;
      }  
    }

    /* 更新共享内存中任务状态 */
    ilRc = swShmupdate_swt_sys_task(sgSwt_sys_task.task_name, sgSwt_sys_task);
    if ( ilRc != SUCCESS ) 
    {
      swVdebug(1,"S0090:  [错误/共享内存] 共享内存中任务[%s]未找到",sgSwt_sys_task.task_name); 
      continue; 
    }
  }

  /* 选择所有任务状态为 cTASKRUNING 的记录,按 start_id 排序 */
  /* modify by nh 20020807
  ilRc = swShmselect_swt_sys_task_mrec_status_t("1", pslSwt_sys_task,&ilCount);*/
  ilRc = swShmselect_swt_sys_task_mrec_status_t("1", &pslSwt_sys_task,&ilCount);
  if ( ilRc != SUCCESS )
  {
    swVdebug(1,"S0100: [错误/共享内存] 操作共享内存出错");
    swQuit( );
  } 
  
  /* 启动所有状态为 cTASKRUNING 的任务进程 */
  for ( i = 0; i < ilCount; i++ )
  {
    memset(&sgSwt_sys_task, 0x00, sizeof(sgSwt_sys_task));
    memcpy(&sgSwt_sys_task, &pslSwt_sys_task[i], sizeof(struct swt_sys_task));
    
    _swTrim(sgSwt_sys_task.task_name);
    _swTrim(sgSwt_sys_task.task_file);
    Parse (sgSwt_sys_task.task_file, agargs);

    /* 启动子进程 */
    swExecSubProcess( 1,sgSwt_sys_task );  
  }
  swVdebug(2,"S0110: 检查、启动平台进程成功!");
 
  /* 打开 SIGCLD 信号 */
  signal(SIGCLD, swSigcld ); 

  for(;;) 
  {
    /* 重新初始化共享内存指针 */
    ilRc = swShmcheck(); 
    if (ilRc != SUCCESS)
    {
      swVdebug(0,"S0120: [错误/共享内存] 检查共享内存出错");
      swQuit();
    }

    /* 以非阻塞方式检查邮箱，处理 Begin and End 命令 */
    swChkMailbox(); 
  
    /* 处理任务多起  */
    /*
    swTaskmanage();
    */
 
    /* 处理定时启动、关闭任务 */
    /*
    swTasktimer();
    */
  }
}

/**************************************************************
 ** 函数名      :  swChkMailbox
 ** 功  能      :  检测任务管理邮箱,处理begin、end命令
 ** 作  者      :
 ** 建立日期    :
 ** 最后修改日期: 2001/08/15
 ** 调用其它函数:
 ** 全局变量    :
 ** 参数含义    :
 ** 返回值      :  SUCCESS , FAIL
***************************************************************/
int swChkMailbox()
{
  unsigned int ilMsglen, ilOrgqid, ilPriority;
  short  ilClass, ilType;
  short  ilSigkill;
  short  ilRc;
  char   alTaskname[51];
  long   llPid;
  struct  msgpack  slMsgpack;  /* 报文定义 */

  ilMsglen = iMSGMAXLEN;
  ilPriority = 0;
  ilClass = 0;
  ilType = 0;

  swVdebug(4,"S0130: [函数调用] swChkMailbox()");

  ilRc =  qreadnw((char *)&slMsgpack, &ilMsglen, &ilOrgqid, \
                &ilPriority, &ilClass, &ilType, iTASKTIMEOUT);
  if( ilRc == SUCCESS)  /* 有报文 */
  {
    
    memset(alTaskname, 0x00, sizeof(alTaskname));
    memcpy(alTaskname, slMsgpack.aMsgbody, slMsgpack.sMsghead.iBodylen);
    slMsgpack.aMsgbody[slMsgpack.sMsghead.iBodylen]='\0';
    switch(slMsgpack.sMsghead.lCode)
    {
      case 701:            /* 启动进程 */
        memset(&sgSwt_sys_task, 0x00, sizeof(sgSwt_sys_task));
        ilRc = swShmselect_swt_sys_task(slMsgpack.aMsgbody, &sgSwt_sys_task);
        if (ilRc != SUCCESS)
        {
          swVdebug(1,"S0140: [错误/共享内存] 任务 [%s] 不存在!", sgSwt_sys_task.task_name);
          return(FAIL); 
        }

        _swTrim(sgSwt_sys_task.task_name);
        _swTrim(sgSwt_sys_task.task_file);

        if (sgSwt_sys_task.task_use[0] != '1')
        {
          swVdebug(1,"S0150: [错误/其它] [%s]进程设定为初始不启动",sgSwt_sys_task.task_name);
          return(FAIL);
        }

        if (sgSwt_sys_task.pid <= 0)
        {
          _swTrim(sgSwt_sys_task.task_name);
          _swTrim(sgSwt_sys_task.task_file);
          Parse(sgSwt_sys_task.task_file, agargs);
          sgSwt_sys_task.restart_num = 0;

          /* 启动子进程 */
          swExecSubProcess( 2,sgSwt_sys_task );
        }
        break;

      case 702:      /* 关闭进程 */
        memset(&sgSwt_sys_task, 0x00, sizeof(sgSwt_sys_task));
        ilRc = swShmselect_swt_sys_task(alTaskname, &sgSwt_sys_task);
        if (ilRc != SUCCESS)
        {
          swVdebug(1,"S0160: [错误/共享内存] 任务 [%s] 不存在!", sgSwt_sys_task.task_name);
          return(FAIL);
        }
        _swTrim(sgSwt_sys_task.task_name);
        _swTrim(sgSwt_sys_task.task_file);
        sgSwt_sys_task.restart_num = 0;
        sgSwt_sys_task.start_time = 0;

        if (sgSwt_sys_task.pid > 0)
        {
          /* 停止进程 */
          ilSigkill = swChkSigkill();
          /* sleep(sgSwt_sys_task.stop_wait);*/
          swVdebug(2,"S0170: END 进程[%s] ", sgSwt_sys_task.task_name);
 
          llPid = sgSwt_sys_task.pid;
          sgSwt_sys_task.pid = -1;
          strcpy(sgSwt_sys_task.task_status, "4");

/*add by gxz 2001.09.11 Begin */
          if ( sgSwt_sys_task.task_flag == 9 )
            sgSwt_sys_task.task_flag=0;
/*add by gxz 2001.09.11 End */

          /* 更新共享内存任务状态 */
          ilRc = swShmupdate_swt_sys_task(sgSwt_sys_task.task_name,
                 sgSwt_sys_task);
          if (ilRc != SUCCESS)
          {
            swVdebug(1,"S0180: [错误/共享内存] 更新共享内存任务[%s]状态失败",\
              sgSwt_sys_task.task_name); 
            break;
          }
          kill(llPid, ilSigkill);
        }
        break;
      default: 
        swVdebug(1,"S0190: [错误/其它] 收到未知命令报文[%ld]", slMsgpack.sMsghead.lCode); 
          break;
    } 
  }
  return(0);
}

/**************************************************************
 ** 函数名       :  swTaskmanage
 ** 功  能       :  处理任务多起
 ** 作  者       :  顾晓忠
 ** 建立日期     :  2001/09/10
 ** 最后修改日期 : 
 ** 调用其它函数 :
 ** 全局变量     :
 ** 参数含义     :
 ** 返回值       :  SUCCESS , FAIL
**************************************************************/
int swTaskmanage()
{
  short ilQid,ilRc;
  short ilSigkill;                                   /* 关闭进程信号 */
  long  llPid;
  struct mbinfo slMbinfo;

  swVdebug(4,"S0200: [函数调用] swTaskmanage()");

  ilQid=1;
  while((ilRc = bmqGetmbinfo(ilQid,&slMbinfo)) != 100)
  {
    if ( ilRc == -1 )
    {
      swVdebug(1,"S0210: [错误/邮箱] bmqGetmbinfo()函数,错误码=%d",ilRc);
      return(FAIL);
    }
    
    if ( slMbinfo.iStatus == 1 )
    {
      memset(&sgSwt_sys_task, 0x00, sizeof(sgSwt_sys_task));
      ilRc=swShmselect_swt_sys_task_qid( ilQid, 9, &sgSwt_sys_task );
      if ( ilRc == 0 )
      {
        _swTrim(sgSwt_sys_task.task_name);
        _swTrim(sgSwt_sys_task.task_file);
        sgSwt_sys_task.restart_num = 0;
        sgSwt_sys_task.start_time = 0;

        if (sgSwt_sys_task.pid > 0)
        {
          /* 停止进程 */
          ilSigkill = swChkSigkill();
          llPid = sgSwt_sys_task.pid;
          swVdebug(2,"S0220: END 进程[%s] ", sgSwt_sys_task.task_name);
 
          sgSwt_sys_task.pid = -1;
          strcpy(sgSwt_sys_task.task_status, "4");
          sgSwt_sys_task.task_flag =0;

          /* 更新共享内存任务状态 */
          ilRc = swShmupdate_swt_sys_task(sgSwt_sys_task.task_name,
               sgSwt_sys_task);
          if (ilRc != SUCCESS)
          {
            swVdebug(1,"S0230: [错误/共享内存] 更新共享内存任务状态失败");
          }
          kill(llPid, ilSigkill);
        }
      }
    }
    else if ( slMbinfo.iStatus == 3 )
    {
      memset(&sgSwt_sys_task, 0x00, sizeof(sgSwt_sys_task));
      ilRc=swShmselect_swt_sys_task_qid( ilQid, 0, &sgSwt_sys_task );
      if ( ilRc == 0 )
      {
   /*     if (sgSwt_sys_task.task_use[0] != '1')
        {
          swVdebug(2,"S0240: [%s] 进程设定为初始不启动!",
            sgSwt_sys_task.task_name);
          break;
        }
   */
        _swTrim(sgSwt_sys_task.task_name);
        _swTrim(sgSwt_sys_task.task_file);
        Parse(sgSwt_sys_task.task_file, agargs);
        swExecSubProcess( 2,sgSwt_sys_task );
      }
    }
    ilQid++;
  }

  swVdebug(4,"S0250: [函数返回] swTaskmanage()返回码=0");
  return(SUCCESS);
}  

/**************************************************************
 ** 函数名       :  swTasktimer
 ** 功  能       :  处理任务的定时启动、关闭�
 ** 作  者       :
 ** 建立日期     :
 ** 最后修改日期 :  2001/08/15
 ** 调用其它函数 :
 ** 全局变量     :
 ** 参数含义     :
 ** 返回值       :  SUCCESS , FAIL
**************************************************************/
int swTasktimer()
{
  short  i,ilSigkill;
  short  ilRc;
  char   alResult[iFLDVALUELEN + 1]  ;
  unsigned int ilLength;
  short  ilCount;
  /* del by nh 20020807
  struct swt_sys_task pslSwt_sys_task[iMAXTASKREC];*/
  struct swt_sys_task *pslSwt_sys_task;/* add by nh 20020807*/
  
  swVdebug(4,"S0260: [函数调用] swTasktimer()");

  /* 查找 task_use = "1" 的记录 */
  /* modify by nh 20020807
  ilRc = swShmselect_swt_sys_task_mrec_use_t("1",pslSwt_sys_task,&ilCount);*/
  ilRc = swShmselect_swt_sys_task_mrec_use_t("1",&pslSwt_sys_task,&ilCount);
  if (ilRc != SUCCESS)
  {
    swVdebug(1,"S0270: [错误/共享内存] 在共享内存中查找可用任务出错!");
    swQuit();
  }
  
  /* 轮询任务表 */
  for (i=0; i < ilCount; i++)
  {
    memcpy(&sgSwt_sys_task,&pslSwt_sys_task[i],sizeof(struct swt_sys_task));
    
    _swTrim(sgSwt_sys_task.task_timer);

    /* 检查任务状态  */
    if (sgSwt_sys_task.pid > 0)
    {
      /* kill(pid,0)检测进程状态,刷新任务表 */
      ilRc = kill(sgSwt_sys_task.pid, 0);
      if ( ilRc ) 
      {
        strcpy(sgSwt_sys_task.task_status, "4");
        sgSwt_sys_task.pid = -1;

        /* add by gxz 2001.09.11 Begin */
        if ( sgSwt_sys_task.task_flag != 1 )
          sgSwt_sys_task.task_flag = 0;
        /* add by gxz 2001.09.11 End */

        /* 更新共享内存 */
        ilRc = swShmupdate_swt_sys_task(sgSwt_sys_task.task_name,
                 sgSwt_sys_task);
        if (ilRc != SUCCESS)
        {
          swVdebug(1,"S0280: [错误/共享内存] 共享内存中任务[%s]未找到",\
            sgSwt_sys_task.task_name); 
          continue; 
        }
      }
    }

    if (sgSwt_sys_task.task_timer[0]) /* 定时启动、关闭 */
    {
      ilRc = _swExpress(sgSwt_sys_task.task_timer, alResult,&ilLength);
      if (ilRc == FAIL)
      {
        swVdebug(1,"S0290: [错误/函数调用] _swExpress()函数,返回码=%d,定时启动表达式[%s]计算出错",ilRc,sgSwt_sys_task.task_timer);
        continue ;
      }

      /* 满足启动条件 */
      if ((alResult[0] == '1') && (sgSwt_sys_task.pid <= 0))
      {
        _swTrim(sgSwt_sys_task.task_name);
        _swTrim(sgSwt_sys_task.task_file);
        Parse(sgSwt_sys_task.task_file, agargs);
/*        signal(SIGCLD,SIG_IGN);*/

        /* 启动任务，更改任务表 */
        swExecSubProcess( 2,sgSwt_sys_task );
/*        signal(SIGCLD,swSigcld);*/
        continue;
      }
      if (alResult[0] == '0' && sgSwt_sys_task.pid > 0) /* 满足关闭条件 */
      {
        swVdebug(2,"S0300: 定时关闭进程[%s]", sgSwt_sys_task.task_name);
 
        /* 关闭任务 */
        ilSigkill = swChkSigkill();
/*        signal(SIGCLD, SIG_IGN);*/
        kill(sgSwt_sys_task.pid, ilSigkill);
        sleep(sgSwt_sys_task.stop_wait);
  /*      signal(SIGCLD, swSigcld);*/

        sgSwt_sys_task.pid = -1;
        strcpy(sgSwt_sys_task.task_status, "4");
        sgSwt_sys_task.restart_num = 0; /* very important */
        sgSwt_sys_task.start_time = 0; /* very important */

        /* 更新共享内存 */
        ilRc = swShmupdate_swt_sys_task(sgSwt_sys_task.task_name,
            sgSwt_sys_task);
        if (ilRc != SUCCESS)
        {
          swVdebug(1,"S0310: [错误/共享内存] 共享内存中任务[%s]未找到",sgSwt_sys_task.task_name); 
          continue; 
        }
      }
    } /* end 定时启动、关闭 */
  } /* end for */
  swVdebug(4,"S0320: [函数返回] swTasktimer()返回码=0");
  return( SUCCESS);
} 
     
/**************************************************************
 ** 函数名:  short swExecSubProcess( )
 ** 功  能       :  启动平台进程
 ** 作  者       :
 ** 建立日期     :
 ** 最后修改日期 :  2001/09/05
 ** 修改人       ： 顾晓忠
 ** 调用其它函数 :
 ** 全局变量     :
 ** 参数含义     :  1--启交换平台时    2--启任务时 
 ** 返回值       :  SUCCESS = 0 , FAIL = -1 
***************************************************************/
int swExecSubProcess( int alFlag,struct swt_sys_task sSwt_sys_task  )
{
  short i,ilRc;
  short ilResult;
  long  llBegintime;
  long  llPid;

  swVdebug(4,"S0330: [函数调用] swExecSubProcess(%d)",alFlag);

  /*  执行文件不存在或无执行权限 */
  if ( (access(agargs[0], F_OK)) == -1 ||  (access(agargs[0], X_OK)) == -1 )
  {
    swVdebug(1,"S0340: [错误/其它] 应用程序[%s]无法启动!",agargs[0]);
    strcpy(sSwt_sys_task.task_status, "5");
    sSwt_sys_task.start_time = 0;
    sSwt_sys_task.pid = -1;

    /* 更新共享内存 */
    ilRc = swShmupdate_swt_sys_task(sSwt_sys_task.task_name, sSwt_sys_task);
    if (ilRc != SUCCESS)
    {
      swVdebug(1,"S0350: [错误/共享内存] 共享内存中任务[%s]未找到",sSwt_sys_task.task_name); 
      return(FAIL);
    }
    return(FAIL);
  }
  /* 启动子进程 */
  if ((llPid = fork()) < 0)  
  {
    swVdebug(1,"S0360: [错误/系统调用] fork()函数,errno=%d,fork子进程失败! ",errno);
    /* swQuit(); delete by szhengye 2001.10.30 */
    return(-1);
  }
  if (llPid == 0)  
  { 
    swVdebug(2,"agargs=[%s][%s][%s][%s]",agargs[0],agargs[1],agargs[2],agargs[3]);
    /* execvp( *agargs, agargs ); */ /* delete by fzj at 2002.03.04 */
    /* begin of added by fzj at 2002.03.04 */
    for (i=0;i<15;i++) 
    {
      if (agargs[i][0] == '\0')
      {
      	execvpargs[i] = NULL;
      	break;
      }	
      execvpargs[i] = (char *)(&agargs[i][0]);
    }  
    execvp(*execvpargs, execvpargs);
    /* end of added by fzj at 2002.03.04 */
    exit(0);   
  }
  /* 检查进程是否启动成功 */
  if ( alFlag == 1 )
    sleep(sSwt_sys_task.start_wait);
  else 
    sleep(1);

  ilRc = kill(llPid, 0);
  if( ilRc == -1 )
  {
    sSwt_sys_task.task_status[0] = cTASKDOWN;
    sSwt_sys_task.start_time = 0; 
    swVdebug(1,"S0370: [错误/其它] 启动 [%s] 失败", sSwt_sys_task.task_name);
    ilResult = FAIL;
  }   
  else      /* 执行进程成功，修改任务状态 */
  {
    time(&llBegintime); 
    sSwt_sys_task.pid = llPid;
    sSwt_sys_task.task_status[0] = cTASKRUNED;

/* add by gxz 2001.09.11 Begin */
    if ( alFlag != 1 )
    {
      if ( sSwt_sys_task.task_flag != 1 )
      {
        sSwt_sys_task.task_flag =9;
      }
    }
/* add by gxz 2001.09.11 End */
       
    sSwt_sys_task.start_time = llBegintime; /* very important */
    swVdebug(2,"S0380: 启动[%s]成功,PID=%ld",sSwt_sys_task.task_name,llPid);
    ilResult = SUCCESS;
  }

  /* 更新共享内存 */
  ilRc = swShmupdate_swt_sys_task(sSwt_sys_task.task_name, sSwt_sys_task);
  if ( ilRc != SUCCESS )
  {
    swVdebug(1,"S0390: [错误/共享内存] 共享内存中任务[%s]未找到",\
      sSwt_sys_task.task_name); 
    return(FAIL);
  }
  swVdebug(4,"S0400: [函数返回] swExecSubProcess()返回码=%d",ilResult);
  return(ilResult); 
}


/**************************************************************
 ** 函数名:  void swSigcld()
 ** 功  能:  处理 SIGCLD 信号 
 ** 作  者:
 ** 建立日期:
 ** 最后修改日期: 2001/08/15
 ** 调用其它函数:
 ** 全局变量:
 ** 参数含义:
 ** 返回值:  
***************************************************************/
void swSigcld(int iSig)
{
  pid_t llPid;
  int ilStat;
  int ilRc;
  long llPidcld;
  struct swt_sys_task slSwt_sys_task;

  swVdebug(4,"S0410: [函数调用] swSigcld(%d)",iSig);

/*  signal(SIGCLD, SIG_IGN);*/
  swDebug("szhengye:swSigcld:%d",iSig);

  while ((llPid = waitpid(0, &ilStat, WNOHANG)) > 0)
  {
    swVdebug(2,"S0420: 子进程退出 Waitpid(),pid=[%ld]",llPid);
    llPidcld = llPid;
    /* 查找已中断的任务 */

    ilRc = swShmselect_swt_sys_task_pid(llPidcld, &slSwt_sys_task);
    if (ilRc != SUCCESS) 
    {
      swVdebug(2,"S0430: 查找 PID = [%ld] 的任务失败 !", llPidcld);
      continue;
    }
    
    /* 判断是否支持重启 */
    if (slSwt_sys_task.restart_flag[0] == '1') 
    { 
      /*比较是否达到最大重起次数*/ 
      if (slSwt_sys_task.restart_max > slSwt_sys_task.restart_num) 
      {
        slSwt_sys_task.restart_num++; 

        /* 解析进程程序路径 */
        _swTrim(slSwt_sys_task.task_name); 
        _swTrim(slSwt_sys_task.task_file);
        Parse(slSwt_sys_task.task_file, agargs);

        /* 启动子进程 */
        swExecSubProcess( 2,slSwt_sys_task );  
      }
      /* fzj: 2002.9.17: 进程重起次数达到最大次数时应将状态置DOWN */
      else
      {
        slSwt_sys_task.pid = -1;
        slSwt_sys_task.start_time = 0; 
        slSwt_sys_task.task_status[0] = cTASKDOWN; 
        ilRc=swShmupdate_swt_sys_task(slSwt_sys_task.task_name,slSwt_sys_task);
        if (ilRc) 
        {
          swVdebug(1,"S0440: [错误/共享内存] 共享内存中任务[%s]未找到",\
            slSwt_sys_task.task_name); 
          continue;
        }
      }
      /**/
      /* add by 顾晓忠 2001.09.05 Begin */
      /* 处理定时启动、关闭任务 */
      /*
      swTasktimer();
      */
      /* add by 顾晓忠 2001.09.05 End */
    }
    else /* 不支持自动重启 */
    {
      slSwt_sys_task.pid = -1;
      slSwt_sys_task.start_time = 0;
      strcpy(slSwt_sys_task.task_status, "5");
      swVdebug(2,"S0450: [%s] 设定为不可重起， 不自动重起",\
        slSwt_sys_task.task_name);

      /* add by gxz 2001.09.11 Begin */
      if ( slSwt_sys_task.task_flag != 1 )
        slSwt_sys_task.task_flag = 0;
      /* add by gxz 2001.09.11 End */

      /* 更新共享内存 */
      ilRc = swShmupdate_swt_sys_task(slSwt_sys_task.task_name, slSwt_sys_task);
      if (ilRc) 
      {
        swVdebug(1,"S0460: [错误/共享内存] 共享内存中任务[%s]未找到",\
          slSwt_sys_task.task_name); 
        continue;
      }
    }
  }  /* end of while */
  signal( SIGCLD, swSigcld );
}

/**************************************************************
 ** 函数名:  int Parse()
 ** 功  能:
 ** 作  者:
 ** 建立日期:
 ** 最后修改日期:
 ** 调用其它函数:
 ** 全局变量:
 ** 参数含义:
 ** 返回值:  SUCCESS , FAIL
***************************************************************/
/* begin of deleted by fzj at 2002.03.04 */
/*
int Parse(buf,args)
char *buf;
char * * args;
{
  char *buf1;

  swVdebug(4,"S0470: [函数调用] Parse()");

  buf1 = (char *) malloc(1024); 
  memcpy(buf1, buf, 1023);
  
  while (*buf1 != '\0')
  {
    while ((*buf1 == ' ') || (*buf1 == '\t') || (*buf1 == '\n'))
      *buf1++ = '\0';

    * args++ = buf1;

    while ((*buf1 != '\0') && (*buf1 != ' ') && (*buf1 != '\t') \
           && (*buf1 != '\n'))
      buf1++;
  }
  *args = '\0';
  return(0);
}
*/
/* end of deleted by fzj at 2002.03.04 */

/* begin of added by fzj at 2002.03.04 */
int Parse(char *buf, char args[][101])
{
  int i,ilLen;
  char *plBuf,*plArg;

  for(i=0;i<10;i++) args[i][0] = '\0';

  i = 0;
  plBuf = buf;
  while(*plBuf != '\0')
  {
    while(*plBuf==' ' || *plBuf=='\t' || *plBuf=='\n')
    {
      plBuf++;
      continue;
    }
    plArg = plBuf;
    ilLen = 0;
    while(*plBuf!=' ' && *plBuf!='\t' && *plBuf!='\n')
    {
      if(*plBuf=='\0') break;
      ilLen++;
      plBuf++;
      continue;
    }
    memcpy(args[i],plArg,ilLen);
    args[i][ilLen] = '\0';
    i++;
  }
  return(0);
}
/* end of added by fzj at 2002.03.04 */

/**************************************************************
 ** 函数名:  int swChkSigkill()
 ** 功  能:  取得设定平台进程中的 kill_id
 ** 作  者:
 ** 建立日期:
 ** 最后修改日期:
 ** 调用其它函数:
 ** 全局变量:
 ** 参数含义:
 ** 返回值:  返回当前的平台进程之 kill_id 设定值，如空则为 9
***************************************************************/
int swChkSigkill()
{
  short ilRc;

  swVdebug(4,"S0480: [函数调用] swChkSigkill()");

  if ((sgSwt_sys_task.kill_id == 0))
  {
    ilRc = 9;
  }
  else
  {
    ilRc = sgSwt_sys_task.kill_id;
  }

  swVdebug(4,"S0490: [函数返回] swChkSigkill()返回码=%d",ilRc);
  return(ilRc);
}

/**************************************************************
 ** 函数名:  void swQuit()
 ** 功  能:  关闭前置进程
 ** 作  者:
 ** 建立日期:
 ** 最后修改日期: 2001/08/15
 ** 调用其它函数:
 ** 全局变量: sgSwt_sys_task
 ** 参数含义:
 ** 返回值:
***************************************************************/
void   swQuit()                          
{
  short  ilSigkill;
  short  ilRc;
  short  i;
  /* del by nh 20020807
  struct swt_sys_task pslSwt_sys_task[iMAXTASKREC];*/
  struct swt_sys_task *pslSwt_sys_task;/* add by nh 20020807*/
  short ilCount;  

  signal(SIGCLD, SIG_IGN);
  swVdebug(1,"S1111: 任务管理进程退出");
  
  qdetach();
 
  /* 选择所有记录，按 stop_id 排序 */
  /* modify by nh 20020807
  ilRc = swShmselect_swt_sys_task_all_p(pslSwt_sys_task, &ilCount);*/
  ilRc = swShmselect_swt_sys_task_all_p(&pslSwt_sys_task, &ilCount);
  if (ilRc)
  {
    swVdebug(2,"S0500: 操作共享内存任务表出错");
    exit (-1);
  }
  for (i = 0; i < ilCount; i++)
  {
    memset(&sgSwt_sys_task, 0x00, sizeof(sgSwt_sys_task));
    memcpy(&sgSwt_sys_task, &pslSwt_sys_task[i], sizeof(struct swt_sys_task));
    /* 关闭已启动的平台进程 */
    if (sgSwt_sys_task.pid > 0)
    {
      _swTrim(sgSwt_sys_task.task_name);
      _swTrim(sgSwt_sys_task.task_file);
      Parse(sgSwt_sys_task.task_file, agargs);

      ilRc = kill(sgSwt_sys_task.pid, 0);
      if ( ilRc == 0)
      {
	ilSigkill = swChkSigkill();
	kill(sgSwt_sys_task.pid, ilSigkill); 
	swVdebug(2,"S0510: kill [%s] 进程", sgSwt_sys_task.task_name);
        sleep ( sgSwt_sys_task.stop_wait );
      }
    }

    strcpy(sgSwt_sys_task.task_status, "4");
    sgSwt_sys_task.pid = -1;
    sgSwt_sys_task.start_time = 0;
    sgSwt_sys_task.restart_num = 0;

/* add by gxz 2001.09.11 Begin */
    if ( sgSwt_sys_task.task_flag != 1 )
      sgSwt_sys_task.task_flag = 0;
/* add by gxz 2001.09.11 End */

    /* 更新共享内存中任务状态 */
    ilRc = swShmupdate_swt_sys_task(sgSwt_sys_task.task_name, sgSwt_sys_task);
    if (ilRc) 
    {
      swVdebug(1,"S0520: [错误/共享内存] 更新共享内存中任务[%s]状态出错",\
        sgSwt_sys_task.task_name); 
      continue;
    }
  }
  swVdebug(2,"S0530: 关闭交换平台进程完毕");
  exit (0);
}         

