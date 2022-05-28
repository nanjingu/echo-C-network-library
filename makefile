DIR_BIN = .
DIR_OBJ = ./obj
DIR_SRC = ./echo
EXAMPLE = ./example
PROTO = ./example/mytest.pb.cc
SRC = $(wildcard $(DIR_SRC)/*.cc)
OBJ = $(patsubst %.cc,$(DIR_OBJ)/%.o,$(notdir $(SRC)))
SERVER_SOURCES = server.cc
CLIENT_SOURCES = client.cc
CLIENTOBJ = $(addsuffix .o, $(basename $(CLIENT_SOURCES))) 
SERVEROBJ = $(addsuffix .o, $(basename $(SERVER_SOURCES))) 

CXX_FLAG = -g -Wall -std=c++11 -pthread -lprotobuf -O3
CC = g++

.PHONY:all
all: server client

server : $(OBJ) $(EXAMPLE)/$(SERVEROBJ)
	$(CC) $(PROTO) $(CXX_FLAG) -o $@ $^

client : $(OBJ) $(EXAMPLE)/$(CLIENTOBJ)
	$(CC) $(PROTO) $(CXX_FLAG) -o $@ $^

$(EXAMPLE)/%.o : $(EXAMPLE)/%.cc
	$(CC) $(CXX_FLAG) -c $< -o $@

$(DIR_OBJ)/%.o : $(DIR_SRC)/%.cc
	if [ ! -d $(DIR_OBJ) ];	then mkdir -p $(DIR_OBJ); fi;
	$(CC) $(CXX_FLAG) -c $< -o $@


.PHONY : clean
clean : 
	-rm -rf $(DIR_OBJ)
