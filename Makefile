NAME		= RPGLauncher
VERSION		= 2.0

CC		= i686-w64-mingw32.static-gcc
LD		= i686-w64-mingw32.static-ld
WINDRES		= i686-w64-mingw32.static-windres

BIN_DIR		= bin
SRC_DIR		= src
OBJ_DIR		= obj

DEFINES		= #-DGUESS_ENCODING

COMMON_CFLAGS	= -std=c99 -Wall -Werror -O2 -I $(SRC_DIR)/common
DLL_CFLAGS	=
DLL_TARGET	= $(BIN_DIR)/$(NAME).dll
DLL_LDFLAGS	= -lshlwapi -lgdi32 -lole32 -lstrmiids -lshell32 -shared -O2
EXE_CFLAGS	=
EXE_TARGET	= $(BIN_DIR)/$(NAME).exe
EXE_LDFLAGS	= -mwindows -lshlwapi -lversion -O2

DLL_SRC_FILES	= $(wildcard $(SRC_DIR)/dll/*.c) $(wildcard $(SRC_DIR)/common/*.c)
EXE_SRC_FILES	= $(wildcard $(SRC_DIR)/exe/*.c) $(wildcard $(SRC_DIR)/common/*.c)

DLL_OBJ_FILES=$(DLL_SRC_FILES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
EXE_OBJ_FILES=$(EXE_SRC_FILES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o) $(OBJ_DIR)/exe/resource.o

all: $(EXE_TARGET) $(DLL_TARGET)

$(EXE_TARGET): $(EXE_OBJ_FILES) | $(BIN_DIR)
	$(CC) $(EXE_OBJ_FILES) -o $(EXE_TARGET) $(EXE_LDFLAGS)

$(DLL_TARGET): $(DLL_OBJ_FILES) | $(BIN_DIR)
	$(CC) $(DLL_OBJ_FILES) -o $(DLL_TARGET) $(DLL_LDFLAGS)

$(OBJ_DIR)/common/%.o: $(SRC_DIR)/common/%.c | $(OBJ_DIR)
	$(CC) -c $< -o $@ $(COMMON_CFLAGS) $(DEFINES)

$(OBJ_DIR)/exe/%.o: $(SRC_DIR)/exe/%.c | $(OBJ_DIR)
	$(CC) -c $< -o $@ $(COMMON_CFLAGS) $(EXE_CFLAGS) $(DEFINES)

$(OBJ_DIR)/dll/%.o: $(SRC_DIR)/dll/%.c | $(OBJ_DIR)
	$(CC) -c $< -o $@ $(COMMON_CFLAGS) $(DLL_CFLAGS) $(DEFINES)

$(OBJ_DIR)/exe/resource.o: $(SRC_DIR)/exe/resource.rc $(SRC_DIR)/icon.ico $(BIN_DIR)/RPGLauncher.dll
	$(WINDRES) $(DEFINES) $< $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)/exe $(OBJ_DIR)/dll $(OBJ_DIR)/common

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR)

dist:
	rm -f $(NAME)-$(VERSION).zip
	zip -9 -j $(NAME)-$(VERSION).zip $(BIN_DIR)/$(NAME).exe README.md LICENSE

distclean: clean
	rm -f $(NAME)-$(VERSION).zip

.PHONY: all clean dist
