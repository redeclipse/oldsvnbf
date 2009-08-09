// main.cpp: initialisation & main loop

#include "engine.h"

SDL_Surface *screen = NULL;
SDL_Cursor *scursor = NULL, *ncursor = NULL;

void showcursor(bool show)
{
	if(show)
	{
		if(scursor) SDL_FreeCursor(scursor);
		scursor = NULL;
		SDL_SetCursor(ncursor);
	}
	else
	{
		if(!scursor)
		{
			Uint8 sd[1] = { 0 };
			if(!(scursor = SDL_CreateCursor(sd, sd, 1, 1, 0, 0)))
				fatal("could not create blank cursor");
		}

		SDL_SetCursor(scursor);
	}
}

void setcaption(const char *text)
{
    static string caption = "";
	defformatstring(newcaption)("%s v%.2f (%s)%s%s - %s", ENG_NAME, float(ENG_VERSION)/100.f, ENG_RELEASE, text ? ": " : "", text ? text : "", ENG_URL);
    if(strcmp(caption, newcaption))
    {
        copystring(caption, newcaption);
	    SDL_WM_SetCaption(caption, NULL);
    }
}

void keyrepeat(bool on)
{
	SDL_EnableKeyRepeat(on ? SDL_DEFAULT_REPEAT_DELAY : 0, SDL_DEFAULT_REPEAT_INTERVAL);
}

void inputgrab(bool on)
{
#ifndef WIN32
    if(!(screen->flags & SDL_FULLSCREEN)) SDL_WM_GrabInput(SDL_GRAB_OFF);
    else
#endif
    SDL_WM_GrabInput(on ? SDL_GRAB_ON : SDL_GRAB_OFF);
    showcursor(!on);
}

VARF(grabinput, 0, 0, 1, inputgrab(grabinput!=0));
VARP(autograbinput, 0, 1, 1);

void cleanup()
{
    recorder::stop();
	cleanupserver();
	showcursor(true);
	if(scursor) SDL_FreeCursor(scursor);
    SDL_WM_GrabInput(SDL_GRAB_OFF);
    SDL_SetGamma(1, 1, 1);
	freeocta(worldroot);
	extern void clear_command();	clear_command();
	extern void clear_console();	clear_console();
	extern void clear_mdls();		clear_mdls();
	stopsound();
	SDL_Quit();
}

void quit()					 // normal exit
{
	extern void writeinitcfg();
	inbetweenframes = false;
	writeinitcfg();
	abortconnect();
	disconnect(1);
	writecfg();
	cleanup();
	exit(EXIT_SUCCESS);
}

void fatal(const char *s, ...)    // failure exit
{
    static int errors = 0;
    errors++;

    if(errors <= 2) // print up to one extra recursive error
    {
        defvformatstring(msg,s,s);
        printf("%s\n", msg);

        if(errors <= 1) // avoid recursion
        {
            if(SDL_WasInit(SDL_INIT_VIDEO))
            {
                showcursor(true);
                SDL_WM_GrabInput(SDL_GRAB_OFF);
                SDL_SetGamma(1, 1, 1);
            }
            #ifdef WIN32
                MessageBox(NULL, msg, "Blood Frontier: Error", MB_OK|MB_SYSTEMMODAL);
            #endif
            SDL_Quit();
        }
    }

    exit(EXIT_FAILURE);
}

int initing = NOT_INITING;
static bool restoredinits = false;

bool initwarning(const char *desc, int level, int type)
{
    if(initing < level)
    {
        addchange(desc, type);
        return true;
    }
    return false;
}

#define SCR_MINW 320
#define SCR_MINH 200
#define SCR_MAXW 10000
#define SCR_MAXH 10000
#define SCR_DEFAULTW 1024
#define SCR_DEFAULTH 768
VARF(scr_w, SCR_MINW, -1, SCR_MAXW, initwarning("screen resolution"));
VARF(scr_h, SCR_MINH, -1, SCR_MAXH, initwarning("screen resolution"));
VARF(colorbits, 0, 0, 32, initwarning("color depth"));
VARF(depthbits, 0, 0, 32, initwarning("depth-buffer precision"));
VARF(stencilbits, 0, 0, 32, initwarning("stencil-buffer precision"));
VARF(fsaa, -1, -1, 16, initwarning("anti-aliasing"));
int actualvsync = -1, lastoutofloop = 0;
VARF(vsync, -1, -1, 1, initwarning("vertical sync"));

void writeinitcfg()
{
    if(!restoredinits) return;
	stream *f = openfile("init.cfg", "w");
	if(!f) return;
	f->printf("// automatically written on exit, DO NOT MODIFY\n// modify settings in game\n");
    extern int fullscreen;
    f->printf("fullscreen %d\n", fullscreen);
	f->printf("scr_w %d\n", scr_w);
	f->printf("scr_h %d\n", scr_h);
	f->printf("colorbits %d\n", colorbits);
	f->printf("depthbits %d\n", depthbits);
	f->printf("stencilbits %d\n", stencilbits);
	f->printf("fsaa %d\n", fsaa);
	f->printf("vsync %d\n", vsync);
	f->printf("shaders %d\n", useshaders);
	f->printf("shaderprecision %d\n", shaderprecision);
	f->printf("soundmono %d\n", soundmono);
	f->printf("soundchans %d\n", soundchans);
	f->printf("soundbufferlen %d\n", soundbufferlen);
	f->printf("soundfreq %d\n", soundfreq);
	f->printf("verbose %d\n", verbose);
    delete f;
}

VARP(compresslevel, 0, 9, 9);
VARP(imageformat, IFMT_NONE+1, IFMT_PNG, IFMT_MAX-1);

void screenshot(char *sname)
{
    ImageData image(screen->w, screen->h, 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, screen->w, screen->h, GL_RGB, GL_UNSIGNED_BYTE, image.data);
	defformatstring(fname)("%s", sname && *sname ? sname : mapname);
	saveimage(fname, image, imageformat, compresslevel, true, true);
}

COMMAND(screenshot, "s");
COMMAND(quit, "");

void setfullscreen(bool enable)
{
    if(!screen) return;
#if defined(WIN32) || defined(__APPLE__)
    initwarning(enable ? "fullscreen" : "windowed");
#else
    if(enable == !(screen->flags&SDL_FULLSCREEN))
    {
        SDL_WM_ToggleFullScreen(screen);
        inputgrab(grabinput!=0);
    }
#endif
}

VARF(fullscreen, 0, 1, 1, setfullscreen(fullscreen!=0));

void screenres(int *w, int *h)
{
#if !defined(WIN32) && !defined(__APPLE__)
    if(initing >= INIT_RESET)
	{
#endif
		scr_w = clamp(*w, SCR_MINW, SCR_MAXW);
		scr_h = clamp(*h, SCR_MINH, SCR_MAXH);
#if defined(WIN32) || defined(__APPLE__)
		initwarning("screen resolution");
#else
		return;
	}
    SDL_Surface *surf = SDL_SetVideoMode(clamp(*w, SCR_MINW, SCR_MAXW), clamp(*h, SCR_MINH, SCR_MAXH), 0, SDL_OPENGL|(screen->flags&SDL_FULLSCREEN ? SDL_FULLSCREEN : SDL_RESIZABLE));
	if(!surf) return;
	screen = surf;
	scr_w = screen->w;
	scr_h = screen->h;
	glViewport(0, 0, scr_w, scr_h);
#endif
}

COMMAND(screenres, "ii");

VARFP(gamma, 30, 100, 300,
{
	float f = gamma/100.0f;
	if(SDL_SetGamma(f,f,f)==-1)
	{
		conoutf("\frcould not set gamma (card/driver doesn't support it?)");
		conoutf("\frsdl: %s", SDL_GetError());
	}
});

void resetgamma()
{
    float f = gamma/100.0f;
    if(f==1) return;
    SDL_SetGamma(1, 1, 1);
    SDL_SetGamma(f, f, f);
}

static int moderatio(int w, int h)
{
    w *= 3*4*5;
    return w%h ? 0 : w/h;
}

static int moderatio(SDL_Rect *mode)
{
    return moderatio(mode->w, mode->h);
}

int desktopw = 0, desktoph = 0;

void setupscreen(int &usedcolorbits, int &useddepthbits, int &usedfsaa)
{
    int flags = SDL_RESIZABLE;
    #if defined(WIN32) || defined(__APPLE__)
    flags = 0;
    #endif
    if(fullscreen) flags = SDL_FULLSCREEN;
    SDL_Rect **modes = SDL_ListModes(NULL, SDL_OPENGL|flags);
    if(modes && modes!=(SDL_Rect **)-1)
    {
        int widest = -1, best = -1;
        for(int i = 0; modes[i]; i++)
        {
            if(widest < 0 || modes[i]->w > modes[widest]->w || (modes[i]->w == modes[widest]->w && modes[i]->h > modes[widest]->h))
                widest = i;
        }
        int ratio = desktopw > 0 && desktoph > 0 ? moderatio(desktopw, desktoph) : moderatio(modes[widest]);
        if((scr_w < 0 || scr_h < 0) && ratio > 0)
        {
            int w = scr_w, h = scr_h;
            if(w < 0 && h < 0) { w = SCR_DEFAULTW; h = SCR_DEFAULTH; }
            for(int i = 0; modes[i]; i++) if(moderatio(modes[i]) == ratio)
            {
                if(w <= modes[i]->w && h <= modes[i]->h && (best < 0 || modes[i]->w < modes[best]->w))
                    best = i;
            }
        }
        if(best < 0)
        {
            int w = scr_w, h = scr_h;
            if(w < 0 && h < 0) { w = SCR_DEFAULTW; h = SCR_DEFAULTH; }
            else if(w < 0) w = (h*SCR_DEFAULTW)/SCR_DEFAULTH;
            else if(h < 0) h = (w*SCR_DEFAULTH)/SCR_DEFAULTW;
            for(int i = 0; modes[i]; i++)
            {
                if(w <= modes[i]->w && h <= modes[i]->h && (best < 0 || modes[i]->w < modes[best]->w || (modes[i]->w == modes[best]->w && modes[i]->h < modes[best]->h)))
                    best = i;
            }
        }
        if(flags&SDL_FULLSCREEN)
        {
            if(best >= 0) { scr_w = modes[best]->w; scr_h = modes[best]->h; }
            else if(desktopw > 0 && desktoph > 0) { scr_w = desktopw; scr_h = desktoph; }
            else if(widest >= 0) { scr_w = modes[widest]->w; scr_h = modes[widest]->h; }
        }
        else if(best < 0)
        {
            scr_w = min(scr_w >= 0 ? scr_w : (scr_h >= 0 ? (scr_h*SCR_DEFAULTW)/SCR_DEFAULTH : SCR_DEFAULTW), (int)modes[widest]->w);
            scr_h = min(scr_h >= 0 ? scr_h : (scr_w >= 0 ? (scr_w*SCR_DEFAULTH)/SCR_DEFAULTW : SCR_DEFAULTH), (int)modes[widest]->h);
        }
    }
    if(scr_w < 0 && scr_h < 0) { scr_w = SCR_DEFAULTW; scr_h = SCR_DEFAULTH; }
    else if(scr_w < 0) scr_w = (scr_h*SCR_DEFAULTW)/SCR_DEFAULTH;
    else if(scr_h < 0) scr_h = (scr_w*SCR_DEFAULTH)/SCR_DEFAULTW;

    bool hasbpp = true;
    if(colorbits)
        hasbpp = SDL_VideoModeOK(scr_w, scr_h, colorbits, SDL_OPENGL|flags)==colorbits;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#if SDL_VERSION_ATLEAST(1, 2, 11)
    SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, vsync >= 0 ? vsync : 0);
#endif
    static int configs[] =
    {
        0x7, /* try everything */
        0x6, 0x5, 0x3, /* try disabling one at a time */
        0x4, 0x2, 0x1, /* try disabling two at a time */
        0 /* try disabling everything */
    };
    int config = 0;
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    if(!depthbits) SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    if(!fsaa)
    {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
    }
    loopi(sizeof(configs)/sizeof(configs[0]))
    {
        config = configs[i];
        if(!depthbits && config&1) continue;
        if(!stencilbits && config&2) continue;
        if(fsaa<=0 && config&4) continue;
        if(depthbits) SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, config&1 ? depthbits : 16);
        if(stencilbits)
        {
            hasstencil = config&2 ? stencilbits : 0;
            SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, hasstencil);
        }
        else hasstencil = 0;
        if(fsaa>0)
        {
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, config&4 ? 1 : 0);
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, config&4 ? fsaa : 0);
        }
        screen = SDL_SetVideoMode(scr_w, scr_h, hasbpp ? colorbits : 0, SDL_OPENGL|flags);
        if(screen) break;
    }
    if(!screen) fatal("Unable to create OpenGL screen: %s", SDL_GetError());
    else
    {
        if(!hasbpp) conoutf("\fr%d bit color buffer not supported - disabling", colorbits);
        if(depthbits && (config&1)==0) conoutf("\fr%d bit z-buffer not supported - disabling", depthbits);
        if(stencilbits && (config&2)==0) conoutf("\frStencil buffer not supported - disabling");
        if(fsaa>0 && (config&4)==0) conoutf("\fr%dx anti-aliasing not supported - disabling", fsaa);
#if SDL_VERSION_ATLEAST(1, 2, 11)
		SDL_GL_GetAttribute(SDL_GL_SWAP_CONTROL, &actualvsync); // could be forced on
		if(vsync != actualvsync) conoutf("\fractual value of vsync is: %s", actualvsync ? "enabled" : "disabled");
#endif
    }

    scr_w = screen->w;
    scr_h = screen->h;

    usedcolorbits = hasbpp ? colorbits : 0;
    useddepthbits = config&1 ? depthbits : 0;
    usedfsaa = config&4 ? fsaa : 0;
}

void resetgl()
{
    clearchanges(CHANGE_GFX);

    progress(0, "resetting OpenGL");

    extern void cleanupva();
    extern void cleanupparticles();
    extern void cleanupmodels();
    extern void cleanuptextures();
    extern void cleanuplightmaps();
    extern void cleanupblendmap();
    extern void cleanshadowmap();
    extern void cleanreflections();
    extern void cleanupglare();
    extern void cleanupdepthfx();
    extern void cleanupshaders();
    extern void cleanupgl();
    cleanupva();
    cleanupparticles();
    cleanupmodels();
    cleanuptextures();
    cleanuplightmaps();
    cleanupblendmap();
    cleanshadowmap();
    cleanreflections();
    cleanupglare();
    cleanupdepthfx();
    cleanupshaders();
    cleanupgl();

    SDL_SetVideoMode(0, 0, 0, 0);

    int usedcolorbits = 0, useddepthbits = 0, usedfsaa = 0;
    setupscreen(usedcolorbits, useddepthbits, usedfsaa);
    gl_init(scr_w, scr_h, usedcolorbits, useddepthbits, usedfsaa);

    extern void reloadfonts();
    extern void reloadtextures();
    extern void reloadshaders();

    inbetweenframes = false;
    if(!reloadtexture("textures/notexture") ||
		!reloadtexture("textures/blank") ||
		!reloadtexture("textures/logo") || !reloadtexture("textures/cube2badge") ||
		!reloadtexture("textures/progress"))
			fatal("failed to reload core textures");
    reloadfonts();
    inbetweenframes = true;
    progress(0, "initializing...");
    resetgamma();
    reloadshaders();
    reloadtextures();
    initlights();
    allchanged(true);
}

COMMAND(resetgl, "");

bool activewindow = true, warpmouse = false;
int ignoremouse = 0;

vector<SDL_Event> events;

void pushevent(const SDL_Event &e)
{
    events.add(e);
}

bool interceptkey(int sym)
{
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
        case SDL_KEYDOWN:
            if(event.key.keysym.sym == sym)
                return true;

        default:
            pushevent(event);
            break;
        }
    }
    return false;
}

void resetcursor(bool warp, bool reset)
{
	if(warp && grabinput)
	{
		SDL_WarpMouse(screen->w/2, screen->h/2);
		warpmouse = true;
	}
	if(reset) cursorx = cursory = 0.5f;
}

void checkinput()
{
	SDL_Event event;
	int lasttype = 0, lastbut = 0;
    while(events.length() || SDL_PollEvent(&event))
    {
        if(events.length()) event = events.remove(0);

		switch (event.type)
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
			{
				if(event.active.state & SDL_APPINPUTFOCUS)
				{
					activewindow = event.active.gain ? true : false;

					if(autograbinput)
						setvar("grabinput", event.active.gain ? 1 : 0, true);
				}
				break;
			}
			case SDL_MOUSEMOTION:
			{
				if(grabinput || activewindow)
				{
#ifdef __APPLE__
					if(event.motion.y == 0) break;  //let mac users drag windows via the title bar
#endif
					//conoutf("mouse: %d %d, %d %d [%s, %d]", event.motion.xrel, event.motion.yrel, event.motion.x, event.motion.y, warpmouse ? "true" : "false", ignoremouse);
					if(warpmouse && event.motion.x == screen->w/2 && event.motion.y == screen->h/2) warpmouse = false;
					else if(ignoremouse) ignoremouse--;
					else if(game::mousemove(event.motion.xrel, event.motion.yrel, event.motion.x, event.motion.y, screen->w, screen->h))
						resetcursor(true, false); // game controls engine cursor
				}
				break;
			}
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			{
				if(lasttype==event.type && lastbut==event.button.button) break; // why?? get event twice without it
				keypress(-event.button.button, event.button.state!=0, 0);
				lasttype = event.type;
				lastbut = event.button.button;
				break;
			}
		}
	}
}

void swapbuffers()
{
    recorder::capture();
    SDL_GL_SwapBuffers();
}

VARP(maxfps, 0, 200, 1000);

void limitfps(int &millis, int curmillis)
{
    if(!maxfps) return;
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
	formatstring(out)("Blood Frontier Win32 Exception: 0x%x [0x%x]\n\n", er->ExceptionCode, er->ExceptionCode==EXCEPTION_ACCESS_VIOLATION ? er->ExceptionInformation[1] : -1);
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
			formatstring(t)("%s - %s [%d]\n", si.sym.Name, del ? del + 1 : li.FileName, li.LineNumber);
			concatstring(out, t);
		}
	}
	fatal(out);
}
#endif

#define MAXFPSHISTORY 60

int fpspos = 0, fpshistory[MAXFPSHISTORY];

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

void getfps_(int *raw)
{
    int fps, bestdiff, worstdiff;
    if(*raw) fps = 1000/fpshistory[(fpspos+MAXFPSHISTORY-1)%MAXFPSHISTORY];
    else getfps(fps, bestdiff, worstdiff);
    intret(fps);
}

COMMANDN(getfps, getfps_, "i");

VAR(curfps, 1, 0, -1);
VAR(bestfps, 1, 0, -1);
VAR(bestfpsdiff, 1, 0, -1);
VAR(worstfps, 1, 0, -1);
VAR(worstfpsdiff, 1, 0, -1);

int lastautoadjust = 0;

VARFP(minfps, 0, 50, 100,				// aim for this fps or higher
	minfps = min(minfps, maxfps-1));

VARP(autoadjust, 0, 1, 1);				// auto performance adjust
VARP(autoadjustmin, 0, 1, 100);			// lowest level to go to
VARP(autoadjustmax, 0, 100, 100);		// highest level to go to
VARP(autoadjustrate, 0, 1000, 10000);	// only adjust this often
VARFP(autoadjustlimit, 0, 25, 100,		// going below this automatically scales to minimum
	autoadjustlimit = min(autoadjustlimit, minfps-1));

void autoadjustset(int level)
{
	enumerate(*idents, ident, i, {
		if(i.type == ID_VAR && (i.flags & IDF_AUTO))
		{
			int n = i.def.i-i.minval > 1 ? int((float(i.def.i-i.minval)/100.f)*float(level))+i.minval : (level ? i.def.i : i.minval);
			int m = clamp(n, i.minval, i.def.i);
			*i.storage.i = m;
			i.changed();
		}
	});
}

VARFP(autoadjustlevel, 0, 100, 100, autoadjustset(autoadjustlevel));

void autoadjustcheck(int frames)
{
	if(autoadjust && (!lastautoadjust || lastmillis-lastautoadjust > autoadjustrate))
	{
		if(lastautoadjust && frames > MAXFPSHISTORY)
		{
			if(worstfps < autoadjustlimit && autoadjustlevel > autoadjustmin)
				setvar("autoadjustlevel", autoadjustmin, true);
			else
			{
				float amt = float(worstfps)/float(minfps);
				if(amt < 1.f)
					setvar("autoadjustlevel", max(autoadjustlevel-int((1.f-amt)*10.f), autoadjustmin), true);
				else if(amt > 1.f)
					setvar("autoadjustlevel", min(autoadjustlevel+int(amt), autoadjustmax), true);
			}
		}
		lastautoadjust = lastmillis;
	}
}

void resetfps()
{
	loopi(MAXFPSHISTORY) fpshistory[i] = 1;
	fpspos = 0;
}

void updatefps(int frames, int millis)
{
	fpshistory[fpspos++] = max(1, min(1000, millis));
	if(fpspos >= MAXFPSHISTORY) fpspos = 0;

	int fps, bestdiff, worstdiff;
	getfps(fps, bestdiff, worstdiff);

	curfps = fps;
	bestfps = fps+bestdiff;
	bestfpsdiff = bestdiff;
	worstfps = fps-worstdiff;
	worstfpsdiff = worstdiff;

	autoadjustcheck(frames);
}

bool inbetweenframes = false, renderedframe = true;

static bool findarg(int argc, char **argv, const char *str)
{
	for(int i = 1; i<argc; i++) if(strstr(argv[i], str)==argv[i]) return true;
	return false;
}

static int clockrealbase = 0, clockvirtbase = 0;
static void clockreset() { clockrealbase = SDL_GetTicks(); clockvirtbase = totalmillis; }
VARFP(clockerror, 990000, 1000000, 1010000, clockreset());
VARFP(clockfix, 0, 0, 1, clockreset());

void rehash(bool reload)
{
	if(reload) writecfg();

	persistidents = false;

	execfile("defaults.cfg");

	persistidents = true;

    initing = INIT_LOAD;
	execfile("config.cfg", false);
	execfile("autoexec.cfg", false);
    initing = NOT_INITING;
}
ICOMMAND(rehash, "i", (int *nosave), rehash(*nosave ? false : true));

const char *loadbackinfo = "";
void eastereggs()
{
	time_t ct = time(NULL); // current time
	struct tm *lt = localtime(&ct);

	/*
	tm_sec		seconds after the minute (0-61)
	tm_min		minutes after the hour (0-59)
	tm_hour		hours since midnight (0-23)
	tm_mday		day of the month (1-31)
	tm_mon		months since January (0-11)
	tm_year		elapsed years since 1900
	tm_wday		days since Sunday (0-6)
	tm_yday		days since January 1st (0-365)
	tm_isdst	1 if daylight savings is on, zero if not,
	*/
	int month = lt->tm_mon+1, day = lt->tm_wday+1, mday = lt->tm_mday;
	if(day == 6 && mday == 13) loadbackinfo = "Friday the 13th";
	else if(month == 10 && mday == 31) loadbackinfo = "Halloween";
	if(month == 2 && mday == 6)		loadbackinfo = "Happy Birthday Ahven!";
	if(month == 2 && mday == 9)		loadbackinfo = "Happy Birthday Quin!";
	if(month == 4 && mday == 18)	loadbackinfo = "Happy Birthday Geartrooper!";
	if(month == 6 && mday == 9)		loadbackinfo = "Happy Birthday W!CK3D!";
	if(month == 7 && mday == 26)	loadbackinfo = "Happy Birthday Acord!";
	if(month == 7 && mday == 2)		loadbackinfo = "Happy Birthday c0rdawg and LedIris!";
	if(month == 9 && mday == 26)	loadbackinfo = "Happy Birthday Dazza!";
	if(month == 12 && mday == 8)	loadbackinfo = "Happy Birthday Hirato!";
}

bool progressing = false;

FVAR(loadprogress, 0, 0, 1);
SVAR(progresstitle, "");
SVAR(progresstext, "");
FVAR(progressamt, 0, 0, 1);
FVAR(progresspart, 0, 0, 1);

void progress(float bar1, const char *text1, float bar2, const char *text2)
{
	if(progressing || !inbetweenframes)// || ((actualvsync > 0 || verbose) && lastoutofloop && SDL_GetTicks()-lastoutofloop < 20))
		return;
	clientkeepalive();

    #ifdef __APPLE__
    interceptkey(SDLK_UNKNOWN); // keep the event queue awake to avoid 'beachball' cursor
    #endif

	setsvar("progresstitle", text1 ? text1 : "please wait...");
	setfvar("progressamt", bar1);
	setsvar("progresstext", text2 ? text2 : "");
	setfvar("progresspart", bar2);
	if(verbose >= 4)
	{
		if(text2) conoutf("\fm%s [%.2f%%], %s [%.2f%%]", text1, bar1*100.f, text2, bar2*100.f);
		else if(text1) conoutf("\fm%s [%.2f%%]", text1, bar1*100.f);
		else conoutf("\fmprogressing [%.2f%%]", text1, bar1*100.f, text2, bar2*100.f);
	}

	progressing = true;
	loopi(2) { drawnoview(); swapbuffers(); }
	progressing = false;

	lastoutofloop = SDL_GetTicks();
	autoadjustlevel = 100;
}

void updatetimer()
{
	int millis = SDL_GetTicks() - clockrealbase;
	if(clockfix) millis = int(millis*(double(clockerror)/1000000));
	millis += clockvirtbase;
	if(millis<totalmillis) millis = totalmillis;
	limitfps(millis, totalmillis);
	int elapsed = millis-totalmillis;
	curtime = elapsed;
	lastmillis += curtime;
	totalmillis = millis;
}

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

	char *initscript = NULL;
	initing = INIT_RESET;

	addpackagedir("data");

	for(int i = 1; i<argc; i++)
	{
		if(argv[i][0]=='-') switch(argv[i][1])
		{
			case 'r': execfile(argv[i][2] ? &argv[i][2] : "init.cfg", false); restoredinits = true; break;
			case 'd':
			{
				switch(argv[i][2])
				{
                    case 'w': scr_w = clamp(atoi(&argv[i][3]), SCR_MINW, SCR_MAXW); if(!findarg(argc, argv, "-dh")) scr_h = -1; break;
                    case 'h': scr_h = clamp(atoi(&argv[i][3]), SCR_MINH, SCR_MAXH); if(!findarg(argc, argv, "-dw")) scr_w = -1; break;
					case 'd': depthbits = atoi(&argv[i][3]); break;
					case 'c': colorbits = atoi(&argv[i][3]); break;
					case 'a': fsaa = atoi(&argv[i][3]); break;
					case 'v': vsync = atoi(&argv[i][3]); break;
					case 'f': fullscreen = atoi(&argv[i][3]); break;
					case 's': stencilbits = atoi(&argv[i][3]); break;
					case 'u':
					{
						extern int useshaders, shaderprecision;
						int n = atoi(&argv[i][3]);
						useshaders = n ? 1 : 0;
						shaderprecision = min(max(n - 1, 0), 3);
						break;
					}
					default: conoutf("\frunknown display option %c", argv[i][2]); break;
				}
				break;
			}
			case 'x': initscript = &argv[i][2]; break;
			default: if(!serveroption(argv[i])) gameargs.add(argv[i]); break;
		}
		else gameargs.add(argv[i]);
	}

	initing = NOT_INITING;

	conoutf("\fmloading enet..");
	if(enet_initialize()<0) fatal("Unable to initialise network module");
    atexit(enet_deinitialize);
    enet_time_set(0);

	conoutf("\fmloading game..");
	initgame();

	conoutf("\fmloading sdl..");
    int par = 0;
	#ifdef _DEBUG
	par = SDL_INIT_NOPARACHUTE;
	#ifdef WIN32
	SetEnvironmentVariable("SDL_DEBUG", "1");
	#endif
	#endif

	//#ifdef WIN32
	//SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	//#endif

	par |= SDL_INIT_TIMER|SDL_INIT_VIDEO|SDL_INIT_JOYSTICK;
	if(SDL_Init(par) < 0) fatal("Unable to initialize SDL: %s", SDL_GetError());
	ignoremouse += 3;

	conoutf("\fmloading video mode..");
    const SDL_VideoInfo *video = SDL_GetVideoInfo();
    if(video)
    {
        desktopw = video->current_w;
        desktoph = video->current_h;
    }
    int usedcolorbits = 0, useddepthbits = 0, usedfsaa = 0;
    setupscreen(usedcolorbits, useddepthbits, usedfsaa);

	conoutf("\fmloading video misc..");
	ncursor = SDL_GetCursor();
	showcursor(false);
	keyrepeat(false);
	setcaption("please wait...");
	eastereggs();

	conoutf("\fmloading gl..");
    gl_checkextensions();
    gl_init(scr_w, scr_h, usedcolorbits, useddepthbits, usedfsaa);
    if(!(notexture = textureload("textures/notexture")) ||
		!(blanktexture = textureload("textures/blank")))
		fatal("could not find core textures");

	conoutf("\fmloading sound..");
	initsound();

	conoutf("\fmloading defaults..");
	persistidents = false;
	if(!execfile("stdlib.cfg", false)) fatal("cannot find data files");
	if(!setfont("default")) fatal("no default font specified");
	inbetweenframes = true;
    progress(0, "please wait...");

    conoutf("\fmloading gl effects..");
    progress(0, "loading gl effects..");
    loadshaders();

	conoutf("\fmloading world..");
    progress(0, "loading world..");
	emptymap(0, true, NULL, true);

	conoutf("\fmloading config..");
    progress(0, "loading config..");
	rehash(false);
	smartmusic(true, false);

	conoutf("\fmloading required data..");
    progress(0, "loading required data..");
    preloadtextures();
	particleinit();
    initdecals();

	trytofindocta();
	conoutf("\fmloading main..");
    progress(0, "loading main..");
	if(initscript) execute(initscript);
	if(autograbinput) setvar("grabinput", 1, true);
    localconnect(false);
	resetfps();
	UI::setup();

	for(int frameloops = 0; ; frameloops = frameloops >= INT_MAX-1 ? MAXFPSHISTORY+1 : frameloops+1)
	{
		updatetimer();
		updatefps(frameloops, curtime);
		checkinput();
		menuprocess();

        if(frameloops)
        {
            RUNWORLD("on_update");
            game::updateworld();
        }

		checksleep(lastmillis);
		serverslice();
#ifdef IRC
		ircslice();
#endif
		if(frameloops)
		{
			game::recomputecamera(screen->w, screen->h);
			setviewcell(camera1->o);
			updatetextures();
			updateparticles();
			updatesounds();
			UI::update();
			inbetweenframes = renderedframe = false;
			if(frameloops > 2)
            {
                gl_drawframe(screen->w, screen->h);
                renderedframe = true;
            }
			swapbuffers();
			inbetweenframes = true;
			defformatstring(cap)("%s - %s", game::gametitle(), game::gametext());
			setcaption(cap);
		}
	}

	ASSERT(0);
	return EXIT_FAILURE;

#if defined(WIN32) && !defined(_DEBUG) && !defined(__GNUC__)
	} __except(stackdumper(0, GetExceptionInformation()), EXCEPTION_CONTINUE_SEARCH) { return 0; }
#endif
}
