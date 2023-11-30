VER_MAJOR:=0
VER_MINOR:=8
VER_BUILD:=7
BIN_VERSION:=$(VER_MAJOR).$(VER_MINOR).$(VER_BUILD)

include ../Makefile.param

# c source files
SRCS := $(wildcard *.c)

# c object files
C_OBJS  := $(patsubst %.c, %.o, $(SRCS))

TARGET := loto_rtmp_402

INC_FLAGS+=-I./include
RTMP_LIBS:=$(REL_LIB)/libmpi.a \
	$(REL_LIB)/libhdmi.a \

# librtmp
INC_FLAGS+=-I../librtmp
RTMP_LIBS+=../librtmp/librtmp.a

# zlib-1.2.8
INC_FLAGS+=-I../zlib-1.2.8/arm/include
RTMP_LIBS+=../zlib-1.2.8/arm/lib/libz.a

# openssl-1.0.1p
INC_FLAGS+=-I../openssl/include
RTMP_LIBS+=../openssl/lib/libssl.a
RTMP_LIBS+=../openssl/lib/libcrypto.a


# RTMP_LIBS=$(REL_LIB)/libmpi.a \
# 	$(REL_LIB)/libhdmi.a \
# 	../hisi_rtmp_hi3520d/lib/librtmp.a \
# 	../hisi_rtmp_hi3520d/lib/libz.a \
# 	../hisi_rtmp_hi3520d/lib/libssl.a \
# 	../hisi_rtmp_hi3520d/lib/libcrypto.a \

LDLIBSOPTIONS=-L./lib/

CFLAGS += $(INC_FLAGS)
# CFLAGS += -std=c99

.PHONY : clean all
# target source

all: $(TARGET)

$(TARGET):$(C_OBJS) $(COMM_OBJ)
	@$(CC) $(CFLAGS) -lpthread -lm -o $@ $^  $(RTMP_LIBS) $(AUDIO_LIBA) $(JPEGD_LIBA) ${LDLIBSOPTIONS} -lfdk-aac -s

%.o: %.c
	@$(CC) $(CFLAGS) -c -o $@ $<

release:
	@mkdir -p ./bin
	@cp $(TARGET) ./bin/$(TARGET)-$(BIN_VERSION)
	@cp $(TARGET) ../../../rtmp_pack/kit/root/WaController/

delete_release:
	@rm -f ./bin/$(TARGET)-$(BIN_VERSION)

rebuild_librtmp:
	make -C ../librtmp clean
	make -C ../librtmp

build_with_librtmp: rebuild_librtmp clean all

clean:
	@rm -f $(TARGET)
	@rm -f $(C_OBJS)
	@rm -f $(COMM_OBJ)

echo:
	@echo $(CFLAGS)
	@echo
	@echo $(C_OBJS) $(COMM_OBJ)
	@echo
	@echo $(CC)
	@echo




# # Hisilicon Hi3516 sample Makefile

# include ../Makefile.param
# #ifeq ($(SAMPLE_PARAM_FILE), )
# #     SAMPLE_PARAM_FILE:=../Makefile.param
# #     include $(SAMPLE_PARAM_FILE)
# #endif

# # target source
# SRCS := $(wildcard ./main.c)
# SRCS += $(wildcard ./ringfifo.c)
# SRCS += $(wildcard ./common.c)
# SRCS += $(wildcard ./Loto_venc.c)

# TARGET := loto_rtmp
# OBJ  := $(SRC:%.c=%.o)
# .PHONY : clean all

# all: $(TARGET)

# MPI_LIBS := $(REL_LIB)/libmpi.a
# MPI_LIBS += $(REL_LIB)/libhdmi.a

# RTMP_LIBS=./lib/libssl.a \
# 	./lib/libcrypto.a \
# 	../librtmp/librtmp.a \
# 	../zlib-1.2.8/arm/lib/libz.a \


# $(TARGET):%:%.o $(COMM_OBJ)
# 	$(CC) $(CFLAGS) -lpthread -lm -o $@ $^ $(MPI_LIBS) $(AUDIO_LIBA) $(JPEGD_LIBA) 

# clean:
# 	@rm -f $(TARGET)
# 	@rm -f $(OBJ)
# 	@rm -f $(COMM_OBJ)

# cleanstream:
# 	@rm -f *.h264
# 	@rm -f *.jpg
# 	@rm -f *.mjp
# 	@rm -f *.mp4