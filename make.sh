# you will likely need to modify these to point to the correct library/include directories
SDL_LINKER_FLAGS="-L/usr/lib/x86_64-linux-gnu -lSDL -lpthread -lm -ldl -lasound -lm -ldl -lpthread -lpulse-simple -lpulse -lX11 -lXext -L/usr/lib/x86_64-linux-gnu -lcaca -lpthread -lSDL_mixer"
SDL_COMPILE_FLAGS="-I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT"
CUDA_LIBS="-L/usr/local/cuda/lib64 -L/opt/cuda/lib64 -L/targets/x86_64-linux/lib"
CODEHAPPY_CUDASDL_LIB="/data/libcodehappy/bin/libcodehappycudas.a"
INCLUDE_DIRS="-I/usr/local/cuda/include -I/opt/cuda/include -I/targets/x86_64-linux/include -I/data/libcodehappy/inc"

g++ -O3 -flto -fPIC -pthread -fuse-linker-plugin -m64 $INCLUDE_DIRS -c jeopardy.cpp $SDL_COMPILE_FLAGS -o jeopardy.o
g++ -O3 -flto -m64 jeopardy.o $CODEHAPPY_CUDASDL_LIB -lcublas -lculibos -lcudart -lcublasLt -lpthread -ldl -lrt $CUDA_LIBS $SDL_LINKER_FLAGS -o jeopardy

