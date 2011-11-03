
include ../Make.conf

OBJS	= smm_chdir.o		\
	  smm_codepage.o	\
	  smm_cwd_alloc.o	\
	  smm_cwd_pop.o		\
	  smm_cwd_push.o	\
	  smm_fstat.o		\
	  smm_init.o		\
	  smm_wcstombs.o	\
	  smm_pathtrek.o	\
	  smm_pwuid.o		\
	  smm_signal_break.o	\
	  smm_time_diff.o	\
	  smm_time_get_epoch.o	\
	  smm_mbstowcs.o

ifeq	($(SYSTOOL),unix)
TESTKIT	= smm
else
TESTKIT	= smm.exe
endif
TARGET	= libsmm.a


.PHONY: all smm clean
	
#all: $(TESTKIT)
all: $(TARGET)

$(TESTKIT): $(TARGET) main.c
	$(CC) $(CFLAGS) -L. -o $@ main.c -lsmm

$(TARGET) : $(OBJS)
	$(AR) crus $(TARGET) $(OBJS)

clean:
	$(RM) $(TARGET) $(TESTKIT) $(OBJS)



