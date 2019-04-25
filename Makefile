.PHONY: master node test_protobuf VvfsTP 
all:clean master
master:
	g++ ./md5.cpp ./common.cpp ./logger.cpp ./msg.pb.cc ./udp_server.cpp ./master.cpp -o master -lprotobuf -lpthread -std=c++11
sync:
	g++ ./udp_server.cpp ./sync_server.cpp ./sync.cpp -o sync -std=c++11

node:
	g++ ./md5.cpp ./msg.pb.cc ./vvfs.cpp ./common.cpp ./logger.cpp ./udp_server.cpp ./node.cpp -g -o node -std=c++11 -lpthread -lprotobuf -DLOG_DEBUG

VvfsTP:
	g++ ./logger.cpp ./md5.cpp ./msg.pb.cc ./common.cpp ./VvfsTP.cpp  -g -o VvfsTP -std=c++11 -lpthread -lprotobuf

msg_proto:
	protoc --cpp_out=. msg.proto

test_protobuf:
	g++ ./msg.pb.cc ./test_protobuf.cpp -o test_protobuf -std=c++11 -lprotobuf -lpthread

clean:
	rm master

