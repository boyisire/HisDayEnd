INCLUDE=-I$(BMQ_PATH)/inc

CFLAGS=-g -c

SYSLIBS=-lsocket
#SYSLIBS=-lecpg -lpq
BMQLIBS=-L$(BMQ_PATH)/lib -lbmqapi 
BMQCLLIBSC=-L$(BMQ_PATH)/lib -lbmqclapi
BMQCLLIBS=-L$(BMQ_PATH)/lib -lbmqapi_c

EXE=$(BMQ_PATH)/bin

SRC=$(BMQ_PATH)/src
TMP=$(BMQ_PATH)/tmp
LIB=$(BMQ_PATH)/lib

CC=cc
AR=ar

all:	ser cli test cls

ser:	libbmqapi.a bmqstartup bmqshutdown bmqmng bmqClsrcv bmqClssnd bmqclean  bmqGrp_snd bmqGrp_rcv bmqSetload bmqmgr

cls:	libbmqclapi.a bmqcls

cli:    libbmqapi_c.a bmqstartup_c bmqshutdown_c bmqmng_c bmqclean_c  bmqSetload_c  bmqClcrcv bmqClcsnd

test: getlicense bmq_test bmq_mtest bmqcl_test bmqcl_mtest bmqrcv_test bmqrcvcl_test bmqsnd_test bmqsnd_mtest bmqsndcl_test bmqsndcl_mtest bmqtest bmq_rcv clean

clean:	
	mv *.o $(TMP)

.c.o:
	./getsrc bmq $*.c
	$(CC) $(CFLAGS) -DOS_SCO -o $*.o $(INCLUDE) $*.c
	./rmsrc $*.c

libbmqapi.a: bmqapi.o bmqLicense.o
	$(AR) rv $(LIB)/libbmqapi.a \
		bmqapi.o bmqLicense.o
	mv bmqapi.o bmqLicense.o $(TMP)

libbmqclapi.a:	bmqclapi.o
	$(AR) rv $(LIB)/libbmqclapi.a \
		bmqclapi.o
	mv bmqclapi.o $(TMP)

bmqcls:	bmqcls.o
	$(CC) -g -o $(EXE)/bmqcls bmqcls.o $(BMQLIBS) $(SYSLIBS)
	mv bmqcls.o $(TMP)

bmqstartup:	bmqstartup.o
	$(CC) -o $(EXE)/bmqstartup bmqstartup.o $(BMQLIBS)
	mv bmqstartup.o $(TMP)

bmqshutdown:	bmqshutdown.o
	$(CC) -o $(EXE)/bmqshutdown bmqshutdown.o $(BMQLIBS)
	mv bmqshutdown.o $(TMP)

bmqmng:	bmqmng.o
	$(CC) -g -o $(EXE)/bmqmng bmqmng.o $(BMQLIBS)
	mv bmqmng.o $(TMP)

bmqmgr: bmqmgr.o bmqwinapi.o
	$(CC) -g -o $(EXE)/bmqmgr bmqmgr.o bmqwinapi.o $(BMQLIBS) \
	-lcurses -ltermcap
	mv bmqmgr.o bmqwinapi.o $(TMP)

bmqClsrcv:	bmqClsrcv.o
	$(CC) -g -o $(EXE)/bmqClsrcv bmqClsrcv.o $(BMQLIBS) $(SYSLIBS)
	mv bmqClsrcv.o $(TMP)

bmqClssnd:	bmqClssnd.o
	$(CC) -g -o $(EXE)/bmqClssnd bmqClssnd.o $(BMQLIBS) $(SYSLIBS)
	mv bmqClssnd.o $(TMP)

bmqGrp_rcv:	bmqGrp_rcv.o
	$(CC) -g -o $(EXE)/bmqGrp_rcv bmqGrp_rcv.o $(BMQLIBS) $(SYSLIBS)
	mv bmqGrp_rcv.o $(TMP)

bmqGrp_snd:	bmqGrp_snd.o
	$(CC) -g -o $(EXE)/bmqGrp_snd bmqGrp_snd.o $(BMQLIBS) $(SYSLIBS)
	mv bmqGrp_snd.o $(TMP)

bmqclean:bmqclean.o
	$(CC) -g -o $(EXE)/bmqclean bmqclean.o $(BMQLIBS) $(SYSLIBS)
	mv bmqclean.o $(TMP)

bmqSetload:bmqSetload.o
	$(CC) -g -o $(EXE)/bmqSetload bmqSetload.o $(BMQLIBS) $(SYSLIBS)
	mv bmqSetload.o $(TMP)

libbmqapi_c.a:	bmqapi_c.o
	$(AR) rv libbmqapi_c.a bmqapi_c.o
	mv libbmqapi_c.a $(BMQ_PATH)/lib
	mv bmqapi_c.o $(TMP)

bmqstartup_c:	bmqstartup_c.o
	$(CC) -o $(EXE)/bmqstartup_c bmqstartup_c.o $(BMQCLLIBS)
	mv bmqstartup_c.o $(TMP)

bmqshutdown_c:	bmqshutdown_c.o
	$(CC) -o $(EXE)/bmqshutdown_c bmqshutdown_c.o $(BMQCLLIBS)
	mv bmqshutdown_c.o $(TMP)

bmqmng_c:	bmqmng_c.o
	$(CC) -g -o $(EXE)/bmqmng_c bmqmng_c.o $(BMQCLLIBS)
	mv bmqmng_c.o $(TMP)

bmqClcrcv:	bmqClcrcv.o
	$(CC) -g -o $(EXE)/bmqClcrcv bmqClcrcv.o $(BMQCLLIBS) $(SYSLIBS)
	mv bmqClcrcv.o $(TMP)

bmqClcsnd:	bmqClcsnd.o
	$(CC) -g -o $(EXE)/bmqClcsnd bmqClcsnd.o $(BMQCLLIBS) $(SYSLIBS)
	mv bmqClcsnd.o $(TMP)

bmqclean_c:bmqclean_c.o
	$(CC) -g -o $(EXE)/bmqclean_c bmqclean_c.o $(BMQCLLIBS) $(SYSLIBS)
	mv bmqclean_c.o $(TMP)

bmqSetload_c:bmqSetload_c.o
	$(CC) -g -o $(EXE)/bmqSetload_c bmqSetload_c.o $(BMQCLLIBS) $(SYSLIBS)
	mv bmqSetload_c.o $(TMP)

bmqMbset_c:bmqMbset_c.o
	$(CC) -g -o $(EXE)/bmqMbset_c bmqMbset_c.o $(BMQCLLIBS) $(SYSLIBS)
	mv bmqMbset_c.o $(TMP)

bmq_rcv:bmq_rcv.o
	$(CC) -g -o $(EXE)/bmq_rcv bmq_rcv.o $(BMQLIBS) $(SYSLIBS)
	mv bmq_rcv.o $(TMP)

bmq_snd:bmq_snd.o
	$(CC) -g -o $(EXE)/bmq_snd bmq_snd.o $(BMQLIBS) $(SYSLIBS)
	mv bmq_snd.o $(TMP)

filter_rcv:filter_rcv.o
	$(CC) -g -o $(EXE)/filter_rcv filter_rcv.o $(BMQLIBS) $(SYSLIBS)
	mv filter_rcv.o $(TMP)

filter_snd:filter_snd.o
	$(CC) -g -o $(EXE)/filter_snd filter_snd.o $(BMQLIBS) $(SYSLIBS)
	mv filter_snd.o $(TMP)

filter_rcvcl:filter_rcv.o
	$(CC) -g -o $(EXE)/filter_rcvcl filter_rcv.o $(BMQCLLIBS) $(SYSLIBS)
	mv filter_rcv.o $(TMP)

filter_sndcl:filter_snd.o
	$(CC) -g -o $(EXE)/filter_sndcl filter_snd.o $(BMQCLLIBS) $(SYSLIBS)
	mv filter_snd.o $(TMP)

bmqMbset:bmqMbset.o
	$(CC) -g -o $(EXE)/bmqMbset bmqMbset.o $(BMQLIBS) $(SYSLIBS)
	mv bmqMbset.o $(TMP)

getlicense:getlicense.o
	$(CC) -g -o $(EXE)/getlicense getlicense.o $(BMQLIBS) $(SYSLIBS)
	mv getlicense.o $(TMP)

bmqtest:bmqtest.o
	$(CC) -g -o $(EXE)/bmqtest bmqtest.o $(BMQLIBS) $(SYSLIBS)
	mv bmqtest.o $(TMP)

test1:test1.o
	$(CC) -g -o $(EXE)/test1 test1.o -lpthread $(BMQLIBS) $(SYSLIBS)
	mv test1.o $(TMP)

bmq_test:bmq_test.o
	$(CC) -g -o $(EXE)/bmq_test bmq_test.o $(BMQLIBS) $(SYSLIBS)

bmq_mtest:bmq_mtest.o
	$(CC) -g -o $(EXE)/bmq_mtest bmq_mtest.o $(BMQLIBS) $(SYSLIBS)

bmqsnd_test:bmqsnd_test.o
	$(CC) -g -o $(EXE)/bmqsnd_test bmqsnd_test.o $(BMQLIBS) $(SYSLIBS)

bmqsnd_mtest:bmqsnd_mtest.o
	$(CC) -g -o $(EXE)/bmqsnd_mtest bmqsnd_mtest.o $(BMQLIBS) $(SYSLIBS)

bmqrcv_test:bmqrcv_test.o
	$(CC) -g -o $(EXE)/bmqrcv_test bmqrcv_test.o $(BMQLIBS) $(SYSLIBS)

bmqcl_test:bmqcl_test.o
	$(CC) -g -o $(EXE)/bmqcl_test bmqcl_test.o $(BMQCLLIBS) $(SYSLIBS)

bmqsndcl_test:bmqsnd_test.o
	$(CC) -g -o $(EXE)/bmqsndcl_test bmqsnd_test.o $(BMQCLLIBS) $(SYSLIBS)

bmqrcvcl_test:bmqrcv_test.o
	$(CC) -g -o $(EXE)/bmqrcvcl_test bmqrcv_test.o $(BMQCLLIBS) $(SYSLIBS)

bmqcl_mtest:bmqcl_mtest.o
	$(CC) -g -o $(EXE)/bmqcl_mtest bmqcl_mtest.o $(BMQCLLIBS) $(SYSLIBS)

bmqsndcl_mtest:bmqsnd_mtest.o
	$(CC) -g -o $(EXE)/bmqsndcl_mtest bmqsnd_mtest.o $(BMQCLLIBS) $(SYSLIBS)

ftp:
	> bmqapi.c
	> bmqapi_c.c
	> bmqChgDebug.c
	> bmqclapi.c
	> bmqClcrcv.c
	> bmqClcsnd.c
	> bmqclean.c
	> bmqclean_c.c
	> bmqcl_mtest.c
	> bmqcls.c
	> bmqClsrcv.c
	> bmqClssnd.c
	> bmqcl_test.c
	> bmqGrp_rcv.c
	> bmqGrp_snd.c
	> bmqLicense.c
	> bmqMbset.c
	> bmqmgr.c
	> bmqmng.c
	> bmqmng_c.c
	> bmq_mtest.c
	> bmq_rcv.c
	> bmqrcv_test.c
	> bmqSetload.c
	> bmqSetload_c.c
	> bmqshutdown.c
	> bmqshutdown_c.c
	> bmq_snd.c
	> bmqsnd_mtest.c
	> bmqsnd_test.c
	> bmqstartup.c
	> bmqstartup_c.c
	> bmq_test.c
	> bmqtest.c
	> bmqwinapi.c
	> filter_rcv.c
	> filter_snd.c
	> getlicense.c
	make

