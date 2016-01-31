# vim:noexpandtab
munge: munge.c
	gcc -O --std=c99 -o munge munge.c -ltiff

