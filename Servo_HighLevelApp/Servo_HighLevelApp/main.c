// This sample C application for Azure Sphere demonstrates how to use Servos with Pulse Width
// Modulation (PWM).
// The sample opens a PWM controller. Changing the servoConfig will affect how the servo behaves
//
// It uses the API for the following Azure Sphere application libraries:
// - pwm (Pulse Width Modulation)
// - log (messages shown in Visual Studio's Device Output window during debugging)

#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#include <applibs/log.h>
#include <applibs/pwm.h>
#include "servo.h"

// By default, this sample is targeted at the MT3620 Reference Development Board (RDB).
// This can be changed using the project property "Target Hardware Definition Directory".
// This #include imports the sample_hardware abstraction from that hardware definition.
#include <hw/sample_hardware.h>

// This sample uses a single-thread event loop pattern, based on epoll and timerfd
#include "epoll_timerfd_utilities.h"

// File descriptors - initialized to invalid value
static int pwmFd = -1;
static int stepTimerFd = -1;
static int epollFd = -1;

struct _SERVO_State* servo;

// Each time the step timer fires (every stepIntervalNs), we increase the desired angle
// (angle) by the step increment (stepIncrementAngle), until the max angle (maxAngle) 
// is reached, at which point the opposite will happen, we decrease the desired angle
// (angle) by the step increment (stepIncrementAngle), until the min angle (minAngle) 
// is reached. Then it starts over.
// Your servos might have slightly different duty cycle so you might want to edit the 
// config values.
static const unsigned int periodNs = 20000000;
static const unsigned int maxDutyCycleNs = 2400000;
static const unsigned int minDutyCycleNs = 600000;
static const int minAngle = 0;
static const int maxAngle = 180;
static int stepIncrementAngle = 1;
static int angle = 0;

// Timer state variables
static const struct timespec stepIntervalNs = {.tv_sec = 0, .tv_nsec = 10000000};

// Termination state
static volatile sig_atomic_t terminationRequired = false;

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    terminationRequired = true;
}

/// <summary>
///     Handle Servo timer event: change Servo angle.
/// </summary>
static void StepTimerEventHandler(EventData *eventData)
{
    if (ConsumeTimerFdEvent(stepTimerFd) != 0) {
        terminationRequired = true;
        return;
    }

    // The step interval has elapsed, so change the angle.
	if ((angle > maxAngle && stepIncrementAngle > 0) || (angle < minAngle && stepIncrementAngle < 0))
		stepIncrementAngle = -1 * stepIncrementAngle;

	angle += stepIncrementAngle;

	int result = SERVO_SetAngle(servo, angle);
	if (result != 0) { terminationRequired = true; }
}

/// <summary>
///     Creates a servo config and initializes a servo with it
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>y
static int InitServo(int pwmFd, unsigned int channel, struct _SERVO_State** servo, int minAngle, int maxAngle)
{
	struct SERVO_Config servoConfig;

	servoConfig.pwmFd = pwmFd;
	servoConfig.pwmChannel = channel;
	servoConfig.minAngleDeg = minAngle;
	servoConfig.maxAngleDeg = maxAngle;
	servoConfig.minPulseNs = minDutyCycleNs;
	servoConfig.maxPulseNs = maxDutyCycleNs;
	servoConfig.periodNs = periodNs;

	if (SERVO_Init(&servoConfig, servo) < 0)
	{
		Log_Debug("Error initializing servo 0\n");
		return -1;
	}

	return 0;
}

// Event handler data structures. Only the event handler field needs to be populated.
static EventData stepTimerEventData = { .eventHandler = &StepTimerEventHandler };

/// <summary>
///     Set up SIGTERM termination handler, initialize peripherals, and set up event handlers.
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
static int InitPeripheralsAndHandlers(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

    epollFd = CreateEpollFd();
    if (epollFd < 0) {
        return -1;
    }

    stepTimerFd = CreateTimerFdAndAddToEpoll(epollFd, &stepIntervalNs, &stepTimerEventData, EPOLLIN);
    if (stepTimerFd < 0) {
        return -1;
    }

    pwmFd = PWM_Open(PWM_CONTROLLER);
    if (pwmFd == -1) {
        Log_Debug(
            "Error opening PWM_CONTROLLER: %s (%d). Check that app_manifest.json "
            "includes the PWM used.\n",
            strerror(errno), errno);
        return -1;
    }

	InitServo(pwmFd, SERVO_PWM_CHANNEL, &servo, minAngle, maxAngle);

    return 0;
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
	SERVO_Destroy(servo);

    Log_Debug("Closing file descriptors.\n");
    CloseFdAndPrintError(pwmFd, "PwmFd");
    CloseFdAndPrintError(stepTimerFd, "stepTimerFd");
    CloseFdAndPrintError(epollFd, "epollFd");
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("Starting PWM Sample\n");
    if (InitPeripheralsAndHandlers() != 0) {
        terminationRequired = true;
    }

    // Use epoll to wait for events and trigger handlers, until an error or SIGTERM happens
    while (!terminationRequired) {
        if (WaitForEventAndCallHandler(epollFd) != 0) {
            terminationRequired = true;
        }
    }

    ClosePeripheralsAndHandlers();
    Log_Debug("Application exiting.\n");

    return 0;
}