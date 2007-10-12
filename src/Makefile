CXXOPTFLAGS= -O3 -fomit-frame-pointer
INCLUDES= -Ishared -Iengine -Ifpsgame -Ifpsgame/bot -Ienet/include -I/usr/X11R6/include `sdl-config --cflags`
CXXFLAGS= -Wall -fsigned-char $(CXXOPTFLAGS) $(INCLUDES)

CLIENT_LIBS=-Lenet -lenet -L/usr/X11R6/lib `sdl-config --libs` -lSDL_image -lSDL_mixer -lz -lGL -lGLU 
CLIENT_OBJS= \
	shared/tools.o \
	shared/geom.o \
	engine/3dgui.o \
	engine/bih.o \
	engine/client.o	\
	engine/command.o \
	engine/console.o \
	engine/cubeloader.o \
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
	fpsgame/fps.o \
	engine/joystick.o

SINCLUDES= -Ishared -Iengine -Ifpsgame -Ifpsgame/bot -Ienet/include
SCXXFLAGS= -Wall -fsigned-char $(CXXOPTFLAGS) $(SINCLUDES)

SERVER_LIBS=-Lenet -lenet -lz
SERVER_OBJS= \
	shared/tools-standalone.o \
	engine/command-standalone.o \
	engine/server-standalone.o \
	fpsgame/fps-standalone.o \

default: all

all: client server

enet/Makefile:
	cd enet; chmod +x ./configure; ./configure
	
libenet: enet/Makefile
	$(MAKE)	-C enet/ all

clean: enet/Makefile
	@rm -fv $(SERVER_OBJS) $(CLIENT_OBJS) bloodfrontier_server bloodfrontier_client
	$(MAKE)	-C enet/ clean

shared/tools-standalone.o:
	$(CXX) $(SCXXFLAGS) -DSTANDALONE	-c -o shared/tools-standalone.o shared/tools.cpp

engine/command-standalone.o:
	$(CXX) $(SCXXFLAGS) -DSTANDALONE	-c -o engine/command-standalone.o engine/command.cpp

engine/server-standalone.o:
	$(CXX) $(SCXXFLAGS) -DSTANDALONE	-c -o engine/server-standalone.o engine/server.cpp
	
fpsgame/fps-standalone.o:
	$(CXX) $(SCXXFLAGS) -DSTANDALONE	-c -o fpsgame/fps-standalone.o fpsgame/fps.cpp

client:	libenet $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o bloodfrontier_client $(CLIENT_OBJS) $(CLIENT_LIBS)
	strip bloodfrontier_client

server:	libenet $(SERVER_OBJS)
	$(CXX) $(SCXXFLAGS) -o bloodfrontier_server $(SERVER_OBJS) $(SERVER_LIBS)  
	strip bloodfrontier_server

install-client:
	cp -v bloodfrontier_client ../bin/bloodfrontier_client
	chmod +x ../bin/bloodfrontier_client ../bloodfrontier.sh

install-server:
	cp -v bloodfrontier_server ../bin/bloodfrontier_server
	chmod +x ../bin/bloodfrontier_server

install: install-client install-server
