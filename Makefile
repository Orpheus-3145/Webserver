SHELL := /bin/bash

NAME := webserv
SRC_DIR := src
OBJ_DIR := obj
INC_DIR := inc
DEP_DIR := deps
SOURCES := $(shell find $(SRC_DIR) -type f -name '*.cpp')
OBJECTS := $(patsubst $(SRC_DIR)%,$(OBJ_DIR)%,$(SOURCES:.cpp=.o))
DEPS := $(patsubst $(SRC_DIR)%,$(DEP_DIR)%,$(SOURCES:.cpp=.d))

CLI_DIR := _client
CLIENT := $(CLI_DIR)/webclient
CLIENT_SRCS := $(shell find $(CLI_DIR) -type f -name '*.cpp')
CLIENT_INCS := $(shell find $(CLI_DIR) -type f -name '*.hpp')

CC := c++
INC_FLAGS := -I$(INC_DIR) -I$(INC_DIR)/http -I$(INC_DIR)/parser -I$(INC_DIR)/server -I$(INC_DIR)/CGI
CPP_FLAGS := -Wall -Wextra -Werror -Wshadow -Wpedantic -std=c++17 -g3
DEP_FLAGS = -MMD -MF $(DEP_DIR)/$*.d

GREEN := \x1b[32;01m
RED := \x1b[31;01m
BLUE := \x1b[34;01m
RESET := \x1b[0m

all: $(NAME)

run: $(NAME)
	clear
	@./$(NAME)

$(NAME): $(OBJECTS)
	@$(CC) $(CPP_FLAGS) $(INC_FLAGS) $^ -o $@
	@printf "(WebServ) $(GREEN)Created program $@$(RESET)\n"

$(OBJ_DIR) $(DEP_DIR):
	@mkdir -p $@

-include $(DEPS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp Makefile | $(DEP_DIR) $(OBJ_DIR)
	@mkdir -p $(dir $@)
	@mkdir -p $(subst $(OBJ_DIR),$(DEP_DIR),$(dir $@))
	@$(CC) $(CPP_FLAGS) $(INC_FLAGS) $(DEP_FLAGS) -c $< -o $@
	@printf "(WebServ) $(BLUE)Created object $$(basename $@)$(RESET)\n"

client: $(CLIENT)
	@clear
	@./$(CLIENT) "localhost" "8080"

$(CLIENT): $(CLIENT_SRCS) $(CLIENT_INCS)
	@$(CC) $(CPP_FLAGS) $^
	@printf "(WebServ) $(GREEN)Created program $@$(RESET)\n"

clean:
	@for file in $(OBJECTS); do \
		rm -f $$file; \
		printf "(WebServ) $(RED)Removed object $$(basename $$file)$(RESET)\n"; \
	done
	@-rm -rf $(OBJ_DIR)
	@for file in $(DEPS); do \
		rm -f $$file; \
		printf "(WebServ) $(RED)Removed dependency $$(basename $$file)$(RESET)\n"; \
	done
	@-rm -rf $(DEP_DIR)

fclean: clean
	@-rm -f $(NAME)
	@printf "(WebServ) $(RED)Removed executable $(NAME)$(RESET)\n"
	@-rm -f $(CLIENT)
	@printf "(WebServ) $(RED)Removed executable $(CLIENT)$(RESET)\n"

re: fclean all

.PHONY: all run client clean fclean re

.DEFAULT_GOAL:=all