#include <stdio.h>
#include "pid-cntrl.h"
#include "stdint.h"

void ControlAerator(double speed)
{
    // Code to control the VFD drive and set the aerator speed
    // Replace this with the actual VFD control code
    /* Code to control VFD */
    printf("Aerator speed: %.2f\n", speed);
}

// Function to simulate the DO measurement
double MeasureDO()
{
    double doValue;
    // Simulated DO measurement
    // Replace this with the actual DO measurement code

    // doValue = /* Code to read DO value */;
    return doValue;
}

int main()
{
    double dt = 1.0;               // Time interval between control loop iterations (in seconds)
    double doSetpoint = 6.0;       // Desired DO level (setpoint)
    double maxOutput = 100.0;      // Maximum output value
    double minOutput = 0.0;        // Minimum output value
    double maxIntegral = 50.0;     // Maximum integral value
    double minIntegral = -0;       // Minimum integral value
    double setpointWeight = 0.8;   // Setpoint weighting factor
    double derivativeFilter = 0.2; // Derivative filter coefficient

    double doMeasurement;
    double controlSignal;
    uint32_t lastTime = 0;

    // MovingAverageFilter filter;
    // double buffer[128];
    // MovingAverageFilterInit(&filter, buffer, 10);

    // Create a new PID controller instance
    PIDController controller;

    PIDInit(&controller, 1.0, 0.5, 0.2, doSetpoint, maxOutput, minOutput, maxIntegral, minIntegral,
            setpointWeight, derivativeFilter, dt);

    while (1)
    {
        if (millis() - lastTime >= controller.sampleTime * 1000)
        {
            lastTime = millis();
            // Measure the current DO level
            doMeasurement = MeasureDO();
            // double movAvgDoVal = MovingAverageFilterUpdate(&filter, doMeasurement);

            // // Update the PID controller and compute the control signal
            // controlSignal = PIDUpdate(&controller, movAvgDoVal);

            // Update the PID controller and compute the control signal
            controlSignal = PIDUpdate(&controller, doMeasurement);

            // Control the aerator speed based on the control signal
            ControlAerator(controlSignal);
        }

        // Wait for the next iteration
        // Replace this with the actual delay or timing mechanism
        /* Code for delay or timing mechanism */
    }

    return 0;
}

// cmd to set kp, ki, kd, setpoint, maxOutput, minOutput, maxIntegral, minIntegral, setpointWeight, derivativeFilter, sampleTime
// cmd to set movingavg size
// pid code will in pid lib files
// movingavg code will be in movingavg lib files
// add oled code to display the do value
// on button long presss for 5 sec set the setpoint
