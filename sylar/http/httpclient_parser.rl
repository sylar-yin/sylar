/**
 *
 * Copyright (c) 2010, Zed A. Shaw and Mongrel2 Project Contributors.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 *     * Neither the name of the Mongrel2 Project, Zed A. Shaw, nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "httpclient_parser.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <cerrno>
//#include "dbg.h"

#define LEN(AT, FPC) (FPC - buffer - parser->AT)
#define MARK(M,FPC) (parser->M = (FPC) - buffer)
#define PTR_TO(F) (buffer + parser->F)
#define check(A, M, ...) if(!(A)) { /*log_err(M, ##__VA_ARGS__);*/ errno=0; goto error; }


/** machine **/
%%{
    machine httpclient_parser;

    action mark {MARK(mark, fpc); }

    action start_field { MARK(field_start, fpc); }

    action write_field { 
        parser->field_len = LEN(field_start, fpc);
    }

    action start_value { MARK(mark, fpc); }

    action write_content_len { 
        parser->content_len = strtol(PTR_TO(mark), NULL, 10);
    }

    action write_connection_close {
        parser->close = 1;
    }

    action write_value { 
        if(parser->http_field != NULL) {
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, fpc));
        }
    }

    action reason_phrase { 
        if(parser->reason_phrase != NULL)
            parser->reason_phrase(parser->data, PTR_TO(mark), LEN(mark, fpc));
    }

    action status_code { 
        parser->status = strtol(PTR_TO(mark), NULL, 10);

        if(parser->status_code != NULL)
            parser->status_code(parser->data, PTR_TO(mark), LEN(mark, fpc));
    }

    action http_version {	
        if(parser->http_version != NULL)
            parser->http_version(parser->data, PTR_TO(mark), LEN(mark, fpc));
    }

    action chunk_size {
        parser->chunked = 1;
        parser->content_len = strtol(PTR_TO(mark), NULL, 16);
        parser->chunks_done = parser->content_len <= 0;

        if(parser->chunks_done && parser->last_chunk) {
            parser->last_chunk(parser->data, PTR_TO(mark), LEN(mark, fpc));
        } else if(parser->chunk_size != NULL) {
            parser->chunk_size(parser->data, PTR_TO(mark), LEN(mark, fpc));
        } // else skip it
    }

    action trans_chunked {
        parser->chunked = 1;
    }

    action done { 
        parser->body_start = fpc - buffer + 1; 
        if(parser->header_done != NULL)
            parser->header_done(parser->data, fpc + 1, pe - fpc - 1);
        fbreak;
    }

# line endings
    CRLF = ("\r\n" | "\n");

# character types
    CTL = (cntrl | 127);
    tspecials = ("(" | ")" | "<" | ">" | "@" | "," | ";" | ":" | "\\" | "\"" | "/" | "[" | "]" | "?" | "=" | "{" | "}" | " " | "\t");

# elements
    token = (ascii -- (CTL | tspecials));

    Reason_Phrase = (any -- CRLF)+ >mark %reason_phrase;
    Status_Code = digit+ >mark %status_code;
    http_number = (digit+ "." digit+) ;
    HTTP_Version = ("HTTP/" http_number) >mark %http_version ;
    Status_Line = HTTP_Version " " Status_Code " " Reason_Phrase :> CRLF;

    field_name = token+ >start_field %write_field;
    field_value = any* >start_value %write_value;
    fields = field_name ":" space* field_value :> CRLF;

    content_length = (/Content-Length/i >start_field %write_field ":" space *
            digit+ >start_value %write_content_len %write_value) CRLF;

    conn_close = (/Connection/i ":" space* /close/i) CRLF %write_connection_close;

    transfer_encoding_chunked = (/Transfer-Encoding/i >start_field %write_field
            ":" space* /chunked/i >start_value %write_value) CRLF @trans_chunked;

    message_header = transfer_encoding_chunked | conn_close | content_length | fields;

    Response = 	Status_Line (message_header)* CRLF;

    chunk_ext_val = token+;
    chunk_ext_name = token+;
    chunk_extension = (";" chunk_ext_name >start_field %write_field %start_value ("=" chunk_ext_val >start_value)? %write_value )*;
    chunk_size = xdigit+;
    Chunked_Header = chunk_size >mark %chunk_size chunk_extension :> CRLF;

main := (Response | Chunked_Header) @done;
}%%

/** Data **/
%% write data;

int httpclient_parser_init(httpclient_parser *parser)  {
    int cs = 0;

    %% write init;

    parser->cs = cs;
    parser->body_start = 0;
    parser->content_len = -1;
    parser->chunked = 0;
    parser->chunks_done = 0;
    parser->mark = 0;
    parser->nread = 0;
    parser->field_len = 0;
    parser->field_start = 0;    
    parser->close = 0;

    return(1);
}


/** exec **/
int httpclient_parser_execute(httpclient_parser *parser, const char *buffer, size_t len, size_t off)  
{
    const char *p, *pe;
    int cs = parser->cs;

    assert(off <= len && "offset past end of buffer");

    p = buffer+off;
    pe = buffer+len;

    assert(*pe == '\0' && "pointer does not end on NUL");
    assert(pe - p == (int)len - (int)off && "pointers aren't same distance");


    %% write exec;

    parser->cs = cs;
    parser->nread += p - (buffer + off);

    assert(p <= pe && "buffer overflow after parsing execute");
    assert(parser->nread <= len && "nread longer than length");
    assert(parser->body_start <= len && "body starts after buffer end");
    check(parser->mark < len, "mark is after buffer end");
    check(parser->field_len <= len, "field has length longer than whole buffer");
    check(parser->field_start < len, "field starts after buffer end");

    //if(parser->body_start) {
    //    /* final \r\n combo encountered so stop right here */
    //    parser->nread++;
    //}

    return(parser->nread);

error:
    return -1;
}

int httpclient_parser_finish(httpclient_parser *parser)
{
    int cs = parser->cs;

    parser->cs = cs;

    if (httpclient_parser_has_error(parser) ) {
        return -1;
    } else if (httpclient_parser_is_finished(parser) ) {
        return 1;
    } else {
        return 0;
    }
}

int httpclient_parser_has_error(httpclient_parser *parser) {
    return parser->cs == httpclient_parser_error;
}

int httpclient_parser_is_finished(httpclient_parser *parser) {
    return parser->cs == httpclient_parser_first_final;
}

