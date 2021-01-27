CFLAGS = -Wall -O3

#-fsanitize=address
all: weak_collision_resistance strong_collision_resistance

weak_collision_resistance: 
	gcc $(CFLAGS) -o weak_collision_resistance weak_collision_resistance.c $(LDFLAGS) $(HEADER_PATH)

strong_collision_resistance: 
	g++ $(CFLAGS) -o strong_collision_resistance strong_collision_resistance.cpp $(LDFLAGS) $(HEADER_PATH)

clean:
	rm -f weak_collision_resistance strong_collision_resistance
	rm -f *.o