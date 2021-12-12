#include "csort.h"


inline CSort
CSort_mk(const String_View fileName, FILE* file) {
    return (CSort) {
        .fileName = fileName,
        .file = file,
        .arena = CSortMemArena_mk(),
    };
}


inline void
CSort_free(CSort* csort) {
    CSortMemArena_free(&(csort->arena));
    fclose(csort->file);
}


inline void
CSort_panic(CSort* csort, const char* msg) {
    eprintln("panic: %s", msg);
    CSort_free(csort);
    exit(1);
}



// --------------------------------------------------------------------------------------------
internal inline enum CSortTokenType
CSort_gettokentype(const String_View* tok_view) {
    if (SV_isEq(*tok_view, SV("from"))) {
        return CSortTokenFrom;
    } else if (SV_isEq(*tok_view, SV("import"))) {
        return CSortTokenImport;
    }
    return CSortTokenIdentifier;
}


internal CSortToken
CSort_nexttoken(CSort* csort, const String_View* buf_view) {
    varPersist String_View tok_view = {0};
    varPersist enum CSortTokenType tok_type;
    char* tok_begin = SV_begin(*buf_view);
    char* tok_end = SV_end(*buf_view);

    tok_begin = str_findFirstNotOf(tok_begin, tok_end, ' ');

    for (char* c = tok_begin; c != tok_end; ++c) {
        switch (*c) {
            case ' ': {
                tok_view = SV_slice(tok_begin, c);
                char* new_c = str_findFirstNotOf(c, tok_end, ' ');
                tok_type = CSort_gettokentype(&tok_view);
                return CSortToken_mk(tok_view, tok_type, new_c - c);
            }
            break;

            case ',': {
                if (c == tok_begin) {
                    tok_view = SV_buff(c, 1);
                    return CSortToken_mk(tok_view, CSortTokenComma, 0);
                }
                tok_view = SV_slice(tok_begin, c);
                tok_type = CSort_gettokentype(&tok_view);
                return CSortToken_mk(tok_view, tok_type, 0);
            }
            break;

            case '#':
                tok_view = SV_buff(c, 1);
                return CSortToken_mk(tok_view, CSortTokenComment, 0);

            case '\n': {
                if (c == tok_begin) {
                    return CSortToken_mk(tok_view, CSortTokenNewline, 0);
                }
                tok_view = SV_slice(tok_begin, c);
                tok_type = CSort_gettokentype(&tok_view);
                return CSortToken_mk(tok_view, tok_type, 0);
           }
        }
    }

    return CSortToken_mk(tok_view, CSortTokenNewline, 0);
}



// --------------------------------------------------------------------------------------------
internal inline String_View
CSort_inc_buff(const String_View* prev_buff, const CSortToken* tok) {
    return SV_buff(SV_end(tok->tok_view) + tok->next_tok_offset, prev_buff->len - SV_len(tok->tok_view) - tok->next_tok_offset);
}


void
CSort_sortit(CSort* csort) {
    varPersist CSortToken tok = {0};
    varPersist char buf[512];

    if (! fgets(buf, 512, csort->file)) {
        CSort_panic(csort, "file is empty");
    }

    String_View buf_view = SV(buf);
    while (tok = CSort_nexttoken(csort, &buf_view), tok.type != CSortTokenEnd) {
        buf_view = CSort_inc_buff(&buf_view, &tok);

        switch (tok.type) {
            case CSortTokenComment:
                if (! fgets(buf, 512, csort->file)) {
                    return;
                }
                buf_view = SV(buf);
                break;

            case CSortTokenNewline:
                if (! fgets(buf, 512, csort->file)) {
                    return;
                }
                buf_view = SV(buf);
                break;

            case CSortTokenFrom: {
                    /*tok = CSort_nexttoken(csort, &buf_view);*/
                    println("TOK: %s, LEN: %d", tok.tok_view.data, tok.tok_view.len);
                } 
                break;

            case CSortTokenImport:
                println("TOK: %s, LEN: %d", tok.tok_view.data, tok.tok_view.len);
                break;

            case CSortTokenIdentifier:
                println("TOK: %*s, LEN: %d",  tok.tok_view.len, tok.tok_view.data, tok.tok_view.len);
                break;

            case CSortTokenComma:
                break;
        }
    }
}
