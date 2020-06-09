#ifndef ZYH_fixReal_C
#define ZYH_fixReal_C
#include "fixReal.h"
fixReal int2Real(int num) {
  fixReal it = {num << 14};
  return it;
}
int real2IntTrunc(fixReal num) {
  return num.value >> 14;
}
int real2IntRound(fixReal num) {
  return (num.value >= 0) ? 
          ((num.value + (1 << 13)) >> 14) : 
          ((num.value - (1 << 13)) >> 14);
}
/*-----------------------------*/
fixReal realAdd(fixReal a, fixReal b) {
  fixReal it = {a.value + b.value};
  return it;
}
fixReal realAddInt(fixReal a, int b) {
  fixReal it = {a.value + (b << 14)};
  return it;
}
fixReal realSub(fixReal a, fixReal b) {
  fixReal it = {a.value - b.value};
  return it;
}
fixReal realSubInt(fixReal a, int b) {
  fixReal it = {a.value - (b << 14)};
  return it;
}
fixReal intSubReal(int a, fixReal b) {
  fixReal it = {(a << 14) - b.value};
  return it;
}
fixReal realMul(fixReal a, fixReal b) {
  fixReal it = {((long long) a.value * b.value) >> 14};
  return it;
}
fixReal realMulInt(fixReal a, int b) {
  fixReal it = {a.value * b};
  return it;
}
fixReal realDiv(fixReal a, fixReal b) {
  fixReal it = {( ((long long) a.value) << 14) / b.value};
  return it;
}
fixReal realDivInt(fixReal a, int b) {
  fixReal it = {a.value / b};
  return it;
}
fixReal intDivReal(int a, fixReal b) {
  fixReal it = {( ((long long) a) << 14) / b.value};
  return it;
}
fixReal intDiv(int a, int b) {
  fixReal it = {( ((long long) a) << 14) / b};
  return it;
}
#endif