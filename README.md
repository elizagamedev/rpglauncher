RPGLauncher
===========

Builds of RPGLauncher can be downloaded from
[here](https://drive.google.com/folderview?id=0B58Pb3yVNt_3aWlLNDBXX3p0Unc&usp=sharing).

What is RPGLauncher?
--------------------

RPGLauncher is an easy way to run international (primarily Japanese) RPG Maker
games without the use of software such as AppLocale. It has other advanced
features, such as allowing you to choose where to store the game's save files,
where the location of the RTP is, among others.

Quick Start Guide
-----------------

You must have East-Asian language support installed to run Japanese games. Some
versions of Windows require this, some do not. This is the first step.

The easiest way to use RPGLauncher is to copy RPGLauncher.exe into the same
folder as the game's RPG_RT.exe and run it. This should work 99% of the time
for English-language and Japanese-language games.

Note that the new version doesn't require you to have RPGLauncher.dll, so you
can delete that if you were using an older version.

*If you have problems*, check out the troubleshooting section before asking.
If you still have issues, submit a new issue on the tracker or contact me
through my [Tumblr account](http://mathewvq.tumblr.com/).

Advanced Features
-----------------

RPGLauncher can read several command-line arguments to modify its behavior:

* `--fullscreen`
    * Start the game in fullscreen
* `--show-logo`
    * Show the game logo on startup
* `--hide-title`
    * Hide the title screen image
* `--save-path`
    * Change the folder in which the game saves games
* `--rtp-path`
    * Change the folder which the game looks for RTP files
* `--codepage`
    * Change the codepage of the game (defaults to 932, i.e. Japanese)

You may also add options to RPG_RT.ini that RPGLauncher will read when starting,
like so:

    [RPGLauncher]
    logo=0
    title=1
    fullscreen=1
    codepage=1252

This will start the game without the logo, with the title screen, in fullscreen
mode, and with codepage 1252 (Western).

Troubleshooting
---------------

### A file can't load/isn't found.
1. Make sure the RTP is installed, *especially* if you get a warning about the
   RTP not being installed.
2. The filenames might be corrupt, especially for a Japanese-language game,
   though RPGLauncher should mostly compensate for these. In any case...
    * Make sure that you have East-Asian language support installed.
    * Try redownloading the program, extracting the files with a quality file
      archiver such as 7-Zip.
    * If both steps fail, or the game comes in a self-extracting archive,
      you'll need to extract the game files while running in a Japanese locale.
      You can do this by using
      [AppLocale](https://www.microsoft.com/en-us/download/details.aspx?id=13209)
      on versions of Windows less than 10,
      [Locale Emulator](https://github.com/xupefei/Locale-Emulator)
      on Windows 10, or changing your system locale settings. The 64-bit version
      of 7-Zip does not seem to work properly with AppLocale in my experience.

### The text is corrupt.
East-Asian language support must be installed. This should only affect
Windows XP.

### The game isn't fullscreen.
You can enter fullscreen at any time by pressing `Alt+Enter`. Alternatively, see
the above instructions under Advanced Features.
