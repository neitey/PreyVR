/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <SDL_main.h>

#include "idlib/precompiled.h"
#include "framework/Licensee.h"
#include "framework/FileSystem.h"
#include "sys/posix/posix_public.h"
#include "sys/sys_local.h"
#include "config.h"

#include <locale.h>

static char path_argv[PATH_MAX];

bool Sys_GetPath(sysPath_t type, idStr &path) {
	const char *s;
	char buf[MAX_OSPATH];
	char buf2[MAX_OSPATH];
	struct stat st;
	size_t len;

	path.Clear();

	switch(type) {
	case PATH_BASE:
		if (stat(BUILD_DATADIR, &st) != -1 && S_ISDIR(st.st_mode)) {
			path = BUILD_DATADIR;
			return true;
		}

		common->Warning("base path '" BUILD_DATADIR "' does not exist");

		// try next to the executable..
		if (Sys_GetPath(PATH_EXE, path)) {
			path = path.StripFilename();
			// the path should have a base dir in it, otherwise it probably just contains the executable
			idStr testPath = path + "/" BASE_GAMEDIR;
			if (stat(testPath.c_str(), &st) != -1 && S_ISDIR(st.st_mode)) {
				common->Warning("using path of executable: %s", path.c_str());
				return true;
			} else {
				idStr testPath = path + "/demo/demo00.pk4";
				if(stat(testPath.c_str(), &st) != -1 && S_ISREG(st.st_mode)) {
					common->Warning("using path of executable (seems to contain demo game data): %s", path.c_str());
					return true;
				} else {
					path.Clear();
				}
			}
		}

		// fallback to vanilla doom3 install
		if (stat(LINUX_DEFAULT_PATH, &st) != -1 && S_ISDIR(st.st_mode)) {
			common->Warning("using hardcoded default base path: " LINUX_DEFAULT_PATH);

			path = LINUX_DEFAULT_PATH;
			return true;
		}

		return false;

	case PATH_CONFIG:
#ifdef __ANDROID__
		s = getenv("USER_FILES");
		idStr::snPrintf(buf, sizeof(buf), "%s/config", s);
#else
		s = getenv("XDG_CONFIG_HOME");
		if (s)
			idStr::snPrintf(buf, sizeof(buf), "%s/dhewm3", s);
		else
			idStr::snPrintf(buf, sizeof(buf), "%s/.config/dhewm3", getenv("HOME"));
#endif
		path = buf;
		return true;

	case PATH_SAVE:
#ifdef __ANDROID__
		s = getenv("USER_FILES");
		idStr::snPrintf(buf, sizeof(buf), "%s/saves", s);
#else
		s = getenv("XDG_DATA_HOME");
		if (s)
			idStr::snPrintf(buf, sizeof(buf), "%s/dhewm3", s);
		else
			idStr::snPrintf(buf, sizeof(buf), "%s/.local/share/dhewm3", getenv("HOME"));
#endif
		path = buf;
		return true;

	case PATH_EXE:
		idStr::snPrintf(buf, sizeof(buf), "/proc/%d/exe", getpid());
		len = readlink(buf, buf2, sizeof(buf2));
		if (len != -1) {
			if (len < MAX_OSPATH) {
				buf2[len] = '\0';
			} else {
				buf2[MAX_OSPATH - 1] = '\0';
			}
			path = buf2;
			return true;
		}

		if (path_argv[0] != 0) {
			path = path_argv;
			return true;
		}

		return false;
	}

	return false;
}

/*
===============
Sys_Shutdown
===============
*/
void Sys_Shutdown( void ) {
	Posix_Shutdown();
}


/*
===============
Sys_GetClockticks
===============
*/
double Sys_GetClockTicks(void)
{
#if defined( __i386__ )
	unsigned long lo, hi;

	__asm__ __volatile__(
	        "push %%ebx\n"			\
	        "xor %%eax,%%eax\n"		\
	        "cpuid\n"					\
	        "rdtsc\n"					\
	        "mov %%eax,%0\n"			\
	        "mov %%edx,%1\n"			\
	        "pop %%ebx\n"
	        : "=r"(lo), "=r"(hi));
	return (double) lo + (double) 0xFFFFFFFF * hi;
#else
#warning unsupported CPU
	return 0;
#endif
}

/*
===============
MeasureClockTicks
===============
*/
double MeasureClockTicks(void)
{
	double t0, t1;

	t0 = Sys_GetClockTicks();
	Sys_Sleep(1000);
	t1 = Sys_GetClockTicks();
	return t1 - t0;
}

/*
===============
Sys_ClockTicksPerSecond
===============
*/
double Sys_ClockTicksPerSecond(void)
{
	static bool		init = false;
	static double	ret;

	int		fd, len, pos, end;
	char	buf[ 4096 ];

	if (init) {
		return ret;
	}

#if defined(__ANDROID__)
	fd = open("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", O_RDONLY);
#else
	fd = open("/proc/cpuinfo", O_RDONLY);
#endif

	if (fd == -1) {
		//common->Printf("couldn't read /proc/cpuinfo\n");
		ret = MeasureClockTicks();
		init = true;
		//common->Printf("measured CPU frequency: %g MHz\n", ret / 1000000.0);
		return ret;
	}

	len = read(fd, buf, 4096);
	close(fd);

#if defined(__ANDROID__)
	if (len > 0) {
		ret = atof(buf);
		common->Printf("/proc/cpuinfo CPU frequency: %g MHz", ret / 1000.0);
		ret *= 1000;
		init = true;
		return ret;
	}
#else
	pos = 0;

	while (pos < len) {
		if (!idStr::Cmpn(buf + pos, "cpu MHz", 7)) {
			pos = strchr(buf + pos, ':') - buf + 2;
			end = strchr(buf + pos, '\n') - buf;

			if (pos < len && end < len) {
				buf[end] = '\0';
				ret = atof(buf + pos);
			} else {
				common->Printf("failed parsing /proc/cpuinfo\n");
				ret = MeasureClockTicks();
				init = true;
				common->Printf("measured CPU frequency: %g MHz\n", ret / 1000000.0);
				return ret;
			}

			common->Printf("/proc/cpuinfo CPU frequency: %g MHz\n", ret);
			ret *= 1000000;
			init = true;
			return ret;
		}

		pos = strchr(buf + pos, '\n') - buf + 1;
	}
#endif

	common->Printf("failed parsing /proc/cpuinfo\n");
	ret = MeasureClockTicks();
	init = true;
	common->Printf("measured CPU frequency: %g MHz\n", ret / 1000000.0);
	return ret;
}

/*
================
Sys_GetSystemRam
returns in megabytes
================
*/
int Sys_GetSystemRam( void ) {
	long	count, page_size;
	int		mb;

	count = sysconf( _SC_PHYS_PAGES );
	if ( count == -1 ) {
		common->Printf( "GetSystemRam: sysconf _SC_PHYS_PAGES failed\n" );
		return 512;
	}
	page_size = sysconf( _SC_PAGE_SIZE );
	if ( page_size == -1 ) {
		common->Printf( "GetSystemRam: sysconf _SC_PAGE_SIZE failed\n" );
		return 512;
	}
	mb= (int)( (double)count * (double)page_size / ( 1024 * 1024 ) );
	// round to the nearest 16Mb
	mb = ( mb + 8 ) & ~15;
	return mb;
}

/*
==================
Sys_DoStartProcess
if we don't fork, this function never returns
the no-fork lets you keep the terminal when you're about to spawn an installer

if the command contains spaces, system() is used. Otherwise the more straightforward execl ( system() blows though )
==================
*/
void Sys_DoStartProcess( const char *exeName, bool dofork ) {
	bool use_system = false;
	if ( strchr( exeName, ' ' ) ) {
		use_system = true;
	} else {
		// set exec rights when it's about a single file to execute
		struct stat buf;
		if ( stat( exeName, &buf ) == -1 ) {
			printf( "stat %s failed: %s\n", exeName, strerror( errno ) );
		} else {
			if ( chmod( exeName, buf.st_mode | S_IXUSR ) == -1 ) {
				printf( "cmod +x %s failed: %s\n", exeName, strerror( errno ) );
			}
		}
	}
	if ( dofork ) {
		switch ( fork() ) {
		case -1:
			printf( "fork failed: %s\n", strerror( errno ) );
			break;
		case 0:
			if ( use_system ) {
				printf( "system %s\n", exeName );
				if (system( exeName ) == -1)
					printf( "system failed: %s\n", strerror( errno ) );
				_exit( 0 );
			} else {
				printf( "execl %s\n", exeName );
				execl( exeName, exeName, NULL );
				printf( "execl failed: %s\n", strerror( errno ) );
				_exit( -1 );
			}
			break;
		default:
			break;
		}
	} else {
		if ( use_system ) {
			printf( "system %s\n", exeName );
			if (system( exeName ) == -1)
				printf( "system failed: %s\n", strerror( errno ) );
			else
				sleep( 1 );	// on some systems I've seen that starting the new process and exiting this one should not be too close
		} else {
			printf( "execl %s\n", exeName );
			execl( exeName, exeName, NULL );
			printf( "execl failed: %s\n", strerror( errno ) );
		}
		// terminate
		_exit( 0 );
	}
}

/*
===============
Sys_GetProcessorString
===============
*/
const char *Sys_GetProcessorString(void)
{
	return "generic";
}


/*
==================
Sys_GetCallStack
==================
*/
void Sys_GetCallStack(address_t *callStack, const int callStackSize)
{
	for (int i = 0; i < callStackSize; i++) {
		callStack[i] = 0;
	}
}

/*
==================
Sys_GetCallStackStr
==================
*/
const char *Sys_GetCallStackStr(const address_t *callStack, const int callStackSize)
{
	return "";
}

/*
==================
Sys_GetCallStackStr
==================
*/
const char *Sys_GetCallStackCurStr(int depth)
{
	return "";
}

/*
==================
Sys_GetCallStackCurAddressStr
==================
*/
const char 	*Sys_GetCallStackCurAddressStr(int depth)
{
	return "";
}

/*
==================
Sys_ShutdownSymbols
==================
*/
void Sys_ShutdownSymbols(void)
{
}

/*
===============
Sys_FPU_EnableExceptions
===============
*/
void Sys_FPU_EnableExceptions(int exceptions)
{
}

/*
=================
Sys_OpenURL
=================
*/
void idSysLocal::OpenURL( const char *url, bool quit ) {
	const char	*script_path;
	idFile		*script_file;
	char		cmdline[ 1024 ];

	static bool	quit_spamguard = false;

	if ( quit_spamguard ) {
		common->DPrintf( "Sys_OpenURL: already in a doexit sequence, ignoring %s\n", url );
		return;
	}

	common->Printf( "Open URL: %s\n", url );
	// opening an URL on *nix can mean a lot of things ..
	// just spawn a script instead of deciding for the user :-)

	// look in the savepath first, then in the basepath
	script_path = fileSystem->BuildOSPath( cvarSystem->GetCVarString( "fs_savepath" ), "", "openurl.sh" );
	script_file = fileSystem->OpenExplicitFileRead( script_path );
	if ( !script_file ) {
		script_path = fileSystem->BuildOSPath( cvarSystem->GetCVarString( "fs_basepath" ), "", "openurl.sh" );
		script_file = fileSystem->OpenExplicitFileRead( script_path );
	}
	if ( !script_file ) {
		common->Printf( "Can't find URL script 'openurl.sh' in either savepath or basepath\n" );
		common->Printf( "OpenURL '%s' failed\n", url );
		return;
	}
	fileSystem->CloseFile( script_file );

	// if we are going to quit, only accept a single URL before quitting and spawning the script
	if ( quit ) {
		quit_spamguard = true;
	}

	common->Printf( "URL script: %s\n", script_path );

	// StartProcess is going to execute a system() call with that - hence the &
	idStr::snPrintf( cmdline, 1024, "%s '%s' &",  script_path, url );
	sys->StartProcess( cmdline, quit );
}

/*
===============
main
===============
*/

#ifdef __ANDROID__

bool running = true;
int main_android(int argc, const char **argv) {
#else
int main(int argc, char **argv) {
#endif
	// fallback path to the binary for systems without /proc
	// while not 100% reliable, its good enough
	if (argc > 0) {
		if (!realpath(argv[0], path_argv))
			path_argv[0] = 0;
	} else {
		path_argv[0] = 0;
	}

	// some ladspa-plugins (that may be indirectly loaded by doom3 if they're
	// used by alsa) call setlocale(LC_ALL, ""); This sets LC_ALL to $LANG or
	// $LC_ALL which usually is not "C" and will fuck up scanf, strtod
	// etc when using a locale that uses ',' as a float radix.
	// so set $LC_ALL to "C".
	setenv("LC_ALL", "C", 1);

#ifndef __ANDROID__
	Posix_InitSignalHandlers();
#endif
	if ( argc > 1 ) {
		common->Init( argc-1, &argv[1], NULL );
	} else {
		common->Init( 0, NULL, NULL );
	}

	while (running) {
		common->Frame();
	}

	return 0;
}

// APK's native library path on Android.
char *native_library_dir = NULL;
extern "C" void VR_Doom3Main(int argc, const char** argv)
{
	native_library_dir = (char*)getenv("GAMELIBDIR");;
	main_android(argc, argv);
}

const char *workdir()
{
	static char wd[256];
	getcwd(wd,256);
	return wd;
}

const char *Sys_DefaultBasePath(void) { return workdir(); }
const char *Sys_DefaultSavePath(void) { return workdir(); }
const char *Sys_DefaultCDPath(void) { return ""; }

const char *Sys_EXEPath(void) {
	static char	buf[ 1024 ];
	idStr		linkpath;
	int			len;

	buf[ 0 ] = '\0';
	sprintf(linkpath, "/proc/%d/exe", getpid());
	len = readlink(linkpath.c_str(), buf, sizeof(buf));

	if (len == -1) {
		Sys_Printf("couldn't stat exe path link %s\n", linkpath.c_str());
		buf[ len ] = '\0';
	}

	return buf;
}

float analogx=0.0f;
float analogy=0.0f;
int analogenabled=0;

void Sys_DoPreferences(void) { }

void pull_input_event(int execCmd){}

