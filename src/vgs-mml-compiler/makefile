CC=/usr/bin/gcc

all: vgs2mml

vgs2mml: src/vgsmml.c src/vgsmml.h src/cli.c vgs-bgm-decoder/src/miniz.c vgs-bgm-decoder/src/vgsdec_internal.h
	$(CC) -o vgs2mml -I./src -I./vgs-bgm-decoder/src src/vgsmml.c src/cli.c vgs-bgm-decoder/src/miniz.c

test: all
	test "`./vgs2mml ./test/test-error-undefined-macro.mml test.bgm`" = "error(101) line=52: undeclared variable was specified"
	./vgs2mml ./test/test-normal.mml test.bgm
	$(CC) -o test_memchk -I./src -I./vgs-bgm-decoder/src src/vgsmml.c test/test_memchk.c vgs-bgm-decoder/src/miniz.c
	valgrind --tool=memcheck ./test_memchk

