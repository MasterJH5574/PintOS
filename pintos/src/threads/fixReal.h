#ifndef ZYH_fixReal_H
#define ZYH_fixReal_H
typedef struct {
  int value;
} fixReal;

fixReal int2Real(int);
int real2IntTrunc(fixReal);
int real2IntRound(fixReal);
/*-----------------------------*/
fixReal realAdd(fixReal, fixReal);
fixReal realAddInt(fixReal, int);
fixReal realSub(fixReal, fixReal);
fixReal realSubInt(fixReal, int);
fixReal intSubReal(int, fixReal);
fixReal realMul(fixReal, fixReal);
fixReal realMulInt(fixReal, int);
fixReal realDiv(fixReal, fixReal);
fixReal realDivInt(fixReal, int);
fixReal intDivReal(int, fixReal);
fixReal intDiv(int, int);

#endif