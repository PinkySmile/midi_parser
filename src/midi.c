#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "midi_parser.h"

bool	parseMidiTrack(unsigned char *buffer, int buffLen, EventList *list)
{
	unsigned char		statusByte;
	unsigned long int	deltaTime = 0;
	unsigned long int	totalTime = 0;
	unsigned long int	len;

	for (int i = 0; i < 12; printf("%x ", buffer[i++]));
	printf("\n");
	if (buffer[buffLen - 2] != 0x2F || buffer[buffLen - 1] != 0x00) {
		printf("Error: The end of track wasn't found");
		return (false);
	}
	buffLen -= 2;
	for (int i = 0; i < buffLen; ) {
		for (deltaTime = 0; buffer[i] & 0x80; deltaTime = (deltaTime << 7) + (buffer[i++] & 0x7F));
		deltaTime = (deltaTime << 7) + (buffer[i++] & 0x7F);
		statusByte = buffer[i++];
		if (statusByte == 0xFF) {
			switch (buffer[i++]) {
			case 0x00:
				if (buffer[i++] != 0x02) {
					printf("Error: Invalid byte found (%#x found but expected 0x02)\n", buffer[i - 1]);
					return (false);
				} else if (totalTime > 0) {
					printf("Error: Cannot add sequence number after non-zero delta times\n");
					return (false);
				}
				printf("Found sequence number: %i\n", (buffer[i++] << 8) + buffer[i++]);
				break;
			case 0x01:
				for (len = 0; buffer[i] & 0x80; len = (len << 7) + (buffer[i++] & 0x7F));
				len = (len << 7) + (buffer[i++] & 0x7F);
				printf("Text event: '");
				fflush(stdout);
				write(1, &buffer[i], len);
				printf("'\n");
				i += len;
				break;
			case 0x02:
				for (len = 0; buffer[i] & 0x80; len = (len << 7) + (buffer[i++] & 0x7F));
				len = (len << 7) + (buffer[i++] & 0x7F);
				printf("Copyright to : '");
				fflush(stdout);
				write(1, &buffer[i], len);
				printf("'\n");
				i += len;
				break;
			case 0x03:
				for (len = 0; buffer[i] & 0x80; len = (len << 7) + (buffer[i++] & 0x7F));
				len = (len << 7) + (buffer[i++] & 0x7F);
				printf("Track name: '");
				fflush(stdout);
				write(1, &buffer[i], len);
				printf("'\n");
				i += len;
				break;
			case 0x04:
				for (len = 0; buffer[i] & 0x80; len = (len << 7) + (buffer[i++] & 0x7F));
				len = (len << 7) + (buffer[i++] & 0x7F);
				printf("Instrument name: '");
				fflush(stdout);
				write(1, &buffer[i], len);
				printf("'\n");
				i += len;
				break;
			case 0x05:
				for (len = 0; buffer[i] & 0x80; len = (len << 7) + (buffer[i++] & 0x7F));
				len = (len << 7) + (buffer[i++] & 0x7F);
				printf("Lyric: '");
				fflush(stdout);
				write(1, &buffer[i], len);
				printf("'\n");
				i += len;
				break;
			case 0x06:
				for (len = 0; buffer[i] & 0x80; len = (len << 7) + (buffer[i++] & 0x7F));
				len = (len << 7) + (buffer[i++] & 0x7F);
				printf("Marker: '");
				fflush(stdout);
				write(1, &buffer[i], len);
				printf("'\n");
				i += len;
				break;
			case 0x07:
				for (len = 0; buffer[i] & 0x80; len = (len << 7) + (buffer[i++] & 0x7F));
				len = (len << 7) + (buffer[i++] & 0x7F);
				printf("Cue Point: '");
				fflush(stdout);
				write(1, &buffer[i], len);
				printf("'\n");
				i += len;
				break;
			case 0x20:
				if (buffer[i++] != 0x01) {
					printf("Error: Invalid byte found (%#x found but expected 0x01)\n", buffer[i - 1]);
					return (false);
				}
				printf("New channel prefix: %i\n", buffer[i++]);
				break;
			case 0x51:
				if (buffer[i++] != 0x03) {
					printf("Error: Invalid byte found (%#x found but expected 0x00)\n", buffer[i - 1]);
					return (false);
				}
				printf("New tempo: %i\n", (buffer[i++] << 16) + (buffer[i++] << 8) + buffer[i++]);
				break;
			case 0x54:
				if (buffer[i++] != 0x05) {
					printf("Error: Invalid byte found (%#x found but expected 0x05)\n", buffer[i - 1]);
					return (false);
				} else if (totalTime > 0) {
					printf("Error: Cannot add SMTPE Offset after non-zero delta times\n");
					return (false);
				}
				printf("New offset: %ih %im %is %iframes %ihundreths of a frame", buffer[i++], buffer[i++], buffer[i++], buffer[i++], buffer[i++]);
				break;
			case 0x58:
				if (buffer[i++] != 0x04) {
					printf("Error: Invalid byte found (%#x found but expected 0x04)\n", buffer[i - 1]);
					return (false);
				}
				printf("I didn't understand what it does: %i %i %i %i\n", buffer[i++], buffer[i++], buffer[i++], buffer[i++]);
				break;
			case 0x59:
				if (buffer[i++] != 0x02) {
					printf("Error: Invalid byte found (%#x found but expected 0x02)\n", buffer[i - 1]);
					return (false);
				}
				printf("Key signature %i %i\n", buffer[i++], buffer[i++]);
				break;
			case 0x7F:
				for (len = 0; buffer[i] & 0x80; len = (len << 7) + (buffer[i++] & 0x7F));
				len = (len << 7) + (buffer[i++] & 0x7F);
				printf("Sequencer-Specific Meta-event: '");
				fflush(stdout);
				write(1, &buffer[i], len);
				printf("'\n");
				i += len;
				break;
			default:
				printf("Error: Invalid meta event type (%#x)\n", buffer[i - 1]);
				return (false);
			}
		} else if (statusByte >= 0xF0 || statusByte < 0x80) {
			printf("Error: Unsupported event (status byte: %#x, delta time: %lu)\n", statusByte, deltaTime);
			return (false);
		}
		totalTime += deltaTime;
	}
	return (true);
}

MidiParser	*parseMidi(char *path)
{
	char			type[5];
	int			length;
	unsigned char		*buffer = NULL;
	int			buffLen = 0;
	unsigned char		buff;
	int			bytes = 0;
	int			full = 0;
	int			fd = open(path, O_RDONLY);
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
		printf("Type: %s\nlength: %i\n", type, length);
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
		for (int i = 0; i < length; i++){
			bytes = read(fd, buffer + i, 1);
			full += 1;
			if (!bytes)
				lseek(fd, 0, SEEK_CUR);
		}
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
			result.tracks = (buffer[2] << 8) + buffer[3];
			result.events = malloc(sizeof(*result.events) * result.tracks);
			if (buffer[4] >> 15) {
				result.fps = buffer[5] % 128;
				result.ticks = buffer[4];
			} else
				result.ticks = ((buffer[4] << 9) + (buffer[5] << 1)) >> 1;
		} else if(tracksFound++ < result.tracks && !parseMidiTrack(buffer, length, &result.events[tracksFound - 1])) {
			free(buffer);
			return (NULL);
		}
		memset(type, 0, sizeof(type));
		memset(&length, 0, sizeof(length));
		memset(buffer, 0, buffLen + 1);
		//lseek(fd, full, SEEK_SET);
	}
	free(buffer);
	printf("Read %iB of file\n", full);
	if (tracksFound != result.tracks) {
		printf("Error: Invalid header: expected %i tracks but found %i\n", result.tracks, tracksFound);
		return (NULL);
	}
	printf("%s: format %hi, %hi tracks, ", path, result.format, result.tracks);
	if (result.fps) {
		printf("division: %i FPS and %i ticks/frame\n", result.fps, result.ticks);
	} else
		printf("division: %i ticks / ¼ note\n", result.ticks);
	close(fd);
	return (&result);
}