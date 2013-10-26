INCLUDES = \
		   -I../common_src \
		   -I../proto_src

AM_CXXFLAGS =

login_LDADD = -lpthread -levent -lprotobuf

bin_PROGRAMS = login

login_SOURCES = \
			   ../common_src/callback.cpp \
			   ../common_src/msg_protobuf.cpp \
			   ../common_src/net.cpp \
			   ../proto_src/test.pb.cc \
			   client_rpc_callback.cpp \
			   center_rpc_callback.cpp \
			   main.cpp

