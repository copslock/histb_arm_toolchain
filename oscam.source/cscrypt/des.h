#ifndef CSCRYPT_DES_H_
#define CSCRYPT_DES_H_

#ifdef  __cplusplus
extern "C" {
#endif

#define DES_IP              1
#define DES_IP_1            2
#define DES_RIGHT           4
#define DES_HASH            8

#define DES_ECM_CRYPT       0
#define DES_ECM_HASH        DES_HASH
#define DES_ECS2_DECRYPT    (DES_IP | DES_IP_1 | DES_RIGHT)
#define DES_ECS2_CRYPT      (DES_IP | DES_IP_1)

extern int  des_encrypt(unsigned char *buffer, int len, unsigned char *deskey);
extern int  des_decrypt(unsigned char *buffer, int len, unsigned char *deskey);
extern unsigned char *des_login_key_get(unsigned char *key1, unsigned char *key2, int len, unsigned char *des16);
extern void doPC1(unsigned char data[]);
extern void des(unsigned char key[], unsigned char mode, unsigned char data[]);
// sky(n)
extern void DES_seka(unsigned char key[], unsigned char mode, unsigned char data[]);
extern void des_cbc_encrypt(unsigned char *data, const unsigned char *iv, const unsigned char *okey, int len);
extern void des_cbc_decrypt(unsigned char *data, unsigned char *iv, const unsigned char *okey, int len);

#ifdef  __cplusplus
}
#endif

#endif
