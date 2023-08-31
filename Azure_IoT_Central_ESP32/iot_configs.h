// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

// Wifi
#define IOT_CONFIG_WIFI_SSID "MOVISTAR_FD60"
#define IOT_CONFIG_WIFI_PASSWORD "IoTHub2024"

// Enable macro IOT_CONFIG_USE_X509_CERT to use an x509 certificate to authenticate the IoT device.
// The two main modes of authentication are through SAS tokens (automatically generated by the
// sample using the provided device symmetric key) or through x509 certificates. Please choose the
// appropriate option according to your device authentication mode.

// #define IOT_CONFIG_USE_X509_CERT

#ifdef IOT_CONFIG_USE_X509_CERT

/*
 * Please set the define IOT_CONFIG_DEVICE_CERT below with
 * the content of your device x509 certificate.
 *
 * Example:
 * #define IOT_CONFIG_DEVICE_CERT "-----BEGIN CERTIFICATE-----\r\n" \
 * "MIIBJDCBywIUfeHrebBVa2eZAbouBgACp9R3BncwCgYIKoZIzj0EAwIwETEPMA0G\r\n" \
 * "A1UEAwwGRFBTIENBMB4XDTIyMDMyMjazMTAzN1oXDTIzMDMyMjIzMTAzN1owGTEX\r\n" \
 * "MBUGA1UEAwwOY29udG9zby1kZXZpY2UwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNC\r\n" \
 * .......
 * "YmGzdaHTb6P1W+p+jmc+jJn1MAoGCXqGSM49BAMCA0gAMEUCIEnbEMsAdGFroMwl\r\n" \
 * "vTfQahwsxN3xink9z1gtirrjQlqDAiEAyU+6TUJcG6d9JF+uJqsLFpsbbF3IzGAw\r\n" \
 * "yC+koNRC0MU=\r\n" \
 * "-----END CERTIFICATE-----"
 *
 */
#define IOT_CONFIG_DEVICE_CERT "Device Certificate"

/*
 * Please set the define IOT_CONFIG_DEVICE_CERT_PRIVATE_KEY below with
 * the content of your device x509 private key.
 *
 * Example:
 *
 * #define IOT_CONFIG_DEVICE_CERT_PRIVATE_KEY "-----BEGIN EC PRIVATE KEY-----\r\n" \
 * "MHcCAQEEIKGXkMMiO9D7jYpUjUGTBn7gGzeKPeNzCP83wbfQfLd9obAoGCCqGSM49\r\n" \
 * "AwEHoUQDQgAEU6nQoYbjgJvBwaeD6MyAYmOSDg0QhEdyyV337qrlIbDEKvFsn1El\r\n" \
 * "yRabc4dNp2Jhs3Xh02+j9Vvqfo5nPoyZ9Q==\r\n" \
 * "-----END EC PRIVATE KEY-----"
 *
 * Note the type of key may different in your case. Such as BEGIN PRIVATE KEY
 * or BEGIN RSA PRIVATE KEY.
 *
 */
#define IOT_CONFIG_DEVICE_CERT_PRIVATE_KEY "Device Certificate Private Key"

#endif // IOT_CONFIG_USE_X509_CERT

// Azure IoT Central
#define DPS_ID_SCOPE "0ne00A56BD4"
//#define IOT_CONFIG_DEVICE_ID "sbdncr6r4n"
#define IOT_CONFIG_DEVICE_ID "12yvbq8k0uc"              //Aire comprimido
//#define IOT_CONFIG_DEVICE_ID "2a0u68cv1az"              //AC Oficinas (Generalpor conductos)
//#define IOT_CONFIG_DEVICE_ID "5sfzd28vxx"             //Transelevador 1
//#define IOT_CONFIG_DEVICE_ID "288cgdcx94a"            //Transelevador 2
//#define IOT_CONFIG_DEVICE_ID "fmlljozfb7"             //Transelevador 3
//#define IOT_CONFIG_DEVICE_ID "2h8tdztj53u"            //General
//#define IOT_CONFIG_DEVICE_ID "xcs3q36hrp"             //Robot
//#define IOT_CONFIG_DEVICE_ID "2pzlfhqaly8"            //Linea empaquetado
//#define IOT_CONFIG_DEVICE_ID "wxnbsetn9z"             //Modula 4
//#define IOT_CONFIG_DEVICE_ID "pukp1akinp"               //Modula 11

// Use device key if not using certificates
#ifndef IOT_CONFIG_USE_X509_CERT
//#define IOT_CONFIG_DEVICE_KEY "kyFGW9qWk38tmlv0hBoC0KbjZjyhwahrCiv9ETHCa/Q="          //OriolTest device
#define IOT_CONFIG_DEVICE_KEY "qbNgnq2WQ6ybPzFc1qAUPNIiRx9WW6Bhz2VpNhPZRfE="            //Aire comprimido
//#define IOT_CONFIG_DEVICE_KEY "4damq3m2Ji0HgMhc8bc3bf5QPJkq4TrCKTqDuaQxty0="            //AC Oficinas (Generalpor conductos)
//#define IOT_CONFIG_DEVICE_KEY "H2hG0fFfuj/t/32/2YA1ShMZ2fSjdISVxiuXvjYYDqc="            //Transelevador 1 device
//#define IOT_CONFIG_DEVICE_KEY "NGq9caVy8X5wO8pfSHr0QqSoe0sQ/PuthDqho+Ehmzc="            //Transelevador 2 device
//#define IOT_CONFIG_DEVICE_KEY "L0U/1ku7oj3nx1BmNvMape2Mtj0v/D0O1Q0RsAXIJHs="            //Transelevador 3 device
//#define IOT_CONFIG_DEVICE_KEY "joYDqQZ8uPY1PRgB/dLktnV+wWJ8yTivbtgH5URJZ3k="            //General
//#define IOT_CONFIG_DEVICE_KEY "m5DYhN9AEKIVruWnPbPzgbdBoT107fvqsH/Xt/x15Ts="            //Robot
//#define IOT_CONFIG_DEVICE_KEY "Oo1XTUeDeBZYkKKKvIEYMu0UuAuHB759tj8+zJ30508="            //Linea empaquetado
//#define IOT_CONFIG_DEVICE_KEY "o/UV1WxoPevozugeEft1TbYnFxpQ0ZbX4d9DJYjNKo8="            //Modula 4
//#define IOT_CONFIG_DEVICE_KEY "IccoryKXcTwS5Vv7rmoFJN7CuF+vXD1i67c4DdRodrI="            //Modula 11
#endif // IOT_CONFIG_USE_X509_CERT

// User-agent (url-encoded) provided by the MQTT client to Azure IoT Services.
// When developing for your own Arduino-based platform,
// please update the suffix with the format '(ard;<platform>)' as an url-encoded string.
#define AZURE_SDK_CLIENT_USER_AGENT "c%2F" AZ_SDK_VERSION_STRING "(ard%3Besp32)"

// Publish 1 message every 2 seconds.
#define TELEMETRY_FREQUENCY_IN_SECONDS 2

// For how long the MQTT password (SAS token) is valid, in minutes.
// After that, the sample automatically generates a new password and re-connects.
#define MQTT_PASSWORD_LIFETIME_IN_MINUTES 60