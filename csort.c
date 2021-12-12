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
internal CSortToken
CSort_nexttoken(CSort* csort, const String_View* buf_view) {
    varPersist String_View tok_view = {0};
    char* tok_begin = SV_begin(*buf_view);

    for (char* c = tok_begin; c != SV_end(*buf_view); ++c) {
        switch (*c) {
            case ' ':
                if (c == tok_begin) {
                    return CSortToken_mk(tok_view, CSortTokenSpace);
                }
                tok_view = SV_slice(tok_begin, c);
                if (SV_isEq(tok_view, SV("from"))) {
                    return CSortToken_mk(tok_view, CSortTokenFrom);
                } else if (SV_isEq(tok_view, SV("import"))) {
                    return CSortToken_mk(tok_view, CSortTokenImport);
                } else {
                    return CSortToken_mk(tok_view, CSortTokenIdentifier);
                }

            case ',':
                tok_view = SV_buff(c, 1);
                return CSortToken_mk(tok_view, CSortTokenComma);

            case '#':
                tok_view = SV_buff(c, 1);
                return CSortToken_mk(tok_view, CSortTokenComment);

            case '\n': {
                if (c == tok_begin) {
                    return CSortToken_mk(tok_view, CSortTokenNewline);
                }
                tok_view = SV_slice(tok_begin, c);
                return CSortToken_mk(tok_view, CSortTokenIdentifier);
           }
        }
    }

    return CSortToken_mk(tok_view, CSortTokenNewline);
}



// --------------------------------------------------------------------------------------------
void
CSort_sortit(CSort* csort) {
    varPersist CSortToken tok = {0};
    varPersist char buf[512];

    if (! fgets(buf, 512, csort->file)) {
        CSort_panic(csort, "file is empty");
    }

    String_View buf_view = SV(buf);
    while (tok = CSort_nexttoken(csort, &buf_view), tok.type != CSortTokenEnd) {
        buf_view = SV_buff(SV_end(tok.tok_view), buf_view.len - tok.tok_view.len);

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

            case CSortTokenSpace:
                buf_view = SV_buff(SV_end(tok.tok_view) + 1, buf_view.len - tok.tok_view.len);
                break;


            case CSortTokenFrom:
                break;

            case CSortTokenImport:
                break;

            case CSortTokenIdentifier:
                buf_view = SV_buff(SV_end(tok.tok_view), buf_view.len - tok.tok_view.len);
                break;

            case CSortTokenComma:
                break;
        }
    }
}
