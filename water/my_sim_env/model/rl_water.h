/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 *
 * File: rl_water.h
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

#ifndef rl_water_h_
#define rl_water_h_
#include <cmath>
#include "rtwtypes.h"
#include "rtw_continuous.h"
#include "rtw_solver.h"
#include "rt_nonfinite.h"
#include "rl_water_types.h"

extern "C"
{

#include "rtGetInf.h"

}

#include <cstring>
#ifndef ODE3_INTG
#define ODE3_INTG

/* ODE3 Integration Data */
struct ODE3_IntgData {
  real_T *y;                           /* output */
  real_T *f[3];                        /* derivatives */
};

#endif

/* Class declaration for model rl_water */
class rl_water final
{
  /* public data and function members */
 public:
  /* Block signals (default storage) */
  struct B_rl_water_T {
    real_T Sum1;                       /* '<S1>/Sum1' */
    real_T Sum;                        /* '<S2>/Sum' */
  };

  /* Continuous states (default storage) */
  struct X_rl_water_T {
    real_T Integrator_CSTATE;          /* '<S4>/Integrator' */
    real_T H_CSTATE;                   /* '<S2>/H' */
  };

  /* State derivatives (default storage) */
  struct XDot_rl_water_T {
    real_T Integrator_CSTATE;          /* '<S4>/Integrator' */
    real_T H_CSTATE;                   /* '<S2>/H' */
  };

  /* State disabled  */
  struct XDis_rl_water_T {
    boolean_T Integrator_CSTATE;       /* '<S4>/Integrator' */
    boolean_T H_CSTATE;                /* '<S2>/H' */
  };

  /* instance parameters, for system '<Root>' */
  struct InstP_rl_water_T {
    real_T h_goal;                     /* Variable: h_goal
                                        * Referenced by: '<S1>/Desired nWater Level'
                                        */
    real_T h_initial;                  /* Variable: h_initial
                                        * Referenced by: '<S2>/H'
                                        */
  };

  /* External inputs (root inport signals with default storage) */
  struct ExtU_rl_water_T {
    real_T Action;                     /* '<Root>/Action' */
  };

  /* External outputs (root outports fed by signals with default storage) */
  struct ExtY_rl_water_T {
    real_T State[3];                   /* '<Root>/State' */
    real_T Reward;                     /* '<Root>/Reward' */
    boolean_T Done;                    /* '<Root>/Done' */
  };

  /* Real-time Model Data Structure */
  using odeFSubArray = real_T[2];
  struct RT_MODEL_rl_water_T {
    const char_T *errorStatus;
    RTWSolverInfo solverInfo;
    X_rl_water_T *contStates;
    int_T *periodicContStateIndices;
    real_T *periodicContStateRanges;
    real_T *derivs;
    XDis_rl_water_T *contStateDisabled;
    boolean_T zCCacheNeedsReset;
    boolean_T derivCacheNeedsReset;
    boolean_T CTOutputIncnstWithState;
    real_T odeY[2];
    real_T odeF[3][2];
    ODE3_IntgData intgData;

    /*
     * Sizes:
     * The following substructure contains sizes information
     * for many of the model attributes such as inputs, outputs,
     * dwork, sample times, etc.
     */
    struct {
      int_T numContStates;
      int_T numPeriodicContStates;
      int_T numSampTimes;
    } Sizes;

    /*
     * Timing:
     * The following substructure contains information regarding
     * the timing information for the model.
     */
    struct {
      uint32_T clockTick0;
      time_T stepSize0;
      uint32_T clockTick1;
      time_T tStart;
      SimTimeStep simTimeStep;
      boolean_T stopRequestedFlag;
      time_T *t;
      time_T tArray[2];
    } Timing;

    XDis_rl_water_T* getContStateDisabled() const;
    void setContStateDisabled(XDis_rl_water_T* aContStateDisabled);
    const char_T** getErrorStatusPtr();
    X_rl_water_T* getContStates() const;
    void setContStates(X_rl_water_T* aContStates);
    boolean_T getStopRequested() const;
    void setStopRequested(boolean_T aStopRequested);
    ODE3_IntgData getIntgData() const;
    void setIntgData(ODE3_IntgData aIntgData);
    boolean_T getDerivCacheNeedsReset() const;
    void setDerivCacheNeedsReset(boolean_T aDerivCacheNeedsReset);
    const char_T* getErrorStatus() const;
    void setErrorStatus(const char_T* const aErrorStatus);
    boolean_T getContTimeOutputInconsistentWithStateAtMajorStepFlag() const;
    void setContTimeOutputInconsistentWithStateAtMajorStepFlag(boolean_T
      aContTimeOutputInconsistentWithStateAtMajorStepFlag);
    boolean_T isMajorTimeStep() const;
    const odeFSubArray* getOdeF() const;
    boolean_T isMinorTimeStep() const;
    const real_T* getOdeY() const;
    int_T* getPeriodicContStateIndices() const;
    void setPeriodicContStateIndices(int_T* aPeriodicContStateIndices);
    time_T* getTPtr() const;
    void setTPtr(time_T* aTPtr);
    real_T* getPeriodicContStateRanges() const;
    void setPeriodicContStateRanges(real_T* aPeriodicContStateRanges);
    boolean_T* getStopRequestedPtr();
    time_T** getTPtrPtr();
    time_T getTStart() const;
    boolean_T getZCCacheNeedsReset() const;
    void setZCCacheNeedsReset(boolean_T aZCCacheNeedsReset);
    real_T* getdX() const;
    void setdX(real_T* adX);
  };

  /* Copy Constructor */
  rl_water(rl_water const&) = delete;

  /* Assignment Operator */
  rl_water& operator= (rl_water const&) & = delete;

  /* Move Constructor */
  rl_water(rl_water &&) = delete;

  /* Move Assignment Operator */
  rl_water& operator= (rl_water &&) = delete;

  /* Real-Time Model get method */
  rl_water::RT_MODEL_rl_water_T * getRTM();

  /* External inputs */
  ExtU_rl_water_T rl_water_U;

  /* External outputs */
  ExtY_rl_water_T rl_water_Y;

  /* Block signals */
  B_rl_water_T rl_water_B;

  /* Block continuous states */
  X_rl_water_T rl_water_X;

  /* Block Continuous state disabled vector */
  XDis_rl_water_T rl_water_XDis;

  /* model initialize function */
  void initialize();

  /* model step function */
  void step();

  /* model terminate function */
  static void terminate();

  /* Constructor */
  rl_water();

  /* private data and function members */
 public:
  /* instance parameters */
  static const InstP_rl_water_T rl_water_InitInstP;
  InstP_rl_water_T rl_water_InstP{ rl_water_InitInstP };


  /* Global mass matrix */

  /* Continuous states update member function*/
  void rt_ertODEUpdateContinuousStates(RTWSolverInfo *si );

  /* Derivatives member function */
  void rl_water_derivatives();

  /* Real-Time Model */
  RT_MODEL_rl_water_T rl_water_M;
};

/*-
 * These blocks were eliminated from the model due to optimizations:
 *
 * Block '<Root>/Scope' : Unused code path elimination
 */

/*-
 * The generated code includes comments that allow you to trace directly
 * back to the appropriate location in the model.  The basic format
 * is <system>/block_name, where system is the system number (uniquely
 * assigned by Simulink) and block_name is the name of the block.
 *
 * Use the MATLAB hilite_system command to trace the generated code back
 * to the model.  For example,
 *
 * hilite_system('<S3>')    - opens system 3
 * hilite_system('<S3>/Kp') - opens and selects block Kp which resides in S3
 *
 * Here is the system hierarchy for this model
 *
 * '<Root>' : 'rl_water'
 * '<S1>'   : 'rl_water/Subsystem'
 * '<S2>'   : 'rl_water/Subsystem/Water-Tank System'
 * '<S3>'   : 'rl_water/Subsystem/calculate reward'
 * '<S4>'   : 'rl_water/Subsystem/generate observations'
 * '<S5>'   : 'rl_water/Subsystem/stop simulation'
 * '<S6>'   : 'rl_water/Subsystem/calculate reward/Compare To Constant'
 * '<S7>'   : 'rl_water/Subsystem/stop simulation/Compare To Constant1'
 * '<S8>'   : 'rl_water/Subsystem/stop simulation/Compare To Zero'
 */
#endif                                 /* rl_water_h_ */

/*
 * File trailer for generated code.
 *
 * [EOF]
 */
