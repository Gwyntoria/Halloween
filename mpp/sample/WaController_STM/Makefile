include ../Makefile.param


SRCS := $(wildcard ./cJSON.c)
SRCS += $(wildcard ./common.c)
SRCS += $(wildcard ./ConfigParser.c)
SRCS += $(wildcard ./controller.c)
SRCS += $(wildcard ./Detection.c)
SRCS += $(wildcard ./http_parser.c)
SRCS += $(wildcard ./http.c)
SRCS += $(wildcard ./krypton.c)
SRCS += $(wildcard ./lotoSerial.c)
SRCS += $(wildcard ./md5.c)
SRCS += $(wildcard ./vector.c)
SRCS += $(wildcard ./WaController.c)
SRCS += $(wildcard ./WaInit.c)

RTMP_LIBS=$(REL_LIB)/libmpi.a \
	$(REL_LIB)/libhdmi.a \
	../librtmp/librtmp.a \
	../zlib-1.2.8/arm/lib/libz.a \
	../openssl/lib/libssl.a \
	../openssl/lib/libcrypto.a \

LDLIBSOPTIONS=-L./lib/

.PHONY : clean all
# target source
TARGET := loto_rtmp_402
OBJS  := $(SRCS:%.c=%.o)
CFLAGS += $(INC_FLAGS)

all: $(TARGET)

$(TARGET):$(OBJS) $(COMM_OBJ)
	@$(CXX) $(CFLAGS) -lpthread -lm -o $@ $^  $(RTMP_LIBS) $(AUDIO_LIBA) $(JPEGD_LIBA) ${LDLIBSOPTIONS} -leasyaacencoder -s

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