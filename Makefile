.PHONY: master_sync node_sync test_protobuf VvfsTP 
all:clean master
master:
	g++ ./tcp_server.cpp ./http.cpp ./master.cpp -o master -std=c++11
sync:
	g++ ./udp_server.cpp ./sync_server.cpp ./sync.cpp -o sync -std=c++11

master_sync:
	g++ ./common.cpp ./logger.cpp ./msg.pb.cc ./udp_server.cpp ./node.cpp ./master_sync.cpp -o master_sync -lprotobuf -lpthread -std=c++11
	g++ testudp_client.cpp -o testudp_client
node_sync:
	g++ ./md5.cpp ./msg.pb.cc ./vvfs.cpp ./common.cpp ./logger.cpp ./udp_server.cpp ./node_sync.cpp -g -o node_sync -std=c++11 -lpthread -lprotobuf

VvfsTP:
	g++ ./logger.cpp ./md5.cpp ./msg.pb.cc ./common.cpp ./VvfsTP.cpp -g -o VvfsTP -std=c++11 -lpthread -lprotobuf

msg_proto:
	protoc --cpp_out=. msg.proto

test_protobuf:
	g++ ./msg.pb.cc ./test_protobuf.cpp -o test_protobuf -std=c++11 -lprotobuf -lpthread

clean:
	rm master

