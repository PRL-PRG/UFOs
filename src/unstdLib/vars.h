
#define mVar(a, _a) __typeof__(a) _a = (a)
#define swap(a, b) ({ mVar(a, _tmp); a = b; b = _tmp; })

/**
 * Removes an element `from[idx]` by assigning the value into `to` and then assigning `from[idx]` = `zero`
 * Macro prevents double evaluation of everything except "from"
 */
#define takeElement(from, to, idx, zero) ({ \
  mVar(idx, _idx);                          \
  to = from[_idx];                         \
  from[_idx] = zero;                       \
})
