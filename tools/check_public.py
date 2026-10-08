"""Check staged content before a public push. Never print detected secret values."""
import hashlib
import io
import json
import pathlib
import re
import subprocess
import sys
import zipfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
FORBIDDEN_PARTS = {'private', '.pio', '.gradle', '.storage', '.venv', '__pycache__'}
FORBIDDEN_NAMES = {'local.properties', 'mqtt.json', 'passwords', 'dishwasher.json'}
KEY_SUFFIXES = {'.jks', '.keystore', '.p12', '.pem', '.key'}
PATTERNS = [re.compile(rb'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----'),
            re.compile(rb'(?:ghp_|gho_|github_pat_)[A-Za-z0-9_]{25,}')]

def known_secrets():
    secrets = []
    for relative, key in [('mqtt.json', 'password'), ('bench/network.json', 'password'), ('bench/device.json', 'token')]:
        file = ROOT / 'private' / relative
        if file.exists():
            value = json.loads(file.read_text(encoding='utf-8-sig')).get(key, '')
            if len(value) >= 8:
                secrets.append(value.encode())
    return secrets

def contains_secret(data, secrets):
    return any(value in data for value in secrets) or any(p.search(data) for p in PATTERNS)

def check():
    names = subprocess.check_output(['git', 'ls-files', '--cached', '-z'], cwd=ROOT).decode().split('\0')
    names = [n for n in names if n]
    if not names:
        raise RuntimeError('No staged/tracked files. Run git add before checking.')
    secrets = known_secrets()
    errors = []
    for name in names:
        path = pathlib.PurePosixPath(name)
        if FORBIDDEN_PARTS.intersection(path.parts) or path.name in FORBIDDEN_NAMES or path.suffix in KEY_SUFFIXES or path.name.startswith('.env') and path.name != '.env.example':
            errors.append('Forbidden path: ' + name)
        data = subprocess.check_output(['git', 'show', ':' + name], cwd=ROOT)
        if contains_secret(data, secrets):
            errors.append('Secret detected: ' + name)
        if data.startswith(b'PK\x03\x04'):
            with zipfile.ZipFile(io.BytesIO(data)) as archive:
                for item in archive.infolist():
                    if item.file_size > 20_000_000:
                        errors.append('Archive entry too large to inspect: ' + name)
                        continue
                    if contains_secret(archive.read(item), secrets):
                        errors.append('Secret detected inside archive: ' + name)
        if path.suffix == '.md':
            text = data.decode('utf-8-sig')
            text = re.sub(r'```.*?```', '', text, flags=re.S)
            for target in re.findall(r'\]\(([^)]+)\)', text):
                target = target.strip('<>').split('#', 1)[0]
                if not target or '://' in target or target.startswith('mailto:'):
                    continue
                resolved = (ROOT / path.parent / target).resolve()
                if not resolved.is_relative_to(ROOT) or not resolved.exists():
                    errors.append('Broken local link: ' + name + ' -> ' + target)
    table = ROOT / 'dist' / 'SHA256SUMS.txt'
    for line in table.read_text(encoding='utf-8-sig').splitlines():
        expected, name = line.split(maxsplit=1)
        data = (ROOT / 'dist' / name.strip().lstrip('*')).read_bytes()
        if hashlib.sha256(data).hexdigest().lower() != expected.lower():
            errors.append('Artifact hash mismatch: ' + name)
    if errors:
        for error in errors:
            print(error)
        return 1
    print(f'PASS: {len(names)} public files, staged secret/path checks, local document links, archive inspection, artifact hashes')
    return 0

if __name__ == '__main__':
    try:
        sys.exit(check())
    except Exception as error:
        print('Public check failed:', type(error).__name__)
        sys.exit(1)
