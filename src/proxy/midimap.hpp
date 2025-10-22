#ifndef UNHIDENPCS_MIDIMAP_HPP
#define UNHIDENPCS_MIDIMAP_HPP
#pragma once

// midimap.dll
#pragma comment(linker, "/export:DriverProc=C:\\Windows\\System32\\midimap.DriverProc")
#pragma comment(linker, "/export:modMessage=C:\\Windows\\System32\\midimap.modMessage")
#pragma comment(linker, "/export:modmCallback=C:\\Windows\\System32\\midimap.modmCallback")

#endif //UNHIDENPCS_MIDIMAP_HPP