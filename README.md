# Analog Circuit Saturation Workspace

All VST3 plug-in sources live inside the `vst_codex/` directory. Configure and build from the repository root so CMake can pull in the nested project:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

See `vst_codex/README.md` for DSP details, parameters, and installation notes.
