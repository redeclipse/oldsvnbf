CXXOPTFLAGS= -O3 -fomit-frame-pointer
INCLUDES= -Ishared -Iengine -Igame -Ienet/include -I/usr/X11R6/include `sdl-config --cflags` -Iinclude
CXXFLAGS= -Wall -fsigned-char -DINTERFACE -DIRC $(CXXOPTFLAGS) $(INCLUDES)

CLIENT_LIBS=-Lenet -lenet -L/usr/X11R6/lib `sdl-config --libs` -lSDL_image -lSDL_mixer -lz -lGL
ifeq ($(shell uname -s),Linux)
CLIENT_LIBS+= -lrt
endif
CLIENT_OBJS= \
	shared/tools.o \
	shared/hash.o \
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
	shared/hash-standalone.o \
	engine/command-standalone.o \
	engine/irc-standalone.o	\
	engine/server-standalone.o \
	game/server-standalone.o

DINCLUDES= -Ishared -Iengine -Igame -Ienet/include
DCXXFLAGS= -Wall -fsigned-char -DSTANDALONE -DMASTERSERVER -DIRC $(CXXOPTFLAGS) $(DINCLUDES)

DAEMON_LIBS=-Lenet -lenet -lz
DAEMON_OBJS= \
	shared/tools-daemon.o \
	shared/hash-daemon.o \
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
	makedepend -Y -Ishared -Iengine -Igame $(subst .o,.cpp,$(CLIENT_OBJS))
	makedepend -a -o-standalone.o -Y -Ishared -Iengine -Igame $(subst -standalone.o,.cpp,$(SERVER_OBJS))
	makedepend -a -o-daemon.o -Y -Ishared -Iengine -Igame $(subst -daemon.o,.cpp,$(DAEMON_OBJS))

all: client server
# DO NOT DELETE

shared/tools.o: shared/pch.h shared/tools.h
shared/hash.o: shared/pch.h shared/tools.h shared/hash.h
shared/geom.o: shared/pch.h shared/tools.h shared/geom.h
engine/bih.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/bih.o: shared/command.h shared/geom.h shared/ents.h shared/iengine.h
engine/bih.o: shared/igame.h engine/sound.h engine/world.h engine/octa.h
engine/bih.o: engine/lightmap.h engine/bih.h engine/texture.h engine/model.h
engine/blend.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/blend.o: shared/command.h shared/geom.h shared/ents.h shared/iengine.h
engine/blend.o: shared/igame.h engine/sound.h engine/world.h engine/octa.h
engine/blend.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/blend.o: engine/model.h
engine/blob.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/blob.o: shared/command.h shared/geom.h shared/ents.h shared/iengine.h
engine/blob.o: shared/igame.h engine/sound.h engine/world.h engine/octa.h
engine/blob.o: engine/lightmap.h engine/bih.h engine/texture.h engine/model.h
engine/client.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/client.o: shared/command.h shared/geom.h shared/ents.h
engine/client.o: shared/iengine.h shared/igame.h engine/sound.h
engine/client.o: engine/world.h engine/octa.h engine/lightmap.h engine/bih.h
engine/client.o: engine/texture.h engine/model.h
engine/command.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/command.o: shared/command.h shared/geom.h shared/ents.h
engine/command.o: shared/iengine.h shared/igame.h engine/sound.h
engine/command.o: engine/world.h engine/octa.h engine/lightmap.h engine/bih.h
engine/command.o: engine/texture.h engine/model.h
engine/console.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/console.o: shared/command.h shared/geom.h shared/ents.h
engine/console.o: shared/iengine.h shared/igame.h engine/sound.h
engine/console.o: engine/world.h engine/octa.h engine/lightmap.h engine/bih.h
engine/console.o: engine/texture.h engine/model.h
engine/decal.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/decal.o: shared/command.h shared/geom.h shared/ents.h shared/iengine.h
engine/decal.o: shared/igame.h engine/sound.h engine/world.h engine/octa.h
engine/decal.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/decal.o: engine/model.h
engine/dynlight.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/dynlight.o: shared/command.h shared/geom.h shared/ents.h
engine/dynlight.o: shared/iengine.h shared/igame.h engine/sound.h
engine/dynlight.o: engine/world.h engine/octa.h engine/lightmap.h
engine/dynlight.o: engine/bih.h engine/texture.h engine/model.h
engine/glare.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/glare.o: shared/command.h shared/geom.h shared/ents.h shared/iengine.h
engine/glare.o: shared/igame.h engine/sound.h engine/world.h engine/octa.h
engine/glare.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/glare.o: engine/model.h engine/rendertarget.h
engine/grass.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/grass.o: shared/command.h shared/geom.h shared/ents.h shared/iengine.h
engine/grass.o: shared/igame.h engine/sound.h engine/world.h engine/octa.h
engine/grass.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/grass.o: engine/model.h
engine/lightmap.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/lightmap.o: shared/command.h shared/geom.h shared/ents.h
engine/lightmap.o: shared/iengine.h shared/igame.h engine/sound.h
engine/lightmap.o: engine/world.h engine/octa.h engine/lightmap.h
engine/lightmap.o: engine/bih.h engine/texture.h engine/model.h
engine/main.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/main.o: shared/command.h shared/geom.h shared/ents.h shared/iengine.h
engine/main.o: shared/igame.h engine/sound.h engine/world.h engine/octa.h
engine/main.o: engine/lightmap.h engine/bih.h engine/texture.h engine/model.h
engine/material.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/material.o: shared/command.h shared/geom.h shared/ents.h
engine/material.o: shared/iengine.h shared/igame.h engine/sound.h
engine/material.o: engine/world.h engine/octa.h engine/lightmap.h
engine/material.o: engine/bih.h engine/texture.h engine/model.h
engine/menus.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/menus.o: shared/command.h shared/geom.h shared/ents.h shared/iengine.h
engine/menus.o: shared/igame.h engine/sound.h engine/world.h engine/octa.h
engine/menus.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/menus.o: engine/model.h
engine/normal.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/normal.o: shared/command.h shared/geom.h shared/ents.h
engine/normal.o: shared/iengine.h shared/igame.h engine/sound.h
engine/normal.o: engine/world.h engine/octa.h engine/lightmap.h engine/bih.h
engine/normal.o: engine/texture.h engine/model.h
engine/octa.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/octa.o: shared/command.h shared/geom.h shared/ents.h shared/iengine.h
engine/octa.o: shared/igame.h engine/sound.h engine/world.h engine/octa.h
engine/octa.o: engine/lightmap.h engine/bih.h engine/texture.h engine/model.h
engine/octaedit.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/octaedit.o: shared/command.h shared/geom.h shared/ents.h
engine/octaedit.o: shared/iengine.h shared/igame.h engine/sound.h
engine/octaedit.o: engine/world.h engine/octa.h engine/lightmap.h
engine/octaedit.o: engine/bih.h engine/texture.h engine/model.h
engine/octarender.o: shared/pch.h engine/engine.h shared/cube.h
engine/octarender.o: shared/tools.h shared/command.h shared/geom.h
engine/octarender.o: shared/ents.h shared/iengine.h shared/igame.h
engine/octarender.o: engine/sound.h engine/world.h engine/octa.h
engine/octarender.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/octarender.o: engine/model.h
engine/physics.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/physics.o: shared/command.h shared/geom.h shared/ents.h
engine/physics.o: shared/iengine.h shared/igame.h engine/sound.h
engine/physics.o: engine/world.h engine/octa.h engine/lightmap.h engine/bih.h
engine/physics.o: engine/texture.h engine/model.h
engine/pvs.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/pvs.o: shared/command.h shared/geom.h shared/ents.h shared/iengine.h
engine/pvs.o: shared/igame.h engine/sound.h engine/world.h engine/octa.h
engine/pvs.o: engine/lightmap.h engine/bih.h engine/texture.h engine/model.h
engine/rendergl.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/rendergl.o: shared/command.h shared/geom.h shared/ents.h
engine/rendergl.o: shared/iengine.h shared/igame.h engine/sound.h
engine/rendergl.o: engine/world.h engine/octa.h engine/lightmap.h
engine/rendergl.o: engine/bih.h engine/texture.h engine/model.h
engine/rendermodel.o: shared/pch.h engine/engine.h shared/cube.h
engine/rendermodel.o: shared/tools.h shared/command.h shared/geom.h
engine/rendermodel.o: shared/ents.h shared/iengine.h shared/igame.h
engine/rendermodel.o: engine/sound.h engine/world.h engine/octa.h
engine/rendermodel.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/rendermodel.o: engine/model.h engine/ragdoll.h engine/animmodel.h
engine/rendermodel.o: engine/vertmodel.h engine/skelmodel.h engine/md2.h
engine/rendermodel.o: engine/md3.h engine/md5.h engine/obj.h
engine/renderparticles.o: shared/pch.h engine/engine.h shared/cube.h
engine/renderparticles.o: shared/tools.h shared/command.h shared/geom.h
engine/renderparticles.o: shared/ents.h shared/iengine.h shared/igame.h
engine/renderparticles.o: engine/sound.h engine/world.h engine/octa.h
engine/renderparticles.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/renderparticles.o: engine/model.h engine/rendertarget.h
engine/renderparticles.o: engine/depthfx.h engine/lensflare.h
engine/renderparticles.o: engine/explosion.h engine/lightning.h
engine/rendersky.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/rendersky.o: shared/command.h shared/geom.h shared/ents.h
engine/rendersky.o: shared/iengine.h shared/igame.h engine/sound.h
engine/rendersky.o: engine/world.h engine/octa.h engine/lightmap.h
engine/rendersky.o: engine/bih.h engine/texture.h engine/model.h
engine/rendertext.o: shared/pch.h engine/engine.h shared/cube.h
engine/rendertext.o: shared/tools.h shared/command.h shared/geom.h
engine/rendertext.o: shared/ents.h shared/iengine.h shared/igame.h
engine/rendertext.o: engine/sound.h engine/world.h engine/octa.h
engine/rendertext.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/rendertext.o: engine/model.h
engine/renderva.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/renderva.o: shared/command.h shared/geom.h shared/ents.h
engine/renderva.o: shared/iengine.h shared/igame.h engine/sound.h
engine/renderva.o: engine/world.h engine/octa.h engine/lightmap.h
engine/renderva.o: engine/bih.h engine/texture.h engine/model.h
engine/server.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/server.o: shared/command.h shared/geom.h shared/ents.h
engine/server.o: shared/iengine.h shared/igame.h engine/sound.h
engine/server.o: engine/world.h engine/octa.h engine/lightmap.h engine/bih.h
engine/server.o: engine/texture.h engine/model.h
engine/serverbrowser.o: shared/pch.h engine/engine.h shared/cube.h
engine/serverbrowser.o: shared/tools.h shared/command.h shared/geom.h
engine/serverbrowser.o: shared/ents.h shared/iengine.h shared/igame.h
engine/serverbrowser.o: engine/sound.h engine/world.h engine/octa.h
engine/serverbrowser.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/serverbrowser.o: engine/model.h
engine/shader.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/shader.o: shared/command.h shared/geom.h shared/ents.h
engine/shader.o: shared/iengine.h shared/igame.h engine/sound.h
engine/shader.o: engine/world.h engine/octa.h engine/lightmap.h engine/bih.h
engine/shader.o: engine/texture.h engine/model.h
engine/shadowmap.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/shadowmap.o: shared/command.h shared/geom.h shared/ents.h
engine/shadowmap.o: shared/iengine.h shared/igame.h engine/sound.h
engine/shadowmap.o: engine/world.h engine/octa.h engine/lightmap.h
engine/shadowmap.o: engine/bih.h engine/texture.h engine/model.h
engine/shadowmap.o: engine/rendertarget.h
engine/sound.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/sound.o: shared/command.h shared/geom.h shared/ents.h shared/iengine.h
engine/sound.o: shared/igame.h engine/sound.h engine/world.h engine/octa.h
engine/sound.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/sound.o: engine/model.h
engine/texture.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/texture.o: shared/command.h shared/geom.h shared/ents.h
engine/texture.o: shared/iengine.h shared/igame.h engine/sound.h
engine/texture.o: engine/world.h engine/octa.h engine/lightmap.h engine/bih.h
engine/texture.o: engine/texture.h engine/model.h engine/scale.h
engine/ui.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/ui.o: shared/command.h shared/geom.h shared/ents.h shared/iengine.h
engine/ui.o: shared/igame.h engine/sound.h engine/world.h engine/octa.h
engine/ui.o: engine/lightmap.h engine/bih.h engine/texture.h engine/model.h
engine/ui.o: engine/textedit.h
engine/water.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/water.o: shared/command.h shared/geom.h shared/ents.h shared/iengine.h
engine/water.o: shared/igame.h engine/sound.h engine/world.h engine/octa.h
engine/water.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/water.o: engine/model.h
engine/world.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/world.o: shared/command.h shared/geom.h shared/ents.h shared/iengine.h
engine/world.o: shared/igame.h engine/sound.h engine/world.h engine/octa.h
engine/world.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/world.o: engine/model.h
engine/worldio.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
engine/worldio.o: shared/command.h shared/geom.h shared/ents.h
engine/worldio.o: shared/iengine.h shared/igame.h engine/sound.h
engine/worldio.o: engine/world.h engine/octa.h engine/lightmap.h engine/bih.h
engine/worldio.o: engine/texture.h engine/model.h
game/ai.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
game/ai.o: shared/command.h shared/geom.h shared/ents.h shared/iengine.h
game/ai.o: shared/igame.h engine/sound.h engine/world.h engine/octa.h
game/ai.o: engine/lightmap.h engine/bih.h engine/texture.h engine/model.h
game/ai.o: game/game.h game/ctf.h game/stf.h game/vars.h game/scoreboard.h
game/client.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
game/client.o: shared/command.h shared/geom.h shared/ents.h shared/iengine.h
game/client.o: shared/igame.h engine/sound.h engine/world.h engine/octa.h
game/client.o: engine/lightmap.h engine/bih.h engine/texture.h engine/model.h
game/client.o: shared/crypto.h game/game.h game/ctf.h game/stf.h game/vars.h
game/client.o: game/scoreboard.h
game/ctf.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
game/ctf.o: shared/command.h shared/geom.h shared/ents.h shared/iengine.h
game/ctf.o: shared/igame.h engine/sound.h engine/world.h engine/octa.h
game/ctf.o: engine/lightmap.h engine/bih.h engine/texture.h engine/model.h
game/ctf.o: game/game.h game/ctf.h game/stf.h game/vars.h game/scoreboard.h
game/entities.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
game/entities.o: shared/command.h shared/geom.h shared/ents.h
game/entities.o: shared/iengine.h shared/igame.h engine/sound.h
game/entities.o: engine/world.h engine/octa.h engine/lightmap.h engine/bih.h
game/entities.o: engine/texture.h engine/model.h game/game.h game/ctf.h
game/entities.o: game/stf.h game/vars.h game/scoreboard.h
game/hud.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
game/hud.o: shared/command.h shared/geom.h shared/ents.h shared/iengine.h
game/hud.o: shared/igame.h engine/sound.h engine/world.h engine/octa.h
game/hud.o: engine/lightmap.h engine/bih.h engine/texture.h engine/model.h
game/hud.o: game/game.h game/ctf.h game/stf.h game/vars.h game/scoreboard.h
game/physics.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
game/physics.o: shared/command.h shared/geom.h shared/ents.h shared/iengine.h
game/physics.o: shared/igame.h engine/sound.h engine/world.h engine/octa.h
game/physics.o: engine/lightmap.h engine/bih.h engine/texture.h
game/physics.o: engine/model.h game/game.h game/ctf.h game/stf.h game/vars.h
game/physics.o: game/scoreboard.h
game/projs.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
game/projs.o: shared/command.h shared/geom.h shared/ents.h shared/iengine.h
game/projs.o: shared/igame.h engine/sound.h engine/world.h engine/octa.h
game/projs.o: engine/lightmap.h engine/bih.h engine/texture.h engine/model.h
game/projs.o: game/game.h game/ctf.h game/stf.h game/vars.h game/scoreboard.h
game/server.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
game/server.o: shared/command.h shared/geom.h shared/ents.h shared/iengine.h
game/server.o: shared/igame.h engine/sound.h engine/world.h engine/octa.h
game/server.o: engine/lightmap.h engine/bih.h engine/texture.h engine/model.h
game/server.o: game/game.h game/ctf.h game/stf.h game/vars.h
game/server.o: game/scoreboard.h shared/hash.h game/stfmode.h game/ctfmode.h
game/server.o: game/duelmut.h game/aiman.h game/auth.h game/extinfo.h
game/stf.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
game/stf.o: shared/command.h shared/geom.h shared/ents.h shared/iengine.h
game/stf.o: shared/igame.h engine/sound.h engine/world.h engine/octa.h
game/stf.o: engine/lightmap.h engine/bih.h engine/texture.h engine/model.h
game/stf.o: game/game.h game/ctf.h game/stf.h game/vars.h game/scoreboard.h
game/weapons.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
game/weapons.o: shared/command.h shared/geom.h shared/ents.h shared/iengine.h
game/weapons.o: shared/igame.h engine/sound.h engine/world.h engine/octa.h
game/weapons.o: engine/lightmap.h engine/bih.h engine/texture.h
game/weapons.o: engine/model.h game/game.h game/ctf.h game/stf.h game/vars.h
game/weapons.o: game/scoreboard.h
game/world.o: shared/pch.h engine/engine.h shared/cube.h shared/tools.h
game/world.o: shared/command.h shared/geom.h shared/ents.h shared/iengine.h
game/world.o: shared/igame.h engine/sound.h engine/world.h engine/octa.h
game/world.o: engine/lightmap.h engine/bih.h engine/texture.h engine/model.h
game/world.o: game/game.h game/ctf.h game/stf.h game/vars.h game/scoreboard.h

shared/tools-standalone.o: shared/pch.h shared/tools.h
shared/hash-standalone.o: shared/pch.h shared/tools.h shared/hash.h
engine/command-standalone.o: shared/pch.h engine/engine.h shared/cube.h
engine/command-standalone.o: shared/tools.h shared/command.h shared/geom.h
engine/command-standalone.o: shared/ents.h shared/iengine.h shared/igame.h
engine/command-standalone.o: engine/sound.h engine/world.h engine/octa.h
engine/command-standalone.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/command-standalone.o: engine/model.h
engine/server-standalone.o: shared/pch.h engine/engine.h shared/cube.h
engine/server-standalone.o: shared/tools.h shared/command.h shared/geom.h
engine/server-standalone.o: shared/ents.h shared/iengine.h shared/igame.h
engine/server-standalone.o: engine/sound.h engine/world.h engine/octa.h
engine/server-standalone.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/server-standalone.o: engine/model.h
game/server-standalone.o: shared/pch.h engine/engine.h shared/cube.h
game/server-standalone.o: shared/tools.h shared/command.h shared/geom.h
game/server-standalone.o: shared/ents.h shared/iengine.h shared/igame.h
game/server-standalone.o: engine/sound.h engine/world.h engine/octa.h
game/server-standalone.o: engine/lightmap.h engine/bih.h engine/texture.h
game/server-standalone.o: engine/model.h game/game.h game/ctf.h game/stf.h
game/server-standalone.o: game/vars.h shared/hash.h game/stfmode.h
game/server-standalone.o: game/ctfmode.h game/duelmut.h game/aiman.h
game/server-standalone.o: game/auth.h game/extinfo.h

shared/tools-daemon.o: shared/pch.h shared/tools.h
shared/hash-daemon.o: shared/pch.h shared/tools.h shared/hash.h
engine/command-daemon.o: shared/pch.h engine/engine.h shared/cube.h
engine/command-daemon.o: shared/tools.h shared/command.h shared/geom.h
engine/command-daemon.o: shared/ents.h shared/iengine.h shared/igame.h
engine/command-daemon.o: engine/sound.h engine/world.h engine/octa.h
engine/command-daemon.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/command-daemon.o: engine/model.h
engine/master-daemon.o: shared/pch.h engine/engine.h shared/cube.h
engine/master-daemon.o: shared/tools.h shared/command.h shared/geom.h
engine/master-daemon.o: shared/ents.h shared/iengine.h shared/igame.h
engine/master-daemon.o: engine/sound.h engine/world.h engine/octa.h
engine/master-daemon.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/master-daemon.o: engine/model.h shared/crypto.h shared/hash.h
engine/server-daemon.o: shared/pch.h engine/engine.h shared/cube.h
engine/server-daemon.o: shared/tools.h shared/command.h shared/geom.h
engine/server-daemon.o: shared/ents.h shared/iengine.h shared/igame.h
engine/server-daemon.o: engine/sound.h engine/world.h engine/octa.h
engine/server-daemon.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/server-daemon.o: engine/model.h
game/server-daemon.o: shared/pch.h engine/engine.h shared/cube.h
game/server-daemon.o: shared/tools.h shared/command.h shared/geom.h
game/server-daemon.o: shared/ents.h shared/iengine.h shared/igame.h
game/server-daemon.o: engine/sound.h engine/world.h engine/octa.h
game/server-daemon.o: engine/lightmap.h engine/bih.h engine/texture.h
game/server-daemon.o: engine/model.h game/game.h game/ctf.h game/stf.h
game/server-daemon.o: game/vars.h shared/hash.h game/stfmode.h game/ctfmode.h
game/server-daemon.o: game/duelmut.h game/aiman.h game/auth.h game/extinfo.h
