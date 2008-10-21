CXXOPTFLAGS= -O2 -fsigned-char -finline-functions -fomit-frame-pointer -fno-strict-aliasing
INCLUDES= -Ishared -Iengine -Igame -Ienet/include -I/usr/X11R6/include `sdl-config --cflags` -Iinclude
CXXFLAGS= -Wall -DINTERFACE -DIRC $(CXXOPTFLAGS) $(INCLUDES)

CLIENT_LIBS=-Lenet -lenet -L/usr/X11R6/lib `sdl-config --libs` -lSDL_image -lSDL_mixer -lpng -lz -lGL -lGLU
ifeq ($(shell uname -s),Linux)
CLIENT_LIBS+= -lrt
endif
CLIENT_OBJS= \
	shared/tools.o \
	shared/geom.o \
	engine/3dgui.o \
	engine/bih.o \
	engine/blob.o \
	engine/client.o	\
	engine/command.o \
	engine/console.o \
	engine/decal.o \
	engine/dynlight.o \
	engine/glare.o \
	engine/grass.o \
	engine/irc.o	\
	engine/lightmap.o \
	engine/main.o \
	engine/material.o \
	engine/menus.o \
	engine/normal.o	\
	engine/octa.o \
	engine/octaedit.o \
	engine/octarender.o \
	engine/physics.o \
	engine/pvs.o \
	engine/rendergl.o \
	engine/rendermodel.o \
	engine/renderparticles.o \
	engine/rendersky.o \
	engine/rendertext.o \
	engine/renderva.o \
	engine/server.o	\
	engine/serverbrowser.o \
	engine/shader.o \
	engine/shadowmap.o \
	engine/sound.o \
	engine/texture.o \
	engine/water.o \
	engine/world.o \
	engine/worldio.o \
	game/game.o 

SINCLUDES= -Ishared -Iengine -Igame -Ienet/include
SCXXFLAGS= -Wall -DSTANDALONE -DIRC $(CXXOPTFLAGS) $(SINCLUDES)

SERVER_LIBS=-Lenet -lenet -lz
SERVER_OBJS= \
	shared/tools-standalone.o \
	engine/command-standalone.o \
	engine/irc-standalone.o	\
	engine/server-standalone.o \
	game/game-standalone.o

DINCLUDES= -Ishared -Iengine -Igame -Ienet/include
DCXXFLAGS= -Wall -DSTANDALONE -DMASTERSERVER -DIRC $(CXXOPTFLAGS) $(DINCLUDES)

DAEMON_LIBS=-Lenet -lenet -lz
DAEMON_OBJS= \
	shared/tools-daemon.o \
	engine/command-daemon.o \
	engine/irc-daemon.o	\
	engine/master-daemon.o \
	engine/server-daemon.o \
	game/game-daemon.o

default: all

enet/Makefile:
	cd enet; chmod +x ./configure; ./configure

libenet: enet/Makefile
	$(MAKE)	-C enet/ all

clean: clean-client clean-server enet/Makefile

clean-client:
	@rm -fv $(CLIENT_OBJS) bfclient

clean-server:
	@rm -fv $(SERVER_OBJS) bfserver

clean-daemon:
	@rm -fv $(DAEMON_OBJS) bfdaemon

%-standalone.o:
	$(CXX) $(SCXXFLAGS) -c -o $@ $(subst -standalone.o,.cpp,$@)

%-daemon.o:
	$(CXX) $(DCXXFLAGS) -c -o $@ $(subst -daemon.o,.cpp,$@)

client:	libenet $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o bfclient $(CLIENT_OBJS) $(CLIENT_LIBS)
	strip bfclient

server:	libenet $(SERVER_OBJS)
	$(CXX) $(SCXXFLAGS) -o bfserver $(SERVER_OBJS) $(SERVER_LIBS)
	strip bfserver

daemon:	libenet $(DAEMON_OBJS)
	$(CXX) $(DCXXFLAGS) -o bfdaemon $(DAEMON_OBJS) $(DAEMON_LIBS)
	strip bfdaemon

install-client: client
	cp -v bfclient ../bin/bfclient
	chmod +x ../bin/bfclient ../bloodfrontier.sh

install-server: server
	cp -v bfserver ../bin/bfserver
	chmod +x ../bin/bfserver

install-daemon: daemon
	cp -v bfdaemon ../bin/bfdaemon
	chmod +x ../bin/bfdaemon

install: install-client install-server

all: client server
