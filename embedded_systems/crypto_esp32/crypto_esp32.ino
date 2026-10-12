/*
 * Ejemplos de criptografia en un sistema embebido (ESP32 + mbedTLS)
 *
 *   1. Integridad:       hash SHA-256
 *   2. Confidencialidad: cifrado simetrico autenticado AES-256-GCM
 *   3. Autenticidad:     firma digital ECDSA P-256 sobre un "firmware"
 *
 * mbedTLS viene incluida en el core de Arduino para ESP32, asi que no hace
 * falta instalar ninguna libreria. Se puede ejecutar en Wokwi o en una placa real.
 */
#include <Arduino.h>
#include <string.h>
#include "mbedtls/md.h"
#include "mbedtls/gcm.h"
#include "mbedtls/ecdsa.h"
#if __has_include("esp_random.h")
#include "esp_random.h"
#else
#include "esp_system.h"
#endif

// Generador de numeros aleatorios por hardware (TRNG) del ESP32,
// con la firma que espera mbedTLS.
static int rng(void *ctx, unsigned char *out, size_t len) {
  (void)ctx;
  esp_fill_random(out, len);
  return 0;
}

static void printHex(const char *label, const uint8_t *buf, size_t len) {
  Serial.printf("  %-16s", label);
  for (size_t i = 0; i < len; i++) Serial.printf("%02x", buf[i]);
  Serial.println();
}

static void sha256(const uint8_t *data, size_t len, uint8_t out[32]) {
  mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), data, len, out);
}

static int bitsDistintos(const uint8_t *a, const uint8_t *b, size_t len) {
  int n = 0;
  for (size_t i = 0; i < len; i++) n += __builtin_popcount(a[i] ^ b[i]);
  return n;
}

// ---------------------------------------------------------------------------
// 1. Hash SHA-256
// ---------------------------------------------------------------------------
static void ejemploHash() {
  Serial.println("=== 1. Hash SHA-256 (integridad) ===");

  const char *lectura1 = "temperatura=23.5C";
  const char *lectura2 = "temperatura=23.6C";  // cambia un solo caracter
  uint8_t h1[32], h2[32];

  unsigned long t0 = micros();
  sha256((const uint8_t *)lectura1, strlen(lectura1), h1);
  unsigned long t = micros() - t0;
  sha256((const uint8_t *)lectura2, strlen(lectura2), h2);

  Serial.printf("  %-16s%s\n", "Mensaje 1:", lectura1);
  printHex("SHA-256:", h1, 32);
  Serial.printf("  %-16s%s\n", "Mensaje 2:", lectura2);
  printHex("SHA-256:", h2, 32);
  Serial.printf("  Bits distintos entre ambos hashes: %d de 256 (efecto avalancha)\n",
                bitsDistintos(h1, h2, 32));
  Serial.printf("  Tiempo de calculo: %lu us\n\n", t);
}

// ---------------------------------------------------------------------------
// 2. Cifrado simetrico AES-256-GCM
// ---------------------------------------------------------------------------
static void ejemploAES() {
  Serial.println("=== 2. Cifrado simetrico AES-256-GCM (confidencialidad + integridad) ===");

  const char *mensaje = "temperatura=23.5C;humedad=40%";
  const char *aad = "nodo-07";  // datos asociados: viajan en claro pero autenticados
  const size_t len = strlen(mensaje);

  uint8_t clave[32], iv[12], tag[16];
  uint8_t cifrado[64], descifrado[64];
  rng(NULL, clave, sizeof(clave));  // en un producto real: clave unica por dispositivo
  rng(NULL, iv, sizeof(iv));        // el IV nunca debe repetirse con la misma clave

  mbedtls_gcm_context gcm;
  mbedtls_gcm_init(&gcm);
  mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, clave, 256);

  unsigned long t0 = micros();
  mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT, len, iv, sizeof(iv),
                            (const uint8_t *)aad, strlen(aad),
                            (const uint8_t *)mensaje, cifrado, sizeof(tag), tag);
  unsigned long tCifrado = micros() - t0;

  Serial.printf("  %-16s%s\n", "Texto plano:", mensaje);
  printHex("Clave:", clave, sizeof(clave));
  printHex("IV:", iv, sizeof(iv));
  printHex("Cifrado:", cifrado, len);
  printHex("Tag:", tag, sizeof(tag));

  // Descifrado correcto
  t0 = micros();
  int ret = mbedtls_gcm_auth_decrypt(&gcm, len, iv, sizeof(iv),
                                     (const uint8_t *)aad, strlen(aad),
                                     tag, sizeof(tag), cifrado, descifrado);
  unsigned long tDescifrado = micros() - t0;
  descifrado[len] = '\0';
  Serial.printf("  %-16s%s (%s)\n", "Descifrado:", (char *)descifrado,
                ret == 0 ? "tag valido" : "tag INVALIDO");

  // Un atacante modifica un byte del mensaje cifrado en transito
  cifrado[0] ^= 0x01;
  ret = mbedtls_gcm_auth_decrypt(&gcm, len, iv, sizeof(iv),
                                 (const uint8_t *)aad, strlen(aad),
                                 tag, sizeof(tag), cifrado, descifrado);
  Serial.printf("  Cifrado alterado en 1 bit -> %s\n",
                ret == MBEDTLS_ERR_GCM_AUTH_FAILED ? "RECHAZADO (tag invalido)" : "aceptado");

  Serial.printf("  Tiempo cifrado: %lu us, descifrado: %lu us\n\n", tCifrado, tDescifrado);
  mbedtls_gcm_free(&gcm);
}

// ---------------------------------------------------------------------------
// 3. Firma digital ECDSA P-256 (como en Secure Boot / actualizaciones OTA)
// ---------------------------------------------------------------------------
static void ejemploFirma() {
  Serial.println("=== 3. Firma digital ECDSA P-256 (autenticidad + integridad) ===");

  mbedtls_ecp_group grp;
  mbedtls_mpi d, r, s;  // d: clave privada, (r, s): firma
  mbedtls_ecp_point Q;  // Q: clave publica
  mbedtls_ecp_group_init(&grp);
  mbedtls_mpi_init(&d);
  mbedtls_mpi_init(&r);
  mbedtls_mpi_init(&s);
  mbedtls_ecp_point_init(&Q);
  mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);

  // Fabricante: genera el par de claves (en la realidad esto se hace fuera
  // del dispositivo y solo se graba la clave publica en el)
  unsigned long t0 = micros();
  mbedtls_ecp_gen_keypair(&grp, &d, &Q, rng, NULL);
  unsigned long tClaves = micros() - t0;

  uint8_t pub[65];
  size_t pubLen = 0;
  mbedtls_ecp_point_write_binary(&grp, &Q, MBEDTLS_ECP_PF_UNCOMPRESSED,
                                 &pubLen, pub, sizeof(pub));
  printHex("Clave publica:", pub, pubLen);

  // Fabricante: firma el hash del firmware con la clave privada
  const char *firmware = "FIRMWARE v1.2.0 - control de valvula";
  uint8_t hash[32];
  sha256((const uint8_t *)firmware, strlen(firmware), hash);

  t0 = micros();
  mbedtls_ecdsa_sign(&grp, &r, &s, &d, hash, sizeof(hash), rng, NULL);
  unsigned long tFirma = micros() - t0;

  uint8_t rb[32], sb[32];
  mbedtls_mpi_write_binary(&r, rb, sizeof(rb));
  mbedtls_mpi_write_binary(&s, sb, sizeof(sb));
  Serial.printf("  %-16s%s\n", "Firmware:", firmware);
  printHex("SHA-256:", hash, sizeof(hash));
  printHex("Firma r:", rb, sizeof(rb));
  printHex("Firma s:", sb, sizeof(sb));

  // Dispositivo: verifica la firma con la clave publica antes de instalar
  t0 = micros();
  int ret = mbedtls_ecdsa_verify(&grp, hash, sizeof(hash), &Q, &r, &s);
  unsigned long tVerif = micros() - t0;
  Serial.printf("  Firmware original    -> %s\n",
                ret == 0 ? "FIRMA VALIDA, se instala" : "firma invalida, se rechaza");

  // Un atacante modifica el firmware pero no puede volver a firmarlo
  const char *firmwareMalicioso = "FIRMWARE v1.2.0 - control de valvula + backdoor";
  sha256((const uint8_t *)firmwareMalicioso, strlen(firmwareMalicioso), hash);
  ret = mbedtls_ecdsa_verify(&grp, hash, sizeof(hash), &Q, &r, &s);
  Serial.printf("  Firmware modificado  -> %s\n",
                ret == 0 ? "firma valida, se instala" : "FIRMA INVALIDA, se rechaza");

  Serial.printf("  Tiempo generar claves: %lu us, firmar: %lu us, verificar: %lu us\n\n",
                tClaves, tFirma, tVerif);

  mbedtls_ecp_point_free(&Q);
  mbedtls_mpi_free(&s);
  mbedtls_mpi_free(&r);
  mbedtls_mpi_free(&d);
  mbedtls_ecp_group_free(&grp);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nCriptografia en sistemas embebidos - ESP32 + mbedTLS\n");
  ejemploHash();
  ejemploAES();
  ejemploFirma();
  Serial.println("Fin de los ejemplos.");
}

void loop() {}
