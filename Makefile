NAME=proj1

all:
	cc -g $(NAME).c -o hinfosvc

clean:
	rm -f $(NAME) load* hostname* cpu-name*
