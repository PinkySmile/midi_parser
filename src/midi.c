#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "midi_parser.h"

char	*getNoteString(char note)
{
	static char	buffer[5];
	int		nbr;
	
	memset(buffer, 0, sizeof(buffer));
	switch (note % 12) {
	case 0:
		buffer[0] = 'C';
		nbr = 1;
		break;
	case 1:
		buffer[0] = 'C';
		buffer[1] = '#';
		nbr = 2;
		break;
	case 2:
		buffer[0] = 'D';
		nbr = 1;
		break;
	case 3:
		buffer[0] = 'D';
		buffer[1] = '#';
		nbr = 2;
		break;
	case 4:
		buffer[0] = 'E';
		nbr = 1;
		break;
	case 5:
		buffer[0] = 'F';
		nbr = 1;
		break;
	case 6:
		buffer[0] = 'F';
		buffer[1] = '#';
		nbr = 2;
		break;
	case 7:
		buffer[0] = 'G';
		nbr = 1;
		break;
	case 8:
		buffer[0] = 'G';
		buffer[1] = '#';
		nbr = 2;
		break;
	case 9:
		buffer[0] = 'A';
		nbr = 1;
		break;
	case 10:
		buffer[0] = 'A';
		buffer[1] = '#';
		nbr = 2;
		break;
	case 11:
		buffer[0] = 'B';
		nbr = 1;
		break;
	}
	sprintf(buffer + nbr, "%i", note / 12 - 1);
	return (buffer);
}

bool	addMidiEvent(EventList *list, EventType type, int timeToAppear, void *infos)
{
	for (; list->next; list = list->next);
	if (list->data) {
		list->next = malloc(sizeof(*list->next));
		if (!list->next) {
			printf("Error: Cannot alloc %iB\n", (int)sizeof(*list->next));
			return (false);
		}
		list->next->prev = list;
		list->next->next = NULL;
		list = list->next;
	}
	list->data = malloc(sizeof(*list->data));
	if (!list->data) {
		printf("Error: Cannot alloc %iB\n", (int)sizeof(*list->data));
		return (false);
	}
	list->data->timeToAppear = timeToAppear;
	list->data->infos = infos;
	list->data->type = type;
	return (true);
}

void	deleteEventList(EventList *list)
{
	for (; list->next; list = list->next);
	for (; list; list = list->prev) {
		free(list->data->infos);
		free(list->data);
		free(list->next);
	}
}

void	deleteTrack(Track *track)
{
	free(track->copyright);
	free(track->name);
	free(track->instrumentName);
	deleteEventList(&track->events);
}

void	deleteMidiParserStruct(MidiParser *result)
{
	for (int i = 0; i < result->nbOfTracks; i++)
		deleteTrack(&result->tracks[i]);
	free(result->tracks);
}

void	showChunk(unsigned char *buffer, int pos, int len, int posInFile)
{
	int	realPos = pos;

	posInFile = pos > 15 ? posInFile - 15 : posInFile - pos;
	pos = pos > 15 ? pos - 15 : 0;
	for (int j = 0; j < 10 - (posInFile % 10); j++)
		printf("     ");
	for (int i = 10 - posInFile % 10; i < 40 && pos + i < len; i += 10) {
		for (int j = printf("%i", posInFile + i); j < 50; j++)
			printf(" ");
	}
	printf("\n");
	for (int i = 0; i < 40 && pos + i < len; i++)
		printf(i + pos == realPos ? "  V  " : ((i + posInFile) % 10 ? "  '  " : "  |  "));
	printf("\n");
	for (int i = 0; i < 40 && pos + i < len; i++)
		for (int j = printf(buffer[i + pos] ? " %#x" : " 0x0", buffer[i + pos]); j < 5; j++)
			printf(" ");
	printf("\n");
}

bool	parseMidiTrack(unsigned char *buffer, int buffLen, Track *track, bool outputDebug, MidiParser *result, int posInFile)
{
	void			*buff;
	unsigned char		statusByte;
	unsigned long int	deltaTime = 0;
	unsigned long int	totalTime = 0;
	unsigned long int	len;
	EventList		*list = &track->events;

	if (outputDebug) {
		for (int i = 0; i < 30 && i < buffLen; printf("%#x ", buffer[i++]));
		printf("\n");
	}
	for (int i = 0; i < buffLen; ) {
		for (deltaTime = buffer[i] & 0x7F; buffer[i++] & 0x80; deltaTime = (deltaTime << 7) + (buffer[i] & 0x7F));
		statusByte = buffer[i++];
		if (outputDebug)printf("After % 8li ticks: ", deltaTime);
		if (statusByte == 0xFF) {
			switch (buffer[i++]) {
			case 0x00:
				if (buffer[i++] != 0x02) {
					printf("Error: Invalid byte found (%#x found but expected 0x02)\n", buffer[i - 1]);
					if (outputDebug)showChunk(buffer, i, buffLen, posInFile + i);
					return (false);
				} else if (totalTime > 0) {
					printf("Error: Cannot add sequence number after non-zero delta times\n");
					if (outputDebug)showChunk(buffer, i, buffLen, posInFile + i);
					return (false);
				}
				buff = malloc(sizeof(int));
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)sizeof(int));
					return (false);
				}
				if (outputDebug)printf("Found sequence number: %i", (buffer[i] << 8) + buffer[i+2]);
				*(int *)buff = (buffer[i] << 8) + buffer[i+2];
				if (!addMidiEvent(list, MidiSequenceNumber, deltaTime, buff))
					return (false);
				i+=2;
				break;
			case 0x01:
				for (len = buffer[i] & 0x7F; buffer[i++] & 0x80; len = (len << 7) + (buffer[i] & 0x7F));
				buff = malloc(len + 1);
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)len + 1);
					return (false);
				}
				if (outputDebug) {
					printf("Text event: '");
					fflush(stdout);
					write(1, &buffer[i], len);
					printf("'");
				}
				strncpy(buff, (char *)&buffer[i], len);
				((char *)buff)[len] = 0;
				if (!addMidiEvent(list, MidiTextEvent, deltaTime, buff))
					return (false);
				i += len;
				break;
			case 0x02:
				for (len = buffer[i] & 0x7F; buffer[i++] & 0x80; len = (len << 7) + (buffer[i] & 0x7F));
				buff = malloc(len + 1);
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)len + 1);
					return (false);
				}
				if (outputDebug) {
					printf("Copyright to : '");
					fflush(stdout);
					write(1, &buffer[i], len);
					printf("'");
				}
				strncpy(buff, (char *)&buffer[i], len);
				((char *)buff)[len] = 0;
				free(track->copyright);
				track->copyright = buff;
				i += len;
				break;
			case 0x03:
				for (len = buffer[i] & 0x7F; buffer[i++] & 0x80; len = (len << 7) + (buffer[i] & 0x7F));
				buff = malloc(len + 1);
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)len + 1);
					return (false);
				}
				if (outputDebug) {
					printf("Track name: '");
					fflush(stdout);
					write(1, &buffer[i], len);
					printf("'");
				}
				strncpy(buff, (char *)&buffer[i], len);
				((char *)buff)[len] = 0;
				free(track->name);
				track->name = buff;
				i += len;
				break;
			case 0x04:
				for (len = buffer[i] & 0x7F; buffer[i++] & 0x80; len = (len << 7) + (buffer[i] & 0x7F));
				buff = malloc(len + 1);
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)len + 1);
					return (false);
				}
				if (outputDebug) {
					printf("Instrument name: '");
					fflush(stdout);
					write(1, &buffer[i], len);
					printf("'");
				}
				strncpy(buff, (char *)&buffer[i], len);
				((char *)buff)[len] = 0;
				free(track->instrumentName);
				track->instrumentName = buff;
				i += len;
				break;
			case 0x05:
				for (len = buffer[i] & 0x7F; buffer[i++] & 0x80; len = (len << 7) + (buffer[i] & 0x7F));
				buff = malloc(len + 1);
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)len + 1);
					return (false);
				}
				if (outputDebug) {
					printf("Lyric: '");
					fflush(stdout);
					write(1, &buffer[i], len);
					printf("'");
				}
				strncpy(buff, (char *)&buffer[i], len);
				((char *)buff)[len] = 0;
				if (!addMidiEvent(list, MidiNewLyric, deltaTime, buff))
					return (false);
				i += len;
				break;
			case 0x06:
				for (len = buffer[i] & 0x7F; buffer[i++] & 0x80; len = (len << 7) + (buffer[i] & 0x7F));
				buff = malloc(len + 1);
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)len + 1);
					return (false);
				}
				if (outputDebug) {
					printf("Marker: '");
					fflush(stdout);
					write(1, &buffer[i], len);
					printf("'");
				}
				strncpy(buff, (char *)&buffer[i], len);
				((char *)buff)[len] = 0;
				if (!addMidiEvent(list, MidiNewMarker, deltaTime, buff))
					return (false);
				i += len;
				break;
			case 0x07:
				for (len = buffer[i] & 0x7F; buffer[i++] & 0x80; len = (len << 7) + (buffer[i] & 0x7F));
				buff = malloc(len + 1);
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)len + 1);
					return (false);
				}
				if (outputDebug) {
					printf("Cue Point: '");
					fflush(stdout);
					write(1, &buffer[i], len);
					printf("'");
				}
				strncpy(buff, (char *)&buffer[i], len);
				((char *)buff)[len] = 0;
				if (!addMidiEvent(list, MidiNewCuePoint, deltaTime, buff))
					return (false);
				i += len;
				break;
			case 0x20:
				if (buffer[i++] != 0x01) {
					printf("Error: Invalid byte found (%#x found but expected 0x01)\n", buffer[i - 1]);
					if (outputDebug)showChunk(buffer, i, buffLen, posInFile + i);
					return (false);
				}
				buff = malloc(sizeof(int));
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)sizeof(int));
					return (false);
				}
				if (outputDebug)printf("New channel prefix: %i", buffer[i]);
				*(int *)buff = buffer[i];
				if (!addMidiEvent(list, MidiNewChannelPrefix, deltaTime, buff))
					return (false);
				i++;
				break;
			case 0x21:
				if (buffer[i++] != 0x01) {
					printf("Error: Invalid byte found (%#x found but expected 0x01)\n", buffer[i - 1]);
					if (outputDebug)showChunk(buffer, i, buffLen, posInFile + i);
					return (false);
				}
				buff = malloc(sizeof(int));
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)sizeof(int));
					return (false);
				}
				if (outputDebug)printf("New MIDI port: %i", buffer[i]);
				*(int *)buff = buffer[i];
				if (!addMidiEvent(list, MidiPortChange, deltaTime, buff))
					return (false);
				i++;
				break;
			case 0X2F:
				if (buffer[i++] != 0x00) {
					printf("Error: Invalid byte found (%#x found but expected 0x00)\n", buffer[i - 1]);
					if (outputDebug)showChunk(buffer, i, buffLen, posInFile + i);
					return (false);
				} else if (i < buffLen) {
					printf("Error: Found end of track before the last index\n");
					if (outputDebug)showChunk(buffer, i, buffLen, posInFile + i);
					return (false);
				}
				if (outputDebug)printf("End of track !\n");
				return (true);
			case 0x51:
				if (buffer[i++] != 0x03) {
					printf("Error: Invalid byte found (%#x found but expected 0x00)\n", buffer[i - 1]);
					return (false);
				}
				buff = malloc(sizeof(int));
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)sizeof(int));
					return (false);
				}
				if (outputDebug)printf("New tempo: %i", (buffer[i] << 16) + (buffer[i+1] << 8) + buffer[i+2]);
				*(int *)buff = (buffer[i] << 16) + (buffer[i+1] << 8) + buffer[i+2];
				if (!addMidiEvent(list, MidiTempoChanged, deltaTime, buff))
					return (false);
				i+=3;
				break;
			case 0x54:
				if (buffer[i++] != 0x05) {
					printf("Error: Invalid byte found (%#x found but expected 0x05)\n", buffer[i - 1]);
					if (outputDebug)showChunk(buffer, i, buffLen, posInFile + i);
					return (false);
				} else if (totalTime > 0) {
					printf("Error: Cannot add SMTPE Offset after non-zero delta times\n");
					if (outputDebug)showChunk(buffer, i, buffLen, posInFile + i);
					return (false);
				}
				buff = malloc(sizeof(char) * 5);
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)sizeof(char) * 5);
					return (false);
				}
				if (outputDebug)printf("New offset: %ih %im %is %iframes %ihundreths of a frame", buffer[i], buffer[i+1], buffer[i+2], buffer[i+3], buffer[i+4]);
				((char *)buff)[0] = buffer[i];
				((char *)buff)[1] = buffer[i+1];
				((char *)buff)[2] = buffer[i+2];
				((char *)buff)[3] = buffer[i+3];
				((char *)buff)[4] = buffer[i+4];
				if (!addMidiEvent(list, MidiSMTPEOffset, deltaTime, buff))
					return (false);
				i+=5;
				break;
			case 0x58:
				if (buffer[i++] != 0x04) {
					printf("Error: Invalid byte found (%#x found but expected 0x04)\n", buffer[i - 1]);
					return (false);
				}
				buff = malloc(sizeof(char) * 4);
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)sizeof(char) * 4);
					return (false);
				}
				if (outputDebug)printf("Tempo infos: time signature %i/%i 1/4 note is %i ticks %i", buffer[i], 2 << (buffer[i+1] - 1), buffer[i+2], buffer[i+3]);
				((char *)buff)[0] = buffer[i];
				((char *)buff)[1] = 2 << buffer[i+1];
				((char *)buff)[2] = buffer[i+2];
				((char *)buff)[3] = buffer[i+3];
				if (!addMidiEvent(list, MidiNewTimeSignature, deltaTime, buff))
					return (false);
				i+=4;
				break;
			case 0x59:
				if (buffer[i++] != 0x02) {
					printf("Error: Invalid byte found (%#x found but expected 0x02)\n", buffer[i - 1]);
					if (outputDebug)showChunk(buffer, i, buffLen, posInFile + i);
					return (false);
				}
				buff = malloc(sizeof(char) * 2);
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)sizeof(char) * 2);
					return (false);
				}
				if (outputDebug)printf("Key signature %i %i", buffer[i], buffer[i+1]);
				((char *)buff)[0] = buffer[i];
				((char *)buff)[1] = buffer[i+1];
				if (!addMidiEvent(list, MidiNewKeySignature, deltaTime, buff))
					return (false);
				i+=2;
				break;
			case 0x7F:
				for (len = buffer[i] & 0x7F; buffer[i++] & 0x80; len = (len << 7) + (buffer[i] & 0x7F));
				buff = malloc(len + 1);
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)len + 1);
					return (false);
				}
				if (outputDebug) {
					printf("Sequencer-Specific Meta-event: '");
					fflush(stdout);
					write(1, &buffer[i], len);
					printf("'");
				}
				for (unsigned int j = 0; j < len; j++)
					((char *)buff)[j] = buffer[i + j];
				((char *)buff)[len] = 0;
				if (!addMidiEvent(list, MidiNewCuePoint, deltaTime, buff))
					return (false);
				i += len;
				break;
			default:
				if (outputDebug)printf("Error: Invalid meta event type (%#x)\n", buffer[i - 1]);
				return (false);
			}
		} else if (statusByte >= 0x80 && statusByte < 0x90) {
			if (buffer[i] > 127) {
				printf("Error: Note out of range (%i out of range 0-127)\n", buffer[i]);
				if (outputDebug)showChunk(buffer, i, buffLen, posInFile + i);
				return (false);
			} else if (buffer[i + 1] > 127) {
				printf("Error: Velocity out of range (%i out of range 0-127)\n", buffer[i + 1]);
				if (outputDebug)showChunk(buffer, i, buffLen, posInFile + i);
				return (false);
			}
			if (outputDebug)printf("%s off in channel %i (velocity: %i)", getNoteString(buffer[i]), statusByte - 0x80, buffer[i+1]);
			buff = malloc(sizeof(char) * 3);
			if (!buff) {
				printf("Error: Cannot alloc %iB\n", (int)sizeof(char) * 3);
				return (false);
			}
			((char *)buff)[0] = statusByte - 0x80;
			((char *)buff)[1] = buffer[i];
			((char *)buff)[2] = buffer[i+1];
			if (!addMidiEvent(list, MidiNoteReleased, deltaTime, buff))
				return (false);
			i+=2;
		} else if (statusByte >= 0x90 && statusByte < 0xA0) {
			if (buffer[i] > 127) {
				printf("Error: Note out of range (%i out of range 0-127)\n", buffer[i]);
				if (outputDebug)showChunk(buffer, i, buffLen, posInFile + i);
				return (false);
			} else if (buffer[i + 1] > 127) {
				printf("Error: Velocity out of range (%i out of range 0-127)\n", buffer[i + 1]);
				if (outputDebug)showChunk(buffer, i, buffLen, posInFile + i);
				return (false);
			}
			if (outputDebug)printf("%s on in channel %i (velocity: %i)", getNoteString(buffer[i]), statusByte - 0x90, buffer[i+1]);
			buff = malloc(sizeof(char) * 3);
			if (!buff) {
				printf("Error: Cannot alloc %iB\n", (int)sizeof(char) * 3);
				return (false);
			}
			((char *)buff)[0] = statusByte - 0x90;
			((char *)buff)[1] = buffer[i];
			((char *)buff)[2] = buffer[i+1];
			if (!addMidiEvent(list, MidiNotePressed, deltaTime, buff))
				return (false);
			result->nbOfNotes++;
			i+=2;
		} else if (statusByte >= 0xA0 && statusByte < 0xB0) {
			if (buffer[i] > 127) {
				printf("Error: Note out of range (%i out of range 0-127)\n", buffer[i]);
				if (outputDebug)showChunk(buffer, i, buffLen, posInFile + i);
				return (false);
			} else if (buffer[i + 1] > 127) {
				printf("Error: Pressure out of range (%i out of range 0-127)\n", buffer[i + 1]);
				if (outputDebug)showChunk(buffer, i, buffLen, posInFile + i);
				return (false);
			}
			if (outputDebug)printf("Polyphonic pressure on note %s in channel %i (velocity: %i)", getNoteString(buffer[i]), statusByte - 0xA0, buffer[i+1]);
			buff = malloc(sizeof(char) * 3);
			if (!buff) {
				printf("Error: Cannot alloc %iB\n", (int)sizeof(char) * 3);
				return (false);
			}
			((char *)buff)[0] = statusByte - 0xA0;
			((char *)buff)[1] = buffer[i];
			((char *)buff)[2] = buffer[i+1];
			if (!addMidiEvent(list, MidiPolyphonicPressure, deltaTime, buff))
				return (false);
			i+=2;
		} else if (statusByte >= 0xB0 && statusByte < 0xC0) {
			if (buffer[i] > 127) {
				printf("Error: Controller out of range (%i out of range 0-127)\n", buffer[i]);
				if (outputDebug)showChunk(buffer, i, buffLen, posInFile + i);
				return (false);
			} else if (buffer[i + 1] > 127) {
				printf("Error: Value out of range (%i out of range 0-127)\n", buffer[i + 1]);
				if (outputDebug)showChunk(buffer, i, buffLen, posInFile + i);
				return (false);
			}
			if (outputDebug)printf("Controller %i in channel %i is now at value %i", buffer[i], statusByte - 0xB0, buffer[i+1]);
			buff = malloc(sizeof(char) * 3);
			if (!buff) {
				printf("Error: Cannot alloc %iB\n", (int)sizeof(char) * 3);
				return (false);
			}
			((char *)buff)[0] = statusByte - 0xB0;
			((char *)buff)[1] = buffer[i];
			((char *)buff)[2] = buffer[i+1];
			if (!addMidiEvent(list, MidiControllerValueChanged, deltaTime, buff))
				return (false);
			i+=2;
		} else if (statusByte >= 0xC0 && statusByte < 0xD0) {
			if (buffer[i] > 127) {
				printf("Error: Program out of range (%i out of range 0-127)\n", buffer[i]);
				if (outputDebug)showChunk(buffer, i, buffLen, posInFile + i);
				return (false);
			}
			if (outputDebug)printf("Changed program of channel %i to %i", statusByte - 0xC0, buffer[i]);
			buff = malloc(sizeof(char) * 2);
			if (!buff) {
				printf("Error: Cannot alloc %iB\n", (int)sizeof(char) * 2);
				return (false);
			}
			((char *)buff)[0] = statusByte - 0xC0;
			((char *)buff)[1] = buffer[i];
			if (!addMidiEvent(list, MidiProgramChanged, deltaTime, buff))
				return (false);
			i++;
		} else if (statusByte >= 0xD0 && statusByte < 0xE0) {
			if (buffer[i] > 127) {
				printf("Error: Pressure out of range (%i out of range 0-127)\n", buffer[i]);
				if (outputDebug)showChunk(buffer, i, buffLen, posInFile + i);
				return (false);
			}
			if (outputDebug)printf("Changed pressure of all note in channel %i to %i", statusByte - 0xC0, buffer[i]);
			buff = malloc(sizeof(char) * 2);
			if (!buff) {
				printf("Error: Cannot alloc %iB\n", (int)sizeof(char) * 2);
				return (false);
			}
			((char *)buff)[0] = statusByte - 0xD0;
			((char *)buff)[1] = buffer[i];
			if (!addMidiEvent(list, MidiPressureOfChannelChanged, deltaTime, buff))
				return (false);
			i++;
		} else if (statusByte >= 0xE0 && statusByte < 0xF0) {
			if (buffer[i] > 127) {
				printf("Error: Lsb out of range (%i out of range 0-127)\n", buffer[i]);
				if (outputDebug)showChunk(buffer, i, buffLen, posInFile + i);
				return (false);
			} else if (buffer[i] > 127) {
				printf("Error: Msb out of range (%i out of range 0-127)\n", buffer[i]);
				if (outputDebug)showChunk(buffer, i, buffLen, posInFile + i);
				return (false);
			}
			if (outputDebug)printf("Changed pitch bend of all note in channel %i to %i", statusByte - 0xC0, (buffer[i] << 7) + buffer[i+1]);
			buff = malloc(sizeof(char) * 2);
			if (!buff) {
				printf("Error: Cannot alloc %iB\n", (int)sizeof(char) * 2);
				if (outputDebug)showChunk(buffer, i, buffLen, posInFile + i);
				return (false);
			}
			((char *)buff)[0] = statusByte - 0xD0;
			((char *)buff)[1] = (buffer[i] << 7) + buffer[i+1];
			if (!addMidiEvent(list, MidiPitchBendChanged, deltaTime, buff))
				return (false);
			i+=2;
		} else {
			printf("Error: Unsupported event (status byte: %#x, delta time: %lu) (At pos %i)\n", statusByte, deltaTime, i + posInFile);
			if (outputDebug)showChunk(buffer, i, buffLen, posInFile + i);
			return (false);
		}
		if (outputDebug)printf("   New position: %i\n", i + posInFile);
		totalTime += deltaTime;
	}	
	printf("Error: The end of track wasn't found");
	return (false);
}

MidiParser	*parseMidi(char *path, bool outputDebug)
{
	char			type[5];
	int			length;
	unsigned char		*buffer = NULL;
	int			buffLen = 0;
	unsigned char		buff;
	int			bytes = 0;
	int			full = 0;
	FILE			*stream = fopen(path, "rb");
	int			fd = stream ? fileno(stream) : -1;
	static MidiParser	result;
	int			tracksFound = 0;
	bool			foundHeader = false;
	
	if (fd < 0)
		return (NULL);
	memset(type, 0, sizeof(type));
	memset(&length, 0, sizeof(length));
	memset(&result, 0, sizeof(result));
	for (; read(fd, type, 4) > 0; ) {
		full += 4;
		if (strcmp(type, "MThd") && strcmp(type, "MTrk")) {
			printf("Error: Invalid type '%s'\n", type);
			free(buffer);
			return (NULL);
		}
		for (int i = 0; i < 4; i++) {
			length <<= 8;
			bytes = read(fd, &buff, 1);
			full += 1;
			length += buff;
			if (!bytes)
				lseek(fd, 0, SEEK_CUR);
		}
		if (outputDebug)printf("Type: %s\nlength: %i\n", type, length);
		if (length > buffLen) {
			buffer = realloc(buffer, length + 1);
			if (!buffer) {
				printf("Error: Cannot alloc %iB\n", length + 1);
				free(buffer);
				return (NULL);
			}
			memset(buffer, 0, length + 1);
			buffLen = length;
		}
		full += read(fd, buffer, length);
		if (strcmp(type, "MThd") == 0) {
			if (foundHeader) {
				printf("Error: Two headers were found\n");
				free(buffer);
				return (NULL);
			}
			foundHeader = true;
			result.format = (buffer[0] << 8) + buffer[1];
			if (result.format > 1) {
				printf("Error: Unsupported format (%i)\n", result.format);
				free(buffer);
				return (NULL);
			}
			result.nbOfTracks = (buffer[2] << 8) + buffer[3];
			result.tracks = malloc(sizeof(*result.tracks) * result.nbOfTracks);
			memset(result.tracks, 0, sizeof(*result.tracks) * result.nbOfTracks);
			if (buffer[4] >> 15) {
				result.fps = buffer[5] % 128;
				result.ticks = buffer[4];
			} else
				result.ticks = ((buffer[4] << 9) + (buffer[5] << 1)) >> 1;
		} else if (!foundHeader) {
			printf("Error: Tracks starts before headers\n");
			return (NULL);
		} else if(tracksFound++ < result.nbOfTracks && !parseMidiTrack(buffer, length, &result.tracks[tracksFound - 1], outputDebug, &result, full - length)) {
			free(buffer);
			return (NULL);
		}
		if (outputDebug)printf(strcmp(type, "MThd") == 0 ? "Found header !\n\n" : "End of track %i\n\n", tracksFound);
		memset(type, 0, sizeof(type));
		memset(&length, 0, sizeof(length));
		memset(buffer, 0, buffLen + 1);
	}
	free(buffer);
	if (outputDebug)printf("Read %iB of file\n", full);
	if (!foundHeader) {
		printf("Error: No header were found\n");
		return (NULL);
	} else if (tracksFound != result.nbOfTracks) {
		printf("Error: Invalid header: expected %i tracks but found %i\n", result.nbOfTracks, tracksFound);
		return (NULL);
	}
	if (outputDebug) {
		printf("%s: format %hi, %hi tracks, %i notes, ", path, result.format, result.nbOfTracks, result.nbOfNotes);
		if (result.fps) {
			printf("division: %i FPS and %i ticks/frame\n", result.fps, result.ticks);
		} else
			printf("division: %i ticks / 1/4 note\n", result.ticks);
	}
	close(fd);
	return (&result);
}