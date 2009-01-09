CXXOPTFLAGS= -O2 -finline-functions -fomit-frame-pointer
INCLUDES= -Ishared -Iengine -Igame -Ienet/include -I/usr/X11R6/include `sdl-config --cflags` -Iinclude
CXXFLAGS= -Wall -fsigned-char -DINTERFACE -DIRC $(CXXOPTFLAGS) $(INCLUDES)

CLIENT_LIBS=-Lenet -lenet -L/usr/X11R6/lib `sdl-config --libs` -lSDL_image -lSDL_mixer -lpng -lz -lGL
ifeq ($(shell uname -s),Linux)
CLIENT_LIBS+= -lrt
endif
CLIENT_OBJS= \
	shared/tools.o \
	shared/geom.o \
	engine/bih.o \
	engine/blend.o \
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
	engine/ui.o \
	engine/water.o \
	engine/world.o \
	engine/worldio.o \
	game/ai.o \
	game/client.o \
	game/ctf.o \
	game/entities.o \
	game/hud.o \
	game/physics.o \
	game/projs.o \
	game/server.o \
	game/stf.o \
	game/weapons.o \
	game/world.o

SINCLUDES= -Ishared -Iengine -Igame -Ienet/include
SCXXFLAGS= -Wall -fsigned-char -DSTANDALONE -DIRC $(CXXOPTFLAGS) $(SINCLUDES)

SERVER_LIBS=-Lenet -lenet -lz
SERVER_OBJS= \
	shared/tools-standalone.o \
	engine/command-standalone.o \
	engine/irc-standalone.o	\
	engine/server-standalone.o \
	game/server-standalone.o

DINCLUDES= -Ishared -Iengine -Igame -Ienet/include
DCXXFLAGS= -Wall -fsigned-char -DSTANDALONE -DMASTERSERVER -DIRC $(CXXOPTFLAGS) $(DINCLUDES)

DAEMON_LIBS=-Lenet -lenet -lz
DAEMON_OBJS= \
	shared/tools-daemon.o \
	engine/command-daemon.o \
	engine/irc-daemon.o	\
	engine/master-daemon.o \
	engine/server-daemon.o \
	game/server-daemon.o

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

depend:
	makedepend -Y -Ishared -Iengine -Igame shared/*.cpp engine/*.cpp game/*.cpp

all: client server
# DO NOT DELETE

shared/geom.o: shared/pch.h shared/tools.h shared/geom.h
shared/pch.o: shared/pch.h
shared/tools.o: shared/pch.h shared/tools.h
engine/bih.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/bih.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
engine/bih.o: shared/iengine.h shared/igame.h engine/sound.h engine/world.h
engine/bih.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
engine/bih.o: engine/model.h
engine/blend.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/blend.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
engine/blend.o: shared/iengine.h shared/igame.h engine/sound.h engine/world.h
engine/blend.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
engine/blend.o: engine/model.h
engine/blob.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/blob.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
engine/blob.o: shared/iengine.h shared/igame.h engine/sound.h engine/world.h
engine/blob.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
engine/blob.o: engine/model.h
engine/client.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/client.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
engine/client.o: shared/iengine.h shared/igame.h engine/sound.h
engine/client.o: engine/world.h engine/octa.h engine/lightmap.h engine/bih.h
engine/client.o: engine/texture.h engine/model.h
engine/command.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/command.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
engine/command.o: shared/iengine.h shared/igame.h engine/sound.h
engine/command.o: engine/world.h engine/octa.h engine/lightmap.h engine/bih.h
engine/command.o: engine/texture.h engine/model.h
engine/console.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/console.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
engine/console.o: shared/iengine.h shared/igame.h engine/sound.h
engine/console.o: engine/world.h engine/octa.h engine/lightmap.h engine/bih.h
engine/console.o: engine/texture.h engine/model.h
engine/decal.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/decal.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
engine/decal.o: shared/iengine.h shared/igame.h engine/sound.h engine/world.h
engine/decal.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
engine/decal.o: engine/model.h
engine/dynlight.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/dynlight.o: shared/command.h shared/geom.h shared/attrs.h
engine/dynlight.o: shared/ents.h shared/iengine.h shared/igame.h
engine/dynlight.o: engine/sound.h engine/world.h engine/octa.h
engine/dynlight.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/dynlight.o: engine/model.h
engine/glare.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/glare.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
engine/glare.o: shared/iengine.h shared/igame.h engine/sound.h engine/world.h
engine/glare.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
engine/glare.o: engine/model.h engine/rendertarget.h
engine/grass.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/grass.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
engine/grass.o: shared/iengine.h shared/igame.h engine/sound.h engine/world.h
engine/grass.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
engine/grass.o: engine/model.h
engine/lightmap.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/lightmap.o: shared/command.h shared/geom.h shared/attrs.h
engine/lightmap.o: shared/ents.h shared/iengine.h shared/igame.h
engine/lightmap.o: engine/sound.h engine/world.h engine/octa.h
engine/lightmap.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/lightmap.o: engine/model.h
engine/main.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/main.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
engine/main.o: shared/iengine.h shared/igame.h engine/sound.h engine/world.h
engine/main.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
engine/main.o: engine/model.h
engine/master.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/master.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
engine/master.o: shared/iengine.h shared/igame.h engine/sound.h
engine/master.o: engine/world.h engine/octa.h engine/lightmap.h engine/bih.h
engine/master.o: engine/texture.h engine/model.h shared/crypto.h shared/hash.h
engine/material.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/material.o: shared/command.h shared/geom.h shared/attrs.h
engine/material.o: shared/ents.h shared/iengine.h shared/igame.h
engine/material.o: engine/sound.h engine/world.h engine/octa.h
engine/material.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/material.o: engine/model.h
engine/menus.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/menus.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
engine/menus.o: shared/iengine.h shared/igame.h engine/sound.h engine/world.h
engine/menus.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
engine/menus.o: engine/model.h
engine/normal.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/normal.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
engine/normal.o: shared/iengine.h shared/igame.h engine/sound.h
engine/normal.o: engine/world.h engine/octa.h engine/lightmap.h engine/bih.h
engine/normal.o: engine/texture.h engine/model.h
engine/octa.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/octa.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
engine/octa.o: shared/iengine.h shared/igame.h engine/sound.h engine/world.h
engine/octa.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
engine/octa.o: engine/model.h
engine/octaedit.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/octaedit.o: shared/command.h shared/geom.h shared/attrs.h
engine/octaedit.o: shared/ents.h shared/iengine.h shared/igame.h
engine/octaedit.o: engine/sound.h engine/world.h engine/octa.h
engine/octaedit.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/octaedit.o: engine/model.h
engine/octarender.o: shared/pch.h engine/engine.h shared/cube.h
engine/octarender.o: shared/tools.h shared/command.h shared/geom.h
engine/octarender.o: shared/attrs.h shared/ents.h shared/iengine.h
engine/octarender.o: shared/igame.h engine/sound.h engine/world.h
engine/octarender.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/octarender.o: engine/texture.h engine/model.h
engine/physics.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/physics.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
engine/physics.o: shared/iengine.h shared/igame.h engine/sound.h
engine/physics.o: engine/world.h engine/octa.h engine/lightmap.h engine/bih.h
engine/physics.o: engine/texture.h engine/model.h
engine/pvs.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/pvs.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
engine/pvs.o: shared/iengine.h shared/igame.h engine/sound.h engine/world.h
engine/pvs.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
engine/pvs.o: engine/model.h
engine/rendergl.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/rendergl.o: shared/command.h shared/geom.h shared/attrs.h
engine/rendergl.o: shared/ents.h shared/iengine.h shared/igame.h
engine/rendergl.o: engine/sound.h engine/world.h engine/octa.h
engine/rendergl.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/rendergl.o: engine/model.h
engine/rendermodel.o: shared/pch.h engine/engine.h shared/cube.h
engine/rendermodel.o: shared/tools.h shared/command.h shared/geom.h
engine/rendermodel.o: shared/attrs.h shared/ents.h shared/iengine.h
engine/rendermodel.o: shared/igame.h engine/sound.h engine/world.h
engine/rendermodel.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/rendermodel.o: engine/texture.h engine/model.h engine/animmodel.h
engine/rendermodel.o: engine/vertmodel.h engine/skelmodel.h engine/md2.h
engine/rendermodel.o: engine/md3.h engine/md5.h engine/obj.h
engine/renderparticles.o: shared/pch.h engine/engine.h shared/cube.h
engine/renderparticles.o: shared/tools.h shared/command.h shared/geom.h
engine/renderparticles.o: shared/attrs.h shared/ents.h shared/iengine.h
engine/renderparticles.o: shared/igame.h engine/sound.h engine/world.h
engine/renderparticles.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/renderparticles.o: engine/texture.h engine/model.h
engine/renderparticles.o: engine/rendertarget.h engine/depthfx.h
engine/renderparticles.o: engine/explosion.h engine/lensflare.h
engine/renderparticles.o: engine/lightning.h
engine/rendersky.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/rendersky.o: shared/command.h shared/geom.h shared/attrs.h
engine/rendersky.o: shared/ents.h shared/iengine.h shared/igame.h
engine/rendersky.o: engine/sound.h engine/world.h engine/octa.h
engine/rendersky.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/rendersky.o: engine/model.h
engine/rendertext.o: shared/pch.h engine/engine.h shared/cube.h
engine/rendertext.o: shared/tools.h shared/command.h shared/geom.h
engine/rendertext.o: shared/attrs.h shared/ents.h shared/iengine.h
engine/rendertext.o: shared/igame.h engine/sound.h engine/world.h
engine/rendertext.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/rendertext.o: engine/texture.h engine/model.h
engine/renderva.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/renderva.o: shared/command.h shared/geom.h shared/attrs.h
engine/renderva.o: shared/ents.h shared/iengine.h shared/igame.h
engine/renderva.o: engine/sound.h engine/world.h engine/octa.h
engine/renderva.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/renderva.o: engine/model.h
engine/serverbrowser.o: shared/pch.h engine/engine.h shared/cube.h
engine/serverbrowser.o: shared/tools.h shared/command.h shared/geom.h
engine/serverbrowser.o: shared/attrs.h shared/ents.h shared/iengine.h
engine/serverbrowser.o: shared/igame.h engine/sound.h engine/world.h
engine/serverbrowser.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/serverbrowser.o: engine/texture.h engine/model.h
engine/server.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/server.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
engine/server.o: shared/iengine.h shared/igame.h engine/sound.h
engine/server.o: engine/world.h engine/octa.h engine/lightmap.h engine/bih.h
engine/server.o: engine/texture.h engine/model.h
engine/shader.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/shader.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
engine/shader.o: shared/iengine.h shared/igame.h engine/sound.h
engine/shader.o: engine/world.h engine/octa.h engine/lightmap.h engine/bih.h
engine/shader.o: engine/texture.h engine/model.h
engine/shadowmap.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/shadowmap.o: shared/command.h shared/geom.h shared/attrs.h
engine/shadowmap.o: shared/ents.h shared/iengine.h shared/igame.h
engine/shadowmap.o: engine/sound.h engine/world.h engine/octa.h
engine/shadowmap.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/shadowmap.o: engine/model.h engine/rendertarget.h
engine/sound.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/sound.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
engine/sound.o: shared/iengine.h shared/igame.h engine/sound.h engine/world.h
engine/sound.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
engine/sound.o: engine/model.h
engine/texture.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/texture.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
engine/texture.o: shared/iengine.h shared/igame.h engine/sound.h
engine/texture.o: engine/world.h engine/octa.h engine/lightmap.h engine/bih.h
engine/texture.o: engine/texture.h engine/model.h engine/scale.h
engine/ui.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/ui.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
engine/ui.o: shared/iengine.h shared/igame.h engine/sound.h engine/world.h
engine/ui.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
engine/ui.o: engine/model.h engine/textedit.h
engine/water.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/water.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
engine/water.o: shared/iengine.h shared/igame.h engine/sound.h engine/world.h
engine/water.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
engine/water.o: engine/model.h
engine/world.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/world.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
engine/world.o: shared/iengine.h shared/igame.h engine/sound.h engine/world.h
engine/world.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
engine/world.o: engine/model.h
engine/worldio.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/worldio.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
engine/worldio.o: shared/iengine.h shared/igame.h engine/sound.h
engine/worldio.o: engine/world.h engine/octa.h engine/lightmap.h engine/bih.h
engine/worldio.o: engine/texture.h engine/model.h
game/ai.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
game/ai.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
game/ai.o: shared/iengine.h shared/igame.h engine/sound.h engine/world.h
game/ai.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
game/ai.o: engine/model.h game/game.h game/ctf.h game/stf.h game/vars.h
game/ai.o: game/scoreboard.h
game/client.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
game/client.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
game/client.o: shared/iengine.h shared/igame.h engine/sound.h engine/world.h
game/client.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
game/client.o: engine/model.h game/game.h game/ctf.h game/stf.h game/vars.h
game/client.o: game/scoreboard.h
game/ctf.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
game/ctf.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
game/ctf.o: shared/iengine.h shared/igame.h engine/sound.h engine/world.h
game/ctf.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
game/ctf.o: engine/model.h game/game.h game/ctf.h game/stf.h game/vars.h
game/ctf.o: game/scoreboard.h
game/entities.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
game/entities.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
game/entities.o: shared/iengine.h shared/igame.h engine/sound.h
game/entities.o: engine/world.h engine/octa.h engine/lightmap.h engine/bih.h
game/entities.o: engine/texture.h engine/model.h game/game.h game/ctf.h
game/entities.o: game/stf.h game/vars.h game/scoreboard.h
game/hud.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
game/hud.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
game/hud.o: shared/iengine.h shared/igame.h engine/sound.h engine/world.h
game/hud.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
game/hud.o: engine/model.h game/game.h game/ctf.h game/stf.h game/vars.h
game/hud.o: game/scoreboard.h
game/physics.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
game/physics.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
game/physics.o: shared/iengine.h shared/igame.h engine/sound.h engine/world.h
game/physics.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
game/physics.o: engine/model.h game/game.h game/ctf.h game/stf.h game/vars.h
game/physics.o: game/scoreboard.h
game/projs.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
game/projs.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
game/projs.o: shared/iengine.h shared/igame.h engine/sound.h engine/world.h
game/projs.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
game/projs.o: engine/model.h game/game.h game/ctf.h game/stf.h game/vars.h
game/projs.o: game/scoreboard.h
game/server.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
game/server.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
game/server.o: shared/iengine.h shared/igame.h engine/sound.h engine/world.h
game/server.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
game/server.o: engine/model.h game/game.h game/ctf.h game/stf.h game/vars.h
game/server.o: game/scoreboard.h shared/hash.h game/stf.cpp game/ctf.cpp
game/server.o: game/duel.cpp game/ai.cpp game/extinfo.h game/auth.h
game/stf.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
game/stf.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
game/stf.o: shared/iengine.h shared/igame.h engine/sound.h engine/world.h
game/stf.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
game/stf.o: engine/model.h game/game.h game/ctf.h game/stf.h game/vars.h
game/stf.o: game/scoreboard.h
game/weapons.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
game/weapons.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
game/weapons.o: shared/iengine.h shared/igame.h engine/sound.h engine/world.h
game/weapons.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
game/weapons.o: engine/model.h game/game.h game/ctf.h game/stf.h game/vars.h
game/weapons.o: game/scoreboard.h
game/world.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
game/world.o: shared/command.h shared/geom.h shared/attrs.h shared/ents.h
game/world.o: shared/iengine.h shared/igame.h engine/sound.h engine/world.h
game/world.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
game/world.o: engine/model.h game/game.h game/ctf.h game/stf.h game/vars.h
game/world.o: game/scoreboard.h
