include $(ORACLE_HOME)/precomp/lib/env_precomp.mk

#ORACLE ͷ�ļ�
ORA_INCLUDE=include=$(ORACLE_HOME)/precomp/public \
	include=$(ORACLE_HOME)/rdbms/public \
	include=$(ORACLE_HOME)/rdbms/demo \
	include=$(ORACLE_HOME)/plsql/public 
	include=$(ORACLE_HOME)/network/public \
	include=$(HOME)/DayEnd/inc

#����ͷ�ļ�
INCLUDE=-I$(ORACLE_HOME)/precomp/public  \
	-I$(ORACLE_HOME)/precomp/public \
	-I$(ORACLE_HOME)/rdbms/public \
	-I$(ORACLE_HOME)/rdbms/demo \
	-I$(ORACLE_HOME)/plsql/public \
	-I$(HOME)/DayEnd/inc 

#ORACLE ���ļ�
ORALIBS=-L$(ORACLE_HOME)/lib -lclntsh

#proc �������
PROCPLSFLAGS= sqlcheck=syntax mode=oracle dbms=v8 unsafe_null=yes parse=none lines=yes
#PROCPLSFLAGS= sqlcheck=FULL mode=oracle dbms=v8 unsafe_null=yes parse=none USERID=$(BSW_NAME)/$(BSW_USER) lines=yes code=ANSI_C

#��������
CC=gcc
AR=ar
ESQL=proc

#�����б�
DEBUGFLAGS=-D_LINUXES_  -D_GNU_SOURCE
CFLAGS=-Wall -w -pedantic -g -c $(DEBUGFLAGS)


.SUFFIXES: .sqc .c .o
.PHONY:all

OBJ=DT_Main.c DT_Proc_DayEnd.o DT_Func_Db.o  DT_Proc_Mon.o DT_Func_Pub.o

all:DB_TOOLS 
	@echo "���ڱ���ALLģ��......"

DB_TOOLS:$(OBJ)
	@echo "���ڱ���DB_TOOLSģ��......"
	$(CC) -o DB_TOOLS $(OBJ) $(INCLUDE) $(ORALIBS)
	@cp DB_TOOLS ../bin
	@chmod 777 ../bin/DB_TOOLS

DT_Proc_DayEnd.o:
	$(ESQL) $(PROCPLSFLAGS) $(ORA_INCLUDE) iname=$*.sqc
	$(CC) $(CFLAGS) -o $*.o $(INCLUDE) -c $*.c 
	@rm -f $*.c $*.lis

DT_Func_Db.o:
	$(ESQL) $(PROCPLSFLAGS) $(ORA_INCLUDE) iname=$*.sqc
	$(CC) $(CFLAGS) -o $*.o $(INCLUDE) -c $*.c
	@rm -f $*.c $*.lis

DT_Proc_Mon.o:
	$(CC) $(CFLAGS) -o $*.o $(INCLUDE) $(INCDIR) -c $*.c

DT_Func_Pub.o:
	$(CC) $(CFLAGS) -o $*.o $(INCLUDE) $(INCDIR) -c $*.c
	@echo "DB_Func.o ...."

clean:
	@echo "��ʼ����.."
	@rm -f *.o *.lis DB_TOOLS core.*
