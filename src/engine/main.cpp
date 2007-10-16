// main.cpp: initialisation & main loop

#include "pch.h"
#include "engine.h"

void cleanup()
{
	cleangl();
	cleanupserver();
	SDL_ShowCursor(1);
	freeocta(worldroot);
	extern void clear_command(); clear_command();
	extern void clear_console(); clear_console();
	extern void clear_mdls();	clear_mdls();
	extern void clear_sound();	clear_sound();
	SDL_Quit();
}

void quit()					 // normal exit
{
	extern void writeinitcfg();
	writeinitcfg();
	writeservercfg();
	abortconnect();
	disconnect(1);
	writecfg();
	cleanup();
	exit(EXIT_SUCCESS);
}

void fatal(char *s, char *o)	// failure exit
{
    SDL_ShowCursor(1);
	s_sprintfd(msg)("%s%s\n", s, o);
	printf(msg);
	#ifdef WIN32
#ifdef BFRONTIER // blood frontier
		MessageBox(NULL, msg, "Blood Frontier: Error", MB_OK|MB_SYSTEMMODAL);
#else
		MessageBox(NULL, msg, "sauerbraten fatal error", MB_OK|MB_SYSTEMMODAL);
#endif
	#endif
    SDL_Quit();
	exit(EXIT_FAILURE);
}

SDL_Surface *screen = NULL;

int curtime;
int totalmillis = 0, lastmillis = 0;

dynent *player = NULL;

static bool initing = false, restoredinits = false;
bool initwarning()
{
	if(!initing) 
	{
#ifdef BFRONTIER // blood frontier
		if(restoredinits) conoutf("Please restart Blood Frontier for this setting to take effect.");
		else conoutf("Please restart Blood Frontier with the -r command-line option for this setting to take effect.");
#else
		if(restoredinits) conoutf("Please restart Sauerbraten for this setting to take effect.");
		else conoutf("Please restart Sauerbraten with the -r command-line option for this setting to take effect.");
#endif
	}
	return !initing;
}

VARF(scr_w, 0, 1024, 10000, initwarning());
VARF(scr_h, 0, 768, 10000, initwarning());
VARF(colorbits, 0, 0, 32, initwarning());
VARF(depthbits, 0, 0, 32, initwarning());
VARF(stencilbits, 0, 1, 32, initwarning());
VARF(fsaa, 0, 0, 16, initwarning());
VARF(vsync, -1, -1, 1, initwarning());

void writeinitcfg()
{
	FILE *f = openfile("init.cfg", "w");
	if(!f) return;
	fprintf(f, "// automatically written on exit, DO NOT MODIFY\n// modify settings in game\n");
	fprintf(f, "scr_w %d\n", scr_w);
	fprintf(f, "scr_h %d\n", scr_h);
	fprintf(f, "colorbits %d\n", colorbits);
	fprintf(f, "depthbits %d\n", depthbits);
	fprintf(f, "stencilbits %d\n", stencilbits);
	fprintf(f, "fsaa %d\n", fsaa);
	fprintf(f, "vsync %d\n", vsync);
	extern int useshaders, shaderprecision;
	fprintf(f, "shaders %d\n", useshaders);
	fprintf(f, "shaderprecision %d\n", shaderprecision);
	extern int soundchans, soundfreq, soundbufferlen;
	fprintf(f, "soundchans %d\n", soundchans);
	fprintf(f, "soundfreq %d\n", soundfreq);
	fprintf(f, "soundbufferlen %d\n", soundbufferlen);
	fclose(f);
}

void screenshot(char *filename)
{
	SDL_Surface *image = SDL_CreateRGBSurface(SDL_SWSURFACE, screen->w, screen->h, 24, 0x0000FF, 0x00FF00, 0xFF0000, 0);
	if(!image) return;
	uchar *tmp = new uchar[screen->w*screen->h*3];
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, screen->w, screen->h, GL_RGB, GL_UNSIGNED_BYTE, tmp);
	uchar *dst = (uchar *)image->pixels;
	loopi(scr_h)
	{
		memcpy(dst, &tmp[3*screen->w*(screen->h-i-1)], 3*screen->w);
		endianswap(dst, 3, screen->w);
		dst += image->pitch;
	}
	delete[] tmp;
#ifdef BFRONTIER
	char *pname = "";
	if (!*filename) pname = "packages/";
	SDL_SaveBMP(image, findfile(makefile(*filename ? filename : mapname, pname, ".bmp", true, true), "wb"));
#else
	if(!filename[0])
	{
		static string buf;
		s_sprintf(buf)("screenshot_%d.bmp", lastmillis);
		filename = buf;
	}
    else path(filename);
	SDL_SaveBMP(image, findfile(filename, "wb"));
#endif
	SDL_FreeSurface(image);
}

COMMAND(screenshot, "s");
COMMAND(quit, "");

void computescreen(const char *text, Texture *t)
{
	int w = screen->w, h = screen->h;
	gettextres(w, h);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
#ifdef BFRONTIER // black background
	glClearColor(0.f, 0.f, 0.f, 1);
#else
	glClearColor(0.15f, 0.15f, 0.15f, 1);
	glColor3f(1, 1, 1);
#endif
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glMatrixMode(GL_PROJECTION);
	loopi(2)
	{
#ifdef BFRONTIER
		glClear(GL_COLOR_BUFFER_BIT);
		glLoadIdentity();
		glOrtho(0, w, h, 0, -1, 1);
		settexture("packages/textures/loadback.jpg");
	
		glColor3f(1, 1, 1);
		glBegin(GL_QUADS);
	
		glTexCoord2f(0, 0); glVertex2i(0, 0);
		glTexCoord2f(1, 0); glVertex2i(w, 0);
		glTexCoord2f(1, 1); glVertex2i(w, h);
		glTexCoord2f(0, 1); glVertex2i(0, h);
					
		glEnd();
		glLoadIdentity();
		glOrtho(0, w*3, h*3, 0, -1, 1);
#else
		glLoadIdentity();
		glOrtho(0, w*3, h*3, 0, -1, 1);
		glClear(GL_COLOR_BUFFER_BIT);
#endif
		draw_text(text, 70, 2*FONTH + FONTH/2);
		glLoadIdentity();
		glOrtho(0, w, h, 0, -1, 1);
		if(t)
		{
			glDisable(GL_BLEND);
			glBindTexture(GL_TEXTURE_2D, t->gl);
#if 0
			int x = (w-640)/2, y = (h-320)/2;
			glBegin(GL_TRIANGLE_FAN);
			glTexCoord2f(0.5f, 0.5f); glVertex2f(x+640/2.0f, y+320/2.0f);
			loopj(64+1) 
			{ 
				float c = 0.5f+0.5f*cosf(2*M_PI*j/64.0f), s = 0.5f+0.5f*sinf(2*M_PI*j/64.0f);
				glTexCoord2f(c, 320.0f/640.0f*(s-0.5f)+0.5f);
				glVertex2f(x+640*c, y+320*s);
			}
#else
			int sz = 256, x = (w-sz)/2, y = min(384, h-256);
			glBegin(GL_QUADS);
			glTexCoord2f(0, 0); glVertex2i(x,	y);
			glTexCoord2f(1, 0); glVertex2i(x+sz, y);
			glTexCoord2f(1, 1); glVertex2i(x+sz, y+sz);
			glTexCoord2f(0, 1); glVertex2i(x,	y+sz);
#endif
			glEnd();
			glEnable(GL_BLEND);
		}
		int x = (w-512)/2, y = 128;
#ifdef BFRONTIER // blood frontier and moved data
		settexture("packages/textures/bfrontierlogo.png");
#else
		settexture("data/sauer_logo_512_256a.png");
#endif
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex2i(x,	 y);
		glTexCoord2f(1, 0); glVertex2i(x+512, y);
		glTexCoord2f(1, 1); glVertex2i(x+512, y+256);
		glTexCoord2f(0, 1); glVertex2i(x,	 y+256);
		glEnd();
		SDL_GL_SwapBuffers();
	}
	glMatrixMode(GL_MODELVIEW);
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
}

static void bar(float bar, int w, int o, float r, float g, float b)
{
	int side = 2*FONTH;
	float x1 = side, x2 = bar*(w*3-2*side)+side;
	float y1 = o*FONTH;
	glColor3f(r, g, b);
	glBegin(GL_TRIANGLE_STRIP);
	loopk(10)
	{
		float c = cosf(M_PI/2 + k/9.0f*M_PI), s = 1 + sinf(M_PI/2 + k/9.0f*M_PI);
		glVertex2f(x2 - c*FONTH, y1 + s*FONTH);
		glVertex2f(x1 + c*FONTH, y1 + s*FONTH);
	}
	glEnd();

#if 0
	glColor3f(0.3f, 0.3f, 0.3f);
	glBegin(GL_LINE_LOOP);
	loopk(10)
	{
		float c = cosf(M_PI/2 + k/9.0f*M_PI), s = 1 + sinf(M_PI/2 + k/9.0f*M_PI);
		glVertex2f(x1 + c*FONTH, y1 + s*FONTH);
	}
	loopk(10)
	{
		float c = cosf(M_PI/2 + k/9.0f*M_PI), s = 1 - sinf(M_PI/2 + k/9.0f*M_PI);
		glVertex2f(x2 - c*FONTH, y1 + s*FONTH);
	}
	glEnd();
#endif
}

void show_out_of_renderloop_progress(float bar1, const char *text1, float bar2, const char *text2, GLuint tex)	// also used during loading
{
	if(!inbetweenframes) return;

#ifdef BFRONTIER // unconnected state
	updateframe(false);
#else
	clientkeepalive();	  // make sure our connection doesn't time out while loading maps etc.
#endif

	int w = screen->w, h = screen->h;
	gettextres(w, h);

	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, w*3, h*3, 0, -1, 1);
	notextureshader->set();

	glLineWidth(3);

	if(text1)
	{
		bar(1, w, 4, 0, 0, 0.8f);
		if(bar1>0) bar(bar1, w, 4, 0, 0.5f, 1);
	}

	if(bar2>0)
	{
		bar(1, w, 6, 0.5f, 0, 0);
		bar(bar2, w, 6, 0.75f, 0, 0);
	}

	glLineWidth(1);

	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	defaultshader->set();

	if(text1) draw_text(text1, 2*FONTH, 4*FONTH + FONTH/2);
	if(bar2>0) draw_text(text2, 2*FONTH, 6*FONTH + FONTH/2);
	
	glDisable(GL_BLEND);

	if(tex)
	{
		glBindTexture(GL_TEXTURE_2D, tex);
		int sz = 256, x = (w-sz)/2, y = min(384, h-256);
		sz *= 3;
		x *= 3;
		y *= 3;
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex2i(x,	y);
		glTexCoord2f(1, 0); glVertex2i(x+sz, y);
		glTexCoord2f(1, 1); glVertex2i(x+sz, y+sz);
		glTexCoord2f(0, 1); glVertex2i(x,	y+sz);
		glEnd();
	}

	glDisable(GL_TEXTURE_2D);

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);
	SDL_GL_SwapBuffers();
}

void setfullscreen(bool enable)
{
	if(enable == !(screen->flags&SDL_FULLSCREEN))
	{
#if defined(WIN32) || defined(__APPLE__)
		conoutf("\"fullscreen\" variable not supported on this platform. Use the -t command-line option.");
		extern int fullscreen;
		fullscreen = !enable;
#else
		SDL_WM_ToggleFullScreen(screen);
		SDL_WM_GrabInput((screen->flags&SDL_FULLSCREEN) ? SDL_GRAB_ON : SDL_GRAB_OFF);
#endif
	}
}

void screenres(int *w, int *h)
{
#if !defined(WIN32) && !defined(__APPLE__)
	if(initing)
	{
#endif
		scr_w = *w;
		scr_h = *h;
#if defined(WIN32) || defined(__APPLE__)
		initwarning();
#else
		return;
	}
	SDL_Surface *surf = SDL_SetVideoMode(*w, *h, 0, SDL_OPENGL|SDL_RESIZABLE|(screen->flags&SDL_FULLSCREEN));
	if(!surf) return;
	screen = surf;
	scr_w = screen->w;
	scr_h = screen->h;
	glViewport(0, 0, scr_w, scr_h);
#endif
}

VARF(fullscreen, 0, 0, 1, setfullscreen(fullscreen!=0));

COMMAND(screenres, "ii");

VARFP(gamma, 30, 100, 300,
{
	float f = gamma/100.0f;
	if(SDL_SetGamma(f,f,f)==-1)
	{
		conoutf("Could not set gamma (card/driver doesn't support it?)");
		conoutf("sdl: %s", SDL_GetError());
	}
});

void keyrepeat(bool on)
{
	SDL_EnableKeyRepeat(on ? SDL_DEFAULT_REPEAT_DELAY : 0,
							 SDL_DEFAULT_REPEAT_INTERVAL);
}

VARF(gamespeed, 10, 100, 1000, if(multiplayer()) gamespeed = 100);

VARF(paused, 0, 0, 1, if(multiplayer()) paused = 0);

VAR(maxfps, 5, 200, 500);

void limitfps(int &millis, int curmillis)
{
	static int fpserror = 0;
	int delay = 1000/maxfps - (millis-curmillis);
	if(delay < 0) fpserror = 0;
	else
	{
		fpserror += 1000%maxfps;
		if(fpserror >= maxfps)
		{
			++delay;
			fpserror -= maxfps;
		}
		if(delay > 0)
		{
			SDL_Delay(delay);
			millis += delay;
		}
	}
}

#if defined(WIN32) && !defined(_DEBUG) && !defined(__GNUC__)
void stackdumper(unsigned int type, EXCEPTION_POINTERS *ep)
{
	if(!ep) fatal("unknown type");
	EXCEPTION_RECORD *er = ep->ExceptionRecord;
	CONTEXT *context = ep->ContextRecord;
	string out, t;
#ifdef BFRONTIER // blood frontier
	s_sprintf(out)("Blood Frontier Win32 Exception: 0x%x [0x%x]\n\n", er->ExceptionCode, er->ExceptionCode==EXCEPTION_ACCESS_VIOLATION ? er->ExceptionInformation[1] : -1);
#else
	s_sprintf(out)("Sauerbraten Win32 Exception: 0x%x [0x%x]\n\n", er->ExceptionCode, er->ExceptionCode==EXCEPTION_ACCESS_VIOLATION ? er->ExceptionInformation[1] : -1);
#endif
	STACKFRAME sf = {{context->Eip, 0, AddrModeFlat}, {}, {context->Ebp, 0, AddrModeFlat}, {context->Esp, 0, AddrModeFlat}, 0};
	SymInitialize(GetCurrentProcess(), NULL, TRUE);

	while(::StackWalk(IMAGE_FILE_MACHINE_I386, GetCurrentProcess(), GetCurrentThread(), &sf, context, NULL, ::SymFunctionTableAccess, ::SymGetModuleBase, NULL))
	{
		struct { IMAGEHLP_SYMBOL sym; string n; } si = { { sizeof( IMAGEHLP_SYMBOL ), 0, 0, 0, sizeof(string) } };
		IMAGEHLP_LINE li = { sizeof( IMAGEHLP_LINE ) };
		DWORD off;
		if(SymGetSymFromAddr(GetCurrentProcess(), (DWORD)sf.AddrPC.Offset, &off, &si.sym) && SymGetLineFromAddr(GetCurrentProcess(), (DWORD)sf.AddrPC.Offset, &off, &li))
		{
			char *del = strrchr(li.FileName, '\\');
			s_sprintf(t)("%s - %s [%d]\n", si.sym.Name, del ? del + 1 : li.FileName, li.LineNumber);
			s_strcat(out, t);
		}
	}
	fatal(out);
}
#endif

#define MAXFPSHISTORY 60

int fpspos = 0, fpshistory[MAXFPSHISTORY];

void resetfpshistory()
{
	loopi(MAXFPSHISTORY) fpshistory[i] = 1;
	fpspos = 0;
}

void updatefpshistory(int millis)
{
	fpshistory[fpspos++] = max(1, min(1000, millis));
	if(fpspos>=MAXFPSHISTORY) fpspos = 0;
}

void getfps(int &fps, int &bestdiff, int &worstdiff)
{
	int total = fpshistory[MAXFPSHISTORY-1], best = total, worst = total;
	loopi(MAXFPSHISTORY-1)
	{
		int millis = fpshistory[i];
		total += millis;
		if(millis < best) best = millis;
		if(millis > worst) worst = millis;
	}

	fps = (1000*MAXFPSHISTORY)/total;
	bestdiff = 1000/best-fps;
	worstdiff = fps-1000/worst;
}

bool inbetweenframes = false;

static bool findarg(int argc, char **argv, char *str)
{
	for(int i = 1; i<argc; i++) if(strstr(argv[i], str)==argv[i]) return true;
	return false;
}

static int clockrealbase = 0, clockvirtbase = 0;
static void clockreset() { clockrealbase = SDL_GetTicks(); clockvirtbase = totalmillis; }
VARFP(clockerror, 990000, 1000000, 1010000, clockreset());
VARFP(clockfix, 0, 0, 1, clockreset());

#ifdef BFRONTIER // blood frontier, grabmouse, auto performance, unconnected state, rehashing
VAR(bloodfrontier, 1, BFRONTIER, -1); // for scripts
VARP(online, 0, 1, 1); // if so, then enable certain actions
VARP(verbose, 0, 0, 2); // be more or less expressive to console

void _grabmouse(int n)
{
	SDL_WM_GrabInput(n ? SDL_GRAB_ON : SDL_GRAB_OFF);
	keyrepeat(n ? false : true);
	SDL_ShowCursor(n ? 0 : 1);
}
VARF(grabmouse, 0, 1, 1, _grabmouse(grabmouse););

int getmatvec(vec v)
{
	cube &c = lookupcube((int)v.x, (int)v.y, (int)v.z);
	
	if (c.ext) return c.ext->material;
	return MAT_AIR;
}

int lastperf = 0;

VARP(perfmin, 0, 1, 100);
VARP(perfmax, 0, 100, 100);
VARFP(perffps, 0, 20, 100, perffps = min(perffps, maxfps-1));
VARP(perfrate, 0, 500, 10000);
VARFP(perflimit, 0, 10, 100, perflimit = min(perflimit, perffps-1));
VARP(perfall, 0, 0, 1);

void perfset(int level)
{
	if (level >= 0 && level <= 100)
	{
		#define perfvarscl(s,v,n0,n1) \
			{ \
				int _##s = int((float(n0-n1)/float(v))*float(level))+n1; \
				setvar(#s, max(min(_##s, n0), n1), true); \
			} \

		if (perfall || !level || level < perfmin)
		{
			perfvarscl(shaderdetail,	100,	3,			0);
			perfvarscl(waterreflect,	2,		1,			0);
			perfvarscl(waterrefract,	2,		1,			0);
			perfvarscl(grass,			1,		1,			0);
			perfvarscl(glassenv,		1,		1,			0);
			perfvarscl(envmapmodels,	1,		1,			0);
			perfvarscl(glowmodels,		1,		1,			0);
			perfvarscl(shadowmap,		1,		1,			0);
		}

		perfvarscl(decalfade,			100,	60000,		1);
		perfvarscl(dynlightdist,		100,	10000,		128);
		perfvarscl(emitfps,				100,	60,			1);
		perfvarscl(grassanimdist,		100,	10000,		0);
		perfvarscl(grassdist,			100,	10000,		0);
		perfvarscl(grassfalloff,		100,	1000,		0);
		perfvarscl(grasslod,			100,	1000,		0);
		perfvarscl(grasslodz,			100,	10000,		0);
		perfvarscl(grasstaper,			100,	200,		0);
		perfvarscl(loddistance,			100,	10000,		0);
		perfvarscl(maxparticledistance,	100,	1024,		256);
		perfvarscl(maxreflect,			100,	8,			1);
		perfvarscl(reflectdist,			100,	10000,		128);

		//perfvarscl(shadowmapradius,		100,	256,		64);
		//perfvarscl(shadowmapdist,		100,	512,		128);
		//perfvarscl(shadowmapfalloff,	100,	512,		0);
		//perfvarscl(shadowmapbias,		100,	1024,		0);

		lastperf = lastmillis;
	}
}

VARFP(perf, 0, 100, 100, perfset(perf));
VAR(curfps, 1, 0, -1);
VAR(bestfps, 1, 0, -1);
VAR(worstfps, 1, 0, -1);

void perfcheck()
{
	extern void getfps(int &fps, int &bestdiff, int &worstdiff);
	int fps, bestdiff, worstdiff;
	getfps(fps, bestdiff, worstdiff);
	
	setvar("curfps", fps);
	setvar("bestfps", fps+bestdiff);
	setvar("worstfps", fps-worstdiff);
	
	if (perffps && (lastmillis-lastperf > perfrate || fps-worstdiff < perflimit))
	{
		float amt = float(fps-worstdiff)/float(perffps);
		
		if (fps-worstdiff < perflimit && perf > perfmin) setvar("perf", perfmin, true);
		else if (amt < 1.f) setvar("perf", max(perf-int((1.f-amt)*10.f), perfmin), true);
		else if (amt > 1.f) setvar("perf", min(perf+int(amt), perfmax), true);
	}
}

void rehash(bool reload)
{
	if (reload)
	{
		writeservercfg();
		writecfg();
	}

	persistidents = false;
	exec("packages/defaults.cfg");
	persistidents = true;

	execfile("servers.cfg");
	execfile("config.cfg");
	execfile("autoexec.cfg");
}
ICOMMAND(rehash, "i", (int *nosave), rehash(*nosave ?  false : true));

void startgame(char *load, char *initscript)
{
	sv->changemap(load ? load : sv->defaultmap(), 0);

	if (initscript) execute(initscript);
	if (load) localconnect();
	else showgui("main");
}

int frames = 0;

void updateframe(bool dorender)
{
	int millis = SDL_GetTicks() - clockrealbase;
	
	if (clockfix) millis = int(millis*(double(clockerror)/1000000));
	if ((millis += clockvirtbase) < totalmillis) millis = totalmillis;

	if (dorender) limitfps(millis, totalmillis);
	
	int elapsed = millis-totalmillis;

	if (paused) curtime = 0;
	else curtime = (elapsed*gamespeed)/100;
	
	if (dorender)
	{
		cl->updateworld(worldpos, curtime, lastmillis);
		menuprocess();
	}

	lastmillis += curtime;
	totalmillis = millis;

	serverslice(0);

	if (dorender)
	{
		if (frames) updatefpshistory(elapsed);
		frames++;
		
		perfcheck();

		checksleep(lastmillis);
		
		if (!connpeer)
		{
			findorientation();
			entity_particles();
			updatevol();
			checkmapsounds();
	
			inbetweenframes = false;
			SDL_GL_SwapBuffers();
			if(frames>2) gl_drawframe(screen->w, screen->h);
			inbetweenframes = true;
		}
	}
	else clientkeepalive();
}
#endif
int main(int argc, char **argv)
{
	#ifdef WIN32
	//atexit((void (__cdecl *)(void))_CrtDumpMemoryLeaks);
	#ifndef _DEBUG
	#ifndef __GNUC__
	__try {
	#endif
	#endif
	#endif

	bool dedicated = false;
	int fs = SDL_FULLSCREEN, par = 0;
	char *load = NULL, *initscript = NULL;

#ifdef BFRONTIER // so we have it in the console too
	#define log(s) conoutf("init: %s", s);
#else
	#define log(s) puts("init: " s)
#endif

	initing = true;
	for(int i = 1; i<argc; i++)
	{
		if(argv[i][0]=='-') switch(argv[i][1])
		{
			case 'q': sethomedir(&argv[i][2]); break;
			case 'k': addpackagedir(&argv[i][2]); break;
			case 'r': execfile(argv[i][2] ? &argv[i][2] : (char *)"init.cfg"); restoredinits = true; break;
			case 'd': dedicated = true; break;
			case 'w': scr_w = atoi(&argv[i][2]); if(!findarg(argc, argv, "-h")) scr_h = (scr_w*3)/4; break;
			case 'h': scr_h = atoi(&argv[i][2]); if(!findarg(argc, argv, "-w")) scr_w = (scr_h*4)/3; break;
			case 'z': depthbits = atoi(&argv[i][2]); break;
			case 'b': colorbits = atoi(&argv[i][2]); break;
			case 'a': fsaa = atoi(&argv[i][2]); break;
			case 'v': vsync = atoi(&argv[i][2]); break;
			case 't': fs = 0; break;
			case 's': stencilbits = atoi(&argv[i][2]); break;
			case 'f': 
			{
				extern int useshaders, shaderprecision; 
				int n = atoi(&argv[i][2]);
				useshaders = n ? 1 : 0;
				shaderprecision = min(max(n - 1, 0), 3);
				break;
			}
			case 'l': 
			{
				char pkgdir[] = "packages/"; 
#ifdef BFRONTIER
				load = strstr(&argv[i][2], pkgdir); 
#else
				load = strstr(path(&argv[i][2]), path(pkgdir)); 
#endif
				if(load) load += sizeof(pkgdir)-1; 
				else load = &argv[i][2]; 
				break;
			}
			case 'x': initscript = &argv[i][2]; break;
			default: if(!serveroption(argv[i])) gameargs.add(argv[i]); break;
		}
		else gameargs.add(argv[i]);
	}
	initing = false;

	log("sdl");

	#ifdef _DEBUG
	par = SDL_INIT_NOPARACHUTE;
	fs = 0;
	#ifdef WIN32
	SetEnvironmentVariable("SDL_DEBUG", "1");
	#endif
	#endif

	//#ifdef WIN32
	//SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	//#endif
#ifdef BFRONTIER // joytick support
	par |= SDL_INIT_JOYSTICK;
#endif

	if(SDL_Init(SDL_INIT_TIMER|SDL_INIT_VIDEO|SDL_INIT_AUDIO|par)<0) fatal("Unable to initialize SDL: ", SDL_GetError());

	log("enet");
	if(enet_initialize()<0) fatal("Unable to initialise network module");

#ifdef BFRONTIER
	log("server");
#endif
	initserver(dedicated);  // never returns if dedicated

	log("video: mode");
	int resize = SDL_RESIZABLE;
	#if defined(WIN32) || defined(__APPLE__)
	resize = 0;
	#endif
	SDL_Rect **modes = SDL_ListModes(NULL, SDL_OPENGL|resize|fs);
	if(modes && modes!=(SDL_Rect **)-1)
	{
		bool hasmode = false;
		for(int i = 0; modes[i]; i++)
		{
			if(scr_w <= modes[i]->w && scr_h <= modes[i]->h) { hasmode = true; break; }
		}
		if(!hasmode) { scr_w = modes[0]->w; scr_h = modes[0]->h; }
	}
    bool hasbpp = true;
    if(colorbits && modes) 
        hasbpp = SDL_VideoModeOK(modes!=(SDL_Rect **)-1 ? modes[0]->w : scr_w, modes!=(SDL_Rect **)-1 ? modes[0]->h : scr_h, colorbits, SDL_OPENGL|resize|fs)==colorbits;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#if SDL_VERSION_ATLEAST(1, 2, 11)
    if(vsync>=0) SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, vsync);
#endif
    static int configs[] = 
    { 
        0x7, /* try everything */
        0x6, 0x5, 0x3, /* try disabling one at a time */
        0x4, 0x2, 0x1, /* try disabling two at a time */
        0 /* try disabling everything */
    };
    int config = 0;
    loopi(sizeof(configs)/sizeof(configs[0]))
	{
        config = configs[i];
        if(!depthbits && config&1) continue;
        if(!stencilbits && config&2) continue;
        if(!fsaa && config&4) continue;
        if(depthbits) SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, config&1 ? depthbits : 0);
		if(stencilbits) 
		{
            SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, config&2 ? 1 : 0);
            hasstencil = (config&2)!=0;
        }
        if(fsaa)
        {
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, config&4 ? 1 : 0);
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, config&4 ? fsaa : 0);
        }
        screen = SDL_SetVideoMode(scr_w, scr_h, hasbpp ? colorbits : 0, SDL_OPENGL|resize|fs);
        if(screen) break;
	}
	if(!screen) fatal("Unable to create OpenGL screen: ", SDL_GetError());
    else
    {
        if(!hasbpp) conoutf("%d bit color buffer not supported - disabling", colorbits);
        if(depthbits && (config&1)==0) conoutf("%d bit z-buffer not supported - disabling", depthbits);
        if(stencilbits && (config&2)==0) conoutf("Stencil buffer not supported - disabling");
        if(fsaa && (config&4)==0) conoutf("%dx anti-aliasing not supported - disabling", fsaa);
	}
    
	scr_w = screen->w;
	scr_h = screen->h;

	fullscreen = fs!=0;

	log("video: misc");
#ifdef BFRONTIER // blood frontier, game name support
	s_sprintfd(caption)("Blood Frontier: %s", sv->gamename());
	SDL_WM_SetCaption(caption, NULL);
#else
	SDL_WM_SetCaption("sauerbraten engine", NULL);
#endif
	#ifndef WIN32
	if(fs)
	#endif
	SDL_WM_GrabInput(SDL_GRAB_ON);
	keyrepeat(false);
	SDL_ShowCursor(0);

	log("gl");
	persistidents = false;
#ifdef BFRONTIER // moved data
	if(!execfile("packages/stdlib.cfg")) fatal("cannot find data files (you are running from the wrong folder, try .bat file in the main folder)");	// this is the first file we load.
    gl_init(scr_w, scr_h, hasbpp ? colorbits : 0, config&1 ? depthbits : 0, config&4 ? fsaa : 0);
    notexture = textureload("packages/textures/notexture.png");
#else
	if(!execfile("data/stdlib.cfg")) fatal("cannot find data files (you are running from the wrong folder, try .bat file in the main folder)");	// this is the first file we load.
    gl_init(scr_w, scr_h, hasbpp ? colorbits : 0, config&1 ? depthbits : 0, config&4 ? fsaa : 0);
    notexture = textureload("data/notexture.png");
#endif
    if(!notexture) fatal("could not find core textures");

	log("console");
#ifdef BFRONTIER // unbuffered i/o
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	
	if(!execfile("packages/font.cfg")) fatal("cannot find font definitions");
#else
	if(!execfile("data/font.cfg")) fatal("cannot find font definitions");
#endif
	if(!setfont("default")) fatal("no default font specified");

	computescreen("initializing...");
	inbetweenframes = true;
	particleinit();

	log("world");
	camera1 = player = cl->iterdynents(0);
#ifdef BFRONTIER
	emptymap(0, true);
#else	
	emptymap(0, true);
#endif

	log("sound");
	initsound();
#ifdef BFRONTIER // joystick support
	log("joystick");
	initjoy();
#endif

	log("cfg");
#ifdef BFRONTIER // external functions for config and game initialisation
	rehash(false);
	startgame(load, initscript);
#else
	exec("data/keymap.cfg");
	exec("data/stdedit.cfg");
	exec("data/menus.cfg");
	exec("data/sounds.cfg");
	exec("data/brush.cfg");
	execfile("mybrushes.cfg");
	if(cl->savedservers()) execfile(cl->savedservers());
	
	persistidents = true;
	
	if(!execfile(cl->savedconfig())) exec(cl->defaultconfig());
	execfile(cl->autoexec());

	persistidents = false;

	string gamecfgname;
	s_strcpy(gamecfgname, "data/game_");
	s_strcat(gamecfgname, cl->gameident());
	s_strcat(gamecfgname, ".cfg");
	exec(gamecfgname);

	persistidents = true;

	log("localconnect");
	localconnect();
	cc->gameconnect(false);
	cc->changemap(load ? load : cl->defaultmap());

	if(initscript) execute(initscript);
#endif

	log("mainloop");

	resetfpshistory();

#ifdef BFRONTIER // grabmouse support
	int ignore = 5;
#else
	int ignore = 5, grabmouse = 0;
#endif
	for(;;)
	{
#ifdef BFRONTIER // external frame rendering
		updateframe(true);
#else
		static int frames = 0;
        int millis = SDL_GetTicks() - clockrealbase;
        if(clockfix) millis = int(millis*(double(clockerror)/1000000));
        millis += clockvirtbase;
        if(millis<totalmillis) millis = totalmillis;
		limitfps(millis, totalmillis);
		int elapsed = millis-totalmillis;
		curtime = (elapsed*gamespeed)/100;
        //if(curtime>200) curtime = 200;
        //else if(curtime<1) curtime = 1;
		if(paused) curtime = 0;

		if(lastmillis) cl->updateworld(worldpos, curtime, lastmillis);
		
		menuprocess();

		lastmillis += curtime;
		totalmillis = millis;

        checksleep(lastmillis);

        serverslice(0);

        if(frames) updatefpshistory(elapsed);
        frames++;

		// miscellaneous general game effects
		findorientation();
		entity_particles();
		updatevol();
		checkmapsounds();
	
		inbetweenframes = false;
		SDL_GL_SwapBuffers();
		if(frames>2) gl_drawframe(screen->w, screen->h);
		inbetweenframes = true;
#endif

		SDL_Event event;
		int lasttype = 0, lastbut = 0;
		while(SDL_PollEvent(&event))
		{
			switch(event.type)
			{
				case SDL_QUIT:
					quit();
					break;

				#if !defined(WIN32) && !defined(__APPLE__)
				case SDL_VIDEORESIZE:
					screenres(&event.resize.w, &event.resize.h);
					break;
				#endif

				case SDL_KEYDOWN:
				case SDL_KEYUP:
					keypress(event.key.keysym.sym, event.key.state==SDL_PRESSED, event.key.keysym.unicode);
					break;

				case SDL_ACTIVEEVENT:
#ifdef BFRONTIER // grabmouse support
					if(event.active.state & SDL_APPINPUTFOCUS)
						setvar("grabmouse", event.active.gain, true);
#else
					if(event.active.state & SDL_APPINPUTFOCUS)
						grabmouse = event.active.gain;
					else
					if(event.active.gain)
						grabmouse = 1;
#endif
					break;

				case SDL_MOUSEMOTION:
					if(ignore) { ignore--; break; }
#ifdef BFRONTIER // grabmouse support
					if(!(screen->flags&SDL_FULLSCREEN) && grabmouse)
					{	
						#ifdef __APPLE__
						if(event.motion.y == 0) break;  //let mac users drag windows via the title bar
						#endif
						if(event.motion.x == screen->w / 2 && event.motion.y == screen->h / 2) break;
						SDL_WarpMouse(screen->w / 2, screen->h / 2);
					}
					if((screen->flags&SDL_FULLSCREEN) || grabmouse)
						if(!g3d_movecursor(event.motion.xrel, event.motion.yrel))
							mousemove(event.motion.xrel, event.motion.yrel);
#else
					#ifndef WIN32
					if(!(screen->flags&SDL_FULLSCREEN) && grabmouse)
					{	
						#ifdef __APPLE__
						if(event.motion.y == 0) break;  //let mac users drag windows via the title bar
						#endif
						if(event.motion.x == screen->w / 2 && event.motion.y == screen->h / 2) break;
						SDL_WarpMouse(screen->w / 2, screen->h / 2);
					}
					if((screen->flags&SDL_FULLSCREEN) || grabmouse)
					#endif
					if(!g3d_movecursor(event.motion.xrel, event.motion.yrel))
						mousemove(event.motion.xrel, event.motion.yrel);
#endif
					break;

				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					if(lasttype==event.type && lastbut==event.button.button) break; // why?? get event twice without it
					keypress(-event.button.button, event.button.state!=0, 0);
					lasttype = event.type;
					lastbut = event.button.button;
					break;
			}
#ifdef BFRONTIER // joystick support
			processjoy(&event);
#endif
		}
#ifdef BFRONTIER // joystick support, text colorpos reset
		movejoy();
		colorpos = 0; // last but not least.
#endif
	 }
	
	ASSERT(0);	
	return EXIT_FAILURE;

	#if defined(WIN32) && !defined(_DEBUG) && !defined(__GNUC__)
	} __except(stackdumper(0, GetExceptionInformation()), EXCEPTION_CONTINUE_SEARCH) { return 0; }
	#endif
}
