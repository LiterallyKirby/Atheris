CXX := g++
CXXFLAGS := -Iinclude -Wall -Wextra -std=c++17
LDFLAGS := -lglfw -lGL -ldl -lpthread
SRC_CPP := $(wildcard src/*.cpp)
SRC_C   := $(wildcard src/*.c)
SRC     := $(SRC_CPP) $(SRC_C)
OBJ := $(patsubst src/%, build/%, $(SRC:.cpp=.o))
OBJ := $(OBJ:.c=.o)
TARGET := BMCC

all: $(TARGET)

$(TARGET): $(OBJ)
	@echo "Linking $(TARGET)..."
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

build/%.o: src/%.cpp
	@mkdir -p build
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/%.o: src/%.c
	@mkdir -p build
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf build $(TARGET)

# Debug target to see what's happening
debug: $(OBJ)
	@echo "Objects: $(OBJ)"
	@echo "Linking with verbose output..."
	$(CXX) -v $(OBJ) -o $(TARGET) $(LDFLAGS)
