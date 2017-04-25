CC       = g++
LIBS     = -lrt -lcrypto -lcurl -lgssapi_krb5 -lz lib/libssh.a
LDFLAGS  =
DEFINES  = -D LIBSSH_STATIC
INCLUDES = -I include
CXXFLAGS = $(DEFINES) $(INCLUDES)

TARGET   = rcc
SRCS     = $(wildcard *.cc)
OBJS     = $(SRCS:.cc = .o)

$(TARGET) : $(OBJS)
	$(CC) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)
%.o : %.cc
	$(CC) $(CXXFLAGS) -c $<

clean :
	rm -rf *.o rcc
.PHONY:clean
