/*
  Xtea.cpp - Xtea encryption/decryption
  Written by Frank Kienast in November, 2010
  https://github.com/franksmicro/Arduino/tree/master/libraries/Xtea
*/
#include <stdint.h>
#include "Xtea.h"

#define NUM_ROUNDS 32

Xtea::Xtea(unsigned long key[4])
{
	_key[0] = key[0];
	_key[1] = key[1];
	_key[2] = key[2];
	_key[3] = key[3];
}

void Xtea::encrypt(unsigned long v[2]) 
{
    unsigned int i;
    unsigned long v0=v[0], v1=v[1], sum=0, delta=0x9E3779B9;
    
    for (i=0; i < NUM_ROUNDS; i++) 
    {
        v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + _key[sum & 3]);
        sum += delta;
        v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + _key[(sum>>11) & 3]);
    }
    
    v[0]=v0; v[1]=v1;
}
 
void Xtea::decrypt(unsigned long v[2]) 
{
    unsigned int i;
    uint32_t v0=v[0], v1=v[1], delta=0x9E3779B9, sum=delta*NUM_ROUNDS;
    
    for (i=0; i < NUM_ROUNDS; i++) 
    {
        v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + _key[(sum>>11) & 3]);
        sum -= delta;
        v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + _key[sum & 3]);
    }
    
    v[0]=v0; v[1]=v1;
}
 
