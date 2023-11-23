rm jeopardy.wasm
rm jeopardy.js
rm jeopardy.data
emcc -O3 -flto -std=c++11 -sUSE_SDL -DCODEHAPPY_WASM -I../libcodehappy/inc jeopardy.cpp ../libcodehappy/bin/libcodehappy.bc ../libcodehappy/bin/sqlite3.bc -o jeopardy.js --preload-file assets/ -s ALLOW_MEMORY_GROWTH=1
