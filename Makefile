# vim:noexpandtab
munge: munge.c
	gcc -O3 -march=btver1 --std=c99 -o munge munge.c -ltiff

