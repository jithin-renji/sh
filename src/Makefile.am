AM_YFLAGS = -d -Wno-yacc -Wcounterexamples

bin_PROGRAMS = sh
sh_SOURCES = main.c vec.h vec.c cmd_ast.h cmd_ast.c eval.h eval.c builtins.h builtins.c \
			 exec_cmd.h exec_cmd.c parse.y
sh_LDFLAGS = -lreadline
sh_CFLAGS = -Wall

CLEANFILES = parse.c parse.h
