#!/usr/bin/env bash
# Configura 2FA (TOTP) para SSH y fail2ban en Debian 12.
# Uso: sudo ./setup.sh
# Despues, cada usuario debe ejecutar `google-authenticator` (sin sudo).
set -euo pipefail

if [[ $EUID -ne 0 ]]; then
    echo "Ejecutar como root: sudo $0" >&2
    exit 1
fi

DIR="$(cd "$(dirname "$0")" && pwd)"

echo "[*] Instalando paquetes"
apt-get update
apt-get install -y openssh-server libpam-google-authenticator \
    fail2ban python3-systemd nftables

echo "[*] Configurando PAM para SSH"
PAM_SSHD=/etc/pam.d/sshd
if ! grep -q pam_google_authenticator.so "$PAM_SSHD"; then
    cp "$PAM_SSHD" "$PAM_SSHD.bak"
    printf '\n# 2FA con TOTP\nauth required pam_google_authenticator.so nullok\n' >> "$PAM_SSHD"
fi

echo "[*] Configurando SSH"
cat > /etc/ssh/sshd_config.d/10-2fa.conf <<'EOF'
UsePAM yes
KbdInteractiveAuthentication yes
EOF
sshd -t
systemctl restart ssh

echo "[*] Configurando fail2ban"
install -m 644 "$DIR/jail.local" /etc/fail2ban/jail.local
systemctl enable fail2ban
systemctl restart fail2ban
fail2ban-client status

echo
echo "[+] Listo. Ahora, como el usuario que va a entrar por SSH, ejecutar:"
echo "      google-authenticator"
echo "    y NO cerrar esta sesion hasta probar el acceso desde otra terminal."
