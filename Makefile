TARGET = bin/dbview
SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))

default: clean build test

clean:
	mkdir -p obj bin
	rm -f obj/*.o
	rm -f bin/*
	rm -f *.db

build: $(TARGET)

$(TARGET): $(OBJ)
	gcc -o $@ $?

obj/%.o : src/%.c
	gcc -c $< -o $@ -Iinclude

test:
	./$(TARGET) -n -f mydb.db
	./$(TARGET) -f mydb.db -a "Timmy H.,123 Sheshire Ln.,120"
	./$(TARGET) -f mydb.db -a "Sam Gamgee,123 TheShire Ln.,180"
	./$(TARGET) -f mydb.db -a "John Doe,123 SupShire Ln.,80"
	./$(TARGET) -f mydb.db -u "2,200"
	./$(TARGET) -f mydb.db -d 3
	./$(TARGET) -f mydb.db -l