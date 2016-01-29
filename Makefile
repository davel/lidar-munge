# vim:noexpandtab
munge: munge.c
	gcc -g --std=c99 -o munge munge.c -ltiff

