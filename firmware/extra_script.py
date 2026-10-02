# Dijalankan PlatformIO sebelum build: tanam klip animasi ke include/jpeg_clips.h.
import os
Import("env")
budget = env.GetProjectOption("custom_jpeg_budget", "0")
os.environ["MOCHI_JPEG_BUDGET"] = str(budget)
if env.Execute("$PYTHONEXE tools/embed_jpeg.py"):
    env.Exit(1)
