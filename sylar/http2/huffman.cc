#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "huffman.h"
#include "huffman_table.h"

#include <iostream>

#define HEX_TO_HF_CODE(hex)		huffman_codes[hex]
#define HEX_TO_HF_CODE_LEN(hex) huffman_code_len[hex]
#define SYLAR_MAX_HUFFMAN_BUFF_LEN(l) (l*8 + 1)

namespace sylar {
namespace http2 {

enum HM_RETURN{
    HM_RETURN_SUCCESS     = 0,
    HM_RETURN_UNIMPLEMENT = -100,
};

struct node{
	struct node *children[256];
	unsigned char sym;
	unsigned int code;
	int code_len;
	int size;
};

typedef struct node NODE;
extern NODE *ROOT;

int hf_init(NODE** h_node);
void hf_finish(NODE* h_node);
int hf_byte_encode(unsigned char ch, int remain, unsigned char *buff);
int hf_integer_encode(unsigned int enc_binary,int nprefix, unsigned char *buff);
int hf_integer_decode(const char *enc_buff, int nprefix , char *dec_buff);
int hf_string_encode(const char *buff_in, int size, int prefix, unsigned char *buff_out, int *size_out);
int hf_string_decode(NODE* h_node, unsigned char *enc, int enc_sz, char *out_buff, int out_sz);
void hf_print_hex(unsigned char *buff, int size);
int hf_string_encode_len(unsigned char *enc, int enc_sz);



static NODE * node_create(){
	NODE *nnode = (NODE*)malloc(1*sizeof(NODE));
	memset(nnode, 0, sizeof(NODE));
	return nnode;
}

static int _hf_add_node(NODE* h_node, unsigned char sym, int code, int code_len){
	NODE *cur		= h_node;
	unsigned char i	= 0;
	int j			= 0;
	int shift		= 0;
	int start		= 0;
	int end			= 0;
	
	for( ; code_len > 8 ; ){
		code_len -= 8;
		i = (unsigned char)(code >> code_len);
		if(cur->children[i] == NULL ){
			cur->children[i] = node_create();
		}
		cur = cur->children[i];
	}	

	shift = (8-code_len);
	start = (unsigned char)(code<<shift);
	end	  = (1 << shift);

	for(j = start; j < start+end ;j++){
		if( cur->children[j] == NULL){
			cur->children[j] = node_create();
		}
		cur->children[j]->code = code;
		cur->children[j]->sym = sym;
		cur->children[j]->code_len = code_len;
		cur->size++;
	}

	return 0;
}

static int _hf_del_node(NODE* h_node){
    if(h_node)
    {
        for(int i = 0; i < 256; i++)
        {
            if(h_node->children[i])
            {
                _hf_del_node(h_node->children[i]);
            }
        }
        free(h_node);
    }
	return 0;
}

int hf_init(NODE** h_node){
	int i   = 0;
	*h_node    = node_create();
	for(i = 0; i < 256; i++){
		_hf_add_node(*h_node, i, huffman_codes[i], huffman_code_len[i]);
	}
	return 0;
}

void hf_finish(NODE* h_node){
	int i   = 0;
	for(i = 0; i < 256; i++){
		_hf_del_node(h_node->children[i]);
	}
    
    free(h_node);
}

int hf_byte_encode(unsigned char ch, int remain, unsigned char *buff){
	unsigned char t = 0;
	int i			= 0;
	int codes		= HEX_TO_HF_CODE(ch);
	int nbits		= HEX_TO_HF_CODE_LEN(ch);
	//printf("'%c'|codes(%d)|len(%d)\n", ch, codes, nbits );
	for(;;){
		if( remain > nbits){
			t = (unsigned char)(codes << (remain-nbits));
			buff[i++] |= t;
			return remain-nbits;
		}else{
			t = (unsigned char )(codes >> (nbits-remain));
			buff[i++] |= t;
			nbits -= remain;
			remain = 8;
		}
		buff[i] = 0;
		if(nbits == 0) return remain;
	}

}

int hf_string_encode(const char *buff_in, int size, int prefix, unsigned char *buff_out, int *size_out){
	int i		= 0;
	int remain	= (8-prefix);
	int j		= 0;		  //j is instead currently index of buff_out and it is size of buff_out after it has been done.
	int nbytes  = 0;

	for(i = 0; i < size; i++){
		
		if( remain > HEX_TO_HF_CODE_LEN((uint8_t)buff_in[i]) ){
			nbytes = (remain - HEX_TO_HF_CODE_LEN((uint8_t)buff_in[i])) / 8;
		}else{
			nbytes = ((HEX_TO_HF_CODE_LEN((uint8_t)buff_in[i]) - remain) / 8)+1;
		}
		remain = hf_byte_encode( buff_in[i], remain, &buff_out[j]);
		j += nbytes;
	}

	// Special EOS sybol
	if( remain < 8 ){
		unsigned int codes = 0x3fffffff;
		int nbits = (char)30;
		buff_out[j++] |= (unsigned char )(codes >> (nbits-remain));
	}

	*size_out = j;
	return 0;
}

int hf_string_decode(NODE* h_node, unsigned char *enc, int enc_sz, char *out_buff, int out_sz){
	NODE *n			  = h_node;
	unsigned int cur  = 0;
	int	nbits		  = 0;
	int i			  = 0;	
	int idx			  = 0;
	int at			  = 0;
	for(i=0; i < enc_sz ; i++){
		cur = (cur<<8)|enc[i];
		nbits += 8;
		for( ;nbits >= 8; ){
			idx = (unsigned char)(cur >> (nbits-8));
			n = n->children[idx];
			if( n == NULL ){
				printf("invalid huffmand code\n");
				return -1; //invalid huffmand code
			}
			//printf("n->sym : %c , n->size = %d\n", n->sym, n->size);	
			//if( n->children == NULL){
			if( n->size == 0){
				if( out_sz > 0 && at > out_sz){
					printf("out of length\n");
					return -2; // lenght out of bound
				}
				out_buff[at++] = (char) n->sym; 
				nbits -= n->code_len;
				n = h_node;
			}else{
				nbits -= 8;
			}
		}
	}

	for( ;nbits > 0; ){
		n = n->children[ (unsigned char)( cur<<(8-nbits) ) ];
		if( n->size != 0 || n->code_len  > nbits){
			break;
		}

		out_buff[at++] = (char) n->sym;
		nbits -= n->code_len;
		n = h_node;
	}

	return at;
}

int hf_integer_encode(unsigned int enc_binary, int nprefix, unsigned char *buff){
	int i = 0;
	unsigned int ch	    = enc_binary;
	unsigned int ch2    = 0;
	unsigned int prefix = (1 << nprefix) - 1;

    if( ch < prefix  && (ch < 0xff) ){
		buff[i++] = ch & prefix;
	}else{
		buff[i++] = prefix;
		ch -= prefix;
		while(ch > 128)
		{
			ch2 = (ch % 128);
			ch2 += 128;
			buff[i++] = ch2;
			ch = ch/128;
		}
		buff[i++] = ch;
	}
	return i;
}

int hf_integer_decode(const char *enc_buff, int nprefix , char *dec_buff){
	int i				= 0;
	int j				= 0;
	unsigned int M		= 0;
	unsigned int B		= 0;
	unsigned int ch	    = enc_buff[i++];
	unsigned int prefix = (1 << nprefix) - 1;

	if( ch < prefix ){
		dec_buff[j++] = ch;
	}else{
		M = 0;
		do{
			B = enc_buff[i++];
			ch = ch + ((B & 127) * (1 << M));
			M = M + 7;
		}
		while(B & 128);
		dec_buff[j] = ch;
	}
	return i;
}

int hf_string_encode_len(unsigned char *enc, int enc_sz){
    int i       = 0;
    int len     = 0;
    for(i = 0; i < enc_sz; i++){
        len += huffman_code_len[(int)enc[i]];
    }
    
    return (len+7)/8;
}
    
void hf_print_hex(unsigned char *buff, int size){
	static char hex[] = {'0','1','2','3','4','5','6','7',
								'8','9','a','b','c','d','e','f'};
	int i = 0;
	for( i = 0;i < size; i++){
		unsigned char ch = buff[i];
		printf("(%u)%c", ch,hex[(ch>>4)]);
		printf("%c ", hex[(ch&0x0f)]);
	}
	printf("\n");
}

int Huffman::EncodeString(const std::string& in, std::string& out, int prefix) {
    return EncodeString(in.c_str(), in.length(), out, prefix);
}

int Huffman::EncodeString(const char* in, int in_len, std::string& out, int prefix) {
    int len = SYLAR_MAX_HUFFMAN_BUFF_LEN(in_len);
    out.resize(len);
    int rt = hf_string_encode(in, in_len, prefix, (unsigned char*)&out[0], &len);
    out.resize(len);
    return rt;
}

int Huffman::DecodeString(const std::string& in, std::string& out) {
    return DecodeString(in.c_str(), in.length(), out);
}

int Huffman::DecodeString(const char* in, int in_len, std::string& out) {
    NODE* h_node;
    hf_init(&h_node);
    int len = SYLAR_MAX_HUFFMAN_BUFF_LEN(in_len);
    out.resize(len);
    int rt = hf_string_decode(h_node, (unsigned char*)in, in_len, &out[0], len);
    hf_finish(h_node);
    out.resize(rt);
    return rt;
}

int Huffman::EncodeLen(const std::string& in) {
    return EncodeLen(in.c_str(), in.length());
}
int Huffman::EncodeLen(const char* in, int in_len) {
    return hf_string_encode_len((uint8_t*)in, in_len);
}

bool Huffman::ShouldEncode(const std::string& in) {
    return ShouldEncode(in.c_str(), in.length());
}

bool Huffman::ShouldEncode(const char* in, int in_len) {
    return EncodeLen(in, in_len) < in_len;
}

//void testHuffman() {
//    std::string str = "hello huffman,你好,世界";
//    std::string out;
//    Huffman::EncodeString(str, out, 0);
//    std::cout << "str.size=" << str.size()
//              << " out.size=" << out.size()
//              << std::endl;
//    hf_print_hex((unsigned char*)out.c_str(), out.length());
//    std::string str2;
//    Huffman::DecodeString(out, str2);
//    std::cout << str2 << std::endl;
//    std::cout << "str.size=" << str.size()
//              << " out.size=" << out.size() << std::endl;
//
//    std::cout << "hf_string_encode_len: " << hf_string_encode_len((uint8_t*)str.c_str(), str.size()) << std::endl;
//}

}
}
