import subprocess
import os

revision = (
    subprocess.check_output(["git", "rev-parse", "--short", "HEAD"])
    .strip()
    .decode("utf-8")
)
print("-DGIT_REV=\\\"%s\\\"" % revision)
print("-DWS_VERSION=\\\"v0.9\\\"")
print("-DIOT_CONFIG_WIFI_SSID=%s" % os.environ.get('WIFI_SSID'))
print("-DIOT_CONFIG_WIFI_PASSWORD=%s" % os.environ.get('WIFI_PASS'))
