INCLUDES = \
		   -I../common_src \
		   -I../proto_src

AM_CXXFLAGS =

game_LDADD = -lpthread -levent -lprotobuf

bin_PROGRAMS = game 

game_SOURCES = \
			   ../common_src/cb.cpp \
			   ../common_src/log.cpp \
			   ../common_src/msg_protobuf.cpp \
			   ../common_src/net.cpp \
			   ../proto_src/cmd_util.cpp \
			   ../proto_src/test.pb.cc \
			   gate_cb.cpp \
			   center_cb.cpp \
			   main.cpp

