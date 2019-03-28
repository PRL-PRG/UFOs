
#define mVar(a, _a) __typeof__(a) _a = (a)
#define swap(a, b) ({ mVar(a, _tmp); a = b; b = _tmp; })

/**
 * Removes an element `from[idx]` by assigning the value into `to` and then assigning `from[idx]` = `zero`
 * Macro prevents double evaluation by using temporary variable for from and idx
 */
#define takeElement(from, to, idx, zero) ({ \
  mVar(from, _from);                        \
  mVar(idx, _idx);                          \
  to = _from[_idx];                         \
  _from[_idx] = zero;                       \
})
