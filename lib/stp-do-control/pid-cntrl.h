#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

typedef struct
{
    double Kp; // Proportional gain
    double Ki; // Integral gain
    double Kd; // Derivative gain

    double setpoint;  // Desired DO level
    double integral;  // Integral term
    double prevError; // Previous error for derivative term

    double maxOutput;   // Maximum output value
    double minOutput;   // Minimum output value
    double maxIntegral; // Maximum integral value
    double minIntegral; // Minimum integral value

    double setpointWeight;   // Setpoint weighting factor
    double derivativeFilter; // Derivative filter coefficient

    double prevInput;  // Previous input value
    double sampleTime; // Sampling time interval

} PIDController;

void PIDInit(PIDController *controller, double Kp, double Ki, double Kd, double setpoint,
             double maxOutput, double minOutput, double maxIntegral, double minIntegral,
             double setpointWeight, double derivativeFilter, double sampleTime);
void PIDReset(PIDController *controller);
double PIDUpdate(PIDController *controller, double input);
void PIDSetSetpoint(PIDController *controller, double setpoint);
void PIDSetKp(PIDController *controller, double Kp);
void PIDSetKi(PIDController *controller, double Ki);
void PIDSetKd(PIDController *controller, double Kd);
void PIDSetMaxOutput(PIDController *controller, double maxOutput);
void PIDSetMinOutput(PIDController *controller, double minOutput);
void PIDSetMaxIntegral(PIDController *controller, double maxIntegral);
void PIDSetMinIntegral(PIDController *controller, double minIntegral);
void PIDSetSetpointWeight(PIDController *controller, double setpointWeight);
void PIDSetDerivativeFilter(PIDController *controller, double derivativeFilter);
void PIDSetSampleTime(PIDController *controller, double sampleTime);

#endif // PID_CONTROLLER_H
