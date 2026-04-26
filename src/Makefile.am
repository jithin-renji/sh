AM_YFLAGS = -d -Wno-yacc -Wcounterexamples

bin_PROGRAMS = clara
clara_SOURCES = main.c colors.h vec.h vec.c cmd_ast.h cmd_ast.c eval.h eval.c builtins.h builtins.c \
				proc.h proc.c jobs.h jobs.c parse.y
clara_LDFLAGS = -lreadline
clara_CFLAGS = -Wall

CLEANFILES = parse.c parse.h
