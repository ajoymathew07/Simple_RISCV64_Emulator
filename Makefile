# This target is to ensure accidental execution of Makefile as a bash script 
# will not execute commands like rm in unexpected directories and exit gracefully.
.prevent_execution:
	exit 0

CXX = g++

#remove @ for no make command prints
DEBUG = @

# If you have a main.c file, this will generate an object file called main
# If you have another name for your main.c file, enter that in place of $(APP_NAME)
# Make sure the name of the file you enter here matches exactly with the file saved
# in your project directory.
APP_NAME = main
APP_SRC_FILES = $(APP_NAME).cpp

# The . means current directory. Make sure you keep the Makefile in the same
# directory as your project. 
MAIN_DIR = .

# The -I is a linux label to include. This command includes all files from 
# our include directory 
INCLUDE_DIRS = -I $(MAIN_DIR)/includes

# This command finds all .c files within our src folder
LIB_SRC_FILES = $(shell find $(MAIN_DIR)/src/ -name '*.cpp')

SRC_FILES += $(APP_SRC_FILES)
SRC_FILES += $(LIB_SRC_FILES)

# Compile command
MAKE_CMD = $(CXX) $(SRC_FILES) -o $(APP_NAME) $(INCLUDE_DIRS)

all:
	$(DEBUG)$(MAKE_CMD)

# This command is issued before you recompile the project after making changes
clean:
	rm -f $(MAIN_DIR)/$(APP_NAME)