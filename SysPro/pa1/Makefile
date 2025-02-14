file = pa1

all: $(file)

$(file): $(file).c
	gcc -Wall -Werror -fsanitize=address,undefined -g $(file).c -o $(file).out -lm
clean:
	rm -rf $(file).out*
