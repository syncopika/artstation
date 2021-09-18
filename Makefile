CXX = g++

# using mingw32
ECHO_MESSAGE = "MinGW"

# -LC:\SDL2\lib -lSDL2main -lSDL2
SDL_LIB = C:\libraries\SDL2-2.0.10\i686-w64-mingw32\lib -lSDL2main -lSDL2

# -IC:\SDL2\include\SDL2
SDL_INCLUDE = C:\libraries\SDL2-2.0.10\i686-w64-mingw32\include\SDL2

# point to opengl location (if you don't want to do 3d stuff, just using MinGW\include\GL should be fine. otherwise, we use GLEW, which includes OpenGL)
#C:\MinGW\include\GL
OPENGL_INCLUDE = C:\libraries\glew-2.1.0\include

# point to the location of the GLM library
GLM_INCLUDE = C:\libraries\glm

APPS_DIR = apps
APPS = $(APPS_DIR)/image_editor.hh

# other dependencies (e.g. the apps, the imgui stuff)
OTHER_DIR = external
OTHER_DEPS = $(OTHER_DIR)/tiny_obj_loader.h $(APPS)

IMGUI_DIR = imgui
SOURCES = artstation.cpp
SOURCES += $(APPS_DIR)/image_editor.cpp
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp

# specific to our backend (sdl + opengl)
SOURCES += $(IMGUI_DIR)/imgui_impl_sdl.cpp $(IMGUI_DIR)/imgui_impl_opengl3.cpp

CXXFLAGS = -Wall -Wformat -std=c++14 -I$(SDL_INCLUDE) -I$(IMGUI_DIR)

GLEW_LIBS = -lglew32 -lglu32

# add '-mwindows' to LIBS to prevent an additional comand line terminal from appearing (but it's useful for debugging)
LIBS = -mwindows -lmingw32 -lgdi32 $(GLEW_LIBS) -lopengl32 -limm32 -static-libstdc++ -static-libgcc -L$(SDL_LIB)

# object files needed
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))

EXE = artstation


# build main
%.o:%.cpp $(OTHER_DEPS)
	$(CXX) $(CXXFLAGS) -I$(OPENGL_INCLUDE) -I$(GLM_INCLUDE) -c -o $@ $<

# build imgui dependencies
%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(OBJS)
