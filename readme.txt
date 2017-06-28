Dream is an AM and DRM (Digital Radio Mondiale) software receiver and decoder. 
It is a very complete receiver, featuring compliant decoding of HE AAC v2 audio streams, journaline decoding, a feature rich statistics section providing invaluable information about the streams being received, and automatic update of available stations from the web, among several other features.

It also features a limited transmission mode, that uses the FAAC AAC encoder.

Note: AAC functionality requires additional libraries. See
 http://sourceforge.net/apps/mediawiki/drm/index.php?title=Instructions_for_building_the_AAC_decoder#Linux


Dream is multi-platform and has been tested on OSX, Win32 and Linux.

The source files are available here: http://drm.sourceforge.net

For more linux downloads please see the installation instructions at:

https://sourceforge.net/apps/mediawiki/drm/index.php?title=Main_Page

and the openSUSE Build Service at:

https://build.opensuse.org/project/show?project=home:juliancable


Changes since version 2.1:

This is a bug fix release.

New Features

Ticket
206 Allow patch level version numbering
205 Remove conflict between SDC signalling of Opus codec and new drm specification 4.1
202 Display name of currently playing input file in dream window title
199 Implement sdci processing
195 File Open now handles RSCI files
198 Enable decoding of Newstar DR111 recordings, add --permissive command line option


Bug Fixes

Ticket
207 Fix Data Decoding was broken
204 Save Transmitter settings on exit   
203 Fix build problem
201 Fixed low SNR/?MER 
200 Prevent tray icon staying after closing dream on windows xp
194 Fix File Framing format reader
193 clear DRM schedule when changing to AM mode
185 Fix crash on windows in release (/?O2) mode in audio codec
182 Fix crash with some input channel selections
180 Fix crash when using the --rsiin command and opening the evaluation dialogue
179 Fix crash in AM mode with the RSCI output on when faac is unavailable
163 Fix RSCI


Features Removed 

Ticket
186 Support for fftw2

