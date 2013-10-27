INCLUDES = \
		   -I../common_src \
		   -I../proto_src

AM_CXXFLAGS =

gate_LDADD = -lpthread -levent -lprotobuf

bin_PROGRAMS = gate 

gate_SOURCES = \
			   ../common_src/cb.cpp \
			   ../common_src/log.cpp \
			   ../common_src/msg_protobuf.cpp \
			   ../common_src/net.cpp \
			   ../proto_src/cmd_util.cpp \
			   ../proto_src/test.pb.cc \
			   client_cb.cpp \
			   gate_cb.cpp \
			   main.cpp

