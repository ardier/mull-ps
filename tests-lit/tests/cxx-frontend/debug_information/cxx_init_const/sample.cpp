extern "C" {
extern int printf(const char *, ...);
}

int plus10(int arg) {
  int base = 10;
  return base + arg;
}

int main() {
  if (plus10(5) == 15) {
    printf("NORMAL\n");
    return 0;
  } else {
    printf("MUTATED\n");
    return 1;
  }
}

// clang-format off

/**
RUN: %CLANG_EXEC -Xclang -load -Xclang %mull_frontend_cxx -Xclang -add-plugin -Xclang mull-cxx-frontend -Xclang -plugin-arg-mull-cxx-frontend -Xclang mutators=cxx_init_const %s -o %s.exe | %FILECHECK_EXEC %s --dump-input=fail --strict-whitespace --match-full-lines --check-prefix=FRONTEND
FRONTEND:Recording mutation point: cxx_init_const:{{.*}}/sample.cpp:6:14 (end: 6:16)

RUN: %s.exe | %FILECHECK_EXEC %s --dump-input=fail --strict-whitespace --match-full-lines --check-prefix=STANDALONE_WITHOUT_MUTATION
RUN: (env "cxx_init_const:%s:6:14"=1 %s.exe || true) | %FILECHECK_EXEC %s --dump-input=fail --strict-whitespace --match-full-lines --check-prefix=STANDALONE_WITH_MUTATION

STANDALONE_WITHOUT_MUTATION:NORMAL
STANDALONE_WITH_MUTATION:MUTATED

RUN: %mull_runner %s.exe -ide-reporter-show-killed | %FILECHECK_EXEC %s --dump-input=fail --strict-whitespace --match-full-lines --check-prefix=MULL_RUNNER

MULL_RUNNER:[info] Killed mutants (1/1):
MULL_RUNNER:{{.*}}sample.cpp:6:14: warning: Killed: Replaced 'T a = b' with 'T a = 42' [cxx_init_const]
*/
