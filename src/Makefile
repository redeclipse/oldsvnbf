#CXXFLAGS= -ggdb3
CXXFLAGS= -O3 -fomit-frame-pointer
override CXXFLAGS+= -Wall -fsigned-char

PLATFORM= $(shell uname -s)
INCLUDES= -Ishared -Iengine -Igame -Ienet/include

ifneq (,$(findstring MINGW,$(PLATFORM)))
WINDRES= windres
CLIENT_INCLUDES= -DINTERFACE -DIRC $(INCLUDES) -Iinclude
CLIENT_LIBS= -mwindows -lmingw32 -Llib -lSDLmain -lSDL -lSDL_image -lSDL_mixer -lzdll -lopengl32 -lenet -lws2_32 -lwinmm
else
CLIENT_INCLUDES= -DINTERFACE -DIRC $(INCLUDES) -I/usr/X11R6/include `sdl-config --cflags`
CLIENT_LIBS= -Lenet -lenet -L/usr/X11R6/lib `sdl-config --libs` -lSDL_image -lSDL_mixer -lz -lGL
endif
ifeq ($(PLATFORM),Linux)
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
CLIENT_PCH= shared/cube.h.gch

ifneq (,$(findstring MINGW,$(PLATFORM)))
SERVER_INCLUDES= -DSTANDALONE -DMASTERSERVER -DIRC $(INCLUDES) -Iinclude
SERVER_LIBS= -Llib -lzdll -lenet -lws2_32 -lwinmm
else
SERVER_INCLUDES= -DSTANDALONE -DMASTERSERVER -DIRC $(INCLUDES)
SERVER_LIBS= -Lenet -lenet -lz
endif
SERVER_OBJS= \
	shared/tools-standalone.o \
	shared/hash-standalone.o \
	engine/command-standalone.o \
	engine/irc-standalone.o	\
	engine/master-standalone.o \
	engine/server-standalone.o \
	game/server-standalone.o

default: all

enet/Makefile:
	cd enet; chmod +x ./configure; ./configure

libenet: enet/Makefile
	$(MAKE)	-C enet/ all

clean-enet: enet/Makefile
	$(MAKE) -C enet/ clean

clean: clean-client clean-server

clean-client:
	@rm -fv $(CLIENT_PCH) $(CLIENT_OBJS) bfclient

clean-server:
	@rm -fv $(SERVER_OBJS) bfserver

%.h.gch:
	$(CXX) $(CXXFLAGS) -o $@.tmp $(subst .h.gch,.h,$@)
	mv $@.tmp $@

%-standalone.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $(subst -standalone.o,.cpp,$@)

$(CLIENT_OBJS): CXXFLAGS += $(CLIENT_INCLUDES)
$(CLIENT_OBJS): $(CLIENT_PCH)

$(SERVER_OBJS): CXXFLAGS += $(SERVER_INCLUDES)

ifneq (,$(findstring MINGW,$(PLATFORM)))
client: $(CLIENT_OBJS)
	$(WINDRES) -i bloodfrontier.rc -J rc -o bloodfrontier.res -O coff 
	$(CXX) $(CXXFLAGS) -o ../bin/bfclient.exe bloodfrontier.res $(CLIENT_OBJS) $(CLIENT_LIBS)

server: $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o ../bin/bfserver.exe $(SERVER_OBJS) $(SERVER_LIBS)

install-client: client
ifeq (,$(findstring -g,$(CXXFLAGS)))
	strip ../bin/bfclient.exe
endif

install-server: server
ifeq (,$(findstring -g,$(CXXFLAGS)))
	strip ../bin/bfserver.exe
endif
else
client:	libenet $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o bfclient $(CLIENT_OBJS) $(CLIENT_LIBS)
ifeq (,$(findstring -g,$(CXXFLAGS)))
	strip bfclient
endif

server:	libenet $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o bfserver $(SERVER_OBJS) $(SERVER_LIBS)
ifeq (,$(findstring -g,$(CXXFLAGS)))
	strip bfserver
endif

install-client: client
	cp -v bfclient ../bin/bfclient
	chmod +x ../bin/bfclient ../bloodfrontier.sh

install-server: server
	cp -v bfserver ../bin/bfserver
	chmod +x ../bin/bfserver
endif

install: install-client install-server

depend:
	makedepend -Y -Ishared -Iengine -Igame $(subst .o,.cpp,$(CLIENT_OBJS))
	makedepend -a -o-standalone.o -Y -Ishared -Iengine -Igame $(subst -standalone.o,.cpp,$(SERVER_OBJS))
	makedepend -a -o.h.gch -Y -Ishared -Iengine -Igame $(subst .h.gch,.h,$(CLIENT_PCH))

all: client server
# DO NOT DELETE

shared/tools.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
shared/tools.o: shared/ents.h shared/iengine.h shared/igame.h
shared/hash.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
shared/hash.o: shared/ents.h shared/iengine.h shared/igame.h shared/hash.h
shared/geom.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
shared/geom.o: shared/ents.h shared/iengine.h shared/igame.h
engine/bih.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
engine/bih.o: shared/ents.h shared/iengine.h shared/igame.h engine/engine.h
engine/bih.o: engine/sound.h engine/world.h engine/octa.h engine/lightmap.h
engine/bih.o: engine/bih.h engine/texture.h engine/model.h
engine/blend.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
engine/blend.o: shared/ents.h shared/iengine.h shared/igame.h engine/engine.h
engine/blend.o: engine/sound.h engine/world.h engine/octa.h engine/lightmap.h
engine/blend.o: engine/bih.h engine/texture.h engine/model.h
engine/blob.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
engine/blob.o: shared/ents.h shared/iengine.h shared/igame.h engine/engine.h
engine/blob.o: engine/sound.h engine/world.h engine/octa.h engine/lightmap.h
engine/blob.o: engine/bih.h engine/texture.h engine/model.h
engine/client.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
engine/client.o: shared/ents.h shared/iengine.h shared/igame.h
engine/client.o: engine/engine.h engine/sound.h engine/world.h engine/octa.h
engine/client.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/client.o: engine/model.h
engine/command.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
engine/command.o: shared/ents.h shared/iengine.h shared/igame.h
engine/command.o: engine/engine.h engine/sound.h engine/world.h engine/octa.h
engine/command.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/command.o: engine/model.h
engine/console.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
engine/console.o: shared/ents.h shared/iengine.h shared/igame.h
engine/console.o: engine/engine.h engine/sound.h engine/world.h engine/octa.h
engine/console.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/console.o: engine/model.h
engine/decal.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
engine/decal.o: shared/ents.h shared/iengine.h shared/igame.h engine/engine.h
engine/decal.o: engine/sound.h engine/world.h engine/octa.h engine/lightmap.h
engine/decal.o: engine/bih.h engine/texture.h engine/model.h
engine/dynlight.o: shared/cube.h shared/tools.h shared/command.h
engine/dynlight.o: shared/geom.h shared/ents.h shared/iengine.h
engine/dynlight.o: shared/igame.h engine/engine.h engine/sound.h
engine/dynlight.o: engine/world.h engine/octa.h engine/lightmap.h
engine/dynlight.o: engine/bih.h engine/texture.h engine/model.h
engine/glare.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
engine/glare.o: shared/ents.h shared/iengine.h shared/igame.h engine/engine.h
engine/glare.o: engine/sound.h engine/world.h engine/octa.h engine/lightmap.h
engine/glare.o: engine/bih.h engine/texture.h engine/model.h
engine/glare.o: engine/rendertarget.h
engine/grass.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
engine/grass.o: shared/ents.h shared/iengine.h shared/igame.h engine/engine.h
engine/grass.o: engine/sound.h engine/world.h engine/octa.h engine/lightmap.h
engine/grass.o: engine/bih.h engine/texture.h engine/model.h
engine/lightmap.o: shared/cube.h shared/tools.h shared/command.h
engine/lightmap.o: shared/geom.h shared/ents.h shared/iengine.h
engine/lightmap.o: shared/igame.h engine/engine.h engine/sound.h
engine/lightmap.o: engine/world.h engine/octa.h engine/lightmap.h
engine/lightmap.o: engine/bih.h engine/texture.h engine/model.h
engine/main.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
engine/main.o: shared/ents.h shared/iengine.h shared/igame.h engine/engine.h
engine/main.o: engine/sound.h engine/world.h engine/octa.h engine/lightmap.h
engine/main.o: engine/bih.h engine/texture.h engine/model.h
engine/material.o: shared/cube.h shared/tools.h shared/command.h
engine/material.o: shared/geom.h shared/ents.h shared/iengine.h
engine/material.o: shared/igame.h engine/engine.h engine/sound.h
engine/material.o: engine/world.h engine/octa.h engine/lightmap.h
engine/material.o: engine/bih.h engine/texture.h engine/model.h
engine/menus.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
engine/menus.o: shared/ents.h shared/iengine.h shared/igame.h engine/engine.h
engine/menus.o: engine/sound.h engine/world.h engine/octa.h engine/lightmap.h
engine/menus.o: engine/bih.h engine/texture.h engine/model.h
engine/normal.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
engine/normal.o: shared/ents.h shared/iengine.h shared/igame.h
engine/normal.o: engine/engine.h engine/sound.h engine/world.h engine/octa.h
engine/normal.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/normal.o: engine/model.h
engine/octa.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
engine/octa.o: shared/ents.h shared/iengine.h shared/igame.h engine/engine.h
engine/octa.o: engine/sound.h engine/world.h engine/octa.h engine/lightmap.h
engine/octa.o: engine/bih.h engine/texture.h engine/model.h
engine/octaedit.o: shared/cube.h shared/tools.h shared/command.h
engine/octaedit.o: shared/geom.h shared/ents.h shared/iengine.h
engine/octaedit.o: shared/igame.h engine/engine.h engine/sound.h
engine/octaedit.o: engine/world.h engine/octa.h engine/lightmap.h
engine/octaedit.o: engine/bih.h engine/texture.h engine/model.h
engine/octarender.o: shared/cube.h shared/tools.h shared/command.h
engine/octarender.o: shared/geom.h shared/ents.h shared/iengine.h
engine/octarender.o: shared/igame.h engine/engine.h engine/sound.h
engine/octarender.o: engine/world.h engine/octa.h engine/lightmap.h
engine/octarender.o: engine/bih.h engine/texture.h engine/model.h
engine/physics.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
engine/physics.o: shared/ents.h shared/iengine.h shared/igame.h
engine/physics.o: engine/engine.h engine/sound.h engine/world.h engine/octa.h
engine/physics.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/physics.o: engine/model.h
engine/pvs.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
engine/pvs.o: shared/ents.h shared/iengine.h shared/igame.h engine/engine.h
engine/pvs.o: engine/sound.h engine/world.h engine/octa.h engine/lightmap.h
engine/pvs.o: engine/bih.h engine/texture.h engine/model.h
engine/rendergl.o: shared/cube.h shared/tools.h shared/command.h
engine/rendergl.o: shared/geom.h shared/ents.h shared/iengine.h
engine/rendergl.o: shared/igame.h engine/engine.h engine/sound.h
engine/rendergl.o: engine/world.h engine/octa.h engine/lightmap.h
engine/rendergl.o: engine/bih.h engine/texture.h engine/model.h
engine/rendermodel.o: shared/cube.h shared/tools.h shared/command.h
engine/rendermodel.o: shared/geom.h shared/ents.h shared/iengine.h
engine/rendermodel.o: shared/igame.h engine/engine.h engine/sound.h
engine/rendermodel.o: engine/world.h engine/octa.h engine/lightmap.h
engine/rendermodel.o: engine/bih.h engine/texture.h engine/model.h
engine/rendermodel.o: engine/ragdoll.h engine/animmodel.h engine/vertmodel.h
engine/rendermodel.o: engine/skelmodel.h engine/md2.h engine/md3.h
engine/rendermodel.o: engine/md5.h engine/obj.h
engine/renderparticles.o: shared/cube.h shared/tools.h shared/command.h
engine/renderparticles.o: shared/geom.h shared/ents.h shared/iengine.h
engine/renderparticles.o: shared/igame.h engine/engine.h engine/sound.h
engine/renderparticles.o: engine/world.h engine/octa.h engine/lightmap.h
engine/renderparticles.o: engine/bih.h engine/texture.h engine/model.h
engine/renderparticles.o: engine/rendertarget.h engine/depthfx.h
engine/renderparticles.o: engine/lensflare.h engine/explosion.h
engine/renderparticles.o: engine/lightning.h
engine/rendersky.o: shared/cube.h shared/tools.h shared/command.h
engine/rendersky.o: shared/geom.h shared/ents.h shared/iengine.h
engine/rendersky.o: shared/igame.h engine/engine.h engine/sound.h
engine/rendersky.o: engine/world.h engine/octa.h engine/lightmap.h
engine/rendersky.o: engine/bih.h engine/texture.h engine/model.h
engine/rendertext.o: shared/cube.h shared/tools.h shared/command.h
engine/rendertext.o: shared/geom.h shared/ents.h shared/iengine.h
engine/rendertext.o: shared/igame.h engine/engine.h engine/sound.h
engine/rendertext.o: engine/world.h engine/octa.h engine/lightmap.h
engine/rendertext.o: engine/bih.h engine/texture.h engine/model.h
engine/renderva.o: shared/cube.h shared/tools.h shared/command.h
engine/renderva.o: shared/geom.h shared/ents.h shared/iengine.h
engine/renderva.o: shared/igame.h engine/engine.h engine/sound.h
engine/renderva.o: engine/world.h engine/octa.h engine/lightmap.h
engine/renderva.o: engine/bih.h engine/texture.h engine/model.h
engine/server.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
engine/server.o: shared/ents.h shared/iengine.h shared/igame.h
engine/server.o: engine/engine.h engine/sound.h engine/world.h engine/octa.h
engine/server.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/server.o: engine/model.h
engine/serverbrowser.o: shared/cube.h shared/tools.h shared/command.h
engine/serverbrowser.o: shared/geom.h shared/ents.h shared/iengine.h
engine/serverbrowser.o: shared/igame.h engine/engine.h engine/sound.h
engine/serverbrowser.o: engine/world.h engine/octa.h engine/lightmap.h
engine/serverbrowser.o: engine/bih.h engine/texture.h engine/model.h
engine/shader.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
engine/shader.o: shared/ents.h shared/iengine.h shared/igame.h
engine/shader.o: engine/engine.h engine/sound.h engine/world.h engine/octa.h
engine/shader.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/shader.o: engine/model.h
engine/shadowmap.o: shared/cube.h shared/tools.h shared/command.h
engine/shadowmap.o: shared/geom.h shared/ents.h shared/iengine.h
engine/shadowmap.o: shared/igame.h engine/engine.h engine/sound.h
engine/shadowmap.o: engine/world.h engine/octa.h engine/lightmap.h
engine/shadowmap.o: engine/bih.h engine/texture.h engine/model.h
engine/shadowmap.o: engine/rendertarget.h
engine/sound.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
engine/sound.o: shared/ents.h shared/iengine.h shared/igame.h engine/engine.h
engine/sound.o: engine/sound.h engine/world.h engine/octa.h engine/lightmap.h
engine/sound.o: engine/bih.h engine/texture.h engine/model.h
engine/texture.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
engine/texture.o: shared/ents.h shared/iengine.h shared/igame.h
engine/texture.o: engine/engine.h engine/sound.h engine/world.h engine/octa.h
engine/texture.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/texture.o: engine/model.h engine/scale.h
engine/ui.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
engine/ui.o: shared/ents.h shared/iengine.h shared/igame.h engine/engine.h
engine/ui.o: engine/sound.h engine/world.h engine/octa.h engine/lightmap.h
engine/ui.o: engine/bih.h engine/texture.h engine/model.h engine/textedit.h
engine/water.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
engine/water.o: shared/ents.h shared/iengine.h shared/igame.h engine/engine.h
engine/water.o: engine/sound.h engine/world.h engine/octa.h engine/lightmap.h
engine/water.o: engine/bih.h engine/texture.h engine/model.h
engine/world.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
engine/world.o: shared/ents.h shared/iengine.h shared/igame.h engine/engine.h
engine/world.o: engine/sound.h engine/world.h engine/octa.h engine/lightmap.h
engine/world.o: engine/bih.h engine/texture.h engine/model.h
engine/worldio.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
engine/worldio.o: shared/ents.h shared/iengine.h shared/igame.h
engine/worldio.o: engine/engine.h engine/sound.h engine/world.h engine/octa.h
engine/worldio.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/worldio.o: engine/model.h
game/ai.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
game/ai.o: shared/ents.h shared/iengine.h shared/igame.h engine/engine.h
game/ai.o: engine/sound.h engine/world.h engine/octa.h engine/lightmap.h
game/ai.o: engine/bih.h engine/texture.h engine/model.h game/game.h
game/ai.o: game/ctf.h game/stf.h game/vars.h game/scoreboard.h
game/client.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
game/client.o: shared/ents.h shared/iengine.h shared/igame.h engine/engine.h
game/client.o: engine/sound.h engine/world.h engine/octa.h engine/lightmap.h
game/client.o: engine/bih.h engine/texture.h engine/model.h shared/crypto.h
game/client.o: game/game.h game/ctf.h game/stf.h game/vars.h
game/client.o: game/scoreboard.h
game/ctf.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
game/ctf.o: shared/ents.h shared/iengine.h shared/igame.h engine/engine.h
game/ctf.o: engine/sound.h engine/world.h engine/octa.h engine/lightmap.h
game/ctf.o: engine/bih.h engine/texture.h engine/model.h game/game.h
game/ctf.o: game/ctf.h game/stf.h game/vars.h game/scoreboard.h
game/entities.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
game/entities.o: shared/ents.h shared/iengine.h shared/igame.h
game/entities.o: engine/engine.h engine/sound.h engine/world.h engine/octa.h
game/entities.o: engine/lightmap.h engine/bih.h engine/texture.h
game/entities.o: engine/model.h game/game.h game/ctf.h game/stf.h game/vars.h
game/entities.o: game/scoreboard.h
game/hud.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
game/hud.o: shared/ents.h shared/iengine.h shared/igame.h engine/engine.h
game/hud.o: engine/sound.h engine/world.h engine/octa.h engine/lightmap.h
game/hud.o: engine/bih.h engine/texture.h engine/model.h game/game.h
game/hud.o: game/ctf.h game/stf.h game/vars.h game/scoreboard.h
game/physics.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
game/physics.o: shared/ents.h shared/iengine.h shared/igame.h engine/engine.h
game/physics.o: engine/sound.h engine/world.h engine/octa.h engine/lightmap.h
game/physics.o: engine/bih.h engine/texture.h engine/model.h game/game.h
game/physics.o: game/ctf.h game/stf.h game/vars.h game/scoreboard.h
game/projs.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
game/projs.o: shared/ents.h shared/iengine.h shared/igame.h engine/engine.h
game/projs.o: engine/sound.h engine/world.h engine/octa.h engine/lightmap.h
game/projs.o: engine/bih.h engine/texture.h engine/model.h game/game.h
game/projs.o: game/ctf.h game/stf.h game/vars.h game/scoreboard.h
game/server.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
game/server.o: shared/ents.h shared/iengine.h shared/igame.h engine/engine.h
game/server.o: engine/sound.h engine/world.h engine/octa.h engine/lightmap.h
game/server.o: engine/bih.h engine/texture.h engine/model.h game/game.h
game/server.o: game/ctf.h game/stf.h game/vars.h game/scoreboard.h
game/server.o: shared/hash.h game/stfmode.h game/ctfmode.h game/duelmut.h
game/server.o: game/aiman.h game/auth.h game/extinfo.h
game/stf.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
game/stf.o: shared/ents.h shared/iengine.h shared/igame.h engine/engine.h
game/stf.o: engine/sound.h engine/world.h engine/octa.h engine/lightmap.h
game/stf.o: engine/bih.h engine/texture.h engine/model.h game/game.h
game/stf.o: game/ctf.h game/stf.h game/vars.h game/scoreboard.h
game/weapons.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
game/weapons.o: shared/ents.h shared/iengine.h shared/igame.h engine/engine.h
game/weapons.o: engine/sound.h engine/world.h engine/octa.h engine/lightmap.h
game/weapons.o: engine/bih.h engine/texture.h engine/model.h game/game.h
game/weapons.o: game/ctf.h game/stf.h game/vars.h game/scoreboard.h
game/world.o: shared/cube.h shared/tools.h shared/command.h shared/geom.h
game/world.o: shared/ents.h shared/iengine.h shared/igame.h engine/engine.h
game/world.o: engine/sound.h engine/world.h engine/octa.h engine/lightmap.h
game/world.o: engine/bih.h engine/texture.h engine/model.h game/game.h
game/world.o: game/ctf.h game/stf.h game/vars.h game/scoreboard.h

shared/tools-standalone.o: shared/cube.h shared/tools.h shared/command.h
shared/tools-standalone.o: shared/geom.h shared/ents.h shared/iengine.h
shared/tools-standalone.o: shared/igame.h
shared/hash-standalone.o: shared/cube.h shared/tools.h shared/command.h
shared/hash-standalone.o: shared/geom.h shared/ents.h shared/iengine.h
shared/hash-standalone.o: shared/igame.h shared/hash.h
engine/command-standalone.o: shared/cube.h shared/tools.h shared/command.h
engine/command-standalone.o: shared/geom.h shared/ents.h shared/iengine.h
engine/command-standalone.o: shared/igame.h engine/engine.h engine/sound.h
engine/command-standalone.o: engine/world.h engine/octa.h engine/lightmap.h
engine/command-standalone.o: engine/bih.h engine/texture.h engine/model.h
engine/master-standalone.o: shared/cube.h shared/tools.h shared/command.h
engine/master-standalone.o: shared/geom.h shared/ents.h shared/iengine.h
engine/master-standalone.o: shared/igame.h engine/engine.h engine/sound.h
engine/master-standalone.o: engine/world.h engine/octa.h engine/lightmap.h
engine/master-standalone.o: engine/bih.h engine/texture.h engine/model.h
engine/master-standalone.o: shared/crypto.h shared/hash.h
engine/server-standalone.o: shared/cube.h shared/tools.h shared/command.h
engine/server-standalone.o: shared/geom.h shared/ents.h shared/iengine.h
engine/server-standalone.o: shared/igame.h engine/engine.h engine/sound.h
engine/server-standalone.o: engine/world.h engine/octa.h engine/lightmap.h
engine/server-standalone.o: engine/bih.h engine/texture.h engine/model.h
game/server-standalone.o: shared/cube.h shared/tools.h shared/command.h
game/server-standalone.o: shared/geom.h shared/ents.h shared/iengine.h
game/server-standalone.o: shared/igame.h engine/engine.h engine/sound.h
game/server-standalone.o: engine/world.h engine/octa.h engine/lightmap.h
game/server-standalone.o: engine/bih.h engine/texture.h engine/model.h
game/server-standalone.o: game/game.h game/ctf.h game/stf.h game/vars.h
game/server-standalone.o: shared/hash.h game/stfmode.h game/ctfmode.h
game/server-standalone.o: game/duelmut.h game/aiman.h game/auth.h
game/server-standalone.o: game/extinfo.h

shared/cube.h.gch: shared/tools.h shared/command.h shared/geom.h
shared/cube.h.gch: shared/ents.h shared/iengine.h shared/igame.h
