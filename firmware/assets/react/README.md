# React GIF — pindah ke `firmware/assets/builtin/`

Sejak 0.5.6 semua GIF+WAV bawaan (30 slot, termasuk 11 reaksi sentuh) ada di
`firmware/assets/builtin/gif/<tema>/<stem>.gif` + `firmware/assets/builtin/sfx/<tema>/<stem>.wav`.
Daftar dan urutan: `firmware/assets/meta.json` (`builtins`, `reacts`). Reaksi: `MOCHI_REACT` di `include/MochiRzmong.h`.
