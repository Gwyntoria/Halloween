include ../Makefile.param

INC_FLAGS +=-I../librtmp -I./include

# INC_FLAGS +=-I../hisi_rtmp_hi3520d/include

SRCS := $(wildcard ./main.c)
SRCS += $(wildcard ./ringfifo.c)
SRCS += $(wildcard ./common.c)
SRCS += $(wildcard ./Loto_venc.c)
SRCS += $(wildcard ./Loto_aenc.c)
SRCS += $(wildcard ./ConfigParser.c)
SRCS += $(wildcard ./libfaac/aacquant.c)
SRCS += $(wildcard ./libfaac/backpred.c)
SRCS += $(wildcard ./libfaac/bitstream.c)
SRCS += $(wildcard ./libfaac/channels.c)
SRCS += $(wildcard ./libfaac/fft.c)
SRCS += $(wildcard ./libfaac/filtbank.c)
SRCS += $(wildcard ./libfaac/frame.c)
SRCS += $(wildcard ./libfaac/huffman.c)
SRCS += $(wildcard ./libfaac/ltp.c)
SRCS += $(wildcard ./libfaac/midside.c)
SRCS += $(wildcard ./libfaac/psychkni.c)
SRCS += $(wildcard ./libfaac/tns.c)
SRCS += $(wildcard ./libfaac/util.c)

RTMP_LIBS=$(REL_LIB)/libmpi.a \
	$(REL_LIB)/libhdmi.a \
	../librtmp/librtmp.a \
	../zlib-1.2.8/arm/lib/libz.a \
	../openssl/lib/libssl.a \
	../openssl/lib/libcrypto.a \
	# ./lib/libfaac.a \

# RTMP_LIBS=$(REL_LIB)/libmpi.a \
# 	$(REL_LIB)/libhdmi.a \
# 	../hisi_rtmp_hi3520d/lib/librtmp.a \
# 	../hisi_rtmp_hi3520d/lib/libz.a \
# 	../hisi_rtmp_hi3520d/lib/libssl.a \
# 	../hisi_rtmp_hi3520d/lib/libcrypto.a \

LDLIBSOPTIONS=-L./lib/

.PHONY : clean all
# target source
TARGET := loto_rtmp_402
OBJS  := $(SRCS:%.c=%.o)
CFLAGS += $(INC_FLAGS)

all: $(TARGET)

$(TARGET):$(OBJS) $(COMM_OBJ)
	@$(CXX) $(CFLAGS) -lpthread -lm -o $@ $^  $(RTMP_LIBS) $(AUDIO_LIBA) $(JPEGD_LIBA) ${LDLIBSOPTIONS} -leasyaacencoder -lfdk-aac -s

clean:
	@rm -f $(TARGET)
	@rm -f $(OBJS)
	@rm -f $(COMM_OBJ)


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