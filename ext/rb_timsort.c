#include <ruby.h>
#include "timSort.h"

static VALUE sort_reentered(VALUE);
static int   tsort_1(const void *, const void *, void *);
static int   tsort_2(const void *, const void *, void *);
static VALUE ary_tsort_bang(VALUE);
static VALUE enum_tsort(VALUE);
static VALUE ary_tsort(VALUE);

struct ary_sort_data {
  VALUE ary;
  int opt_methods;
  int opt_inited;
};

static VALUE
sort_reentered(VALUE ary)
{
  if (RBASIC(ary)->klass) {
    rb_raise(rb_eRuntimeError, "sort reentered");
  }
  return Qnil;
}

static int
tsort_1(const void *ap, const void *bp, void *dummy)
{
  struct ary_sort_data *data = dummy;
  VALUE retval = sort_reentered(data->ary);
  VALUE a = *(const VALUE *)ap, b = *(const VALUE *)bp;
  int n;
  
  retval = rb_yield_values(2, a, b);
  n = rb_cmpint(retval, a, b);
  sort_reentered(data->ary);
  return n;
}

static int
tsort_2(const void *ap, const void *bp, void *dummy)
{
  struct ary_sort_data *data = dummy;
  VALUE retval = sort_reentered(data->ary);
  VALUE a = *(const VALUE *)ap, b = *(const VALUE *)bp;
  int n;
  
  if (FIXNUM_P(a) && FIXNUM_P(b) && SORT_OPTIMIZABLE(data, Fixnum)) {
    if ((long)a > (long)b) return 1;
    if ((long)a < (long)b) return -1;
    return 0;
  }
  if (STRING_P(a) && STRING_P(b) && SORT_OPTIMIZABLE(data, String)) {
    return rb_str_cmp(a, b);
  }
  
  retval = rb_funcall(a, id_cmp, 1, b);
  n = rb_cmpint(retval, a, b);
  sort_reentered(data->ary);
  
  return n;
}

static VALUE
ary_tsort_bang(VALUE ary)
{
  rb_ary_modify(ary);
  assert(!ARY_SHARED_P(ary));
  if (RARRAY_LEN(ary) > 1) {
    VALUE tmp = ary_make_substitution(ary); /* only ary refers tmp */
    struct ary_sort_data data;
    
    RBASIC(tmp)->klass = 0;
    data.ary = tmp;
    data.opt_methods = 0;
    data.opt_inited = 0;

    
    timSort(RARRAY_PTR(tmp), RARRAY_LEN(tmp), rb_block_given_p()?tsort_1:tsort_2);


    if (ARY_EMBED_P(tmp)) {
      assert(ARY_EMBED_P(tmp));
      if (ARY_SHARED_P(ary)) { /* ary might be destructively operated in the given block */
        rb_ary_unshare(ary);
      }
      FL_SET_EMBED(ary);
      MEMCPY(RARRAY_PTR(ary), ARY_EMBED_PTR(tmp), VALUE, ARY_EMBED_LEN(tmp));
      ARY_SET_LEN(ary, ARY_EMBED_LEN(tmp));
    }
    else {
      assert(!ARY_EMBED_P(tmp));
      if (ARY_HEAP_PTR(ary) == ARY_HEAP_PTR(tmp)) {
        assert(!ARY_EMBED_P(ary));
        FL_UNSET_SHARED(ary);
        ARY_SET_CAPA(ary, ARY_CAPA(tmp));
      }
      else {
        assert(!ARY_SHARED_P(tmp));
        if (ARY_EMBED_P(ary)) {
          FL_UNSET_EMBED(ary);
        }
        else if (ARY_SHARED_P(ary)) {
          /* ary might be destructively operated in the given block */
          rb_ary_unshare(ary);
        }
        else {
          xfree(ARY_HEAP_PTR(ary));
        }
        ARY_SET_PTR(ary, RARRAY_PTR(tmp));
        ARY_SET_HEAP_LEN(ary, RARRAY_LEN(tmp));
        ARY_SET_CAPA(ary, ARY_CAPA(tmp));
      }
      /* tmp was lost ownership for the ptr */
      FL_UNSET(tmp, FL_FREEZE);
      FL_SET_EMBED(tmp);
      ARY_SET_EMBED_LEN(tmp, 0);
      FL_SET(tmp, FL_FREEZE);
    }
    /* tmp will be GC'ed. */
    RBASIC(tmp)->klass = rb_cArray;
  }
  return ary;
}

static VALUE
enum_tsort(VALUE self)
{
  return ary_tsort(enum_to_a(0, 0, self));
}

static VALUE
ary_tsort(VALUE ary)
{
  ary = rb_ary_dup(ary);
  ary_tsort_bang(ary);
  return ary;
}

/*
static VALUE
enum_tsort_by(VALUE self)
{

}

static VALUE
ary_tsort_by(VALUE self)
{

}

static VALUE
ary_tsort_by_bang(VALUE self)
{

}
*/

void
Init_timsort()
{
  rb_define_method(rb_cArray, "tsort", ary_tsort, 0);
  rb_define_method(rb_cArray, "tsort!", ary_tsort_bang, 0);
  rb_define_method(rb_mEnumerable, "tsort", enum_tsort, 0);

  // rb_define_method(rb_cArray, "tsort_by", ary_tsort_by, 0);
  // rb_define_method(rb_cArray, "tsort_by!", ary_tsort_by_bang, 0);
  // rb_define_method(rb_mEnumerable, "tsort_by", enum_tsort_by, 0);
}
