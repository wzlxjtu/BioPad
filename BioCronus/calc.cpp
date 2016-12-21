#include "calc.h"

// estimate stress from respiration
double RESP2STRESS(double resp)
{
	double stress;
	double H=H_RR, L=L_RR;
	if (resp < L)
		stress = 0;
	else if (resp > H)
		stress = 1;
	else 
		stress = 1/(H-L)*(resp-L);
	return stress;
}

// estimate stress from hrv
double HRV2STRESS(double hrv_relax, double hrv_stress, int hrv)
{
	double stress;
	if (hrv < hrv_stress)
		stress = 1;
	else if (hrv > hrv_relax)
		stress = 0;
	else
		stress = 1/(hrv_stress-hrv_relax)*(hrv-hrv_relax);
	return stress;
}
 
// the jitter interference
void Jittered_Mod(int8_t output[], float STRESS)
{
	int jittered;
	//output[XB360_LT] = min(output[XB360_LT] + 90 * STRESS, 100); //speed retard
	jittered = output[XB360_LX] + MaxJitter * Jitter * intensity;
	//jittered = output[XB360_LX] + JitterDegree * STRESS * MaxJitter * intensity;
	if (jittered > 100)
		jittered = 100;
	if (jittered < -100)
		jittered = -100;
	output[XB360_LX] = jittered;
}

// the speed interference
void Speed_Only(int8_t output[], float STRESS)
{
	if(!ControlActive)
		output[XB360_RT] = 35;
	else
		output[XB360_RT] = (0.25 * (1 - STRESS) + 0.1) * 100;
	//STRESS 0 - 1
	//SpeedX 0.35 - 0.1
}

// decrease the speed button
void Speed_Decrease(int8_t output[], float STRESS)
{
	if(ControlActive)
	{
		/*output[XB360_RT] = pow(1 - STRESS, 3) * output[XB360_RT];*/
		output[XB360_RT] = max(output[XB360_RT] - 150 * STRESS, 0);
		//output[XB360_LT] = min(output[XB360_LT] + 50 * STRESS, 100);
		//output[XB360_RT] = 80;
		if (STRESS > 0)
			output[XB360_LT] = min(output[XB360_LT] + 30, 100);
	}
}

// increase the speed button
void Speed_Increase(int8_t output[], float STRESS)
{
	if(ControlActive)
	{
		output[XB360_RT] = min(output[XB360_RT] + 100 * STRESS, 100);
		if (STRESS > 0)
			output[XB360_LT] = max(output[XB360_LT] - 500 * STRESS, 0);
	}
}

// applying break button
void Break_Mod(int8_t output[], float STRESS)
{
	if (Break)
	{
		output[XB360_LT] = 100;
		output[XB360_RT] = 0;
	}
}

// change sensitivity of buttons
void Sensitive_Mod(int8_t output[], float STRESS, int sensitive)
{
	int sensed;
	//output[XB360_LT] = min(output[XB360_LT] + 90 * STRESS, 100); //speed retard
	sensed = output[XB360_LX] * (1+ sensitive * STRESS);
	if ( sensed >= 0)
		output[XB360_LX] = min(sensed, 100);
	else
		output[XB360_LX] = max(sensed, -100);
}
 