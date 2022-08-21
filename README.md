# Multi-Processes Servers and Clients

In cryptography, the one-time pad (OTP) is an encryption technique that cannot be cracked, but requires the use of a single-use pre-shared key that is not smaller than the message being sent. In this technique, a plaintext is paired with a random secret key (also referred to as a one-time pad). Then, each bit or character of the plaintext is encrypted by combining it with the corresponding bit or character from the pad using modular addition. (https://en.wikipedia.org/wiki/One-time_pad)

This repo includes a random key generator, an encrypt-server, an encrypt-client, a decrypt-server and a decrypt-client to achieve the OTP encryption system described above. The servers can handle multiple clients simultaneously by creating multiple processes for different connection request from clients. 
