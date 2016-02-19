# vim:noexpandtab
munge: munge.c
	gcc -O2 --std=c99 -o munge munge.c -ltiff

