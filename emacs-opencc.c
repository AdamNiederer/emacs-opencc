#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "stdarg.h"
#include "emacs-module.h"
#include "opencc/opencc.h"

typedef int stallman_t;
stallman_t plugin_is_GPL_compatible;

#define assert_funcall(env, fn, argc, argv, errval)                     \
  do {                                                                  \
    emacs_value fn_res = funcall(env, fn, argc, argv);                  \
    if(!env->is_not_nil(env, fn_res)) {                                 \
      env->non_local_exit_throw(env, env->intern(env, "error"), env->make_string(env, errval, strlen(errval))); \
      return env->intern(env, "nil");                                   \
    }                                                                   \
  } while(0);

#define throw(env, errval)                                              \
  do {                                                                  \
    env->non_local_exit_throw(env, env->intern(env, "error"), env->make_string(env, errval, strlen(errval))); \
    return env->intern(env, "nil");                                     \
  } while (0);

static emacs_value funcall(emacs_env* env, const char* fn, int argc, emacs_value* argv) {
  emacs_value fn_sym = env->intern(env, fn);
  emacs_value fn_args[argc];
  for(int i = 0; i < argc; i++) {
    fn_args[i] = argv[i];
  }
  return env->funcall(env, fn_sym, argc, fn_args);
}

static emacs_value message(emacs_env* env, int argc, ...) {
  emacs_value fn_sym = env->intern(env, "message");
  emacs_value fn_args[argc];
  va_list args;
  va_start(args, argc);
  for(int i = 0; i < argc; i++) {
    const char* arg = va_arg(args, const char*);
    fn_args[i] = env->make_string(env, arg, strlen(arg));
  }
  va_end(args);
  return env->funcall(env, fn_sym, argc, fn_args);
}

const char* const opencc_convert_doc = "Return a string with the second argument converted according to the configuration name in the first argument.";
static emacs_value opencc_convert(emacs_env* env, ptrdiff_t argc, emacs_value* argv, void* data) {
  // Ensure there are two string arguments
  if(argc != 2) {
    throw(env, "opencc: expected 2 arguments");
  }
  assert_funcall(env, "stringp", 1, argv, "opencc: first agument must be a string");
  assert_funcall(env, "stringp", 1, &argv[1], "opencc: second argument must be a string");

  ptrdiff_t cfg_len = 16;
  char* cfg = calloc(cfg_len, sizeof(char));

  // Turn the first argument into a char*
  if(!env->copy_string_contents(env, argv[0], cfg, &cfg_len)) {
    throw(env, "opencc: first argument is too long");
  }

  // Make sure the config is valid
  size_t accepted_cfgs_len = 14;
  const char* const accepted_cfgs[] = {
    "s2t", "t2s", "s2tw", "tw2s", "s2hk", "hk2s", "s2twp", "tw2sp", "t2tw", "tw2t", "hk2t", "t2hk", "t2jp", "jp2t",
  };

  int valid_cfg = 0;
  for(size_t i = 0; i < accepted_cfgs_len; i++) {
    if(!strcmp(cfg, accepted_cfgs[i])) {
      valid_cfg = 1;
    }
  }
  if(!valid_cfg) {
    throw(env, "opencc: invalid configuration");
  }

  // Initialize the input buffer with the string's length + a zero byte
  ptrdiff_t buf_len = 1 + env->extract_integer(env, funcall(env, "string-bytes", 1, &argv[1]));
  if(buf_len < 0) {
    throw(env, "opencc: second argument length must be >= zero");
  }
  char* buf = calloc(buf_len, sizeof(char));

  // Turn the second argument into a char*
  if(!env->copy_string_contents(env, argv[1], buf, &buf_len)) {
    throw(env, "opencc: second argument is too long");
  }

  // Initialize the result buffer (w/ some extra space in case of a 1:2 mapping)
  ptrdiff_t result_len = buf_len + buf_len / 16 + 16;
  char* result = calloc(result_len, sizeof(char));

  // Do the conversion
  opencc_t sess = opencc_open(cfg);
  int converted_len = opencc_convert_utf8_to_buffer(sess, buf, buf_len, result);
  opencc_close(sess);

  emacs_value ret = env->make_string(env, result, converted_len);
  free(cfg);
  free(buf);
  free(result);
  return ret;
}

int emacs_module_init (struct emacs_runtime *ert) {
  emacs_env *env = ert->get_environment(ert);
  emacs_value fset = env->intern(env, "fset");

  // (defun opencc-convert (string) ...)
  emacs_value opencc_convert_sym = env->intern(env, "opencc-convert");
  emacs_value opencc_convert_fn = env->make_function(env, 2, 2, opencc_convert, opencc_convert_doc, NULL);
  emacs_value opencc_convert_args[] = { opencc_convert_sym, opencc_convert_fn };
  env->funcall(env, fset, 2, opencc_convert_args);

  // (provide 'opencc)
  emacs_value feature = env->intern(env, "opencc");
  emacs_value provide = env->intern(env, "provide");
  emacs_value provide_args[] = { feature };
  env->funcall(env, provide, 1, provide_args);

  return 0;
}
