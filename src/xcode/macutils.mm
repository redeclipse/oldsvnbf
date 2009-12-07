#import <Cocoa/Cocoa.h>

// -- copied from tools.h -- including the full file introduces too many problems
#define MAXSTRLEN 512
typedef char string[MAXSTRLEN];
inline char *copystring(char *d, const char *s, size_t len = MAXSTRLEN) { strncpy(d, s, len); d[len-1] = 0; return d; }
inline char *concatstring(char *d, const char *s) { size_t len = strlen(d); return copystring(d+len, s, MAXSTRLEN-len); }
// --

void mac_pasteconsole(char *commandbuf)
{	
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSString *type = [pasteboard availableTypeFromArray:[NSArray arrayWithObject:NSStringPboardType]];
    if (type != nil) {
        NSString *contents = [pasteboard stringForType:type];
        if (contents != nil)
			concatstring(commandbuf, [contents lossyCString]);
    }
}

/*
 * 0x1030 = 10.3
 * 0x1040 = 10.4
 * 0x1050 = 10.5
 */
int mac_osversion() 
{
    SInt32 MacVersion;
    Gestalt(gestaltSystemVersion, &MacVersion);
    return MacVersion;
}

const char *mac_personaldir() {
    static string dir;
    NSString *path = nil;
    FSRef folder;
    if (FSFindFolder(kUserDomain, kApplicationSupportFolderType, NO, &folder) == noErr) 
    {
        CFURLRef url = CFURLCreateFromFSRef(kCFAllocatorDefault, &folder);
        path = [(NSURL *)url path];
        CFRelease(url);
    }
    return path ? copystring(dir, [path fileSystemRepresentation]) : NULL;
}

const char *mac_sauerbratendir() {
    static string dir;
    NSString *path = [[NSWorkspace sharedWorkspace] fullPathForApplication:@"sauerbraten"];
    if(path) path = [path stringByAppendingPathComponent:@"Contents/gamedata"];
    return path ? copystring(dir, [path fileSystemRepresentation]) : NULL;
}
