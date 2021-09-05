CXX = g++ 

# using mingw32
ECHO_MESSAGE = "MinGW"

# -LC:\SDL2\lib -lSDL2main -lSDL2
SDL_LIB = C:\SDL2\lib -lSDL2main -lSDL2

# -IC:\SDL2\include\SDL2
SDL_INCLUDE = C:\SDL2\include\SDL2

# opengl
OPENGL_INCLUDE = C:\MinGW\include\GL

IMGUI_DIR = imgui
SOURCES = boilerplate.cpp
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp

# specific to our backend (sdl + opengl)
SOURCES += $(IMGUI_DIR)/imgui_impl_sdl.cpp $(IMGUI_DIR)/imgui_impl_opengl3.cpp

# set up flags 
CXXFLAGS = -Wall -Wformat -std=c++14 -I$(SDL_INCLUDE) -I$(IMGUI_DIR)

# add '-mwindows' to LIBS to prevent an additional comand line terminal from appearing (but it's useful for debugging)
LIBS = -mwindows -lmingw32 -lgdi32 -lopengl32 -limm32 -static-libstdc++ -static-libgcc -L$(SDL_LIB)

# object files needed 
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))

EXE = imgui_boilerplate

# build main
%.o:%.cpp stb_image.h
	$(CXX) $(CXXFLAGS) -I$(OPENGL_INCLUDE) -c -o $@ $<

# build imgui dependencies
%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(OBJS)
