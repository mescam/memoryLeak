all:
	gcc server/list.c server/cache.c server/main.c -o memoryLeak -lpthread