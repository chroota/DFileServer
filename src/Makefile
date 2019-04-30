CXX = g++

CXXFLAGS = -g -std=c++11

LIBS = -lprotobuf -lpthread -lcrypto

INCLUDE_DIR = ./include

INCLUDE_FLAG = -I$(INCLUDE_DIR) 

OBJ_DIR = objs

OBJS = $(OBJ_DIR)/md5.o $(OBJ_DIR)/common.o $(OBJ_DIR)/msg.o $(OBJ_DIR)/tcp_server.o $(OBJ_DIR)/udp_server.o $(OBJ_DIR)/msg.pb.o $(OBJ_DIR)/logger.o

MASTER_OBJS = $(OBJ_DIR)/master.o

NODE_OBJS = $(OBJ_DIR)/node.o $(OBJ_DIR)/vvfs.o $(OBJ_DIR)/pushfile_server.o

TP_OBJS = $(OBJ_DIR)/tp.o

TARGET_DIR = build


.PHONY: 

$(OBJ_DIR)/logger.o: common/logger.cpp $(INCLUDE_DIR)/logger.hpp
	$(CXX) $(INCLUDE_FLAG) ./common/logger.cpp $(LIBS) $(CXXFLAGS) -c -o $@

$(OBJ_DIR)/md5.o: common/md5.cpp $(INCLUDE_DIR)/md5.hpp
	$(CXX) $(INCLUDE_FLAG) ./common/md5.cpp $(LIBS) $(CXXFLAGS)  -c -o $@

$(OBJ_DIR)/common.o: common/common.cpp $(INCLUDE_DIR)/common.hpp
	@cp ./common/msg.pb.h $(INCLUDE_DIR)
	$(CXX) $(INCLUDE_FLAG) ./common/common.cpp $(LIBS) $(CXXFLAGS) -c -o $@

$(OBJ_DIR)/msg.o: common/msg.cpp $(INCLUDE_DIR)/msg.hpp
	$(CXX) $(INCLUDE_FLAG) ./common/msg.cpp $(LIBS) $(CXXFLAGS) -c -o $@

$(OBJ_DIR)/tcp_server.o: common/tcp_server.cpp $(INCLUDE_DIR)/tcp_server.hpp
	$(CXX) $(INCLUDE_FLAG) ./common/tcp_server.cpp $(LIBS) $(CXXFLAGS) -c -o $@

$(OBJ_DIR)/udp_server.o: common/udp_server.cpp $(INCLUDE_DIR)/udp_server.hpp
	$(CXX) $(INCLUDE_FLAG) ./common/udp_server.cpp $(LIBS) $(CXXFLAGS) -c -o $@

$(OBJ_DIR)/master.o: master/master.cpp $(INCLUDE_DIR)/master.hpp
	$(CXX) $(INCLUDE_FLAG) ./master/master.cpp $(LIBS) $(CXXFLAGS) -c -o $@

$(OBJ_DIR)/pushfile_server.o: node/pushfile_server.cpp $(INCLUDE_DIR)/pushfile_server.hpp
	$(CXX) $(INCLUDE_FLAG) ./node/pushfile_server.cpp $(LIBS) $(CXXFLAGS) -c -o $@

$(OBJ_DIR)/msg.pb.o: common/msg.pb.cc include/msg.pb.h
	$(CXX) $(INCLUDE_FLAG) ./common/msg.pb.cc $(CXXFLAGS) -c -o $@

$(OBJ_DIR)/vvfs.o: node/vvfs.cpp $(INCLUDE_DIR)/vvfs.hpp
	$(CXX) $(INCLUDE_FLAG) ./node/vvfs.cpp $(CXXFLAGS) -c -o $@
	

$(OBJ_DIR)/node.o: node/node.cpp $(INCLUDE_DIR)/node.hpp
	$(CXX) $(INCLUDE_FLAG) ./node/node.cpp $(CXXFLAGS) -c -o $@

$(OBJ_DIR)/tp.o: tp/tp.cpp $(INCLUDE_DIR)/tp.hpp
	$(CXX) $(INCLUDE_FLAG) ./tp/tp.cpp $(LIBS) $(CXXFLAGS) -c -o $@

msg_proto: msg.proto
	protoc --cpp_out=. msg.proto
	@mv msg.pb.cc common
	@mv msg.pb.h include

master: $(OBJS) $(MASTER_OBJS)
	$(CXX) $(INCLUDE_FLAG) $(OBJS) $(MASTER_OBJS) $(LIBS)  $(CXXFLAGS) -o $(TARGET_DIR)/master

node: $(OBJS) $(NODE_OBJS)
	$(CXX) $(INCLUDE_FLAG) $(OBJS) $(NODE_OBJS) $(LIBS) $(CXXFLAGS) -o $(TARGET_DIR)/node

tp: $(OBJS) $(TP_OBJS)
	$(CXX) $(INCLUDE_FLAG) $(OBJS) $(TP_OBJS) $(LIBS) $(CXXFLAGS) -o $(TARGET_DIR)/tp

all: master node tp
	

clean:
	rm $(OBJ_DIR)/*.o
