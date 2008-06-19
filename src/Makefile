CXXOPTFLAGS= -O3 -fsigned-char -fomit-frame-pointer -ffast-math -fno-strict-aliasing
INCLUDES= -Ishared -Iengine -Ifpsgame -Ienet/include -I/usr/X11R6/include `sdl-config --cflags` -Iinclude
CXXFLAGS= -Wall -DINTERFACE $(CXXOPTFLAGS) $(INCLUDES)

CLIENT_LIBS=-Lenet -lenet -L/usr/X11R6/lib `sdl-config --libs` -lSDL_image -lSDL_mixer -lpng -lz -lGL -lGLU
ifeq ($(shell uname -s),Linux)
CLIENT_LIBS+= -lrt
endif
CLIENT_OBJS= \
	shared/tools.o \
	shared/geom.o \
	engine/3dgui.o \
	engine/bih.o \
	engine/client.o	\
	engine/command.o \
	engine/console.o \
	engine/decal.o \
	engine/dynlight.o \
	engine/glare.o \
	engine/grass.o \
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
	fpsgame/fps.o 

SINCLUDES= -Ishared -Iengine -Ifpsgame -Ienet/include
SCXXFLAGS= -Wall -DSTANDALONE $(CXXOPTFLAGS) $(SINCLUDES)

SERVER_LIBS=-Lenet -lenet -lz
SERVER_OBJS= \
	shared/tools-standalone.o \
	engine/command-standalone.o \
	engine/server-standalone.o \
	fpsgame/fps-standalone.o

DINCLUDES= -Ishared -Iengine -Ifpsgame -Ienet/include
DCXXFLAGS= -Wall -DSTANDALONE -DMASTERSERVER $(CXXOPTFLAGS) $(DINCLUDES)

DAEMON_LIBS=-Lenet -lenet -lz
DAEMON_OBJS= \
	shared/tools-daemon.o \
	engine/command-daemon.o \
	engine/master-daemon.o \
	engine/server-daemon.o \
	fpsgame/fps-daemon.o

TINCLUDES= -Ishared `sdl-config --cflags`
TCXXFLAGS= -Wall -DTTF2FONT $(CXXOPTFLAGS) $(TINCLUDES)

TTF_LIBS= -L/usr/X11R6/lib `sdl-config --libs` -lSDL_image -lSDL_ttf -lpng -lz
TTF_OBJS= \
	shared/tools-ttf.o \
	shared/ttf2font-ttf.o

AINCLUDES= -Ishared `sdl-config --cflags`
ACXXFLAGS= -Wall -DIMG2ANIM $(CXXOPTFLAGS) $(TINCLUDES)

ANIM_LIBS= -L/usr/X11R6/lib `sdl-config --libs` -lSDL_image -lpng -lz
ANIM_OBJS= \
	shared/tools-anim.o \
	shared/img2anim-anim.o

default: all

enet/Makefile:
	cd enet; chmod +x ./configure; ./configure

libenet: enet/Makefile
	$(MAKE)	-C enet/ all

clean: clean-client clean-server enet/Makefile

clean-client:
	@rm -fv $(CLIENT_OBJS) bloodfrontier_client

clean-server:
	@rm -fv $(SERVER_OBJS) bloodfrontier_server

clean-daemon:
	@rm -fv $(DAEMON_OBJS) bloodfrontier_daemon

clean-ttf:
	@rm -fv $(TTF_OBJS) ttf2font

clean-anim:
	@rm -fv $(ANIM_OBJS) img2anim

%-standalone.o:
	$(CXX) $(SCXXFLAGS) -c -o $@ $(subst -standalone.o,.cpp,$@)

%-daemon.o:
	$(CXX) $(DCXXFLAGS) -c -o $@ $(subst -daemon.o,.cpp,$@)

%-ttf.o:
	$(CXX) $(TCXXFLAGS) -c -o $@ $(subst -ttf.o,.cpp,$@)

%-anim.o:
	$(CXX) $(ACXXFLAGS) -c -o $@ $(subst -anim.o,.cpp,$@)

client:	libenet $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o bloodfrontier_client $(CLIENT_OBJS) $(CLIENT_LIBS)
	strip bloodfrontier_client

server:	libenet $(SERVER_OBJS)
	$(CXX) $(SCXXFLAGS) -o bloodfrontier_server $(SERVER_OBJS) $(SERVER_LIBS)
	strip bloodfrontier_server

daemon:	libenet $(DAEMON_OBJS)
	$(CXX) $(DCXXFLAGS) -o bloodfrontier_daemon $(DAEMON_OBJS) $(DAEMON_LIBS)
	strip bloodfrontier_daemon

ttf: $(TTF_OBJS)
	$(CXX) $(TCXXFLAGS) -o ttf2font $(TTF_OBJS) $(TTF_LIBS)
	strip ttf2font

anim: $(ANIM_OBJS)
	$(CXX) $(ACXXFLAGS) -o img2anim $(ANIM_OBJS) $(ANIM_LIBS)
	strip img2anim

install-client: client
	cp -v bloodfrontier_client ../bin/bloodfrontier_client
	chmod +x ../bin/bloodfrontier_client ../bloodfrontier.sh

install-server: server
	cp -v bloodfrontier_server ../bin/bloodfrontier_server
	chmod +x ../bin/bloodfrontier_server

install-daemon: daemon
	cp -v bloodfrontier_daemon ../bin/bloodfrontier_daemon
	chmod +x ../bin/bloodfrontier_daemon

install-ttf: ttf
	cp -v ttf2font ../bin/ttf2font
	chmod +x ../bin/ttf2font

install-anim: anim
	cp -v img2anim ../bin/img2anim
	chmod +x ../bin/img2anim

install: install-client install-server

all: client server
