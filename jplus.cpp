#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>

// ===== ENCODER =====

// MOVZ Xd, #imm  (imm ≤ 16 bit)
uint32_t encode_mov(int rd, int imm) {
    uint32_t inst = 0;
    inst |= (1)    << 31;
    inst |= (2)    << 29;
    inst |= (0x25) << 23;
    inst |= ((imm) & 0xFFFF) << 5;
    inst |= (rd)   << 0;
    return inst;
}

// MOV Xd, Xn  (ORR Xd, XZR, Xn)
uint32_t encode_mov_reg(int rd, int rn) {
    // ORR Xd, XZR, Xn  — base 0xAA0003E0 รองรับ xzr(31) ที่ Rn field
    uint32_t inst = 0xAA0003E0;
    inst |= ((rn & 0x1F) << 16);
    inst |= (rd & 0x1F);
    return inst;
}

// ADD Xd, Xn, Xm
uint32_t encode_add(int rd, int rn, int rm) {
    uint32_t inst = 0;
    inst |= (1)   << 31;
    inst |= (0xB) << 24;
    inst |= (rm)  << 16;
    inst |= (rn)  << 5;
    inst |= (rd)  << 0;
    return inst;
}

// SUB Xd, Xn, Xm
uint32_t encode_sub(int rd, int rn, int rm) {
    uint32_t inst = 0;
    inst |= (1)   << 31;
    inst |= (2)   << 29;
    inst |= (0xB) << 24;
    inst |= (rm)  << 16;
    inst |= (rn)  << 5;
    inst |= (rd)  << 0;
    return inst;
}

// STR Xt, [Xn, #offset]   — unsigned offset, offset ต้องหารด้วย 8 ลงตัว
uint32_t encode_str(int rt, int rn, int offset) {
    uint32_t uimm12 = (offset / 8) & 0xFFF;
    uint32_t inst   = 0;
    inst |= (0xF9) << 24;
    inst |= (0)    << 22;          // L=0 → store
    inst |= uimm12 << 10;
    inst |= (rn)   << 5;
    inst |= (rt)   << 0;
    return inst;
}

// LDR Xt, [Xn, #offset]   — unsigned offset, offset ต้องหารด้วย 8 ลงตัว
uint32_t encode_ldr(int rt, int rn, int offset) {
    uint32_t uimm12 = (offset / 8) & 0xFFF;
    uint32_t inst   = 0;
    inst |= (0xF9) << 24;
    inst |= (1)    << 22;          // L=1 → load
    inst |= uimm12 << 10;
    inst |= (rn)   << 5;
    inst |= (rt)   << 0;
    return inst;
}

// SVC #imm
uint32_t encode_svc(int imm) {
    uint32_t inst = 0;
    inst |= (0xD4) << 24;
    inst |= (imm)  << 5;
    inst |= (1)    << 0;
    return inst;
}

// ADR Xd, #offset  (PC-relative, ±1MB)
uint32_t encode_adr(int rd, int offset) {
    uint32_t inst  = 0;
    uint32_t immlo = offset & 0x3;
    uint32_t immhi = (offset >> 2) & 0x7FFFF;
    inst |= (0)     << 31;
    inst |= (immlo) << 29;
    inst |= (0x10)  << 24;
    inst |= (immhi) << 5;
    inst |= (rd)    << 0;
    return inst;
}

// ADD Xd, Xn, #imm12  (immediate)
uint32_t encode_add_imm(int rd, int rn, int imm) {
    // 0x91000000 | (imm12 << 10) | (rn << 5) | rd
    return 0x91000000 | ((imm & 0xFFF) << 10) | (rn << 5) | rd;
}

// SUB Xd, Xn, #imm12  (immediate)
uint32_t encode_sub_imm(int rd, int rn, int imm) {
    // 0xD1000000 | (imm12 << 10) | (rn << 5) | rd
    return 0xD1000000 | ((imm & 0xFFF) << 10) | (rn << 5) | rd;
}

// STRB Wt, [Xn]   — byte store, offset=0
uint32_t encode_strb(int rt, int rn) {
    // 0x39000000 | (0<<10) | (rn<<5) | rt
    return 0x39000000 | (rn << 5) | rt;
}

// ===== ERROR REPORTING =====

int get_line(const char* src, int pos) {
    int line = 1;
    for (int i = 0; i < pos; i++)
        if (src[i] == '\n') line++;
    return line;
}

void error(const char* src, int pos, const char* msg) {
    int line = get_line(src, pos);
    printf("\n╔══ [jplus ERROR] ══════════════════════╗\n");
    printf("║  บรรทัด %-3d: %s\n", line, msg);
    printf("╚═══════════════════════════════════════╝\n");

    // แสดงบรรทัดที่ผิด
    int start = pos;
    while (start > 0 && src[start-1] != '\n') start--;
    int end = pos;
    while (src[end] != '\0' && src[end] != '\n') end++;

    printf("  %3d | ", line);
    for (int i = start; i < end; i++) printf("%c", src[i]);
    printf("\n       | ");
    for (int i = start; i < pos; i++) printf(" ");
    printf("^~~~\n\n");

    // hint
    if (strstr(msg, "ลืมใส่ ;")) {
        printf("  💡 hint: เพิ่ม ; ที่ท้ายบรรทัดนี้\n\n");
    } else if (strstr(msg, "ลืมปิด }")) {
        printf("  💡 hint: เพิ่ม } เพื่อปิด block\n\n");
    } else if (strstr(msg, "ไม่ได้ประกาศ")) {
        printf("  💡 hint: ประกาศตัวแปรก่อนใช้ เช่น int x = 0;\n\n");
    } else if (strstr(msg, "{")) {
        printf("  💡 hint: function ต้องมี { } เช่น int main {\n\n");
    }

    exit(1);
}

void warn(const char* src, int pos, const char* msg) {
    int line = get_line(src, pos);
    printf("\n⚠  [jplus WARN] บรรทัด %d: %s\n", line, msg);
}

// ===== LEXER =====

enum TokenType {
    TOK_TYPE,     // int
    TOK_NAME,     // ชื่อตัวแปร
    TOK_NUM,      // ตัวเลข
    TOK_EQ,       // =
    TOK_SEMI,     // ;
    TOK_PLUS,     // +
    TOK_MINUS,    // -
    TOK_STAR,     // *
    TOK_SLASH,    // /
    TOK_LBRACE,   // {
    TOK_RBRACE,   // }
    TOK_RETURN,   // return
    TOK_INCLUDE,  // #include
    TOK_FILENAME, // <jst.p>
    TOK_MAIN,     // main
    TOK_EOF,      // จบไฟล์
    TOK_LSHIFT,   // <<
    TOK_RSHIFT,   // >>
    TOK_COUNTJ,   // countj
    TOK_ENDJ,     // endj
    TOK_STRING,   // "..."
    TOK_LPAREN,   // (
    TOK_RPAREN,   // )
    TOK_IF,       // if
    TOK_ELSE,     // else
    TOK_LOOP,     // loop
    TOK_WHILE,    // while
    TOK_FROM,     // from
    TOK_TO,       // to (ใช้ใน loop i from 0 >> 10)
    TOK_TRUE,     // true
    TOK_BREAK,    // break
    TOK_CHAR,     // char
    TOK_ASSIGN,   // x = expr (assignment ไม่ใช่ declare)
    TOK_EQEQ,     // ==
    TOK_NEQ,      // !=
    TOK_LT,       // <
    TOK_GT,       // >
    TOK_LTE,      // <=
    TOK_GTE,      // >=
    TOK_COMMA,    // ,
    TOK_FTYPE,    // f32 / f64
    TOK_FNUM,     // ตัวเลขทศนิยม เช่น 3.14
};

struct Token {
    TokenType type;
    char      value[256];
    int       pos;
};

struct Lexer {
    const char* src;
    int         pos;
};

Token next_token(Lexer* lex) {
    Token tok;
    tok.value[0] = '\0';

    // ข้าม whitespace และ comment //
    for (;;) {
        while (lex->src[lex->pos] == ' '  ||
               lex->src[lex->pos] == '\n' ||
               lex->src[lex->pos] == '\r' ||
               lex->src[lex->pos] == '\t') {
            lex->pos++;
        }
        // single-line comment
        if (lex->src[lex->pos] == '/' && lex->src[lex->pos+1] == '/') {
            while (lex->src[lex->pos] != '\n' && lex->src[lex->pos] != '\0')
                lex->pos++;
            continue;
        }
        break;
    }

    tok.pos = lex->pos;
    char c  = lex->src[lex->pos];

    if (c == '\0') { tok.type = TOK_EOF; return tok; }

    // string "..."
    if (c == '"') {
        lex->pos++;
        int i = 0;
        while (lex->src[lex->pos] != '"' && lex->src[lex->pos] != '\0') {
            // handle \n escape
            if (lex->src[lex->pos] == '\\' && lex->src[lex->pos+1] == 'n') {
                tok.value[i++] = '\n';
                lex->pos += 2;
            } else {
                tok.value[i++] = lex->src[lex->pos++];
            }
        }
        tok.value[i] = '\0';
        if (lex->src[lex->pos] == '"') lex->pos++;
        tok.type = TOK_STRING;
        return tok;
    }

    // #include
    if (c == '#') {
        // อ่านจนจบคำว่า #include
        int i = 0;
        while (lex->src[lex->pos] != ' ' && lex->src[lex->pos] != '\t' &&
               lex->src[lex->pos] != '\0' && lex->src[lex->pos] != '\n') {
            tok.value[i++] = lex->src[lex->pos++];
        }
        tok.value[i] = '\0';
        tok.type = TOK_INCLUDE;
        return tok;
    }

    // << หรือ <= หรือ < หรือ <filename>
    if (c == '<') {
        if (lex->src[lex->pos+1] == '<') {
            lex->pos += 2;
            tok.type = TOK_LSHIFT;
            strcpy(tok.value, "<<");
            return tok;
        }
        if (lex->src[lex->pos+1] == '=') {
            lex->pos += 2;
            tok.type = TOK_LTE;
            strcpy(tok.value, "<=");
            return tok;
        }
        // <filename>
        lex->pos++;
        int i = 0;
        while (lex->src[lex->pos] != '>' && lex->src[lex->pos] != '\0') {
            tok.value[i++] = lex->src[lex->pos++];
        }
        tok.value[i] = '\0';
        if (lex->src[lex->pos] == '>') lex->pos++;
        tok.type = TOK_FILENAME;
        return tok;
    }

    // >> หรือ >=
    if (c == '>') {
        if (lex->src[lex->pos+1] == '>') {
            lex->pos += 2;
            tok.type = TOK_RSHIFT;
            strcpy(tok.value, ">>");
            return tok;
        }
        if (lex->src[lex->pos+1] == '=') {
            lex->pos += 2;
            tok.type = TOK_GTE;
            strcpy(tok.value, ">=");
            return tok;
        }
        lex->pos++;
        tok.type = TOK_GT;
        strcpy(tok.value, ">");
        return tok;
    }

    // == หรือ =
    if (c == '=') {
        if (lex->src[lex->pos+1] == '=') {
            lex->pos += 2;
            tok.type = TOK_EQEQ;
            strcpy(tok.value, "==");
            return tok;
        }
        lex->pos++;
        tok.type = TOK_EQ;
        strcpy(tok.value, "=");
        return tok;
    }

    // !=
    if (c == '!' && lex->src[lex->pos+1] == '=') {
        lex->pos += 2;
        tok.type = TOK_NEQ;
        strcpy(tok.value, "!=");
        return tok;
    }

    // ตัวเลข (int หรือ float)
    if (isdigit(c)) {
        int i = 0;
        while (isdigit(lex->src[lex->pos])) {
            tok.value[i++] = lex->src[lex->pos++];
        }
        if (lex->src[lex->pos] == '.' && isdigit(lex->src[lex->pos+1])) {
            tok.value[i++] = lex->src[lex->pos++];  // '.'
            while (isdigit(lex->src[lex->pos])) {
                tok.value[i++] = lex->src[lex->pos++];
            }
            tok.value[i] = '\0';
            tok.type = TOK_FNUM;
        } else {
            tok.value[i] = '\0';
            tok.type = TOK_NUM;
        }
        return tok;
    }

    // คำ keyword หรือ name
    if (isalpha(c) || c == '_') {
        int i = 0;
        while (isalpha(lex->src[lex->pos]) ||
               isdigit(lex->src[lex->pos]) ||
               lex->src[lex->pos] == '_') {
            tok.value[i++] = lex->src[lex->pos++];
        }
        tok.value[i] = '\0';

        if      (strcmp(tok.value, "int")    == 0) tok.type = TOK_TYPE;
        else if (strcmp(tok.value, "f32")    == 0) tok.type = TOK_FTYPE;
        else if (strcmp(tok.value, "f64")    == 0) tok.type = TOK_FTYPE;
        else if (strcmp(tok.value, "return") == 0) tok.type = TOK_RETURN;
        else if (strcmp(tok.value, "main")   == 0) tok.type = TOK_MAIN;
        else if (strcmp(tok.value, "countj") == 0) tok.type = TOK_COUNTJ;
        else if (strcmp(tok.value, "endj")   == 0) tok.type = TOK_ENDJ;
        else if (strcmp(tok.value, "if")     == 0) tok.type = TOK_IF;
        else if (strcmp(tok.value, "else")   == 0) tok.type = TOK_ELSE;
        else if (strcmp(tok.value, "loop")   == 0) tok.type = TOK_LOOP;
        else if (strcmp(tok.value, "while")  == 0) tok.type = TOK_WHILE;
        else if (strcmp(tok.value, "from")   == 0) tok.type = TOK_FROM;
        else if (strcmp(tok.value, "to")     == 0) tok.type = TOK_TO;
        else if (strcmp(tok.value, "true")   == 0) tok.type = TOK_TRUE;
        else if (strcmp(tok.value, "break")  == 0) tok.type = TOK_BREAK;
        else if (strcmp(tok.value, "char")   == 0) tok.type = TOK_CHAR;
        else                                        tok.type = TOK_NAME;
        return tok;
    }

    // , ( )
    if (c == ',') { lex->pos++; tok.type = TOK_COMMA;  strcpy(tok.value, ",");  return tok; }
    if (c == '(') { lex->pos++; tok.type = TOK_LPAREN; strcpy(tok.value, "(");  return tok; }
    if (c == ')') { lex->pos++; tok.type = TOK_RPAREN; strcpy(tok.value, ")");  return tok; }
    if (c == '{') { lex->pos++; tok.type = TOK_LBRACE; strcpy(tok.value, "{");  return tok; }
    if (c == '}') { lex->pos++; tok.type = TOK_RBRACE; strcpy(tok.value, "}");  return tok; }
    if (c == ';') { lex->pos++; tok.type = TOK_SEMI;   strcpy(tok.value, ";");  return tok; }
    if (c == '+') { lex->pos++; tok.type = TOK_PLUS;   strcpy(tok.value, "+");  return tok; }
    if (c == '-') { lex->pos++; tok.type = TOK_MINUS;  strcpy(tok.value, "-");  return tok; }
    if (c == '*') { lex->pos++; tok.type = TOK_STAR;   strcpy(tok.value, "*");  return tok; }
    if (c == '/') { lex->pos++; tok.type = TOK_SLASH;  strcpy(tok.value, "/");  return tok; }

    // ไม่รู้จัก — ข้ามไป
    lex->pos++;
    tok.type     = TOK_EOF;
    tok.value[0] = c;
    tok.value[1] = '\0';
    return tok;
}

// ===== AST =====

struct Node {
    char  type[32];
    char  value[256];
    Node* left;
    Node* right;
    Node* next;   // linked-list statements
};

Node* make_node(const char* type, const char* value) {
    Node* n = (Node*)malloc(sizeof(Node));
    strcpy(n->type,  type);
    strcpy(n->value, value);
    n->left  = NULL;
    n->right = NULL;
    n->next  = NULL;
    return n;
}

// ===== PARSER =====

struct Parser {
    Lexer*      lex;
    Token       cur;
    const char* src;
};

void advance(Parser* p) {
    p->cur = next_token(p->lex);
}

void expect_semi(Parser* p) {
    if (p->cur.type != TOK_SEMI) {
        char msg[128];
        sprintf(msg, "ลืมใส่ ; หลัง '%s'", p->cur.value);
        error(p->src, p->cur.pos, msg);
    }
    advance(p);
}

Node* parse_expr(Parser* p);
Node* parse_stmt(Parser* p);

// primary: NUM | NAME | ( expr )
Node* parse_primary(Parser* p) {
    if (p->cur.type == TOK_NUM) {
        Node* n = make_node("NUM", p->cur.value);
        advance(p);
        return n;
    }
    if (p->cur.type == TOK_FNUM) {
        Node* n = make_node("FNUM", p->cur.value);
        advance(p);
        return n;
    }
    if (p->cur.type == TOK_NAME || p->cur.type == TOK_MAIN) {
        Node* n = make_node("NAME", p->cur.value);
        advance(p);
        return n;
    }
    if (p->cur.type == TOK_LPAREN) {
        advance(p);
        Node* n = parse_expr(p);
        if (p->cur.type == TOK_RPAREN) advance(p);
        return n;
    }
    char msg[128];
    sprintf(msg, "คาดว่าจะเป็นตัวเลขหรือชื่อตัวแปร แต่เจอ '%s'", p->cur.value);
    error(p->src, p->cur.pos, msg);
    return NULL;
}

// term: primary (( * | / ) primary)*
Node* parse_term(Parser* p) {
    Node* left = parse_primary(p);

    while (p->cur.type == TOK_STAR || p->cur.type == TOK_SLASH) {
        int   is_mul = (p->cur.type == TOK_STAR);
        advance(p);
        Node* right = parse_primary(p);
        Node* op    = make_node(is_mul ? "MUL" : "DIV", is_mul ? "*" : "/");
        op->left    = left;
        op->right   = right;
        left        = op;
    }

    return left;
}

// expr: term (( + | - ) term)*
Node* parse_expr(Parser* p) {
    Node* left = parse_term(p);

    while (p->cur.type == TOK_PLUS || p->cur.type == TOK_MINUS) {
        int   is_add = (p->cur.type == TOK_PLUS);
        advance(p);
        Node* right = parse_term(p);
        Node* op    = make_node(is_add ? "ADD" : "SUB", is_add ? "+" : "-");
        op->left    = left;
        op->right   = right;
        left        = op;
    }

    return left;
}

Node* parse_declare(Parser* p) {
    char vartype[32];
    strcpy(vartype, p->cur.value);
    advance(p);
    if (p->cur.type != TOK_NAME && p->cur.type != TOK_MAIN) {
        char msg[128];
        sprintf(msg, "คาดว่าจะเป็นชื่อตัวแปรหลัง '%s'", vartype);
        error(p->src, p->cur.pos, msg);
    }

    char varname[64];
    strcpy(varname, p->cur.value);
    advance(p);

    char label[96];
    sprintf(label, "%s %s", vartype, varname);
    Node* decl = make_node("DECLARE", label);

    if (p->cur.type == TOK_EQ) {
        advance(p);
        decl->right = parse_expr(p);
    }

    expect_semi(p);
    return decl;
}

Node* parse_return(Parser* p) {
    advance(p);
    Node* ret  = make_node("RETURN", "return");
    ret->right = parse_expr(p);
    expect_semi(p);
    return ret;
}

// countj << expr >> endj;
// countj << "string" >> endj;
Node* parse_countj(Parser* p) {
    advance(p);  // ข้าม countj

    if (p->cur.type != TOK_LSHIFT)
        error(p->src, p->cur.pos, "คาดว่าจะเป็น << หลัง countj");
    advance(p);

    Node* n = make_node("COUNTJ", "");

    if (p->cur.type == TOK_STRING) {
        n->left = make_node("STRING", p->cur.value);
        advance(p);
    } else {
        n->left = parse_expr(p);
    }

    if (p->cur.type != TOK_RSHIFT)
        error(p->src, p->cur.pos, "คาดว่าจะเป็น >> หลังค่า");
    advance(p);

    if (p->cur.type != TOK_ENDJ)
        error(p->src, p->cur.pos, "คาดว่าจะเป็น endj");
    advance(p);

    expect_semi(p);
    return n;
}

// parse condition operand (NUM หรือ NAME)
Node* parse_cond_operand(Parser* p) {
    if (p->cur.type == TOK_NUM) {
        Node* n = make_node("NUM", p->cur.value); advance(p); return n;
    }
    if (p->cur.type == TOK_NAME || p->cur.type == TOK_MAIN) {
        Node* n = make_node("NAME", p->cur.value); advance(p); return n;
    }
    char msg[128];
    sprintf(msg, "คาดว่าจะเป็นตัวเลขหรือตัวแปรใน condition แต่เจอ '%s'", p->cur.value);
    error(p->src, p->cur.pos, msg);
    return NULL;
}

// สร้าง block node จาก body array
Node* make_block(const char* tag, Node** body, int len) {
    Node* blk  = make_node("BLOCK", tag);
    if (len == 0) return blk;
    blk->left  = body[0];
    Node* cur  = blk->left;
    for (int i = 1; i < len; i++) {
        cur->next = body[i];
        cur = cur->next;
    }
    return blk;
}

Node* parse_if(Parser* p) {
    advance(p);  // ข้าม if

    // parse condition
    Node* lhs = parse_cond_operand(p);

    // operator
    char op[4] = "";
    if      (p->cur.type == TOK_EQEQ)  { strcpy(op, "=="); advance(p); }
    else if (p->cur.type == TOK_NEQ)   { strcpy(op, "!="); advance(p); }
    else if (p->cur.type == TOK_LTE)   { strcpy(op, "<="); advance(p); }
    else if (p->cur.type == TOK_GTE)   { strcpy(op, ">="); advance(p); }
    else if (p->cur.type == TOK_LT)    { strcpy(op, "<");  advance(p); }
    else if (p->cur.type == TOK_GT)    { strcpy(op, ">");  advance(p); }
    else error(p->src, p->cur.pos, "คาดว่าจะเป็น operator เปรียบเทียบ");

    Node* rhs = parse_cond_operand(p);

    Node* cmp  = make_node("CMP", op);
    cmp->left  = lhs;
    cmp->right = rhs;

    // parse { if body }
    if (p->cur.type != TOK_LBRACE)
        error(p->src, p->cur.pos, "คาดว่าจะเป็น { หลัง if condition");
    advance(p);

    Node* if_body[64]; int if_len = 0;
    while (p->cur.type != TOK_RBRACE && p->cur.type != TOK_EOF)
        if_body[if_len++] = parse_stmt(p);
    advance(p);  // ข้าม }
    if (p->cur.type == TOK_SEMI) advance(p);

    Node* ifnode    = make_node("IF", op);
    ifnode->left    = cmp;
    ifnode->right   = make_block("if_body", if_body, if_len);

    // parse else ถ้ามี
    if (p->cur.type == TOK_ELSE) {
        advance(p);
        if (p->cur.type != TOK_LBRACE)
            error(p->src, p->cur.pos, "คาดว่าจะเป็น { หลัง else");
        advance(p);

        Node* else_body[64]; int else_len = 0;
        while (p->cur.type != TOK_RBRACE && p->cur.type != TOK_EOF)
            else_body[else_len++] = parse_stmt(p);
        advance(p);
        if (p->cur.type == TOK_SEMI) advance(p);

        Node* else_node  = make_node("ELSE", "else");
        else_node->left  = make_block("else_body", else_body, else_len);
        // ต่อ else ไว้ที่ right ของ if_block
        ifnode->right->right = else_node;
    }

    return ifnode;
}

// loop i from 0 >> 10 { ... };
Node* parse_loop(Parser* p) {
    advance(p);  // ข้าม loop

    // ชื่อตัวแปร counter
    if (p->cur.type != TOK_NAME)
        error(p->src, p->cur.pos, "คาดว่าจะเป็นชื่อตัวแปรหลัง loop");
    char varname[64];
    strcpy(varname, p->cur.value);
    advance(p);

    // from
    if (p->cur.type != TOK_FROM)
        error(p->src, p->cur.pos, "คาดว่าจะเป็น from หลังชื่อตัวแปร");
    advance(p);

    // start value
    Node* start = parse_expr(p);

    // >>
    if (p->cur.type != TOK_RSHIFT)
        error(p->src, p->cur.pos, "คาดว่าจะเป็น >> หลังค่าเริ่มต้น");
    advance(p);

    // end value
    Node* end = parse_expr(p);

    // {
    if (p->cur.type != TOK_LBRACE)
        error(p->src, p->cur.pos, "คาดว่าจะเป็น { หลัง loop range");
    advance(p);

    // body
    Node* body[128]; int body_len = 0;
    while (p->cur.type != TOK_RBRACE && p->cur.type != TOK_EOF) {
        Node* s = parse_stmt(p);
        if (s) body[body_len++] = s;
    }
    advance(p);  // ข้าม }
    if (p->cur.type == TOK_SEMI) advance(p);

    Node* loop     = make_node("LOOP", varname);
    loop->left     = start;
    loop->right    = end;
    loop->next     = make_block("loop_body", body, body_len);
    return loop;
}

// while cond { ... };
// while true { ... };
Node* parse_while(Parser* p) {
    advance(p);  // ข้าม while

    Node* whilenode = make_node("WHILE", "");

    // while true
    if (p->cur.type == TOK_TRUE) {
        advance(p);
        strcpy(whilenode->value, "true");
    } else {
        // while x < 10
        Node* lhs = parse_cond_operand(p);
        char op[4] = "";
        if      (p->cur.type == TOK_EQEQ) { strcpy(op, "=="); advance(p); }
        else if (p->cur.type == TOK_NEQ)  { strcpy(op, "!="); advance(p); }
        else if (p->cur.type == TOK_LTE)  { strcpy(op, "<="); advance(p); }
        else if (p->cur.type == TOK_GTE)  { strcpy(op, ">="); advance(p); }
        else if (p->cur.type == TOK_LT)   { strcpy(op, "<");  advance(p); }
        else if (p->cur.type == TOK_GT)   { strcpy(op, ">");  advance(p); }
        else error(p->src, p->cur.pos, "คาดว่าจะเป็น operator เปรียบเทียบหลัง while");
        Node* rhs  = parse_cond_operand(p);
        Node* cmp  = make_node("CMP", op);
        cmp->left  = lhs;
        cmp->right = rhs;
        whilenode->left = cmp;
        strcpy(whilenode->value, op);
    }

    // {
    if (p->cur.type != TOK_LBRACE)
        error(p->src, p->cur.pos, "คาดว่าจะเป็น { หลัง while condition");
    advance(p);

    Node* body[128]; int body_len = 0;
    while (p->cur.type != TOK_RBRACE && p->cur.type != TOK_EOF) {
        Node* s = parse_stmt(p);
        if (s) body[body_len++] = s;
    }
    advance(p);
    if (p->cur.type == TOK_SEMI) advance(p);

    whilenode->right = make_block("while_body", body, body_len);
    return whilenode;
}

// x = expr;  (assignment)
Node* parse_assign(Parser* p) {
    char varname[64];
    strcpy(varname, p->cur.value);
    advance(p);   // ข้าม name
    advance(p);   // ข้าม =
    Node* n    = make_node("ASSIGN", varname);
    n->right   = parse_expr(p);
    expect_semi(p);
    return n;
}

Node* parse_stmt(Parser* p) {
    if (p->cur.type == TOK_TYPE)   return parse_declare(p);
    if (p->cur.type == TOK_FTYPE)  return parse_declare(p);
    if (p->cur.type == TOK_CHAR)   return parse_declare(p);
    if (p->cur.type == TOK_RETURN) return parse_return(p);
    if (p->cur.type == TOK_COUNTJ) return parse_countj(p);
    if (p->cur.type == TOK_IF)     return parse_if(p);
    if (p->cur.type == TOK_LOOP)   return parse_loop(p);
    if (p->cur.type == TOK_WHILE)  return parse_while(p);
    // NAME = expr  (assignment)
    if (p->cur.type == TOK_NAME) {
        // peek ถัดไปว่าเป็น = ไหม
        Lexer tmp = {p->lex->src, p->lex->pos};
        Token peek = next_token(&tmp);
        if (peek.type == TOK_EQ) return parse_assign(p);
    }
    // token ที่ไม่รู้จัก — ข้ามไป
    advance(p);
    return NULL;
}

Node* parse_function(Parser* p) {
    char rettype[32];
    strcpy(rettype, p->cur.value);
    advance(p);

    char funcname[64];
    strcpy(funcname, p->cur.value);
    advance(p);

    // ข้าม () parameters
    if (p->cur.type == TOK_LPAREN) {
        advance(p);
        while (p->cur.type != TOK_RPAREN && p->cur.type != TOK_EOF)
            advance(p);
        if (p->cur.type == TOK_RPAREN) advance(p);
    }

    if (p->cur.type != TOK_LBRACE)
        error(p->src, p->cur.pos, "คาดว่าจะเป็น { หลังชื่อ function");
    advance(p);

    char label[96];
    sprintf(label, "%s %s", rettype, funcname);
    Node* func = make_node("FUNCTION", label);

    Node* body[128]; int body_len = 0;
    while (p->cur.type != TOK_RBRACE && p->cur.type != TOK_EOF) {
        Node* s = parse_stmt(p);
        if (s) body[body_len++] = s;
    }

    if (p->cur.type != TOK_RBRACE)
        error(p->src, p->cur.pos, "ลืมปิด }");
    advance(p);
    if (p->cur.type == TOK_SEMI) advance(p);

    // chain body
    if (body_len > 0) {
        func->left = body[0];
        Node* cur  = func->left;
        for (int i = 1; i < body_len; i++) {
            cur->next = body[i];
            cur = cur->next;
        }
    }

    return func;
}

// ===== SYMBOL TABLE =====
// offset เริ่มจาก 0 เพิ่มทีละ 8 ไปเรื่อยๆ
// local vars อยู่ที่ [sp + offset]

struct Symbol {
    char name[64];
    int  offset;     // byte offset จาก sp
    int  is_float;   // 0=int, 1=f32, 2=f64
};

Symbol symbols[128];
int    sym_count = 0;
int    stack_top = 0;   // next free offset (bytes)

int find_symbol(const char* name) {
    for (int i = 0; i < sym_count; i++)
        if (strcmp(symbols[i].name, name) == 0)
            return symbols[i].offset;
    return -1;  // ไม่พบ
}

int find_symbol_float(const char* name) {
    for (int i = 0; i < sym_count; i++)
        if (strcmp(symbols[i].name, name) == 0)
            return symbols[i].is_float;
    return 0;
}

int add_symbol(const char* name) {
    int existing = find_symbol(name);
    if (existing >= 0) return existing;
    strcpy(symbols[sym_count].name,   name);
    symbols[sym_count].offset   = stack_top;
    symbols[sym_count].is_float = 0;
    int off = stack_top;
    stack_top += 8;
    sym_count++;
    return off;
}

int add_symbol_f(const char* name, int ftype) {
    int existing = find_symbol(name);
    if (existing >= 0) return existing;
    strcpy(symbols[sym_count].name,   name);
    symbols[sym_count].offset   = stack_top;
    symbols[sym_count].is_float = ftype;
    int off = stack_top;
    stack_top += 8;
    sym_count++;
    return off;
}

// ===== FP ENCODERS =====

// FMOV S0, W0  (GP→FP f32)
uint32_t encode_fmov_s_from_w(int fd, int rn) { return 0x1E270000 | (rn<<5) | fd; }
// FMOV W0, S0  (FP→GP f32)
uint32_t encode_fmov_w_from_s(int rd, int fn) { return 0x1E260000 | (fn<<5) | rd; }
// FMOV D0, X0  (GP→FP f64)
uint32_t encode_fmov_d_from_x(int fd, int rn) { return 0x9E670000 | (rn<<5) | fd; }
// FMOV X0, D0  (FP→GP f64)
uint32_t encode_fmov_x_from_d(int rd, int fn) { return 0x9E660000 | (fn<<5) | rd; }

// FADD/FSUB/FMUL/FDIV Sd,Sn,Sm
uint32_t encode_fadd_s(int fd, int fn, int fm) { return 0x1E202800 | (fm<<16) | (fn<<5) | fd; }
uint32_t encode_fsub_s(int fd, int fn, int fm) { return 0x1E203800 | (fm<<16) | (fn<<5) | fd; }
uint32_t encode_fmul_s(int fd, int fn, int fm) { return 0x1E200800 | (fm<<16) | (fn<<5) | fd; }
uint32_t encode_fdiv_s(int fd, int fn, int fm) { return 0x1E201800 | (fm<<16) | (fn<<5) | fd; }

// FADD/FSUB/FMUL/FDIV Dd,Dn,Dm
uint32_t encode_fadd_d(int fd, int fn, int fm) { return 0x1E602800 | (fm<<16) | (fn<<5) | fd; }
uint32_t encode_fsub_d(int fd, int fn, int fm) { return 0x1E603800 | (fm<<16) | (fn<<5) | fd; }
uint32_t encode_fmul_d(int fd, int fn, int fm) { return 0x1E600800 | (fm<<16) | (fn<<5) | fd; }
uint32_t encode_fdiv_d(int fd, int fn, int fm) { return 0x1E601800 | (fm<<16) | (fn<<5) | fd; }

// STR/LDR Sd, [Xn, #off]  (f32, offset หารด้วย 4)
uint32_t encode_str_s(int ft, int rn, int offset) {
    uint32_t uimm12 = (offset / 4) & 0xFFF;
    return 0xBD000000 | (uimm12<<10) | (rn<<5) | ft;
}
uint32_t encode_ldr_s(int ft, int rn, int offset) {
    uint32_t uimm12 = (offset / 4) & 0xFFF;
    return 0xBD400000 | (uimm12<<10) | (rn<<5) | ft;
}

// STR/LDR Dd, [Xn, #off]  (f64, offset หารด้วย 8)
uint32_t encode_str_d(int ft, int rn, int offset) {
    uint32_t uimm12 = (offset / 8) & 0xFFF;
    return 0xFD000000 | (uimm12<<10) | (rn<<5) | ft;
}
uint32_t encode_ldr_d(int ft, int rn, int offset) {
    uint32_t uimm12 = (offset / 8) & 0xFFF;
    return 0xFD400000 | (uimm12<<10) | (rn<<5) | ft;
}

// FCVTZS Wn, Sn  (f32→int truncate)
uint32_t encode_fcvtzs_w_s(int rd, int fn) { return 0x1E380000 | (fn<<5) | rd; }
// FCVTZS Xn, Dn  (f64→int truncate)
uint32_t encode_fcvtzs_x_d(int rd, int fn) { return 0x9E780000 | (fn<<5) | rd; }

// SCVTF Sd, Wn  (int→f32)
uint32_t encode_scvtf_s_w(int fd, int rn) { return 0x1E220000 | (rn<<5) | fd; }
// SCVTF Dd, Xn  (int→f64)
uint32_t encode_scvtf_d_x(int fd, int rn) { return 0x9E620000 | (rn<<5) | fd; }

// FCVT Dd, Sn  (f32→f64)
uint32_t encode_fcvt_d_s(int fd, int fn) { return 0x1E22C000 | (fn<<5) | fd; }

// FABS Sd, Sn
uint32_t encode_fabs_s(int fd, int fn) { return 0x1E20C000 | (fn<<5) | fd; }
// FABS Dd, Dn
uint32_t encode_fabs_d(int fd, int fn) { return 0x1E60C000 | (fn<<5) | fd; }
// FNEG Sd, Sn
uint32_t encode_fneg_s(int fd, int fn) { return 0x1E214000 | (fn<<5) | fd; }
// FCMP Sn, #0.0
uint32_t encode_fcmp_s_zero(int fn) { return 0x1E202008 | (fn<<5); }

// ===== CODE BUFFER =====

#define MAX_CODE 4096
uint32_t code[MAX_CODE];
int      code_len = 0;

void emit(uint32_t inst) {
    if (code_len >= MAX_CODE) {
        printf("[ERROR] code buffer เต็ม\n");
        exit(1);
    }
    code[code_len++] = inst;
}

uint8_t strdata[8192];
int     strdata_len = 0;

int add_string(const char* s) {
    int offset = strdata_len;
    int len    = strlen(s);
    memcpy(strdata + strdata_len, s, len);
    strdata_len += len;
    strdata[strdata_len++] = '\n';
    return offset;
}

// patch table สำหรับ adr (string)
int patch_idx    [128];
int patch_str_off[128];
int patch_count = 0;

// ===== CODE GENERATOR =====

// ITOA_BUF_OFFSET: offset ใน stack frame ที่ใช้เป็น buffer ของ itoa
// วางไว้ที่ท้าย frame (เช่น sp + FRAME_SIZE - 32)
// ต้องไม่ทับ local vars — ใช้ค่าคงที่สูง เช่น 128
// (frame จอง 256 bytes ใน main)
#define ITOA_BUF_SP_OFFSET  128   // buffer อยู่ที่ [sp+128] ถึง [sp+159]
#define ITOA_BUF_SIZE       32    // 32 bytes พอสำหรับ int 64 bit

void gen_expr(Node* n) {
    if (!n) return;

    if (strcmp(n->type, "NUM") == 0) {
        int val = atoi(n->value);
        emit(encode_mov(0, val));

    } else if (strcmp(n->type, "NAME") == 0) {
        int off = find_symbol(n->value);
        if (off < 0) {
            printf("[ERROR] ตัวแปร '%s' ไม่ได้ประกาศ\n", n->value);
            exit(1);
        }
        // LDR x0, [sp, #off]
        emit(encode_ldr(0, 31, off));

    } else if (strcmp(n->type, "ADD") == 0) {
        // gen left → x0, save to tmp slot, gen right → x0
        // ใช้ x9 เก็บ left ชั่วคราว (ไม่แตะ stack เพราะอาจมี side-effect ซ้อน)
        gen_expr(n->left);
        emit(encode_mov_reg(9, 0));   // x9 = left
        gen_expr(n->right);
        emit(encode_add(0, 9, 0));    // x0 = x9 + x0

    } else if (strcmp(n->type, "SUB") == 0) {
        gen_expr(n->left);
        emit(encode_mov_reg(9, 0));   // x9 = left
        gen_expr(n->right);
        emit(encode_sub(0, 9, 0));    // x0 = x9 - x0

    } else if (strcmp(n->type, "MUL") == 0) {
        gen_expr(n->left);
        emit(encode_mov_reg(9, 0));   // x9 = left
        gen_expr(n->right);
        // MUL x0, x9, x0  (MADD x0, x9, x0, xzr)
        emit(0x9B000000 | (0<<16) | (0<<15) | (31<<10) | (9<<5) | 0);

    } else if (strcmp(n->type, "DIV") == 0) {
        gen_expr(n->left);
        emit(encode_mov_reg(9, 0));   // x9 = left
        gen_expr(n->right);
        // UDIV x0, x9, x0
        emit(0x9AC00C00 | (0<<16) | (9<<5) | 0);
    }
}

void gen_node(Node* n);

void gen_function(Node* n) {
    Node* cur = n->left;
    while (cur) {
        gen_node(cur);
        cur = cur->next;
    }
}

// gen itoa + sys_write สำหรับ x0 = value
// Strategy: เขียนตัวเลขจากหลังไปหน้าใน buffer [sp+128 .. sp+159]
//           แล้วเขียน '\n' ที่ตำแหน่ง end (sp+159) แยกต่างหาก
//           buf layout หลัง itoa: [x10 .. sp+158] = digits, [sp+159] = '\n'
//           length = (sp+160) - x10
void gen_print_int() {
    // x0 = value ที่จะพิมพ์
    // x9  = value (working copy)
    // x10 = write pointer (เริ่มที่ end-1 = sp+158, ไม่ใช้ sp+159 เพราะจอง '\n')
    // x11 = 10 (divisor)
    // x12 = quotient
    // x13 = digit char

    // x14 = end-of-digits slot = sp + (ITOA_BUF_SP_OFFSET + ITOA_BUF_SIZE - 1)
    //        เราจะเขียน '\n' ที่นี่ก่อน แล้วเริ่ม loop ที่ x10 = x14
    // สรุป layout: digits เขียนซ้ายของ '\n' → อ่านจาก x10 ถึง x14 inclusive
    // length = x14 - x10 + 1 + 1  (digits + newline)

    // ง่ายกว่า: เขียน '\n' ที่ sp+159 ก่อนเลย แล้ว loop ใช้ x10 เริ่มจาก sp+158
    // สุดท้าย sys_write(1, x10, (sp+160)-x10)

    // ADD x14, sp, #(ITOA_BUF_SP_OFFSET + ITOA_BUF_SIZE - 1)  → sp+159
    emit(encode_add_imm(14, 31, ITOA_BUF_SP_OFFSET + ITOA_BUF_SIZE - 1));

    // MOV w13, #10 ('\n')
    emit(encode_mov(13, 10));
    // STRB w13, [x14]  — เขียน '\n' ที่ sp+159 ก่อนเลย
    emit(encode_strb(13, 14));

    // x10 = x14 (เริ่ม write pointer ที่ sp+159 แล้วจะ sub ก่อนเขียนแต่ละ digit)
    emit(encode_mov_reg(10, 14));

    // MOV x9, x0  (save value)
    emit(encode_mov_reg(9, 0));

    // CMP x9, #0
    emit(0xF100013F);
    // B.NE → loop_start (patch ทีหลัง เพราะยังไม่รู้ตำแหน่ง loop)
    int bne_idx = code_len;
    emit(0);  // placeholder

    // === zero case ===
    // SUB x10, x10, #1
    emit(encode_sub_imm(10, 10, 1));
    // MOV w13, #48 ('0')
    emit(encode_mov(13, 48));
    // STRB w13, [x10]
    emit(encode_strb(13, 10));
    // B end_itoa — placeholder
    int zero_b = code_len;
    emit(0);

    // === loop: แกะ digit ทีละตัว ===
    int loop_start = code_len;
    // patch b.ne → loop_start
    code[bne_idx] = 0x54000000 | (((loop_start - bne_idx) & 0x7FFFF) << 5) | 1;

    // MOV x11, #10
    emit(encode_mov(11, 10));
    // UDIV x12, x9, x11
    // UDIV X12, X9, X11: op=0x9ACB0D2C
    emit(0x9ACB0D2C);
    // MSUB x13, x12, x11, x9  (x13 = x9 mod 10)
    // MSUB Xd=13,Xn=12,Xm=11,Xa=9: 0x9B0BA58D
    emit(0x9B0BA58D);
    // ADD x13, x13, #48
    emit(encode_add_imm(13, 13, 48));
    // SUB x10, x10, #1   (ptr--)
    emit(encode_sub_imm(10, 10, 1));
    // STRB w13, [x10]
    emit(encode_strb(13, 10));
    // MOV x9, x12
    emit(encode_mov_reg(9, 12));
    // CMP x9, #0
    emit(0xF100013F);
    // B.NE loop
    int loop_off = loop_start - code_len;
    emit(0x54000000 | ((loop_off & 0x7FFFF) << 5) | 1);

    // patch zero_b → ตรงนี้ (end_itoa)
    code[zero_b] = 0x14000000 | ((code_len - zero_b) & 0x3FFFFFF);

    // ตอนนี้ x10 = ptr ชี้ที่ digit แรก (ซ้ายสุด)
    // '\n' อยู่ที่ sp+159 (x14)
    // sys_write(1, x10, (sp+160) - x10)

    // ADD x1, sp, #(ITOA_BUF_SP_OFFSET + ITOA_BUF_SIZE)  → sp+160 (end)
    emit(encode_add_imm(1, 31, ITOA_BUF_SP_OFFSET + ITOA_BUF_SIZE));
    // SUB x2, x1, x10   (length = end - ptr)
    // SUB X2, X1, X10 = 0xCB0A0022
    emit(0xCB0A0022);
    // MOV x1, x10  (buf start)
    emit(encode_mov_reg(1, 10));
    // MOV x0, #1   (stdout)
    emit(encode_mov(0, 1));
    // MOV x8, #64  (sys_write)
    emit(encode_mov(8, 64));
    // SVC 0
    emit(encode_svc(0));
}

// gen_expr_float: คำนวณ float expression → ผลลัพธ์อยู่ใน s0 (f32) หรือ d0 (f64)
// is64: 0=f32, 1=f64
void gen_expr_float(Node* n, int is64);

void gen_expr_float(Node* n, int is64) {
    if (!n) return;

    if (strcmp(n->type, "FNUM") == 0) {
        // โหลด float literal: เขียน bits ลง W0/X0 แล้ว FMOV
        if (!is64) {
            float fval = (float)atof(n->value);
            uint32_t bits;
            memcpy(&bits, &fval, 4);
            // MOVZ W0, #lo16
            emit(0x52800000 | ((bits & 0xFFFF) << 5) | 0);
            // MOVK W0, #hi16, LSL#16
            emit(0x72A00000 | (((bits >> 16) & 0xFFFF) << 5) | 0);
            emit(encode_fmov_s_from_w(0, 0));
        } else {
            double dval = atof(n->value);
            uint64_t bits;
            memcpy(&bits, &dval, 8);
            uint32_t lo = (uint32_t)(bits & 0xFFFFFFFF);
            uint32_t hi = (uint32_t)(bits >> 32);
            // สร้าง X0 = bits
            emit(0xD2800000 | ((lo & 0xFFFF) << 5) | 0);                   // MOVZ X0, #lo0
            emit(0xF2A00000 | (((lo >> 16) & 0xFFFF) << 5) | 0);           // MOVK X0, #lo16, lsl16
            emit(0xF2C00000 | ((hi & 0xFFFF) << 5) | 0);                   // MOVK X0, #hi0, lsl32
            emit(0xF2E00000 | (((hi >> 16) & 0xFFFF) << 5) | 0);           // MOVK X0, #hi16, lsl48
            emit(encode_fmov_d_from_x(0, 0));
        }

    } else if (strcmp(n->type, "NUM") == 0) {
        // int literal → convert to float
        int val = atoi(n->value);
        emit(encode_mov(0, val));
        if (!is64) emit(encode_scvtf_s_w(0, 0));
        else       emit(encode_scvtf_d_x(0, 0));

    } else if (strcmp(n->type, "NAME") == 0) {
        int off = find_symbol(n->value);
        if (off < 0) { printf("[ERROR] ตัวแปร '%s' ไม่ได้ประกาศ\n", n->value); exit(1); }
        int ftype = find_symbol_float(n->value);
        if (ftype == 1)      emit(encode_ldr_s(0, 31, off));
        else if (ftype == 2) emit(encode_ldr_d(0, 31, off));
        else {
            // int var → แปลงเป็น float
            emit(encode_ldr(0, 31, off));
            if (!is64) emit(encode_scvtf_s_w(0, 0));
            else       emit(encode_scvtf_d_x(0, 0));
        }

    } else if (strcmp(n->type, "ADD") == 0) {
        gen_expr_float(n->left, is64);
        // save s0/d0 → s9/d9
        if (!is64) emit(0x1E204009);  // FMOV S9, S0
        else       emit(0x1E604009);  // FMOV D9, D0
        gen_expr_float(n->right, is64);
        if (!is64) emit(encode_fadd_s(0, 9, 0));
        else       emit(encode_fadd_d(0, 9, 0));

    } else if (strcmp(n->type, "SUB") == 0) {
        gen_expr_float(n->left, is64);
        if (!is64) emit(0x1E204009);
        else       emit(0x1E604009);
        gen_expr_float(n->right, is64);
        if (!is64) emit(encode_fsub_s(0, 9, 0));
        else       emit(encode_fsub_d(0, 9, 0));

    } else if (strcmp(n->type, "MUL") == 0) {
        gen_expr_float(n->left, is64);
        if (!is64) emit(0x1E204009);
        else       emit(0x1E604009);
        gen_expr_float(n->right, is64);
        if (!is64) emit(encode_fmul_s(0, 9, 0));
        else       emit(encode_fmul_d(0, 9, 0));

    } else if (strcmp(n->type, "DIV") == 0) {
        gen_expr_float(n->left, is64);
        if (!is64) emit(0x1E204009);
        else       emit(0x1E604009);
        gen_expr_float(n->right, is64);
        if (!is64) emit(encode_fdiv_s(0, 9, 0));
        else       emit(encode_fdiv_d(0, 9, 0));
    }
}

// gen_print_float: พิมพ์ค่า float ใน s0 (is64=0) หรือ d0 (is64=1)
// format: [-]integer.fraction\n  (6 หลักทศนิยม)
// ใช้ ITOA_BUF + พื้นที่เพิ่มอีก 32 bytes ที่ sp+160
#define FTOA_BUF_SP_OFFSET 160
#define FTOA_BUF_SIZE      32

void gen_print_float(int is64) {
    // s0/d0 = ค่าที่จะพิมพ์
    // strategy:
    //   1. ตรวจ negative → ถ้าลบ พิมพ์ '-' แล้ว FABS
    //   2. FCVTZS W9, S0  → integer part
    //   3. SCVTF S1, W9   → float ของ integer part
    //   4. FSUB S0, S0, S1 → frac part (0 ≤ frac < 1)
    //   5. FMUL S0, S0, #1000000.0 (load via bits)
    //   6. FCVTZS W10, S0 → 6 decimal digits
    //   7. พิมพ์ W9 ด้วย itoa, พิมพ์ '.', พิมพ์ W10 (pad 6 หลัก)

    // โหลด 1000000.0 ไว้ใน s8/d8
    // f32 bits ของ 1000000.0 = 0x49742400
    emit(0x52800000 | ((0x2400) << 5) | 8);           // MOVZ W8, #0x2400
    emit(0x72A00000 | ((0x4974) << 5) | 8);           // MOVK W8, #0x4974, lsl16
    emit(encode_fmov_s_from_w(8, 8));                 // FMOV S8, W8

    if (is64) emit(encode_fcvt_d_s(8, 8));            // FCVT D8, S8 (1e6 → f64)

    // ตรวจลบ
    if (!is64) emit(encode_fcmp_s_zero(0));            // FCMP S0, #0.0
    else       emit(0x1E602008 | (0<<5));              // FCMP D0, #0.0

    int bpl_idx = code_len;
    emit(0);  // B.PL → skip_neg (cond=5: PL = positive or zero)

    // พิมพ์ '-'
    // เก็บ float ไว้ก่อนใน s7/d7
    if (!is64) emit(0x1E204007);  // FMOV S7, S0
    else       emit(0x1E604007);  // FMOV D7, D0

    // สร้าง '-' ใน buf แล้ว write
    emit(encode_add_imm(1, 31, FTOA_BUF_SP_OFFSET)); // X1 = sp+160
    emit(encode_mov(9, 45));                          // W9 = '-'
    emit(encode_strb(9, 1));                          // STRB W9, [X1]
    emit(encode_mov(0, 1));                           // X0 = stdout
    emit(encode_mov(2, 1));                           // X2 = 1
    emit(encode_mov(8, 64));                          // X8 = sys_write
    emit(encode_svc(0));

    // FABS
    if (!is64) { emit(0x1E20C0E0); }                  // FMOV S0, S7 แล้ว FABS
    else       { emit(0x1E60C0E0); }

    // patch B.PL
    code[bpl_idx] = 0x54000000 | (((code_len - bpl_idx) & 0x7FFFF) << 5) | 5;

    // แยก integer part
    if (!is64) {
        emit(encode_fcvtzs_w_s(9, 0));                // W9 = (int)S0
        emit(encode_scvtf_s_w(1, 9));                 // S1 = (float)W9
        emit(encode_fsub_s(0, 0, 1));                 // S0 = frac
        emit(encode_fmul_s(0, 0, 8));                 // S0 = frac * 1e6
        emit(encode_fcvtzs_w_s(10, 0));               // W10 = frac_digits
    } else {
        emit(encode_fcvtzs_x_d(9, 0));
        emit(encode_scvtf_d_x(1, 9));
        emit(encode_fsub_d(0, 0, 1));
        emit(encode_fmul_d(0, 0, 8));
        emit(encode_fcvtzs_x_d(10, 0));
    }

    // พิมพ์ integer part แบบ inline (ไม่ใช้ gen_print_int เพราะมัน append \n)
    // เซฟ W10 ก่อน
    emit(encode_str(10, 31, FTOA_BUF_SP_OFFSET + 16));  // [sp+176] = W10

    // *** Reimplemented inline ***
    // โหลด W10 กลับ
    emit(encode_ldr(10, 31, FTOA_BUF_SP_OFFSET + 16));

    // -- integer part itoa (เขียนลง ITOA_BUF) --
    // x9 มีค่า integer part อยู่แล้วจาก fcvtzs ข้างบน
    emit(encode_add_imm(14, 31, ITOA_BUF_SP_OFFSET + ITOA_BUF_SIZE - 1));
    emit(encode_mov_reg(11, 14));   // x11 = write ptr
    // CMP x9, #0
    emit(0xF100013F);
    int bne2 = code_len; emit(0);
    // zero case
    emit(encode_sub_imm(11, 11, 1));
    emit(encode_mov(12, 48));
    emit(encode_strb(12, 11));
    int zb2 = code_len; emit(0);
    // loop
    int ls2 = code_len;
    code[bne2] = 0x54000000 | (((ls2 - bne2) & 0x7FFFF) << 5) | 1;
    emit(encode_mov(13, 10));         // W13 = 10 (divisor) — reuse x13
    // UDIV x12, x9, x13 (÷10)
    emit(0x9AC00C00 | (13<<16) | (9<<5) | 12);
    // MSUB x12r, x12, x13, x9  (remainder)
    // MSUB Xd=15,Xn=12,Xm=13,Xa=9
    uint32_t msub2 = 0x9B000000 | (13<<16) | (1<<15) | (9<<10) | (12<<5) | 15;
    emit(msub2);
    emit(encode_add_imm(15, 15, 48));
    emit(encode_sub_imm(11, 11, 1));
    emit(encode_strb(15, 11));
    emit(encode_mov_reg(9, 12));
    emit(0xF100013F);
    int loff2 = ls2 - code_len;
    emit(0x54000000 | ((loff2 & 0x7FFFF) << 5) | 1);
    code[zb2] = 0x14000000 | ((code_len - zb2) & 0x3FFFFFF);

    // เขียน '.' ที่ end-of-buf = sp + ITOA_BUF_SP_OFFSET + ITOA_BUF_SIZE - 1
    emit(encode_mov(12, 46));         // '.'
    emit(encode_strb(12, 14));        // STRB '.' ที่ x14 (sp+159)

    // sys_write integer + '.'
    emit(encode_add_imm(1, 31, ITOA_BUF_SP_OFFSET + ITOA_BUF_SIZE)); // end = sp+160
    emit(0xCB0B0022);  // SUB X2, X1, X11  (length)
    emit(encode_mov_reg(1, 11));
    emit(encode_mov(0, 1));
    emit(encode_mov(8, 64));
    emit(encode_svc(0));

    // -- fractional part: W10 = 6 digits, พิมพ์ด้วย pad 0 ให้ครบ 6 หลัก --
    // เขียน 6 digit + '\n' ลง FTOA_BUF (sp+160)
    // เริ่มจากหลักสุดท้ายไปต้น (เหมือน itoa) แต่ต้องครบ 6 หลัก
    emit(encode_add_imm(14, 31, FTOA_BUF_SP_OFFSET + 7)); // ptr = sp+167 (byte 7 = ที่ index 6 = หลังตัวเลข 6 ตัว)
    // เขียน '\n' ที่ sp+167
    emit(encode_mov(12, 10));
    emit(encode_strb(12, 14));
    emit(encode_mov_reg(11, 14));  // end = sp+167
    // loop 6 รอบ
    emit(encode_mov(9, 6));        // counter
    int floop = code_len;
    // SUB x14, x14, #1
    emit(encode_sub_imm(14, 14, 1));
    // digit = W10 % 10
    emit(encode_mov(13, 10));
    emit(0x9AC00C00 | (13<<16) | (10<<5) | 12);  // UDIV X12,X10,X13
    uint32_t msub3 = 0x9B000000 | (13<<16) | (1<<15) | (10<<10) | (12<<5) | 15;
    emit(msub3);   // MSUB X15,X12,X13,X10 = remainder
    emit(encode_add_imm(15, 15, 48));
    emit(encode_strb(15, 14));
    emit(encode_mov_reg(10, 12));   // W10 /= 10
    emit(encode_sub_imm(9, 9, 1)); // counter--
    emit(0xF100013F | (9<<5) & ~(0x1F<<5) | (9<<5));
    // CMP x9, #0
    emit(0xF1000120 | (9<<5) | 0x1F);
    // B.NE floop
    int floff = floop - code_len;
    emit(0x54000000 | ((floff & 0x7FFFF) << 5) | 1);

    // sys_write frac + '\n'  (x14..x11+1)
    emit(encode_add_imm(2, 31, FTOA_BUF_SP_OFFSET + 8)); // end = sp+168
    emit(0xCB0E0042);  // SUB X2, X2, X14
    emit(encode_mov_reg(1, 14));
    emit(encode_mov(0, 1));
    emit(encode_mov(8, 64));
    emit(encode_svc(0));
}

// คืนค่า 0=int, 1=f32, 2=f64 — ดูว่า expression มี float operand ไหม
int node_has_float(Node* n) {
    if (!n) return 0;
    if (strcmp(n->type, "FNUM") == 0) return 1;
    if (strcmp(n->type, "NAME") == 0) return find_symbol_float(n->value);
    int l = node_has_float(n->left);
    int r = node_has_float(n->right);
    return l > r ? l : r;
}

void gen_node(Node* n) {
    if (!n) return;

    // ===== DECLARE =====
    if (strcmp(n->type, "DECLARE") == 0) {
        char vartype[32], varname[64];
        sscanf(n->value, "%31s %63s", vartype, varname);

        if (strcmp(vartype, "f32") == 0) {
            if (n->right) gen_expr_float(n->right, 0);
            else { emit(encode_mov(0, 0)); emit(encode_scvtf_s_w(0, 0)); }
            int off = add_symbol_f(varname, 1);
            emit(encode_str_s(0, 31, off));

        } else if (strcmp(vartype, "f64") == 0) {
            if (n->right) gen_expr_float(n->right, 1);
            else { emit(encode_mov(0, 0)); emit(encode_scvtf_d_x(0, 0)); }
            int off = add_symbol_f(varname, 2);
            emit(encode_str_d(0, 31, off));

        } else if (strcmp(vartype, "char") == 0) {
            // char เก็บเป็น int 8-bit ใน stack
            if (n->right) gen_expr(n->right);
            else emit(encode_mov(0, 0));
            int off = add_symbol(varname);
            emit(encode_str(0, 31, off));

        } else {
            if (n->right) gen_expr(n->right);
            else emit(encode_mov(0, 0));
            int off = add_symbol(varname);
            emit(encode_str(0, 31, off));
        }

    // ===== ASSIGN =====
    } else if (strcmp(n->type, "ASSIGN") == 0) {
        // x = expr  — ตัวแปรต้องมีอยู่แล้ว
        const char* varname = n->value;
        int off = find_symbol(varname);
        if (off < 0) {
            printf("[jplus ERROR] ตัวแปร '%s' ไม่ได้ประกาศ\n", varname);
            exit(1);
        }
        int ftype = find_symbol_float(varname);
        if (ftype == 1) {
            gen_expr_float(n->right, 0);
            emit(encode_str_s(0, 31, off));
        } else if (ftype == 2) {
            gen_expr_float(n->right, 1);
            emit(encode_str_d(0, 31, off));
        } else {
            gen_expr(n->right);
            emit(encode_str(0, 31, off));
        }

    // ===== RETURN =====
    } else if (strcmp(n->type, "RETURN") == 0) {
        if (n->right) gen_expr(n->right);
        else emit(encode_mov(0, 0));
        // sys_exit(x0)
        emit(encode_mov(8, 93));
        emit(encode_svc(0));

    // ===== FUNCTION =====
    } else if (strcmp(n->type, "FUNCTION") == 0) {
        gen_function(n);

    // ===== BLOCK =====
    } else if (strcmp(n->type, "BLOCK") == 0) {
        Node* stmt = n->left;
        while (stmt) {
            gen_node(stmt);
            stmt = stmt->next;
        }

    // ===== IF / ELSE =====
    } else if (strcmp(n->type, "IF") == 0) {
        Node* cmp = n->left;   // CMP node

        // gen lhs → x0,  x9 = lhs
        gen_expr(cmp->left);
        emit(encode_mov_reg(9, 0));

        // gen rhs → x0,  x1 = rhs
        gen_expr(cmp->right);
        emit(encode_mov_reg(1, 0));

        // CMP x9, x1  (SUBS xzr, x9, x1)
        // 0xEB010120  = SUBS XZR, X9, X1
        emit(0xEB01013F);

        // B.cond (inverted) → else/end
        int branch_idx = code_len;
        emit(0);  // placeholder

        // gen if body
        Node* if_block = n->right;  // BLOCK node
        if (if_block) {
            Node* stmt = if_block->left;
            while (stmt) { gen_node(stmt); stmt = stmt->next; }
        }

        // B skip_else (placeholder — ใส่ก็ต่อเมื่อมี else)
        Node* else_node = (if_block && if_block->right) ? if_block->right : NULL;
        int   skip_else_idx = -1;
        if (else_node) {
            skip_else_idx = code_len;
            emit(0);
        }

        // patch branch_idx → ตรงนี้ (ชี้ไป else หรือ end)
        {
            int offset = code_len - branch_idx;
            const char* op = n->value;
            int cond;
            if      (strcmp(op,"==") == 0) cond = 1;   // NE
            else if (strcmp(op,"!=") == 0) cond = 0;   // EQ
            else if (strcmp(op,"<")  == 0) cond = 10;  // GE
            else if (strcmp(op,">")  == 0) cond = 13;  // LE
            else if (strcmp(op,"<=") == 0) cond = 12;  // GT
            else if (strcmp(op,">=") == 0) cond = 11;  // LT
            else cond = 1;
            code[branch_idx] = 0x54000000 | ((offset & 0x7FFFF) << 5) | cond;
        }

        // gen else body
        if (else_node && strcmp(else_node->type, "ELSE") == 0) {
            Node* else_block = else_node->left;
            if (else_block) {
                Node* stmt = else_block->left;
                while (stmt) { gen_node(stmt); stmt = stmt->next; }
            }
            // patch skip_else → ตรงนี้
            int offset = code_len - skip_else_idx;
            code[skip_else_idx] = 0x14000000 | (offset & 0x3FFFFFF);
        }

    // ===== LOOP i from 0 >> 10 =====
    } else if (strcmp(n->type, "LOOP") == 0) {
        const char* varname = n->value;

        // init counter
        gen_expr(n->left);
        int off = add_symbol(varname);
        emit(encode_str(0, 31, off));

        // loop_start:
        int loop_start = code_len;

        // โหลด counter → x20 (callee-saved ไม่ชนกับ gen_print_int)
        emit(encode_ldr(20, 31, off));

        // gen end → x21
        gen_expr(n->right);
        emit(encode_mov_reg(21, 0));

        // CMP x20, x21
        // SUBS XZR, X20, X21
        emit(0xEB15029F);  // SUBS XZR, X20, X21

        // B.GE loop_end
        int bge_idx = code_len;
        emit(0);

        // อัปเดต symbol ให้ชี้ที่ stack offset จริง ไม่ใช่ register
        // body — ทุก ref ถึง i จะ ldr จาก [sp+off]
        if (n->next) {
            Node* stmt = n->next->left;
            while (stmt) { gen_node(stmt); stmt = stmt->next; }
        }

        // counter++ 
        emit(encode_ldr(20, 31, off));
        emit(encode_add_imm(20, 20, 1));
        emit(encode_str(20, 31, off));

        // B loop_start
        int back_off = loop_start - code_len;
        emit(0x14000000 | (back_off & 0x3FFFFFF));

        // patch bge → loop_end
        int loop_end = code_len;
        // B.GE = cond 10 (0xA)
        code[bge_idx] = 0x54000000 | (((loop_end - bge_idx) & 0x7FFFF) << 5) | 10;

    // ===== WHILE =====
    } else if (strcmp(n->type, "WHILE") == 0) {

        // while true
        if (strcmp(n->value, "true") == 0) {
            int loop_start = code_len;

            // body
            if (n->right) {
                Node* stmt = n->right->left;
                while (stmt) { gen_node(stmt); stmt = stmt->next; }
            }

            // B loop_start
            int back_off = loop_start - code_len;
            emit(0x14000000 | (back_off & 0x3FFFFFF));

        } else {
            // while cond
            int loop_start = code_len;

            Node* cmp = n->left;
            gen_expr(cmp->left);
            emit(encode_mov_reg(9, 0));   // x9 = lhs
            gen_expr(cmp->right);
            emit(encode_mov_reg(1, 0));   // x1 = rhs

            // SUBS XZR, X9, X1
            emit(0xEB01013F);

            // B.cond(inverted) → exit
            int branch_idx = code_len;
            emit(0);

            // body
            if (n->right) {
                Node* stmt = n->right->left;
                while (stmt) { gen_node(stmt); stmt = stmt->next; }
            }

            // B loop_start
            int back_off = loop_start - code_len;
            emit(0x14000000 | (back_off & 0x3FFFFFF));

            // patch branch → loop_end
            int loop_end = code_len;
            const char* op = n->value;
            int cond;
            if      (strcmp(op,"==") == 0) cond = 1;   // NE
            else if (strcmp(op,"!=") == 0) cond = 0;   // EQ
            else if (strcmp(op,"<")  == 0) cond = 10;  // GE
            else if (strcmp(op,">")  == 0) cond = 13;  // LE
            else if (strcmp(op,"<=") == 0) cond = 12;  // GT
            else if (strcmp(op,">=") == 0) cond = 11;  // LT
            else cond = 1;
            code[branch_idx] = 0x54000000 | (((loop_end - branch_idx) & 0x7FFFF) << 5) | cond;
        }

    // ===== COUNTJ =====
    } else if (strcmp(n->type, "COUNTJ") == 0) {

        if (strcmp(n->left->type, "STRING") == 0) {
            int str_offset = add_string(n->left->value);
            int len        = strlen(n->left->value) + 1;
            emit(encode_mov(0, 1));
            int adr_idx = code_len;  // เก็บ index ก่อน emit placeholder
            emit(0);
            emit(encode_mov(2, len));
            emit(encode_mov(8, 64));
            emit(encode_svc(0));
            patch_idx    [patch_count] = adr_idx;
            patch_str_off[patch_count] = str_offset;
            patch_count++;

        } else if (strcmp(n->left->type, "NAME") == 0) {
            // ตรวจว่าเป็น float หรือ int
            int ftype = find_symbol_float(n->left->value);
            if (ftype == 1) {
                gen_expr_float(n->left, 0);
                gen_print_float(0);
            } else if (ftype == 2) {
                gen_expr_float(n->left, 1);
                gen_print_float(1);
            } else {
                gen_expr(n->left);
                gen_print_int();
            }

        } else if (strcmp(n->left->type, "FNUM") == 0) {
            gen_expr_float(n->left, 0);
            gen_print_float(0);

        } else {
            // ตรวจว่า expression มี float operand ไหม (เช่น a/b ที่ a,b เป็น f64)
            int ftype = node_has_float(n->left);
            if (ftype == 2) {
                gen_expr_float(n->left, 1);
                gen_print_float(1);
            } else if (ftype == 1) {
                gen_expr_float(n->left, 0);
                gen_print_float(0);
            } else {
                gen_expr(n->left);
                gen_print_int();
            }
        }
    }
}

// ===== PATCH ADR =====

void apply_patches() {
    uint64_t load_addr   = 0x400000;
    uint64_t code_base   = load_addr + 64 + 56;
    uint64_t string_base = code_base + (uint64_t)code_len * 4;

    for (int i = 0; i < patch_count; i++) {
        int      idx      = patch_idx[i];
        int      str_off  = patch_str_off[i];
        uint64_t adr_addr = code_base + (uint64_t)idx * 4;
        uint64_t str_addr = string_base + str_off;
        int      offset   = (int)(str_addr - adr_addr);
        code[idx]         = encode_adr(1, offset);
    }
}

// ===== ELF WRITER =====

void write_elf(const char* filename) {
    uint64_t load_addr = 0x400000;
    uint64_t entry     = load_addr + 64 + 56;
    int      code_size = code_len * 4;
    uint64_t filesize  = 64 + 56 + code_size + strdata_len;

    uint8_t elf[64] = {};
    elf[0] = 0x7f; elf[1] = 'E'; elf[2] = 'L'; elf[3] = 'F';
    elf[4] = 2;    // 64-bit
    elf[5] = 1;    // little-endian
    elf[6] = 1;    // ELF version
    elf[7] = 0;    // OS/ABI = System V
    elf[16] = 2;   // ET_EXEC
    elf[17] = 0;
    elf[18] = 0xb7; // EM_AARCH64
    elf[19] = 0;
    elf[20] = 1;    // EV_CURRENT
    memcpy(elf + 24, &entry, 8);         // e_entry
    uint64_t ph_off = 64;
    memcpy(elf + 32, &ph_off, 8);        // e_phoff
    elf[52] = 64;   // e_ehsize
    elf[54] = 56;   // e_phentsize
    elf[56] = 1;    // e_phnum

    uint8_t ph[56] = {};
    uint32_t type  = 1;  // PT_LOAD
    memcpy(ph + 0,  &type,      4);
    uint32_t flags = 5;  // PF_R | PF_X
    memcpy(ph + 4,  &flags,     4);
    uint64_t offset = 0;
    memcpy(ph + 8,  &offset,    8);
    memcpy(ph + 16, &load_addr, 8);
    memcpy(ph + 24, &load_addr, 8);
    memcpy(ph + 32, &filesize,  8);
    memcpy(ph + 40, &filesize,  8);
    uint64_t align = 0x1000;
    memcpy(ph + 48, &align,     8);

    FILE* f = fopen(filename, "wb");
    if (!f) { printf("ไม่สามารถเขียนไฟล์ %s\n", filename); exit(1); }
    fwrite(elf,     1, 64,          f);
    fwrite(ph,      1, 56,          f);
    fwrite(code,    4, code_len,    f);
    fwrite(strdata, 1, strdata_len, f);
    fclose(f);

    // chmod +x
    char cmd[300];
    sprintf(cmd, "chmod +x %s", filename);
    system(cmd);
}

// ===== READ FILE =====

char* read_file(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) { printf("ไม่พบไฟล์ %s\n", path); exit(1); }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char* buf = (char*)malloc(size + 1);
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    return buf;
}

// ===== MAIN =====

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("J+ Compiler v0.1 — ARM64\n");
        printf("วิธีใช้: jplus <ไฟล์.jpp>\n");
        return 1;
    }

    char* src = read_file(argv[1]);

    Lexer  lex = {src, 0};
    Parser p   = {&lex, {}, src};
    advance(&p);

    int main_count   = 0;
    int return_count = 0;

    // Stack frame: 256 bytes
    // [sp+0  .. sp+127] = local variables  (สูงสุด 16 ตัว × 8 bytes)
    // [sp+128 .. sp+159] = itoa buffer (32 bytes)
    // [sp+160 .. sp+255] = reserved
    // SUB SP, SP, #256
    emit(0xD10403FF);  // sub sp, sp, #256

    while (p.cur.type != TOK_EOF) {
        if (p.cur.type == TOK_INCLUDE) {
            advance(&p);
            if (p.cur.type == TOK_FILENAME) advance(&p);

        } else if (p.cur.type == TOK_TYPE || p.cur.type == TOK_FTYPE) {
            // peek เพื่อแยก function กับ declare
            // ใช้ p.lex->pos (หลัง type token) ไม่ใช่ p.cur.pos
            Lexer tmp  = {p.lex->src, p.lex->pos};
            next_token(&tmp);  // ข้าม name
            Token peek = next_token(&tmp);  // ดู token ที่ 3
            if (peek.type == TOK_LPAREN) {
                while (peek.type != TOK_RPAREN && peek.type != TOK_EOF)
                    peek = next_token(&tmp);
                peek = next_token(&tmp);
            }

            if (peek.type == TOK_LBRACE) {
                // เช็ค main ซ้ำ
                Lexer tmp2 = {p.lex->src, p.lex->pos};
                next_token(&tmp2);  // ข้าม name
                // peek funcname
                Lexer tmp3 = {p.lex->src, p.cur.pos};
                next_token(&tmp3);
                Token fname = next_token(&tmp3);
                if (strcmp(fname.value, "main") == 0) {
                    main_count++;
                    if (main_count > 1)
                        error(src, p.cur.pos, "มี main มากกว่า 1 ตัว — อนุญาตได้แค่ 1");
                }
                Node* n = parse_function(&p);
                gen_node(n);
            } else {
                Node* n = parse_declare(&p);
                gen_node(n);
            }

        } else if (p.cur.type == TOK_RETURN) {
            Node* n = parse_return(&p);
            gen_node(n);

        } else if (p.cur.type == TOK_COUNTJ) {
            Node* n = parse_countj(&p);
            gen_node(n);

        } else {
            advance(&p);
        }
    }

    // apply_patches ก่อน emit add sp เพื่อให้ code_len ถูกต้อง
    apply_patches();

    // ADD SP, SP, #256
    emit(0x910403FF);

    // สร้างชื่อ output จาก input (ตัด .jpp ออก)
    char outname[256];
    strcpy(outname, argv[1]);
    char* dot = strrchr(outname, '.');
    if (dot) *dot = '\0';

    write_elf(outname);
    printf("[jplus] ✓ สร้าง %s เรียบร้อย\n", outname);

    free(src);
    return 0;
}

