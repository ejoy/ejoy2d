#include "matrix.h"
#include <stdint.h>

static inline int
_inverse_scale(const int *m , int *o) {
	if (m[0] == 0 || m[3] == 0)
		return 1;
	o[0] = (1024 * 1024) / m[0];
	o[1] = 0;
	o[2] = 0;
	o[3] = (1024 * 1024) / m[3];
	o[4] = - (m[4] * o[0]) / 1024;
	o[5] = - (m[5] * o[3]) / 1024;
	return 0;
}

static inline int
_inverse_rot(const int *m, int *o) {
	if (m[1] == 0 || m[2] == 0)
		return 1;
	o[0] = 0;
	o[1] = (1024 * 1024) / m[2];
	o[2] = (1024 * 1024) / m[1];
	o[3] = 0;
	o[4] = - (m[5] * o[2]) / 1024;
	o[5] = - (m[4] * o[1]) / 1024;
	return 0;
}

int
matrix_inverse(const struct matrix *mm, struct matrix *mo) {
	const int *m = mm->m;
	int *o = mo->m;
	if (m[1] == 0 && m[2] == 0) {
		return _inverse_scale(m,o);
	}
	if (m[0] == 0 && m[3] == 0) {
		return _inverse_rot(m,o);
	}
	int t = m[0] * m[3] - m[1] * m[2] ;
	if (t == 0)
		return 1;
	o[0] = (int32_t)((int64_t)m[3] * (1024 * 1024) / t);
	o[1] = (int32_t)(- (int64_t)m[1] * (1024 * 1024) / t);
	o[2] = (int32_t)(- (int64_t)m[2] * (1024 * 1024) / t);
	o[3] = (int32_t)((int64_t)m[0] * (1024 * 1024) / t);
	o[4] = - (m[4] * o[0] + m[5] * o[2]) / 1024;
	o[5] = - (m[4] * o[1] + m[5] * o[3]) / 1024;
	return 0;
}

// SRT to matrix

static inline int
icost(int dd) {
	static int t[256] = {
		1024,1023,1022,1021,1019,1016,1012,1008,1004,999,993,986,979,972,964,955,
		946,936,925,914,903,890,878,865,851,837,822,807,791,775,758,741,
		724,706,687,668,649,629,609,589,568,547,526,504,482,460,437,414,
		391,368,344,321,297,273,248,224,199,175,150,125,100,75,50,25,
		0,-25,-50,-75,-100,-125,-150,-175,-199,-224,-248,-273,-297,-321,-344,-368,
		-391,-414,-437,-460,-482,-504,-526,-547,-568,-589,-609,-629,-649,-668,-687,-706,
		-724,-741,-758,-775,-791,-807,-822,-837,-851,-865,-878,-890,-903,-914,-925,-936,
		-946,-955,-964,-972,-979,-986,-993,-999,-1004,-1008,-1012,-1016,-1019,-1021,-1022,-1023,
		-1024,-1023,-1022,-1021,-1019,-1016,-1012,-1008,-1004,-999,-993,-986,-979,-972,-964,-955,
		-946,-936,-925,-914,-903,-890,-878,-865,-851,-837,-822,-807,-791,-775,-758,-741,
		-724,-706,-687,-668,-649,-629,-609,-589,-568,-547,-526,-504,-482,-460,-437,-414,
		-391,-368,-344,-321,-297,-273,-248,-224,-199,-175,-150,-125,-100,-75,-50,-25,
		0,25,50,75,100,125,150,175,199,224,248,273,297,321,344,368,
		391,414,437,460,482,504,526,547,568,589,609,629,649,668,687,706,
		724,741,758,775,791,807,822,837,851,865,878,890,903,914,925,936,
		946,955,964,972,979,986,993,999,1004,1008,1012,1016,1019,1021,1022,1023,
	};
	if (dd < 0) {
		dd = 256 - (-dd % 256);
	} else {
		dd %= 256;
	}

	return t[dd];
}

static inline int
icosd(int d) {
	int dd = d/4;
	return icost(dd);
}

static inline int
isind(int d) {
	int dd = 64 - d/4;
	return icost(dd);
}

static inline void
rot_mat(int *m, int d) {
	if (d==0)
		return;
	int cosd = icosd(d);
	int sind = isind(d);

	int m0_cosd = m[0] * cosd;
	int m0_sind = m[0] * sind;
	int m1_cosd = m[1] * cosd;
	int m1_sind = m[1] * sind;
	int m2_cosd = m[2] * cosd;
	int m2_sind = m[2] * sind;
	int m3_cosd = m[3] * cosd;
	int m3_sind = m[3] * sind;
	int m4_cosd = m[4] * cosd;
	int m4_sind = m[4] * sind;
	int m5_cosd = m[5] * cosd;
	int m5_sind = m[5] * sind;

	m[0] = (m0_cosd - m1_sind) /1024;
	m[1] = (m0_sind + m1_cosd) /1024;
	m[2] = (m2_cosd - m3_sind) /1024;
	m[3] = (m2_sind + m3_cosd) /1024;
	m[4] = (m4_cosd - m5_sind) /1024;
	m[5] = (m4_sind + m5_cosd) /1024;
}

static inline void
scale_mat(int *m, int sx, int sy) {
	if (sx != 1024) {
		m[0] = m[0] * sx / 1024;
		m[2] = m[2] * sx / 1024;
		m[4] = m[4] * sx / 1024;
	}
	if (sy != 1024) {
		m[1] = m[1] * sy / 1024;
		m[3] = m[3] * sy / 1024;
		m[5] = m[5] * sy / 1024;
	}
}

void
matrix_srt(struct matrix *mm, const struct srt *srt) {
	if (!srt) {
		return;
	}
	scale_mat(mm->m, srt->scalex, srt->scaley);
	rot_mat(mm->m, srt->rot);
	mm->m[4] += srt->offx;
	mm->m[5] += srt->offy;
}

void
matrix_rot(struct matrix *m, int rot) {
	rot_mat(m->m, rot);
}

void
matrix_sr(struct matrix *mat, int sx, int sy, int d) {
	int *m = mat->m;
	int cosd = icosd(d);
	int sind = isind(d);

	int m0_cosd = sx * cosd;
	int m0_sind = sx * sind;
	int m3_cosd = sy * cosd;
	int m3_sind = sy * sind;

	m[0] = m0_cosd /1024;
	m[1] = m0_sind /1024;
	m[2] = -m3_sind /1024;
	m[3] = m3_cosd /1024;
}

void
matrix_rs(struct matrix *mat, int sx, int sy, int d) {
	int *m = mat->m;
	int cosd = icosd(d);
	int sind = isind(d);

	int m0_cosd = sx * cosd;
	int m0_sind = sx * sind;
	int m3_cosd = sy * cosd;
	int m3_sind = sy * sind;

	m[0] = m0_cosd /1024;
	m[1] = m3_sind /1024;
	m[2] = -m0_sind /1024;
	m[3] = m3_cosd /1024;
}

void
matrix_scale(struct matrix *m, int sx, int sy) {
	scale_mat(m->m, sx, sy);
}

