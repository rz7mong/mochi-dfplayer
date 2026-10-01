Import("env")
env.Execute("$PYTHONEXE tools/embed_assets.py")
env.Execute("$PYTHONEXE tools/embed_jpeg.py")
