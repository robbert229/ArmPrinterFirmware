#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ARMStrongHost.h>

// The incorrectly named algorithm that approximates a line on 5 dimensions. There are no differences between axis. This allows multiple extruders to be added easily.
void breshenham(int x0, int y0, int z0, int a0, int b0, int x1, int y1, int z1,
		int a1, int b1, long speed, FILE *fout) {

	int dx, dy, dz, da, db; // Signed distances
	int sx, sy, sz, sa, sb; // Signs of the distances
	int adx, ady, adz, ada, adb; // Absolute distances
	int adx2, ady2, adz2, ada2, adb2; // Absolute distances * 2
	int err1, err2, err3, err4; // Error/Deviation Ratio
	int xIter, yIter, zIter, aIter, bIter; // Iteration Variables for axes
	int x_step, y_step, z_step, a_step, b_step;

	dx = x1 - x0;
	dy = y1 - y0;
	dz = z1 - z0;
	da = a1 - a0;
	db = b1 - b0;

	if (dx < 0)
		sx = -1;
	else
		sx = 1;

	if (dy < 0)
		sy = -1;
	else
		sy = 1;

	if (dz < 0)
		sz = -1;
	else
		sz = 1;

	if (da < 0)
		sa = -1;
	else
		sa = 1;

	if (db < 0)
		sb = -1;
	else
		sb = 1;

	adx = abs(dx);
	ady = abs(dy);
	adz = abs(dz);
	ada = abs(da);
	adb = abs(db);

	adx2 = 2 * adx;
	ady2 = 2 * ady;
	adz2 = 2 * adz;
	ada2 = 2 * ada;
	adb2 = 2 * adb;

	// If x is the major axis
	if (adx >= ady && adx >= adz && adx >= ada && adx >= adb) {
		int x_step, y_step, z_step, a_step, b_step;
		err1 = ady2 - adx; // err1 = ad2
		err2 = adz2 - adx; // err2 = ad3
		err3 = ada2 - adx; // err3 = ad4
		err4 = adb2 - adx; // err4 = ad5

		int i;
		for (i = 0; i < adx; i++) {
			x_step = 0;
			y_step = 0;
			z_step = 0;
			a_step = 0;
			b_step = 0;

			if (err1 > 0) {
				yIter += sy;
				y_step = sy;
				err1 -= adx2;
			}

			if (err2 > 0) {
				zIter += sz;
				z_step = sz;
				err2 -= adx2;
			}

			if (err3 > 0) {
				aIter += sa;
				a_step = sa;
				err3 -= adx2;
			}

			if (err4 > 0) {
				bIter += sb;
				b_step = sb;
				err4 -= adx2;
			}

			err1 += ady2;
			err2 += adz2;
			err3 += ada2;
			err4 += adb2;

			xIter += sx;
			x_step = sx;

			write_to_fiq(x_step, y_step, z_step, a_step, b_step, speed, fout);

		}
	}

	// If y is the major axis
	if (ady > adx && ady >= adz && ady >= ada && ady >= adb) {
		x_step = 0;
		y_step = 0;
		z_step = 0;
		a_step = 0;
		b_step = 0;

		err1 = adx2 - ady;
		err2 = adz2 - ady;
		err3 = ada2 - ady;
		err4 = adb2 - ady;

		int i;
		for (i = 0; i < ady; i++) {
			x_step = 0;
			y_step = 0;
			z_step = 0;
			a_step = 0;
			b_step = 0;

			if (err1 > 0) {
				xIter += sx;
				x_step = sx;
				err1 -= ady2;
			}

			if (err2 > 0) {
				zIter += sz;
				z_step = sz;
				err2 -= ady2;
			}

			if (err3 > 0) {
				aIter += sa;
				a_step = sa;
				err3 -= ady2;
			}

			if (err4 > 0) {
				bIter += sb;
				b_step = sb;
				err4 -= ady2;
			}

			err1 += adx2;
			err2 += adz2;
			err3 += ada2;
			err4 += adb2;

			yIter += sy;
			y_step = sy;

			write_to_fiq(x_step, y_step, z_step, a_step, b_step, speed, fout);
		}
	}

	// If z is the major axis
	if (adz > adx && adz > ady && adz >= ada && adz >= adb) {
		x_step = 0;
		y_step = 0;
		z_step = 0;
		a_step = 0;
		b_step = 0;

		err1 = ady2 - adz;
		err2 = adx2 - adz;
		err3 = ada2 - adz;
		err4 = adb2 - adz;

		int i;
		for (i = 0; i < adz; i++) {
			x_step = 0;
			y_step = 0;
			z_step = 0;
			a_step = 0;
			b_step = 0;

			if (err1 > 0) {
				yIter += sy;
				y_step = sy;
				err1 -= adz2;
			}

			if (err2 > 0) {
				xIter += sx;
				x_step = sx;
				err2 -= adz2;
			}

			if (err3 > 0) {
				aIter += sa;
				a_step = sa;
				err3 -= adz2;
			}

			if (err4 > 0) {
				bIter += sb;
				b_step = sb;
				err4 -= adz2;
			}

			err1 += ady2;
			err2 += adx2;
			err3 += ada2;
			err4 += adb2;

			zIter += sz;
			z_step = sz;

			write_to_fiq(x_step, y_step, z_step, a_step, b_step, speed, fout);

		}
	}

	// If a is the major axis
	if (ada > adx && ada > ady && ada > adz && ada >= adb) {
		x_step = 0;
		y_step = 0;
		z_step = 0;
		a_step = 0;
		b_step = 0;

		err1 = adx2 - ada;
		err2 = ady2 - ada;
		err3 = adz2 - ada;
		err4 = adb2 - ada;

		int i;
		for (i = 0; i < ada; i++) {
			x_step = 0;
			y_step = 0;
			z_step = 0;
			a_step = 0;
			b_step = 0;

			if (err1 > 0) {
				xIter += sx;
				x_step = sx;
				err1 -= ada2;
			}

			if (err2 > 0) {
				yIter += sy;
				y_step = sy;
				err2 -= ada2;
			}

			if (err3 > 0) {
				zIter += sz;
				z_step = sz;
				err3 -= ada2;
			}

			if (err4 > 0) {
				bIter += sb;
				b_step = sb;
				err4 -= ada2;
			}

			err1 += adx2;
			err2 += ady2;
			err3 += adz2;
			err4 += adb2;

			aIter += sa;
			a_step = sa;
			write_to_fiq(x_step, y_step, z_step, a_step, b_step, speed, fout);

		}
	}

	if (adb > adx && adb > ady && adb > adz && adb > ada) {
		x_step = 0;
		y_step = 0;
		z_step = 0;
		a_step = 0;
		b_step = 0;

		err1 = adx2 - adb;
		err2 = ady2 - adb;
		err3 = adz2 - adb;
		err4 = ada2 - adb;

		int i;
		for (i = 0; i < adb; i++) {
			x_step = 0;
			y_step = 0;
			z_step = 0;
			a_step = 0;
			b_step = 0;

			if (err1 > 0) {
				xIter += sx;
				x_step = sx;
				err1 -= adb2;
			}

			if (err2 > 0) {
				yIter += sy;
				y_step = sy;
				err2 -= adb2;
			}

			if (err3 > 0) {
				zIter += sz;
				z_step = sz;
				err3 -= adb2;
			}

			if (err4 > 0) {
				bIter += sb;
				b_step = sb;
				err4 -= adb2;
			}

			err1 += adx2;
			err2 += ady2;
			err3 += adz2;
			err4 += ada2;

			bIter += sb;
			b_step = sb;

			write_to_fiq(x_step, y_step, z_step, a_step, b_step, speed, fout);
		}
	}
}
