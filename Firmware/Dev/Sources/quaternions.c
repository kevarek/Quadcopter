/*
*Filename: 	quaternions.c
*Author: 	Stanislav Subrt
*Year:		2014
*/

#include "main.h"
#include "quaternions.h"
//TODO IMPLEMENT CORTEX M4 DSP optimalisation


//Calculates quaternion product
void quat_GetProduct(quat_QuatTd *result, quat_QuatTd *qa, quat_QuatTd *qb){
  result->w = qa->w * qb->w - qa->x * qb->x - qa->y * qb->y - qa->z * qb->z;
  result->x = qa->w * qb->x + qa->x * qb->w + qa->y * qb->z - qa->z * qb->y;
  result->y = qa->w * qb->y - qa->x * qb->z + qa->y * qb->w + qa->z * qb->x;
  result->z = qa->w * qb->z + qa->x * qb->y - qa->y * qb->x + qa->z * qb->w;
}


//Calculates quaternion conjugate
void quat_GetConjugate(quat_QuatTd *qa){
	if( qa == 0 ){
		return;
	}

	qa->w *= 1;
	qa->x *= -1;
	qa->y *= -1;
	qa->z *= -1;
}


//Converts quaternion to Euler angles (Thanks to FreeIMU dev team!!!)
void quat_GetYawPitchRoll(quat_QuatTd * qa, float *yaw, float *pitch, float *roll){
	float gx, gy, gz;

	if( qa==0 ){
		return;
	}

	//Get gravity direction from quaternion
	gx = 2 * ((qa->x)*(qa->z) - (qa->w)*(qa->y));
	gy = 2 * ((qa->w)*(qa->x) + (qa->y)*(qa->z));
	gz = (qa->w)*(qa->w) - (qa->x)*(qa->x) - (qa->y)*(qa->y) + (qa->z)*(qa->z);



	//Calculate yaw pitch and roll if needed
	if( yaw != 0 ){
		*yaw = atan2(2 * (qa->x) * (qa->y) - 2 * (qa->w) * (qa->z), 2 * (qa->w)*(qa->w) + 2 * (qa->x) * (qa->x) - 1);
	}

	if( pitch != 0 ){
		*pitch = atan(gx / sqrt(gy*gy + gz*gz));
	}

	if( roll != 0 ){
		*roll = atan(gy / sqrt(gx*gx + gz*gz));
	}
}


//Converts quaternion to Euler angles (Thanks to FreeIMU dev team!!!)
void quat_GetYawPitchRollDeg(quat_QuatTd * qa, float *yaw, float *pitch, float *roll){
	float y, p, r;

	if( qa==0 ){
		return;
	}

	quat_GetYawPitchRoll(qa, &y, &p, &r);

	if( yaw != 0 ){
		*yaw = util_RadToDeg(y);
	}

	if( pitch != 0 ){
		*pitch = util_RadToDeg(p);
	}

	if( roll != 0 ){
		*roll = util_RadToDeg(r);
	}
}


//Fast inverse square-root
//See: http://en.wikipedia.org/wiki/Fast_inverse_square_root
static float InvSqrt(float x) {
	float halfx = 0.5f * x;
	float y = x;
	long i = *(long*)&y;
	i = 0x5f3759df - (i>>1);
	y = *(float*)&i;
	y = y * (1.5f - (halfx * y * y));
	return y;
}


void quat_Normalize(quat_QuatTd *q){
	float InvMag, w, x, y, z;

	if( q == 0 ){
		return;
	}

	w = q->w;
	x = q->x;
	y = q->y;
	z = q->z;

	InvMag = InvSqrt(w*w + x*x + y*y + z*z);
	q->w = w * InvMag;
	q->x = x * InvMag;
	q->y = y * InvMag;
	q->z = z * InvMag;
}


void quat_GetAngleAxis(quat_QuatTd *res, float angleRad, float x, float y, float z){
	float Tmp;

	if( res == 0 ){
		return;
	}

	Tmp = angleRad / 2;
	res->w = cos(Tmp);

	Tmp = sin(Tmp);
	res->x = x * Tmp;
	res->y = y * Tmp;
	res->z = z * Tmp;
}


void quat_Copy(quat_QuatTd *destQ, quat_QuatTd *srcQ){
	if( ( destQ == 0 )  || ( srcQ == 0 ) ){
		return;
	}

	destQ->w = srcQ->w;
	destQ->x = srcQ->x;
	destQ->y = srcQ->y;
	destQ->z = srcQ->z;
}
