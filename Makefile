NAME =	libmidiparser.a

SRC = 	midi.c			\

OBJ =	$(SRC:%.c=src/%.o)

INC =	-Iinclude

LDFLAGS =

CFLAGS= $(INC)		\
	-W		\
	-Wall		\
	-Wextra		\

CC =	gcc

RULE =	all


all:	$(NAME)

$(NAME):$(OBJ)
	$(AR) rc $(NAME) $(OBJ)

clean:
	$(RM) $(OBJ)

fclean:	clean
	$(RM) $(NAME)

re:	fclean all

dbg:	CFLAGS += -g -O0
dbg:	RULE = dbg
dbg:	fclean all
