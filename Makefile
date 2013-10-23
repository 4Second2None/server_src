all:
	g++ -g -Wall -levent -lpthread -o user_mt_test user.cpp user_mt_test.cpp
clean:
	rm -f user_mt_test
