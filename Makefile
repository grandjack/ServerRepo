
CC          :=g++
LDFLAGS     := -L$(HOME)/usr/bin/usr/local/lib -L/data/gbx386/myProjects/mysql-5.7.11-linux-glibc2.5-x86_64/lib
LIB         :=$(LDFLAGS) -lpthread -levent -lprotobuf -lmysqlclient
CXXFLAGS    :=-Wall -I/data/gbx386/myProjects/mysql-5.7.11-linux-glibc2.5-x86_64/include -I/data/gbx386/myProjects/protobuf-2.4.1/src 
CXXFLAGS    += -I$(HOME)/usr/bin/usr/local/include
CXXFLAGS    += -Werror -g
SOURCE      :=mainthread.cpp thread.cpp server.cpp usersession.cpp debug.cpp utils.cpp \
		chessboard.cpp stateauth.cpp statecommon.cpp stategameready.cpp \
		stategameplay.cpp message.pb.cpp mysqldb.cpp md5.cpp

OBJS        :=$(patsubst %.cpp, %.o, $(SOURCE))
TARGE       :=Server
TARGE_CLI   :=Client
TARGE_AD    :=AdInit

all : $(TARGE)

$(TARGE) : $(OBJS)
	@$(CC) $(OBJS) $(LIB) -o $(TARGE)
	@echo "Generate $(TARGE)"

%.o: %.cpp
	@$(CC) $(CXXFLAGS) -c $< -o $@
	@echo "Compiling $<"
    
CLIENT_SRC	:= client.cpp message.pb.cpp
$(TARGE_CLI) :$(CLIENT_SRC)
	@echo "Generating Client..."
	@g++ -Wall $(CXXFLAGS) $(LIB) -o $@ $(CLIENT_SRC)

.PHONY:clean install prepare client ad

client:$(TARGE_CLI)

ad : $(TARGE_AD)
TARGE_AD_SRC := md5.cpp adinit.cpp debug.cpp utils.cpp mysqldb.cpp
$(TARGE_AD) : $(TARGE_AD_SRC)
	@echo "Generating AdInit..."
	@g++ -Wall $(CXXFLAGS) $(LIB) -o $@ $(TARGE_AD_SRC)

install:
	cp $(TARGE) /usr/bin/$(TARGE)

prepare:
	../protobuf-2.4.1/src/protoc --cpp_out=./ ./message.proto
	@mv -f ./message.pb.cc ./message.pb.cpp

clean:
	@rm -rf $(OBJS)
	@rm -rf $(TARGE)
	@rm -rf $(TARGE_CLI)
	@rm -rf $(TARGE_AD)
	@rm -rf *.d
	@rm -rf *.o
