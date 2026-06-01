# Po uspešnem build avtomatsko zaženi upload na napravo
Import("env")

def upload_after_build(source, target, env):
    print("Build uspel -> nalagam na napravo ...")
    # Klic pio run -t upload je zanesljivejši (še posebej na Windows)
    env.Execute("pio run -t upload -e %s" % env["PIOENV"])

if not env.IsIntegrationDump():
    env.AddPostAction("buildprog", upload_after_build)
