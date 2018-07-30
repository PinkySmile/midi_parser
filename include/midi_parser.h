#ifndef __LIB_MIDI_PARSER_HEADER_
#define __LIB_MIDI_PARSER_HEADER_

#include <stdbool.h>

typedef enum {
	TextEvent,
	NotePressed,
	NoteReleased,
} EventType;

typedef	struct {
	EventType	type;
	int		timeToAppear;
	void		*infos;
} Event;

typedef	struct EventList_s EventList;
struct EventList_s {
	Event		*data;
	EventList	*prev;
	EventList	*next;
};

typedef struct {
	unsigned short	format;
	short		tracks;
	char		fps;
	short		ticks;
	EventList	*events;
} MidiParser;

bool		parseMidiTrack(unsigned char *buffer, int buffLen, EventList *list, bool outputDebug);
MidiParser	*parseMidi(char *path, bool outputDebug);

#endif