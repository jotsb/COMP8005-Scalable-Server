# make for select server

CC=gcc
CFLAGS=-Wall -ggdb -lpthread

exec: s_svr e_svr tcp_clnt clnt t_svr t_clnt clean_bak

s_svr: llist.o s_svr.o 
	$(CC) $(CFLAGS) llist.o s_svr.o -o s_svr

e_svr: llist.o e_svr.o 
	$(CC) $(CFLAGS) llist.o e_svr.o -o e_svr

tcp_clnt: 
	 $(CC) $(CFLAGS) -o tcp_clnt tcp_clnt.c

t_svr: 
	$(CC) $(CFLAGS) -o t_svr thread_svr.c

t_clnt:
	$(CC) $(CFLAGS) -o t_clnt thread_tcp_clnt.c
	
clnt:
	$(CC) $(CFLAGS) -o clnt Exec_Clnt.c

e_svr.o: e_svr.c
	$(CC) $(CFLAGS) -O -c e_svr.c
	
llist.o: llist.c
	$(CC) $(CFLAGS) -O -c llist.c

s_svr.o: s_svr.c
	$(CC) $(CFLAGS) -O -c s_svr.c

tcp_clnt.o: tcp_clnt.c
	$(CC) $(CFLAGS) -O -c tcp_clnt.c
	
clean:
	rm -f *.o *.bak tcp_clnt s_svr clnt e_svr t_svr t_clnt
	
clean_bak:
	rm -f *.o *.bak *.csv

#select_clnt.o: select_clnt.c
#	$(CC) $(CFLAGS) -O -c select_clnt.c
#s_clnt: select_clnt.o
#	$(CC) $(CFLAGS) -o s_clnt select_clnt.c
