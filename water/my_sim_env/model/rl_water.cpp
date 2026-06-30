/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 *
 * File: rl_water.cpp
 *
 * Code generated for Simulink model 'rl_water'.
 *
 * Model version                  : 11.18
 * Simulink Coder version         : 24.2 (R2024b) 21-Jun-2024
 * C/C++ source code generated on : Tue Jun 30 00:53:43 2026
 *
 * Target selection: ert.tlc
 * Embedded hardware selection: Specified
 * Code generation objective: Execution efficiency
 * Validation result: Not run
 */

#include "rl_water.h"
#include <cmath>
#include "rtwtypes.h"
#include "rl_water_private.h"

/*
 * This function updates continuous states using the ODE3 fixed-step
 * solver algorithm
 */
void rl_water::rt_ertODEUpdateContinuousStates(RTWSolverInfo *si )
{
  /* Solver Matrices */
  static const real_T rt_ODE3_A[3]{
    1.0/2.0, 3.0/4.0, 1.0
  };

  static const real_T rt_ODE3_B[3][3]{
    { 1.0/2.0, 0.0, 0.0 },

    { 0.0, 3.0/4.0, 0.0 },

    { 2.0/9.0, 1.0/3.0, 4.0/9.0 }
  };

  time_T t { rtsiGetT(si) };

  time_T tnew { rtsiGetSolverStopTime(si) };

  time_T h { rtsiGetStepSize(si) };

  real_T *x { rtsiGetContStates(si) };

  ODE3_IntgData *id { static_cast<ODE3_IntgData *>(rtsiGetSolverData(si)) };

  real_T *y { id->y };

  real_T *f0 { id->f[0] };

  real_T *f1 { id->f[1] };

  real_T *f2 { id->f[2] };

  real_T hB[3];
  int_T i;
  int_T nXc { 2 };

  rtsiSetSimTimeStep(si,MINOR_TIME_STEP);

  /* Save the state values at time t in y, we'll use x as ynew. */
  (void) std::memcpy(y, x,
                     static_cast<uint_T>(nXc)*sizeof(real_T));

  /* Assumes that rtsiSetT and ModelOutputs are up-to-date */
  /* f0 = f(t,y) */
  rtsiSetdX(si, f0);
  rl_water_derivatives();

  /* f(:,2) = feval(odefile, t + hA(1), y + f*hB(:,1), args(:)(*)); */
  hB[0] = h * rt_ODE3_B[0][0];
  for (i = 0; i < nXc; i++) {
    x[i] = y[i] + (f0[i]*hB[0]);
  }

  rtsiSetT(si, t + h*rt_ODE3_A[0]);
  rtsiSetdX(si, f1);
  this->step();
  rl_water_derivatives();

  /* f(:,3) = feval(odefile, t + hA(2), y + f*hB(:,2), args(:)(*)); */
  for (i = 0; i <= 1; i++) {
    hB[i] = h * rt_ODE3_B[1][i];
  }

  for (i = 0; i < nXc; i++) {
    x[i] = y[i] + (f0[i]*hB[0] + f1[i]*hB[1]);
  }

  rtsiSetT(si, t + h*rt_ODE3_A[1]);
  rtsiSetdX(si, f2);
  this->step();
  rl_water_derivatives();

  /* tnew = t + hA(3);
     ynew = y + f*hB(:,3); */
  for (i = 0; i <= 2; i++) {
    hB[i] = h * rt_ODE3_B[2][i];
  }

  for (i = 0; i < nXc; i++) {
    x[i] = y[i] + (f0[i]*hB[0] + f1[i]*hB[1] + f2[i]*hB[2]);
  }

  rtsiSetT(si, tnew);
  rtsiSetSimTimeStep(si,MAJOR_TIME_STEP);
}

/* Model step function */
void rl_water::step()
{
  int8_T rtb_Gain1;
  uint8_T rtb_Gain;
  boolean_T rtb_OR;
  if ((&rl_water_M)->isMajorTimeStep()) {
    /* set solver stop time */
    rtsiSetSolverStopTime(&(&rl_water_M)->solverInfo,(((&rl_water_M)
      ->Timing.clockTick0+1)*(&rl_water_M)->Timing.stepSize0));
  }                                    /* end MajorTimeStep */

  /* Update absolute time of base rate at minor time step */
  if ((&rl_water_M)->isMinorTimeStep()) {
    (&rl_water_M)->Timing.t[0] = rtsiGetT(&(&rl_water_M)->solverInfo);
  }

  /* Integrator: '<S4>/Integrator' */
  /* Limited  Integrator  */
  if (rl_water_X.Integrator_CSTATE >= 10.0) {
    rl_water_X.Integrator_CSTATE = 10.0;
  } else if (rl_water_X.Integrator_CSTATE <= -10.0) {
    rl_water_X.Integrator_CSTATE = -10.0;
  }

  /* Integrator: '<S2>/H' */
  /* Limited  Integrator  */
  if ((!(rl_water_X.H_CSTATE >= (rtInf))) && (rl_water_X.H_CSTATE <= 0.0)) {
    rl_water_X.H_CSTATE = 0.0;
  }

  /* Sum: '<S1>/Sum1' incorporates:
   *  Constant: '<S1>/Desired nWater Level'
   *  Integrator: '<S2>/H'
   */
  rl_water_B.Sum1 = rl_water_InstP.h_goal - rl_water_X.H_CSTATE;

  /* Outport: '<Root>/State' incorporates:
   *  Integrator: '<S2>/H'
   *  Integrator: '<S4>/Integrator'
   */
  rl_water_Y.State[0] = rl_water_X.Integrator_CSTATE;
  rl_water_Y.State[1] = rl_water_B.Sum1;
  rl_water_Y.State[2] = rl_water_X.H_CSTATE;

  /* RelationalOperator: '<S6>/Compare' incorporates:
   *  Abs: '<S3>/Abs'
   *  Constant: '<S6>/Constant'
   */
  rtb_OR = (std::abs(rl_water_B.Sum1) < 0.1);

  /* Gain: '<S3>/Gain' */
  rtb_Gain = static_cast<uint8_T>(rtb_OR ? 160 : 0);

  /* Gain: '<S3>/Gain1' incorporates:
   *  Logic: '<S3>/NOT'
   */
  rtb_Gain1 = static_cast<int8_T>(!rtb_OR ? -128 : 0);

  /* Logic: '<S5>/OR' incorporates:
   *  Constant: '<S7>/Constant'
   *  Constant: '<S8>/Constant'
   *  Integrator: '<S2>/H'
   *  RelationalOperator: '<S7>/Compare'
   *  RelationalOperator: '<S8>/Compare'
   */
  rtb_OR = ((rl_water_X.H_CSTATE >= 20.0) || (rl_water_X.H_CSTATE <= 0.0));

  /* Outport: '<Root>/Reward' incorporates:
   *  Gain: '<S3>/Gain'
   *  Gain: '<S3>/Gain1'
   *  Gain: '<S3>/Gain2'
   *  Sum: '<S3>/Sum'
   */
  rl_water_Y.Reward = (static_cast<real_T>(rtb_Gain) * 0.0625 + static_cast<
                       real_T>(rtb_Gain1) * 0.0078125) + static_cast<real_T>
    (rtb_OR ? -100 : 0);

  /* Outport: '<Root>/Done' */
  rl_water_Y.Done = rtb_OR;

  /* Sum: '<S2>/Sum' incorporates:
   *  Gain: '<S2>/a//A'
   *  Gain: '<S2>/b//A'
   *  Inport: '<Root>/Action'
   *  Integrator: '<S2>/H'
   *  Sqrt: '<S2>/Sqrt'
   */
  rl_water_B.Sum = 0.25 * rl_water_U.Action - 0.1 * std::sqrt
    (rl_water_X.H_CSTATE);
  if ((&rl_water_M)->isMajorTimeStep()) {
    rt_ertODEUpdateContinuousStates(&(&rl_water_M)->solverInfo);

    /* Update absolute time for base rate */
    /* The "clockTick0" counts the number of times the code of this task has
     * been executed. The absolute time is the multiplication of "clockTick0"
     * and "Timing.stepSize0". Size of "clockTick0" ensures timer will not
     * overflow during the application lifespan selected.
     */
    ++(&rl_water_M)->Timing.clockTick0;
    (&rl_water_M)->Timing.t[0] = rtsiGetSolverStopTime(&(&rl_water_M)
      ->solverInfo);

    {
      /* Update absolute timer for sample time: [1.0s, 0.0s] */
      /* The "clockTick1" counts the number of times the code of this task has
       * been executed. The resolution of this integer timer is 1.0, which is the step size
       * of the task. Size of "clockTick1" ensures timer will not overflow during the
       * application lifespan selected.
       */
      (&rl_water_M)->Timing.clockTick1++;
    }
  }                                    /* end MajorTimeStep */
}

/* Derivatives for root system: '<Root>' */
void rl_water::rl_water_derivatives()
{
  rl_water::XDot_rl_water_T *_rtXdot;
  boolean_T lsat;
  boolean_T usat;
  _rtXdot = ((XDot_rl_water_T *) (&rl_water_M)->derivs);

  /* Derivatives for Integrator: '<S4>/Integrator' */
  lsat = (rl_water_X.Integrator_CSTATE <= -10.0);
  usat = (rl_water_X.Integrator_CSTATE >= 10.0);
  if (((!lsat) && (!usat)) || (lsat && (rl_water_B.Sum1 > 0.0)) || (usat &&
       (rl_water_B.Sum1 < 0.0))) {
    _rtXdot->Integrator_CSTATE = rl_water_B.Sum1;
  } else {
    /* in saturation */
    _rtXdot->Integrator_CSTATE = 0.0;
  }

  /* End of Derivatives for Integrator: '<S4>/Integrator' */

  /* Derivatives for Integrator: '<S2>/H' */
  lsat = (rl_water_X.H_CSTATE <= 0.0);
  usat = (rl_water_X.H_CSTATE >= (rtInf));
  if (((!lsat) && (!usat)) || (lsat && (rl_water_B.Sum > 0.0)) || (usat &&
       (rl_water_B.Sum < 0.0))) {
    _rtXdot->H_CSTATE = rl_water_B.Sum;
  } else {
    /* in saturation */
    _rtXdot->H_CSTATE = 0.0;
  }

  /* End of Derivatives for Integrator: '<S2>/H' */
}

/* Model initialize function */
void rl_water::initialize()
{
  /* Registration code */
  {
    {
      /* Setup solver object */
      rtsiSetSimTimeStepPtr(&(&rl_water_M)->solverInfo, &(&rl_water_M)
                            ->Timing.simTimeStep);
      rtsiSetTPtr(&(&rl_water_M)->solverInfo, (&rl_water_M)->getTPtrPtr());
      rtsiSetStepSizePtr(&(&rl_water_M)->solverInfo, &(&rl_water_M)
                         ->Timing.stepSize0);
      rtsiSetdXPtr(&(&rl_water_M)->solverInfo, &(&rl_water_M)->derivs);
      rtsiSetContStatesPtr(&(&rl_water_M)->solverInfo, (real_T **) &(&rl_water_M)
                           ->contStates);
      rtsiSetNumContStatesPtr(&(&rl_water_M)->solverInfo, &(&rl_water_M)
        ->Sizes.numContStates);
      rtsiSetNumPeriodicContStatesPtr(&(&rl_water_M)->solverInfo, &(&rl_water_M
        )->Sizes.numPeriodicContStates);
      rtsiSetPeriodicContStateIndicesPtr(&(&rl_water_M)->solverInfo,
        &(&rl_water_M)->periodicContStateIndices);
      rtsiSetPeriodicContStateRangesPtr(&(&rl_water_M)->solverInfo,
        &(&rl_water_M)->periodicContStateRanges);
      rtsiSetContStateDisabledPtr(&(&rl_water_M)->solverInfo, (boolean_T**)
        &(&rl_water_M)->contStateDisabled);
      rtsiSetErrorStatusPtr(&(&rl_water_M)->solverInfo, (&rl_water_M)
                            ->getErrorStatusPtr());
      rtsiSetRTModelPtr(&(&rl_water_M)->solverInfo, (&rl_water_M));
    }

    rtsiSetSimTimeStep(&(&rl_water_M)->solverInfo, MAJOR_TIME_STEP);
    rtsiSetIsMinorTimeStepWithModeChange(&(&rl_water_M)->solverInfo, false);
    rtsiSetIsContModeFrozen(&(&rl_water_M)->solverInfo, false);
    (&rl_water_M)->intgData.y = (&rl_water_M)->odeY;
    (&rl_water_M)->intgData.f[0] = (&rl_water_M)->odeF[0];
    (&rl_water_M)->intgData.f[1] = (&rl_water_M)->odeF[1];
    (&rl_water_M)->intgData.f[2] = (&rl_water_M)->odeF[2];
    (&rl_water_M)->contStates = ((X_rl_water_T *) &rl_water_X);
    (&rl_water_M)->contStateDisabled = ((XDis_rl_water_T *) &rl_water_XDis);
    (&rl_water_M)->Timing.tStart = (0.0);
    rtsiSetSolverData(&(&rl_water_M)->solverInfo, static_cast<void *>
                      (&(&rl_water_M)->intgData));
    rtsiSetSolverName(&(&rl_water_M)->solverInfo,"ode3");
    (&rl_water_M)->setTPtr(&(&rl_water_M)->Timing.tArray[0]);
    (&rl_water_M)->Timing.stepSize0 = 1.0;
  }

  /* InitializeConditions for Integrator: '<S4>/Integrator' */
  rl_water_X.Integrator_CSTATE = 0.0;

  /* InitializeConditions for Integrator: '<S2>/H' */
  rl_water_X.H_CSTATE = rl_water_InstP.h_initial;
}

/* Model terminate function */
void rl_water::terminate()
{
  /* (no terminate code required) */
}

rl_water::XDis_rl_water_T* rl_water::RT_MODEL_rl_water_T::getContStateDisabled() const
{
  return contStateDisabled;
}

void rl_water::RT_MODEL_rl_water_T::setContStateDisabled(XDis_rl_water_T
  * aContStateDisabled)
{
  contStateDisabled = aContStateDisabled;
}

const char_T** rl_water::RT_MODEL_rl_water_T::getErrorStatusPtr()
{
  return &errorStatus;
}

rl_water::X_rl_water_T* rl_water::RT_MODEL_rl_water_T::getContStates() const
{
  return contStates;
}

void rl_water::RT_MODEL_rl_water_T::setContStates(X_rl_water_T* aContStates)
{
  contStates = aContStates;
}

boolean_T rl_water::RT_MODEL_rl_water_T::getStopRequested() const
{
  return (Timing.stopRequestedFlag);
}

void rl_water::RT_MODEL_rl_water_T::setStopRequested(boolean_T aStopRequested)
{
  (Timing.stopRequestedFlag = aStopRequested);
}

ODE3_IntgData rl_water::RT_MODEL_rl_water_T::getIntgData() const
{
  return intgData;
}

void rl_water::RT_MODEL_rl_water_T::setIntgData(ODE3_IntgData aIntgData)
{
  intgData = aIntgData;
}

boolean_T rl_water::RT_MODEL_rl_water_T::getDerivCacheNeedsReset() const
{
  return derivCacheNeedsReset;
}

void rl_water::RT_MODEL_rl_water_T::setDerivCacheNeedsReset(boolean_T
  aDerivCacheNeedsReset)
{
  derivCacheNeedsReset = aDerivCacheNeedsReset;
}

const char_T* rl_water::RT_MODEL_rl_water_T::getErrorStatus() const
{
  return (errorStatus);
}

void rl_water::RT_MODEL_rl_water_T::setErrorStatus(const char_T* const
  aErrorStatus)
{
  (errorStatus = aErrorStatus);
}

boolean_T rl_water::RT_MODEL_rl_water_T::
  getContTimeOutputInconsistentWithStateAtMajorStepFlag() const
{
  return CTOutputIncnstWithState;
}

void rl_water::RT_MODEL_rl_water_T::
  setContTimeOutputInconsistentWithStateAtMajorStepFlag(boolean_T
  aContTimeOutputInconsistentWithStateAtMajorStepFlag)
{
  CTOutputIncnstWithState = aContTimeOutputInconsistentWithStateAtMajorStepFlag;
}

boolean_T rl_water::RT_MODEL_rl_water_T::isMajorTimeStep() const
{
  return ((Timing.simTimeStep) == MAJOR_TIME_STEP);
}

const rl_water::odeFSubArray* rl_water::RT_MODEL_rl_water_T::getOdeF() const
{
  return odeF;
}

boolean_T rl_water::RT_MODEL_rl_water_T::isMinorTimeStep() const
{
  return ((Timing.simTimeStep) == MINOR_TIME_STEP);
}

const real_T* rl_water::RT_MODEL_rl_water_T::getOdeY() const
{
  return odeY;
}

int_T* rl_water::RT_MODEL_rl_water_T::getPeriodicContStateIndices() const
{
  return periodicContStateIndices;
}

void rl_water::RT_MODEL_rl_water_T::setPeriodicContStateIndices(int_T
  * aPeriodicContStateIndices)
{
  periodicContStateIndices = aPeriodicContStateIndices;
}

time_T* rl_water::RT_MODEL_rl_water_T::getTPtr() const
{
  return (Timing.t);
}

void rl_water::RT_MODEL_rl_water_T::setTPtr(time_T* aTPtr)
{
  (Timing.t = aTPtr);
}

real_T* rl_water::RT_MODEL_rl_water_T::getPeriodicContStateRanges() const
{
  return periodicContStateRanges;
}

void rl_water::RT_MODEL_rl_water_T::setPeriodicContStateRanges(real_T
  * aPeriodicContStateRanges)
{
  periodicContStateRanges = aPeriodicContStateRanges;
}

boolean_T* rl_water::RT_MODEL_rl_water_T::getStopRequestedPtr()
{
  return (&(Timing.stopRequestedFlag));
}

time_T** rl_water::RT_MODEL_rl_water_T::getTPtrPtr()
{
  return &(Timing.t);
}

time_T rl_water::RT_MODEL_rl_water_T::getTStart() const
{
  return (Timing.tStart);
}

boolean_T rl_water::RT_MODEL_rl_water_T::getZCCacheNeedsReset() const
{
  return zCCacheNeedsReset;
}

void rl_water::RT_MODEL_rl_water_T::setZCCacheNeedsReset(boolean_T
  aZCCacheNeedsReset)
{
  zCCacheNeedsReset = aZCCacheNeedsReset;
}

real_T* rl_water::RT_MODEL_rl_water_T::getdX() const
{
  return derivs;
}

void rl_water::RT_MODEL_rl_water_T::setdX(real_T* adX)
{
  derivs = adX;
}

/* Constructor */
rl_water::rl_water() :
  rl_water_U(),
  rl_water_Y(),
  rl_water_B(),
  rl_water_X(),
  rl_water_XDis(),
  rl_water_M()
{
  /* Currently there is no constructor body generated.*/
}

/* Real-Time Model get method */
rl_water::RT_MODEL_rl_water_T * rl_water::getRTM()
{
  return (&rl_water_M);
}

/*
 * File trailer for generated code.
 *
 * [EOF]
 */
