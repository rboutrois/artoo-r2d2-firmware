Import("env")

def upload_spiffs(source, target, env):
    print("--- Uploading SPIFFS filesystem ---")
    env.Execute(env.subst(
        "$PYTHONEXE -m platformio run -t uploadfs -e $PIOENV"
    ))

env.AddPostAction("upload", upload_spiffs)
