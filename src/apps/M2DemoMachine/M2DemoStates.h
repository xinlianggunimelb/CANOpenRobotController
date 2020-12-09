/**
 * /file M2DemoState.h
 * \author Vincent Crocher
 * \version 0.1
 * \date 2020-12-09
 *
 * \copyright Copyright (c) 2020
 *
 */

#ifndef M2DemoSTATE_H_DEF
#define M2DemoSTATE_H_DEF

#include <time.h>
#include <iostream>

#include "RobotM2.h"
#include "State.h"

using namespace std;

/**
 * \brief Conversion from a timespec structure to seconds (double)
 *
 */
double timeval_to_sec(struct timespec *ts);

/**
 * \brief Generic state type for used with M2DemoMachine, providing running time and iterations number.
 *
 */
class M2TimedState : public State {
   protected:
    /**
    *  \todo Might be good to make these Const
    *
    */
    RobotM2 *robot;                               /*<!Pointer to state machines robot object*/

    M2TimedState(StateMachine *m, RobotM2 *M2, const char *name = NULL): State(m, name), robot(M2){};
   private:
    void entry(void) final {
        std::cout
        << "==================================" << std::endl
        << " STARTING  " << getName() << std::endl
        << "----------------------------------" << std::endl
        << std::endl;

        //Timing
        clock_gettime(CLOCK_MONOTONIC, &initTime);
        lastTime = timeval_to_sec(&initTime);

        elapsedTime=0;
        iterations=0;

        //Actual state entry
        entryCode();
    };
    void during(void) final {
        //Compute some basic time values
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);

        double now = timeval_to_sec(&ts);
        elapsedTime = (now-timeval_to_sec(&initTime));
        dt = now - lastTime;
        lastTime = now;

        iterations++;

        //Actual state during
        duringCode();
    };
    void exit(void) final {
        exitCode();
        std::cout
        << "----------------------------------" << std::endl
        << "EXIT "<< getName() << std::endl
        << "==================================" << std::endl
        << std::endl;
    };

   public:
    virtual void entryCode(){};
    virtual void duringCode(){};
    virtual void exitCode(){};


   protected:
    struct timespec initTime;   /*<! Time of state init */
    double lastTime;            /*<! Time of last during() call (in seconds since state init())*/
    double elapsedTime;         /*<! Time since state init() in seconds*/
    double dt;                  /*<! Time between last two during() calls (in seconds)*/
    unsigned long int iterations;
};


class M2DemoState : public M2TimedState {

   public:
    M2DemoState(StateMachine *m, RobotM2 *M2, const char *name = "M2 Test State"):M2TimedState(m, M2, name){};

    void entryCode(void);
    void duringCode(void);
    void exitCode(void);

    VM2 qi, Xi;
};



/**
 * \brief Position calibration of M2. Go to the bottom left stops of robot at constant torque for absolute position calibration.
 *
 */
class M2CalibState : public M2TimedState {

   public:
    M2CalibState(StateMachine *m, RobotM2 *M2, const char *name = "M2 Calib State"):M2TimedState(m, M2, name){};

    void entryCode(void);
    void duringCode(void);
    void exitCode(void);

    bool isCalibDone() {return calibDone;}

   private:
     VM2 stop_reached_time;
     bool at_stop[2];
     bool calibDone=false;
};


/**
 * \brief Provide end-effector mass compensation on M2. Mass is controllable through keyboard inputs.
 *
 */
class M2Transparent : public M2TimedState {

   public:
    M2Transparent(StateMachine *m, RobotM2 *M2, const char *name = "M2 Transparent"):M2TimedState(m, M2, name){};

    void entryCode(void);
    void duringCode(void);
    void exitCode(void);
};


/**
 * \brief End-effector velocity control through joystick input.
 *
 */
class M2EndEffDemo : public M2TimedState {

   public:
    M2EndEffDemo(StateMachine *m, RobotM2 *M2, const char *name = "M2 Velocity Control Demo"):M2TimedState(m, M2, name){};

    void entryCode(void);
    void duringCode(void);
    void exitCode(void);
};

/**
 * \brief Basic impedance control on a static point.
 *
 */
class M2DemoImpedanceState : public M2TimedState {

   public:
    M2DemoImpedanceState(StateMachine *m, RobotM2 *M2, const char *name = "M2 Demo Impedance State"):M2TimedState(m, M2, name){};

    void entryCode(void);
    void duringCode(void);
    void exitCode(void);

   private:
    VM2 Xi;
    double k = 700;     //! Impedance proportional gain (spring)
    double d = 2;       //! Impedance derivative gain (damper)
    bool init=false;

    unsigned int nb_samples=10000;
    double dts[10000];
    double dX[10000];
    int new_value;
};


/**
 * \brief Path contraint with viscous assistance.
 *
 */
class M2DemoPathState : public M2TimedState {

   public:
    M2DemoPathState(StateMachine *m, RobotM2 *M2, const char *name = "M2 Demo Path State"):M2TimedState(m, M2, name){};

    void entryCode(void);
    void duringCode(void);
    void exitCode(void);

    short int sign(double val) {return (val>0)?1:((val<0)?-1:0); };

   private:
    double k = 600;                 //! Impedance proportional gain (spring)
    double d = 6;                   //! Impedance derivative gain (damper)
    double viscous_assistance=15;   //! Viscous assistance along path
    VM2 Xi;
    VM2 Xf={0.4, 0.3};              //! Path target point
};


/**
 * \brief Point to tpoint position control with min jerk trajectory interpolation
 *
 */
class M2DemoMinJerkPosition: public M2TimedState {

   public:
    M2DemoMinJerkPosition(StateMachine *m, RobotM2 *M2, const char *name = "M2 Demo Minimum Jerk Position"):M2TimedState(m, M2, name){};

    void entryCode(void);
    void duringCode(void);
    void exitCode(void);

   private:
    static const unsigned int TrajNbPts=4;
    unsigned int TrajPtIdx=0;
    double startTime;
    VM2 TrajPt[TrajNbPts]={VM2(0.1, 0.1), VM2(0.3, 0.3), VM2(0.4, 0.1), VM2(0.4, 0.4)};
    double TrajTime[TrajNbPts]={5, 3, 0.5, 0.5};
    VM2 Xi, Xf;
    double T;
    float k_i=1.; //Integral gain
};



/**
 * \brief Sampling frequency estimation state.
 *
 */
class M2SamplingEstimationState : public M2TimedState {

   public:
    M2SamplingEstimationState(StateMachine *m, RobotM2 *M2, const char *name = "M2 Sampling time estimation State"):M2TimedState(m, M2, name){};

    void entryCode(void);
    void duringCode(void);
    void exitCode(void);

   private:
    unsigned int nb_samples=10000;
    double dts[10000];
    double dX[10000];
    int new_value;
};


#endif