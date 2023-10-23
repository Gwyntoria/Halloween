include make.config
CP=cp -fv
MKDIR=mkdir -p
RM=rm -rf
CHMOD=chmod
MV=mv
CD=cd


#HI3520D_BOARD=SOCKET2
HI3520D_BOARD=DEMO


CC_OPT += -D_REENTRANT  -DHAVE_CONFIG_H -D_DETACHED_THREAD_  -D_FILE_OFFSET_BITS=64    -D$(HI3520D_BOARD)
CFLAGS := -Wall -D$(HIARCH) -DHICHIP=$(HICHIP) -D$(HIDBG) -D$(HI_FPGA) -D$(HI3520D_BOARD)    $(INC_FLAGS)  -lpthread -lm

LINK_OPT = -Llib
OBJ_DIR	= obj
BIN_DIR = bin
LIB_DIR = lib
SRC_DIR = src
APP_INC_DIR =include


LIBS += 



INC_DIRS = -I$(APP_INC_DIR)   -I$(APP_INC_DIR)/hi_include 


SRC_CODE  =  $(patsubst $(SRC_DIR)/%.c,%.c, $(wildcard $(SRC_DIR)/*.c)) \
$(patsubst $(SRC_DIR)/hisi/common/%.c,%.c, $(wildcard $(SRC_DIR)/hisi/common/*.c))


HEADERS   = $(wildcard $(APP_INC_DIR)/*.h) 
SRC_CODE_O= $(filter %.o,$(patsubst %.c, $(OBJ_DIR)/%.o, $(SRC_CODE))) \
$(filter %.o,$(patsubst %.cpp, $(OBJ_DIR)/%.o, $(SRC_CODE))) 

DIRS= $(OBJ_DIR) $(BIN_DIR) $(LIB_DIR) 

.PHONY: default
default:
	@echo $(SRC_CODE)
	$(MAKE) $(APP_BIN)
	@echo
	@echo
	@echo

.PHONY: cleanOBJ_SOURCE_C = ./$(SRC_DIR)/%.c	

clean:
	$(RM) -f $(OBJ_DIR)/*
	$(RM) -f $(APP_BIN) 

.PHONY: $(distclean)
distclean:
	$(clean)
	$(RM) -rf share
	$(RM) -rf $(LIB_DIR) 
		
build:	$(APP_BIN)

$(APP_BIN):	$(DIRS)	$(SRC_CODE_O) 
	$(LN) $(SRC_CODE_O) $(LIBS)  $(LINK_OPT)   -lrtmp -ldl -lssl -lcrypto -ldl -lz -lfaac  -lpthread -lm -static  -o $@	
	$(CHMOD) a+x $@
	$(RM) a.out
	$(STRIP) $(APP_BIN)

.PHONY: $(DIRS)
$DIRS):
	$(MKDIR) $@

OBJ_SOURCE_C = $(SRC_DIR)/%.c	
./$(OBJ_DIR)/%.o : $(OBJ_SOURCE_C)
	$(CC) $(CC_OPT) $(CFLAGS) $(INC_DIRS) -o $@ -c $<


OBJ_SOURCE_C = $(SRC_DIR)/hisi/common/%.c	
./$(OBJ_DIR)/%.o : $(OBJ_SOURCE_C)
	$(CC) $(CC_OPT) $(CFLAGS) $(INC_DIRS) -o $@ -c $<

$(SRC_CODE_O): $(HEADERS)
$(SRC_CODE_O): makefile
$(SRC_CODE_O): make.config

