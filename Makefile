all:
	g++ -g -Wall -levent -lpthread -o user_mt_test user.cpp user_mt_test.cpp
	g++ -g -Wall -levent -lprotobuf -lpthread -o gate user.cpp main.cpp thread.cpp callback.cpp rpc_callback.cpp msg_protobuf.cpp
clean:
	rm -f user_mt_test gate
