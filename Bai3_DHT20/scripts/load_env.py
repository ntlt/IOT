import json
from pathlib import Path

Import("env")

project_dir = Path(env["PROJECT_DIR"])
dotenv_path = project_dir.parent / ".env"
settings = {}

if dotenv_path.exists():
    for line in dotenv_path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        value = value.strip()
        if len(value) >= 2 and value[0] == value[-1] and value[0] in ("'", '"'):
            value = value[1:-1]
        settings[key.strip()] = value

required_settings = (
    "ACCESS_TOKEN",
    "ACCESS_USERNAME",
    "DEVICE_CLIENT_ID",
    "WIFI_SSID",
    "WIFI_PASSWORD",
)
missing_settings = [key for key in required_settings if not settings.get(key)]

header_path = project_dir / "include" / "coreiot_secrets.h"
header_path.parent.mkdir(parents=True, exist_ok=True)
header_contents = "#pragma once\n"
for key in required_settings:
    header_contents += f"#define {key} {json.dumps(settings.get(key, ''))}\n"
header_path.write_text(header_contents, encoding="utf-8")

if missing_settings:
    print(
        "CoreIoT telemetry is not fully configured. Add these keys to the "
        f"repository-root .env: {', '.join(missing_settings)}"
    )
else:
    print("CoreIoT and Wi-Fi settings loaded from the repository-root .env.")
