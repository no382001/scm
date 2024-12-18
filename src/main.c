#include <stdio.h>

#include "interpreter.h"
#include "parser.h"
#include "print.h"

const tag_t ATOM = 0x7ff7, PRIM = 0x7ff8, FORE = 0x7ff6, CONS = 0x7ff9, CLOS = 0x7ffa,
            NIL = 0x7ffb, MACR = 0x7ffc, NOP = 0x7ffd, VECTOR = 0x7ffe,
            STRING = 0x7fff;

prim_t token_buffer[TOKEN_BUFFER_SIZE] = {0};

tag_t T(expr_t x) { return *(unsigned long long *)&x >> 48; }

extern parse_ctx *curr_ctx;
extern parse_ctx default_ctx;
extern int paren_count;

extern proc_t prim[];

void init_interpreter(interpreter_t *ctx) {

  ic_sp = N;
  ic_hp = 0;

  ic_c_nil = box(NIL, 0);
  ic_c_err = atom("ERR", ic2llcref);
  ic_c_nop = box(NOP, 0);
  ic_c_tru = atom("#t", ic2llcref);

  ic_env = pair(ic_c_tru, ic_c_tru, ic_c_nil, ic2llcref);

  ic_prim = prim;

  for (int i = 0; ic_prim[i].s; ++i) {
    ic_env =
        pair(atom(ic_prim[i].s, ic2llcref), box(PRIM, i), ic_env, ic2llcref);
  }
}

bool read_line(read_ctx_t *rctx) {

  if (looking_at() == EOF) {
    if (curr_ctx->file != stdin || curr_ctx->injected) {
      if (rctx->f_load_layer != -1) {
        return true; // give back control, we are too deep to switch to stdin
      }
      switch_ctx_to_stdin(curr_ctx);
      flush();
    } else {
      return true;
    }
  }

  while (looking_at() != '\n' && looking_at() != EOF &&
         rctx->i < TOKEN_BUFFER_SIZE) {
    rctx->tb->buffer[rctx->i++] = scan();

    type_t prev = rctx->tb->buffer[rctx->i - 1].t;
    if (prev == TAG_LPAREN || prev == TAG_VECTOR) {
      rctx->open_parens++;
    } else if (prev == TAG_RPAREN) {
      rctx->open_parens--;
    }

    if (!rctx->ic->noprint) {
      // print_token(rctx->tb->buffer[rctx->i - 1]);
      // fflush(stdout);
    }
  }

  if (looking_at() == '\n' && rctx->i < TOKEN_BUFFER_SIZE) {
    prim_t r = {.t = TAG_NEWLINE};
    rctx->tb->buffer[rctx->i++] = r;
    if (!rctx->ic->noprint) {
      // print_token(rctx->tb->buffer[rctx->i - 1]);
    }
  }

  return false;
}

expr_t repl(read_ctx_t *rc) {
  interpreter_t *ctx = rc->ic; // need these for the macros
  expr_t result = ic_c_nop;
  while (1) {

    if (rc->read(rc)) { // implementation defined
      break;
    }
    if (rc->open_parens) {
      flush();
      advance();
      continue;
    }

    if (rc->i >= TOKEN_BUFFER_SIZE) {
      printf("token buffer overflow. increase buffer size!\n");
      break;
    }

    int jmpres = !rc->ic->nosetjmp ? setjmp(rc->ic->llc.jumps.jb) : 0;
    if (jmpres == 1) {
      if (!rc->ic->noreseterr) {
        print_and_reset_error(rc->ic);
        gc(rc->ic);
      }
    } else if (jmpres == 2) {
      result = eval(ic_rcso.x, ic_rcso.e, rc->ic);
    } else {
      result = eval(parse(rc->ic, rc->tb), ic_env, rc->ic);
    }

    if (!equ(ic_c_err, result) && !equ(ic_c_nop, result)) {
      if (!rc->ic->noprint) {
        printf("%u>", ic_sp - ic_hp / 8);
        print(result, rc->ic);
        putchar('\n');
      }
    }

    rc->i = 0;
    memset(rc->tb->buffer, 0, TOKEN_BUFFER_SIZE * sizeof(prim_t));

    if (curr_ctx->once) {
      break;
    }

    reseti(rc->tb);
    flush();
    advance();
  }

  return result;
}

bool reg_foreign_func(interpreter_t* ctx, const char* name, func_ptr_t f){
  if (ic_fore_size >= MAX_FF_COUNT){
    return false;
  }
  proc_t* new_proc = &ic_fore[ic_fore_size];
  new_proc->f = f;
  new_proc->s = name;
  ic_env = pair(atom(name, ic2llcref),box(FORE, ic_fore_size++), ic_env, ic2llcref);
  return true;
}

expr_t ff_test(expr_t t, expr_t *e, interpreter_t *ctx) {
  return num(1);
}

#ifndef UNITY_TEST
extern int token_idx;

int main(int argc, char **argv) {
  default_ctx.file = stdin;
  if (argc > 2 && strcmp(argv[1], "-e") == 0) { // will exit at once
    switch_ctx_inject_string(argv[2]);
  } else if (argc > 2 &&
             strcmp(argv[1], "-f") == 0) { // cant exit context for some reason
    FILE *file = fopen(argv[2], "r");
    if (file == NULL) {
      perror("error opening file");
      return 1;
    }
    switch_ctx_to_file(file);
  } else {
    curr_ctx =
        &default_ctx; // this should just be a config you can never change
  }

  interpreter_t intrp = {0};
  init_interpreter(&intrp);

  reg_foreign_func(&intrp,"test",ff_test);

  advance();

  token_buffer_t tb = {0};
  read_ctx_t rc = {.ic = &intrp, .tb = &tb, .read = read_line};
  repl(&rc);

  return 0;
}
#endif
