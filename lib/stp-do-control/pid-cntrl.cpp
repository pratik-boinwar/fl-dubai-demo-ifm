#include "pid-cntrl.h"
#include <stdio.h>

void PIDInit(PIDController *controller, double Kp, double Ki, double Kd, double setpoint,
             double maxOutput, double minOutput, double maxIntegral, double minIntegral,
             double setpointWeight, double derivativeFilter, double sampleTime)
{
    controller->Kp = Kp;
    controller->Ki = Ki;
    controller->Kd = Kd;

    controller->setpoint = setpoint;
    controller->integral = 0.0;
    controller->prevError = 0.0;

    controller->maxOutput = maxOutput;
    controller->minOutput = minOutput;
    controller->maxIntegral = maxIntegral;
    controller->minIntegral = minIntegral;

    controller->setpointWeight = setpointWeight;
    controller->derivativeFilter = derivativeFilter;

    controller->prevInput = 0.0;
    controller->sampleTime = sampleTime;
}

void PIDReset(PIDController *controller)
{
    controller->integral = 0.0;
    controller->prevError = 0.0;
    controller->prevInput = 0.0;
}

double PIDUpdate(PIDController *controller, double input)
{
    double error = controller->setpoint - input;
    double output;

    // Proportional term
    double proportional = controller->Kp * error;

    // Integral term
    double integral = controller->integral + (controller->Ki * error * controller->sampleTime);
    if (integral > controller->maxIntegral)
        integral = controller->maxIntegral;
    else if (integral < controller->minIntegral)
        integral = controller->minIntegral;

    // Anti-windup
    if (proportional + integral > controller->maxOutput)
        integral = controller->maxOutput - proportional;
    else if (proportional + integral < controller->minOutput)
        integral = controller->minOutput - proportional;

    controller->integral = integral;

    // Derivative term
    double derivative = (controller->Kd * (input - controller->prevInput)) / controller->sampleTime;
    derivative = (1.0 - controller->derivativeFilter) * derivative +
                 controller->derivativeFilter * controller->prevError;

    // Calculate the output
    output = proportional + integral + derivative;

    // Output saturation
    if (output > controller->maxOutput)
        output = controller->maxOutput;
    else if (output < controller->minOutput)
        output = controller->minOutput;

    // Update previous values
    controller->prevInput = input;
    controller->prevError = derivative;

    return output;

    // double error = controller->setpoint - input;
    // double output = 0.0;

    // // Proportional term
    // output += controller->Kp * error;

    // // Integral term
    // if (controller->Ki != 0.0)
    // {
    //     controller->integral += controller->Ki * error * controller->sampleTime;

    //     if (controller->integral > controller->maxIntegral)
    //         controller->integral = controller->maxIntegral;
    //     else if (controller->integral < controller->minIntegral)
    //         controller->integral = controller->minIntegral;

    //     output += controller->integral;
    // }

    // // Derivative term
    // if (controller->Kd != 0.0)
    // {
    //     double derivative = (error - controller->prevError) / controller->sampleTime;

    //     // Low-pass filter
    //     derivative = controller->prevInput + (controller->derivativeFilter * (derivative - controller->prevInput));

    //     output += controller->Kd * derivative;
    //     controller->prevError = error;
    //     controller->prevInput = derivative;
    // }

    // // Setpoint weight
    // // output = (controller->setpointWeight * controller->setpoint) + ((1.0 - controller->setpointWeight) * output);

    // // Restrict output
    // if (output > controller->maxOutput)
    //     output = controller->maxOutput;
    // else if (output < controller->minOutput)
    //     output = controller->minOutput;

    // return output;
}

// double ComputePID(float sensorReadig)
// {
//     double error, P, I, D, PIDvalue, PIDsum;

//     error = setPoint - sensorReadig;

//     P = error * kp;

//     PIDsum += error;

//     I = PIDsum * ki;

//     D = (error - lastError) * kd;

//     PIDvalue = P + I + D;

//     lastError = error;

//     return PIDvalue;
// }

void PIDSetSetpoint(PIDController *controller, double setpoint)
{
    controller->setpoint = setpoint;
}

void PIDSetKp(PIDController *controller, double Kp)
{
    controller->Kp = Kp;
}

void PIDSetKi(PIDController *controller, double Ki)
{
    controller->Ki = Ki;
}

void PIDSetKd(PIDController *controller, double Kd)
{
    controller->Kd = Kd;
}

void PIDSetMaxOutput(PIDController *controller, double maxOutput)
{
    controller->maxOutput = maxOutput;
}

void PIDSetMinOutput(PIDController *controller, double minOutput)
{
    controller->minOutput = minOutput;
}

void PIDSetMaxIntegral(PIDController *controller, double maxIntegral)
{
    controller->maxIntegral = maxIntegral;
}

void PIDSetMinIntegral(PIDController *controller, double minIntegral)
{
    controller->minIntegral = minIntegral;
}

void PIDSetSetpointWeight(PIDController *controller, double setpointWeight)
{
    controller->setpointWeight = setpointWeight;
}

void PIDSetDerivativeFilter(PIDController *controller, double derivativeFilter)
{
    controller->derivativeFilter = derivativeFilter;
}

void PIDSetSampleTime(PIDController *controller, double sampleTime)
{
    controller->sampleTime = sampleTime;
}
