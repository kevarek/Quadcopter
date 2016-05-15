/*
*Filename: quaternions.h
*Author: Stanislav Subrt
*Year: 2014
*/

#ifndef QUATERNIONS_H
#define QUATERNIONS_H

typedef struct{
	float w;
	float x;
	float y;
	float z;
}quat_QuatTd;


extern void quat_Copy(quat_QuatTd *destQ, quat_QuatTd *srcQ);
extern void quat_Normalize(quat_QuatTd *q);

extern void quat_GetAngleAxis(quat_QuatTd *res, float angleRad, float x, float y, float z);
extern void quat_GetProduct(quat_QuatTd *result, quat_QuatTd *qa, quat_QuatTd *qb);
extern void quat_GetConjugate(quat_QuatTd *qa);

extern void quat_GetYawPitchRoll(quat_QuatTd * qa, float *roll, float *pitch, float *yaw) ;
extern void quat_GetYawPitchRollDeg(quat_QuatTd * qa, float *yaw, float *pitch, float *roll);


#endif  //end of QUATERNIONS_H
