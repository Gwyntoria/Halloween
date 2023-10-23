include ../Makefile.param

# CROSS_COMPILE= arm-hisiv100nptl-linux-
# CC= $(CROSS_COMPILE)gcc
# CXX= $(CROSS_COMPILE)g++


SRCS := $(wildcard ./main.c)

.PHONY : clean

# target source
TARGET := sync-time
OBJS := main.o

all: $(TARGET)


$(TARGET): $(OBJS)
	@$(CC) $(CFLAGS) -o $@ $^ 


%.o: %.c
	@$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@rm -f $(TARGET)
	@rm -f $(OBJS)

echo:
	@echo $(CFLAGS)
	@echo


# arm-hisiv100nptl-linux-g++ main.cpp -o opencv-test -I./include -L./lib -L./share/OpenCV/3rdparty/lib -lIlmImf -llibjasper -llibjpeg -llibpng -llibtiff -lopencv_imgproc -lopencv_highgui -lopencv_core   -fopenmp -lzlib



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