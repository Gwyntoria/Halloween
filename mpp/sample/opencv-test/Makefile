include ../Makefile.param

# CROSS_COMPILE= arm-hisiv100nptl-linux-
# CC= $(CROSS_COMPILE)gcc
# CXX= $(CROSS_COMPILE)g++

INC_FLAGS:=-I./include

SRCS := $(wildcard ./main.cpp)

LDFLAGS=-L./lib \
		-L./share/OpenCV/3rdparty/lib

.PHONY : clean

# target source
TARGET := opencv-test
OBJS := main.o
# OBJS  := $(SRCS:%.cpp=%.o)
CFLAGS += $(INC_FLAGS)
# CFLAGS += $(INC_FLAGS)
# CFLAGS += -lopencv_imgproc -lopencv_core -lopencv_highgui -fopenmp
OPENCV_LIBS = ./lib/libopencv_highgui.a \
			./lib/libopencv_imgproc.a \
			./lib/libopencv_core.a \
			./share/OpenCV/3rdparty/lib/libIlmImf.a \
			./share/OpenCV/3rdparty/lib/liblibjasper.a \
			./share/OpenCV/3rdparty/lib/liblibjpeg.a \
			./share/OpenCV/3rdparty/lib/liblibpng.a \
			./share/OpenCV/3rdparty/lib/liblibtiff.a \
			./share/OpenCV/3rdparty/lib/libzlib.a \


# -lIlmImf -llibjasper -llibjpeg -llibpng -llibtiff -lopencv_imgproc -lopencv_highgui -lopencv_core -fopenmp -lzlib


all: $(TARGET)


$(TARGET): $(OBJS)
	@$(CXX) $(CFLAGS) -o $@ $^  $(OPENCV_LIBS) -fopenmp -s

# @$(CXX) $(CFLAGS) -o $@ $^  -Wl, --start-group $(OPENCV_LIBS) -Wl, --end-group -fopenmp -s

#	@$(CXX) $(CFLAGS) -o $@ $^  ${LDFLAGS} -lopencv_core -lopencv_highgui -lopencv_imgproc -fopenmp -s
#	@$(CXX) $(CFLAGS) -o $@ $^ ${LDFLAGS}  -lopencv_imgproc -lopencv_highgui -lopencv_core -fopenmp -lIlmImf -llibjasper -llibjpeg -llibpng -llibtiff -lzlib

%.o: %.cpp
	@$(CXX) $(CFLAGS) -c -o $@ $<

clean:
	@rm -f $(TARGET)
	@rm -f $(OBJS)

echo:
	@echo $(CFLAGS)
	@echo
	@echo $(INC_FLAGS)


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