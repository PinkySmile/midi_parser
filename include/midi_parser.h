#ifndef __LIB_MIDI_PARSER_HEADER_
#define __LIB_MIDI_PARSER_HEADER_

#include <stdbool.h>

typedef enum {
	MidiSequenceNumber,
	MidiTextEvent,
	MidiNewLyric,
	MidiNewMarker,
	MidiNewCuePoint,
	MidiNewChannelPrefix,
	MidiPortChange,
	MidiTempoChanged,
	MidiSMTPEOffset,
	MidiNewTimeSignature,
	MidiNewKeySignature,
	MidiSequencerSpecificEvent,
	MidiNoteReleased,
	MidiNotePressed,
	MidiPolyphonicPressure,
	MidiControllerValueChanged,
	MidiProgramChanged,
	MidiPressureOfChannelChanged,
	MidiPitchBendChanged,
} EventType;

typedef	struct {
	EventType	type;
	int		timeToAppear;
	void		*infos;
} Event;

typedef	struct {
	unsigned char	numerator;
	unsigned char	denominator;
	unsigned char	clockTicksPerMetTick;
	unsigned char	ticksPerQuarterNote;
} MidiTimeSignature;

typedef struct {
	unsigned char	channel;
	unsigned char	pitch;
	unsigned char	velocity;
} MidiNote;

typedef struct {
	char		*copyright;
	char		*name;
	char		*instrumentName;
	int		nbOfEvents;
	Event		*events;
} Track;

typedef struct {
	unsigned short	format;
	short		nbOfTracks;
	char		fps;
	short		ticks;
	int		nbOfNotes;
	Track		*tracks;
} MidiParser;

bool		parseMidiTrack(unsigned char *buffer, int buffLen, Track *track, bool outputDebug, MidiParser *result, int posInFile);
MidiParser	*parseMidi(char *path, bool outputDebug);
char		*getNoteString(char note);
void		deleteTrack(Track *track);
void		deleteMidiParserStruct(MidiParser *result);

#endif