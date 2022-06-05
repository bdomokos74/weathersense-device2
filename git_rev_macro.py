import subprocess

revision = (
    subprocess.check_output(["git", "rev-parse", "--short", "HEAD"])
    .strip()
    .decode("utf-8")
)
print("-DGIT_REV='\"%s\"'" % revision)
