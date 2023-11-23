### Jeopardy simulation

Simulates the famous quiz show "Jeopardy!". You can play the game solo, or against large language model-powered computer opponents (using the GGML/llama.cpp backend -- Llama, Falcon, MPT, Mistral supported). Use an instruction tuned Llama 2-70B for best results.

Here is an [example playthrough video](https://www.youtube.com/watch?v=9wHEt7QMBR0). In this demonstration I'm running a Llama 70b finetune (8 bit quantization) on two RTX 3090 GPUs. Despite a few layers offloaded onto CPU, the speed is quite snappy.

The simulation can also be played without LLMs -- the computer opponents in this case are given a modeled probability of ringing in and getting the correct answer. There are long comments in the code explaining the rationale for the parameters chosen for computer opponent models.

The clues used in the simulation are mostly from [J! Archive](https://j-archive.com/). (A small percentage were extracted from earlier *Jeopardy!* video games.) Sound effects and some of the graphics come from the original TV program.

This app depends on [libcodehappy](https://github.com/codehappy-net/libcodehappy).

This code was originally written in 2011 for Win32, and was swiftly ported from MFC to libcodehappy to run on Linux/Win64/WebAssembly. It could do with a full re-write, it's extremely messy. But the LLM integration is so nifty/entertaining I'm putting it on the Interwebs anyway.

If you have less or more than 24 GB VRAM, you will want to change the number of layers loaded to GPU: see the llama->layers_to_gpu() call in Jeopardy::Initialize().

Since this was written for my use, the player's name currently is always "Chris".

