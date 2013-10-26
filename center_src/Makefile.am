INCLUDES = \
		   -I../common_src \
		   -I../proto_src

AM_CXXFLAGS =

center_LDADD = -lpthread -levent -lprotobuf

bin_PROGRAMS = center 

center_SOURCES = \
			   ../common_src/callback.cpp \
			   ../common_src/msg_protobuf.cpp \
			   ../common_src/net.cpp \
			   ../proto_src/cmd_util.cpp \
			   ../proto_src/test.pb.cc \
			   gate_rpc_callback.cpp \
			   login_rpc_callback.cpp \
			   main.cpp

