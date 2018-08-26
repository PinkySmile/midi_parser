#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "midi_parser.h"

int	main(int argc, char **args)
{
	MidiParser	*result;

	if (argc != 2 && argc != 3) {
		printf("Usage: %s [debug] <file.mid>\n", args[0]);
		return (EXIT_FAILURE);
	}
	for (int i = 1 + (strcmp(args[1], "debug") == 0); i < argc; i++) {
		result = parseMidi(args[i], argc == 3 && strcmp(args[1], "debug") == 0, true);
		if (!result)
			printf("An error occured when reading %s\n", args[i]);
		else {
			printf("Finished to read %s: format %hi, %hi tracks, %i notes, ", args[i], result->format, result->nbOfTracks, result->nbOfNotes);
			if (result->fps) {
				printf("division: %i FPS and %i ticks/frame\n", result->fps, result->ticks);
			} else
				printf("division: %i ticks / 1/4 note\n", result->ticks);
			deleteMidiParserStruct(result);
		}
	}
	return (EXIT_SUCCESS);
}