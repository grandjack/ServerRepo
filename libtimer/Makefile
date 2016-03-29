
define TIMER_SOCKET_FILE_PATH   "."
endef


CC := g++
CFLAGS = -Wall
LIBTIMER := libtimer.so
LIBUNIXDOMAINAPI := libunixsktapi.so
TEST1 := test1
TEST2 := test2

all: $(LIBTIMER) $(LIBUNIXDOMAINAPI)

$(LIBUNIXDOMAINAPI): unixsktapi.c
	$(CC) $(CFLAGS) -fPIC -shared -o $(LIBUNIXDOMAINAPI) $<

$(LIBTIMER): libtimer.c $(LIBUNIXDOMAINAPI)
	$(CC) $(CFLAGS) -DTIMER_SOCKET_FILE_PATH=\".\" -DTIMER_LOG_PATH=\".\" -fPIC -shared -o $(LIBTIMER) $< -lpthread -lrt -L./ -lunixsktapi
	

$(TEST1) : main.c
	$(CC) $(CFLAGS) -o $@ $< -L./ -ltimer -lunixsktapi
	
$(TEST2) : main2.c
	$(CC) $(CFLAGS) -o $@ $< -L./ -ltimer -lunixsktapi
	
.PHONY:clean $(TEST1) $(TEST2)
clean:
	-rm -f $(LIBTIMER) $(LIBUNIXDOMAINAPI) $(TEST1) $(TEST2) *.o
