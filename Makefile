chip8:
	gcc -Wall -Wextra -O2 GuaranaHorizon/*.c -o ghorizon `pkg-config --cflags --libs sdl3`
clean:
	rm -f ghorizon

.PHONY: clean
