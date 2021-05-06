
/******************************************************************************
*                                                                             *
*   Copyright 2006 Queen Mary University of London                            *
*   Copyright 1995 University of Cambridge                                    *
*                                                                             *
*   This file is part of stats.                                                *
*                                                                             *
*   stats is free software; you can redistribute it and/or modify              *
*   it under the terms of the GNU General Public License as published by      *
*   the Free Software Foundation; either version 2 of the License, or         *
*   (at your option) any later version.                                       *
*                                                                             *
*   stats is distributed in the hope that it will be useful,                   *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
*   GNU General Public License for more details.                              *
*                                                                             *
*   You should have received a copy of the GNU General Public License         *
*   along with Trident; if not, write to the Free Software                    *
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *
*                                                                             *
******************************************************************************/

/*
 *
 * graph fitting/stats analysis program by tjg21
 *
 * $Id: curve.c,v 1.6 2000/11/08 12:45:43 tjg21 Exp $
 */



/*
 * currently can take lines from stdin/file in the form of a number of numbers per line
 *
 * if the -1 flag is given, then it will calculate single variable stats on colx
 * else it will produce:
 *        single variable stats for colx, coly, ln(colx), ln(coly)
 *        a fits of the forms:
 *
 *    y = a + b*x           (linear regression line)
 *    y = a + b*log(x)      (natural log regression line)
 *    y = a * exp(b * x)    (exponential regression line)
 *
 * these are calculated by least squares minimisation
 * it gives a fit characteristic for each line ie 0 = useless line, 100 = perfect line
 * (generally to give a useful fit, these figure should at least 95%, and more like 99%)
 * the figure is actually the percentage of the varience in Y that is explained by the
 * model used for the fit)
 * 
 * for x/y figures less than UNDER_FLOW, it ignores them for the log/exp plot (respectively)
 * and prints a warning message that the point has been excluded from the relavant curve
 *
 * which actual columns of the input streams are used for colx and coly are determined by
 * other input options (see below)
 *
 * --------------------------
 *
 * command line options:
 *
 * -f <filename> : takes input from <filename>, rather than stdin
 *
 * -w : turns on watermark option
 *
 * this assumes that the input will be a watermark plot, with x being queue length, and
 * y being the cumulative probability.  It adds the number of samples for each line from
 * stdin depending on the y values - ie more samples for the more likely queue lengths.
 *
 * NB: this does _not_ fit a bounding line, it just weights the curve fitting towards the
 * lower queue length, and (comparatively) ignores the rare events when the queue is very long
 *
 *
 * -n : on last line of output, print linear fit graph
 * -l : on last line of output, print log fit graph
 * -e : on last line of output, print exponential fit graph
 * -b : on last line of output, print best fit graph
 *
 * for all these options, if the data is bogus (for some definition of bogus) and a graph
 * is not applicable, the output will be a '0' on the last line.
 *
 * -v : turns on verbose mode
 * -1 : single stats mode
 * -h : displays help message
 * -u <colx>:<coly> : define colx and coly where the columns of the input stream are
 *           numbered 1, 2, 3... (maximum 10 columns)
 *           and column 0 is the line number (starting at zero).
 *    nb: -u <colx> is also supported, meaning that you want to fit <colx> 
 *        against the line number column (ie set coly=colx, colx=0)
 *    nb: if there is no number in colx or coly for a certain line (which has at least one
 *        number in it, and does not start with a '#' character, then the current line number
 *        is substituted
 *
 */



#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#if(!hpux)
#include <getopt.h>
#endif

#define TRC(x)
#define UNDER_FLOW 1e-8

int max_records = 5000;

void prog_usage(void);
int getline(char s[], int lim, FILE *stream);
int num_iterations(double xprob, double yprob, double *total, double *cum, int mode);
double abs_d(double x);


int main(int argc, char **argv) 
{
    char input[256];
    int rc = 0, res, n, i, j, c;
    int mode = 2, *valid, do_prediction = 0;
    int wmark = 0, log_fit = 0, exp_fit = 0, col1 = 1, col2 = 2;
    int cant_log_x = 0, cant_log_y = 0, which = 0, verbose = 0;

    double *x, *y, sx, sy, sx2, sy2, sxy, sr2;
    double s_xx, s_yy, s_xy, m_x, m_y, x_in, y_in, c_in[11];
    double *xl, sxl, sx2l, sxyl, sr2l, s_xxl, s_xyl, m_xl;
    double sx_e = 0., sx2_e = 0., s_xx_e = 0., m_x_e = 0., sy_l = 0., sy2_l = 0., s_yy_l = 0.;
    double *yl, syl, sy2l, sxyll, sr2ll, s_yyl, s_xyll, m_yl;
    double cum_prob = 0.0, total_prob = 1.0, track_prob = 1.0;
    double grad, inter, gradl, interl, gradll, interll;
    double nd, nl, ne, best_f, pred = 0.;
    FILE *file = stdin;


/*  variable name explanation:

    x - array for storing x values
    y - array for storing y values
    sx - sum of x
    sy - sum of y
    sx2 - sum of x^2
    sy2 - sum of y^2
    sxy - sum of x*y
    sr2 - sum of e^2, where y = a*x + b + e, (a and b being best fit lines)
    s_xx - usually seen as: SSxx = (sum of x^2) - ((sum of x)^2 / n) = sum of ((x-E[x])^2)
    s_yy - usually seen as: SSyy = (sum of y^2) - ((sum of y)^2 / n) = sum of ((y-E[y])^2)
    s_xy - usually seen as: SSxy = (sum of x*y) - ((sum of x)*(sum of y) / n) = sum of ((x-E[x])*(y-E[y]))
    m_x - mean of x
    m_y - mean of y
    grad - gradient of line of best fit
    inter - intercept of line of best fit
    n - number of sample points
    nd - (double) number of sample points
 
    xl - array for storing log(x) values
    sx2l - sum of log(x)^2
    sxyl - sum of log(x)*y
    sy_l - sum of y for points where log(x) is valid
    sy2_l - sum of y^2 for points where log(x) is valid
    s_yy_l - equivilent to s_yy for points where log(x) is valid
    sr2l - sum of e^2, e is the error term for the (log(x),y) fit line
    s_xxl - equiv of SSxx (for logged x)
    m_xl - mean of log(x)
    gradl - gradient of line of best fit for log(x)
    interl - intercept of line of best fit for log(x)
    log_fit - number of points valid for the log(x) fit line
    nl - (double) log_fit

    yl - array for storing log(y) values
    sy2l - sum of log(y)^2
    sxyll - sum of x*log(y)
    sx_e - sum of x for points valid for log(y)
    sx2_e - sum of x^2 for points valid for log(y)
    s_xx_e - equivilent to s_xx for points where log(y) is valid
    m_x_e - equivilent to m_x for points where log(y) is valid
    sr2ll - sum of e^2, e=error term for log(y) best fit line
    m_yl - mean of log(y)
    gradll - gradient of line of best fit for log(y)
    interll - intercept of line of best fit for log(y)
    exp_fit - number of points valid for the log(y) line
    ne - (double) exp_fit

    */

    optarg=NULL;

    while ((c = getopt(argc, argv, "h1vwnlebf:u:r:p:")) != -1)
	switch (c) {

	case 'p':
	    res = sscanf(optarg, "%lg", &pred);
	    if(res != 1)
	    {
		printf("Can't read the x value from '%s'\n", optarg);
		prog_usage();
	    }
	    do_prediction = 1;
	    break;

	case 'r':
	    res = sscanf(optarg, "%d", &max_records);
	    if(res != 1 || max_records < 0)
	    {
		printf("Bad number of records - '%s'\n", optarg);
		prog_usage();
	    }
	    break;

	case 'u' :       /* use non-default columns */
	    res = sscanf(optarg, "%d:%d", &col1, &col2);

	    if(res != 1 && res != 2)
	    {
		printf("Can't decypher column information - %d\n", res);
		prog_usage();
	    }
	    else
	    {
		if(res == 1)
		{
		    col2 = col1;
		    col1 = 0;
		}
	    }
	    if(col1 < 0 || col1 > 10 || col2 < 0 || col2 > 10)
	    {
		printf("Invalid column number (%d:%d)\n", col1, col2);
		prog_usage();
	    }
	    break;

	case 'h' :       /* display usage */
	    printf("\nCurve: linear/exponential/log curve fitting\n"
		   "Contact tjg21 for details\n");
	    prog_usage();
	    break;

	case '1' :       /* one/two coloumn mode */
	    mode = 1;
	    break;

	case 'v' :       /* verbose mode */
	    verbose = 1;
	    break;

	case 'w' :       /* watermark weighting option */
	    wmark = 1;
	    break;

	case 'n' :       /* print linear fit as last line */
	    which = 1;
	    break;
	   
	case 'l' :       /* print log fit as last line */
	    which = 2;
	    break;

	case 'e' :       /* print exponential fit as last line */
	    which = 3;
	    break;
	    
	case 'b' :       /* print best fit as last line */
	    which = 4;
	    break;

	case 'f' :
	    file = fopen(optarg, "r");
	    if(!file)
	    {
		printf("Cannot read file '%s'\n\n", optarg);
		exit(1);
	    }
	    break;

	default :
	    prog_usage();
	}

    TRC(printf("Col1: %d, col2: %d\n", col1, col2));

    if(verbose)
	printf("\n\nLinear regression program, by tjg21\n");

    sx = sy = sx2 = sy2 = sxy = sr2 = nd = 0.0;
    sxl = sx2l = sxyl = sr2l = s_xxl = m_xl = 0.0;
    syl = sy2l = sxyll = sr2ll = s_yyl = m_yl = 0.0;
    n = 0;


    x = (double *) malloc(sizeof(double) * max_records);
    y = (double *) malloc(sizeof(double) * max_records);
    xl = (double *) malloc(sizeof(double) * max_records);
    yl = (double *) malloc(sizeof(double) * max_records);
    valid = (int *) malloc(sizeof(int) * max_records);

    for(i=0; i<max_records; i++)
	valid[i] = 7;


    while(rc != EOF)
    {
	rc = getline(input, 255, file);
	if(rc != EOF)
	{
	    c_in[0] = (double) n;

	    res = sscanf(input, "%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg",
			 &c_in[1], &c_in[2], &c_in[3], &c_in[4], &c_in[5],
			 &c_in[6], &c_in[7], &c_in[8], &c_in[9], &c_in[10]);

	    TRC(printf("rc: %d, res: %d\n", rc, res));
	    
	    if(res > 0 && rc != 0 && input[0] != '#')
	    {
		if(mode == 1)
		{
		    if(col2 <= res)
			x_in = c_in[col2];
		    else
			x_in = c_in[0];
		    y_in = c_in[0];
		    j = 1;
		}
		else
		{
		    if(col1 <= res)
			x_in = c_in[col1];
		    else
			x_in = c_in[0];

		    if(col2 <= res)
			y_in = c_in[col2];
		    else
			y_in = c_in[0];

		    j = num_iterations(x_in, track_prob - y_in, &total_prob, &cum_prob, wmark);
		    track_prob = y_in;
		}
	    }
	    else
		j = 0;

	    for(i=0; i<j; i++)
	    {
		x[n] = x_in;
		y[n] = y_in;

		TRC(printf("%g %g (%d)\n", x_in, y_in, j));

		sx += x_in;
		sx2 += pow(x_in, 2.0);
		
		if(mode == 2)
		{
		    sy += y_in;
		    sy2 += pow(y_in, 2.0);
		    sxy += (x_in*y_in);
		    
		    if(y_in > UNDER_FLOW)
		    {
			exp_fit++;
			yl[n] = log(y_in);
			syl += yl[n];
			sy2l += pow(yl[n], 2.0);
			sxyll += x_in * yl[n];
			sx_e += x_in;
			sx2_e += pow(x_in, 2.0);
		    }
		    else
		    {
			cant_log_y++;
			if(cant_log_y < 8)
			{
			    if(verbose)
				printf("Y~0(%s -> (%g,%g)) - can't log: results may be bogus\n",
				       input, x_in, y_in);
			}
			if(cant_log_y == 8)
			{
			    if(verbose)
				printf("Ceasing to print 'not logging Y' messages\n");
			}
			valid[n] &= (0xffffffff ^ 2);
		    }
		    
		    if(x_in > UNDER_FLOW)
		    {
			log_fit++;
			xl[n] = log(x_in);
			sxl += xl[n];
			sx2l += pow(xl[n], 2.0);
			sxyl += xl[n] * y_in;
			sy_l += y_in;
			sy2_l += pow(y_in, 2.0);
		    }
		    else
		    {
			cant_log_x++;
			if(cant_log_x < 8)
			{
			    if(verbose)
				printf("X~0 (%s -> (%g,%g)) - can't log: results may be bogus\n",
				       input, x_in, y_in);
			}
			if(cant_log_x == 8)
			{
			    if(verbose)
				printf("Ceasing to print 'not logging X' messages\n"); 
			}
			valid[n] &= (0xffffffff ^ 4);
		    }
		    
		    TRC(printf("x:%g y:%g xl:%g yl:%g sx:%g sy:%g sxl:%g syl:%g sx2:%g sy2:%g sxy:%g sx2l:%g sy2l:%g sxyl:%g sxyll:%g\n", x[n], y[n], xl[n], yl[n], sx, sy, sxl, syl, sx2, sy2, sxy, sx2l, sy2l, sxyl, sxyll));
		    
		}
		n++;
		if(n==max_records)
		{
		    i=j;
		}
	    }
	    if(n==max_records)
	    {
		if(verbose)
		    printf("Too much data, will analyse now\n");
		rc = -1;
	    }
	}
    }

    if(n<3)
	printf("Not enough data - really should be >20 items\n");
    else
    {
	if(verbose)
	    printf("Analysing...\n");
	
	nd = (double) n;
	nl = (double) log_fit;
	ne = (double) exp_fit;
	TRC(printf("nd:%g nl:%g ne:%g\n", nd, nl, ne)); 


	m_x = sx / nd;
	s_xx = sx2 - (sx * sx / nd);

	TRC(printf("m_x:%g s_xx:%g\n", m_x, s_xx));

	if(mode == 2)
	{
	    m_y = sy / nd;
	    s_yy = sy2 - ((sy * sy) / nd);
	    s_xy = sxy - ((sx * sy) / nd);

	    TRC(printf("sy2:%g, sy:%g, nd:%g\n", sy2, sy, nd));

	    if(abs_d(s_xx) < UNDER_FLOW)
	    {
		if(verbose)
		    printf("X doesn't vary, so can't make linear fit\n");
	    }
	    else
	    {
		grad = s_xy / s_xx;
		inter = m_y - (grad * m_x);
		if(abs_d(grad) < UNDER_FLOW)    /* XXX generally bad plan, but good for output */
		{
		    grad = 0;
		    if(verbose)
			printf("Grad=%g, squashing to zero\n", grad);
		}
	    }

	    TRC(printf("m_y:%g s_yy:%g s_xy:%g grad:%g inter:%g\n",
		       m_y, s_yy, s_xy, grad, inter));

	    if(exp_fit < 3)
	    {
		if(verbose)
		    printf("Not enough data for exp fit - really should be >20 items\n");
	    }
	    else
	    {

		m_yl = syl / ne;
		s_yyl = sy2l - ((syl * syl) / ne);
		s_xyll = sxyll - ((sx_e * syl) / ne);
		s_xx_e = sx2_e - (sx_e * sx_e / ne);
		m_x_e = sx_e / ne;

		if(abs_d(s_xx) < UNDER_FLOW)
		{
		    if(verbose)
			printf("X doesn't vary, so can't make exp fit\n");
		}
		else
		{
		    gradll = s_xyll / (sx2_e - (sx_e * sx_e / ne));
		    interll = m_yl - (gradll * (sx_e / ne));
		    if(abs_d(gradll) < UNDER_FLOW)    /* XXX generally bad plan, but good for output */
		    {
			gradll = 0;
			if(verbose)
			    printf("Gradll=%g, squashing to zero\n", gradll);
		    }
		}

		TRC(printf("m_yl:%g s_yyl:%g s_xyll:%g gradll:%g interll:%g\n",
			   m_y, s_yyl, s_xyll, gradll, interll));
	    }

	    if(log_fit < 3)
	    {
		if(verbose)
		    printf("Not enough data for log fit - really should be >20 items\n");
	    }
	    else
	    {
		m_xl = sxl / nl;
		s_xxl = sx2l - ((sxl * sxl) / nl);
		s_xyl = sxyl - ((sxl * sy_l) / nl);
		s_yy_l = sy2_l - (sy_l * sy_l / nl);
		
		if(abs_d(s_xxl) < UNDER_FLOW)
		{
		    if(verbose)
			printf("log(X) doesn't vary, so can't make log fit\n");
		}
		else
		{
		    gradl = s_xyl / s_xxl;
		    interl = (sy_l / nl) - (gradl * m_xl);
		    if(abs_d(gradl) < UNDER_FLOW)    /* XXX generally bad plan, but good for output */
		    {
			gradl = 0;
			if(verbose)
			    printf("Gradl=%g, squashing to zero\n", gradl);
		    }
		}

		TRC(printf("m_xl:%g s_xxl:%g s_xyl:%g gradl:%g interl:%g\n",
			   m_xl, s_xxl, s_xyl, gradl, interl));
	    }

	    TRC(printf("Got to here (1)\n"));

	    for(i=0; i<n; i++)
	    {
		if(abs_d(s_xx) > UNDER_FLOW)
		{
		    sr2 += pow(y[i] - inter - (grad*x[i]), 2.0); 
		    TRC(printf("Adding to sr2 (%g)\n", sr2));
		}
		if(log_fit >= 3 && valid[i] & 4)
		{
		    if(abs_d(s_xxl) > UNDER_FLOW)
		    {
			sr2l += pow(y[i] - interl - (gradl*xl[i]), 2.0);
			TRC(printf("Adding to sr2l (%g)\n", sr2l));
		    }
		}
		if(exp_fit >= 3 && valid[i] & 2)
		{
		    if(abs_d(s_xx) > UNDER_FLOW)
		    {
			sr2ll += pow(yl[i] - interll - (gradll*x[i]), 2.0);
			TRC(printf("Adding to sr2ll (%g)\n", sr2ll));
		    }
		}
	    }
	}
	TRC(printf("sr2:%g sr2l:%g sr2ll:%g\n", sr2, sr2l, sr2ll));
	TRC(printf("Got to here (2)\n"));

	if(mode == 1)
	{
	    if(verbose)
		printf("Mean = %g, std = %g\n\n", sx / nd, sqrt(s_xx / nd));
	}

	if(mode == 2 && verbose)
	{
	    printf("\n");
	    if( abs_d(s_xx) > UNDER_FLOW) {
		if(s_yy == 0) {
		    printf("----- linear fit ----- 100 percent\n");
                } else {
		    printf("----- linear fit ----- %2.4g percent\n", 100.0 * (s_yy - sr2) / s_yy);
                }
            }
	    printf("X: Mean = %g, std = %g\n", sx / nd, sqrt(s_xx / nd));
	    printf("Y: Mean = %g, std = %g\n", sy / nd, sqrt(s_yy / nd));
	    if(abs_d(s_xx) > UNDER_FLOW)
		printf("Linear fit: y = %g %s x * %g\n\n",
		       inter, grad < 0.0 ? "-" : "+", grad < 0.0 ? -grad : grad);

	    if(log_fit >= 3)
	    {
		if(abs_d(s_xxl) > UNDER_FLOW)
		    if(s_yy == 0)
			printf("----- log(e) fit ----- 100 percent\n");
		    else
			printf("----- log(e) fit ----- %2.4g percent\n", 100.0 * (s_yy_l - sr2l) / s_yy_l);
		printf("log(X): Mean = %g, std = %g\n", sxl / nl, sqrt(s_xxl / nl));
		if(abs_d(s_xxl) > UNDER_FLOW)
		    printf("Log fit: y = %g %s %g * log(x)\n\n",
			   interl, gradl < 0.0 ? "-" : "+", gradl < 0.0 ? -gradl : gradl);
	    }

	    if(exp_fit >= 3)
	    {
		if(abs_d(s_xx) > UNDER_FLOW)
		    if(s_yyl == 0)
			printf("----- exp fit ----- 100 percent\n");
		    else
			printf("----- exp fit ----- %2.4g percent\n", 100.0 * (s_yyl - sr2ll) / s_yyl);
		printf("log(Y): Mean = %g, std = %g\n", syl / ne, sqrt(s_yyl / ne));
		if(abs_d(s_xx) > UNDER_FLOW)
		    printf("Exp fit: y = %g * exp( %g * x)\n\n", exp(interll), gradll);
	    }

	    if(log_fit >= 3 && exp_fit >= 3 && abs_d(s_xx) > UNDER_FLOW && abs_d(s_xxl) > UNDER_FLOW)
	    {
		if(((s_yy - sr2) / s_yy) < 0.95 &&
		   ((s_yy - sr2l) / s_yy) < 0.95 &&
		   ((s_yyl - sr2ll) / s_yyl) < 0.95)
		{
		    printf("No fit is really good enough\n\n");
		}
		else
		{
		    printf("I recommend the ");
		    if(((s_yy - sr2) / s_yy) > ((s_yy_l - sr2l) / s_yy_l))
		    {
			if(((s_yyl - sr2ll) / s_yyl) > ((s_yy - sr2) / s_yy))
			    printf("exponential fit.\n\n");
			else
			    printf("linear fit.\n\n");
		    }
		    else
		    {
			if(((s_yyl - sr2ll) / s_yyl) > ((s_yy_l - sr2l) / s_yy_l))
			    printf("exponential fit.\n\n");
			else
			    printf("logarithmic fit.\n\n");
		    }
		}
	    }
	}
    }
    if(which && mode == 2)
    {
	if(n<3)
	    printf("0\n");
	else
	{
	    if(which == 4)
	    {
		if(abs_d(s_xx) < UNDER_FLOW || s_yy == 0)
		    which = 1;
		else
		{
		    best_f = (s_yy - sr2) / s_yy;
		    which = 1;

		    if(abs_d(s_xxl) > UNDER_FLOW && ((s_yy - sr2l) / s_yy) > best_f)
		    {
			best_f = (s_yy - sr2l) / s_yy;
			which = 2;
		    }

		    if((s_yyl - sr2ll) / s_yyl > best_f)
			which = 3;
		}
	    }

	    switch(which) {
	    case 1 :
		if(abs_d(s_xx) > UNDER_FLOW)
		{
		    if(do_prediction)
		    {
			printf("linear prediction: x = %g, y = %g, error = t(~,%d) * %g\n",
			       pred, inter+(grad*pred), n-2,
			       sqrt((s_yy - (grad*s_xy)) / (nd-2.)) *
			       sqrt(1.+(1./nd)+((pred-m_x)*(pred-m_x)/s_xx)));
		    }
		    printf("%g %s x * %g\n",
			   inter, grad < 0.0 ? "-" : "+", grad < 0.0 ? -grad : grad);
		}
		else
		    printf("0\n");
		break;
	    case 2 :
		if(log_fit >= 3 && abs_d(s_xxl) > UNDER_FLOW)
		{
		    if(do_prediction)
		    {
			printf("log prediction: x = %g, y = %g, error = t(~,%d) * %g\n",
			       pred, interl+(gradl*log(pred)), log_fit-2,
			       sqrt((s_yy_l - (gradl*s_xyl)) / (nl-2.)) *
			       sqrt(1.+(1./nl)+((pred-m_xl)*(pred-m_xl)/s_xxl)));
		    }
		    printf("%g %s %g * log(x)\n",
			   interl, gradl < 0.0 ? "-" : "+", gradl < 0.0 ? -gradl : gradl);
		}
		else
		    printf("0\n");
		break;
	    case 3 :
		if(exp_fit >= 3 && abs_d(s_xx) > UNDER_FLOW)
		{
		    if(do_prediction)
		    {
			printf("exp prediction: x = %g, y = %g, error = t(~,%d) * %g\n",
			       pred, exp(interll+(gradll*pred)), exp_fit-2,
			       sqrt((s_yyl - (gradll*s_xyll)) / (ne-2.)) *
			       sqrt(1.+(1./ne)+((pred-m_x_e)*(pred-m_x_e)/s_xx_e)));
		    }
		    printf("%g * exp( %g * x)\n", exp(interll), gradll);
		}
		else
		    printf("0\n");
		break;
	    }
	}
    }
		

    return 0;
}

int 
getline(char s[], int lim, FILE *stream)
{
    int c,i;

    for(i=0;i<lim-1 && (c=getc(stream))!=EOF && c!='\n';++i)
	s[i]=c;

    s[i]='\0';

    if(c != EOF) {
	return i;
    }else{
	return c;
    }
}


int
num_iterations(double xprob, double yprob, double *total, double *cum, int mode)
{
    int i = 0;

    if(mode == 0 || xprob < UNDER_FLOW || yprob < UNDER_FLOW)
    {
	i = 1;           /* normal mode (or can't log x/y): 1 entry per line */
	*total -= yprob; /* re-adjust total for prob lost */
    }
    else            /* watermark mode: weight entries */
    {
	*cum += yprob;
	while(*cum >= (*total/(double) max_records))
	{
	    i++;
	    *cum -= (*total/(double) max_records);
	}
    }

    TRC(printf("Doing %d iterations\n", i));
    return i;
}


double
abs_d(double x)
{
    if(x<0.0)
	return -x;
    else
	return x;
}

void
prog_usage()
{
    fprintf(stderr,"\ncurve"
	    "\t-f <filename> : gets input from <filename>\n"
	    "\t-w : turn on weighting to fit watermark plot\n"
	    "\t-n : on last line of output print linear fit graph\n"
	    "\t-l : on last line of output print log fit graph\n"
	    "\t-e : on last line of output print exponential fit graph\n"
	    "\t-b : on last line of output print best fit graph\n");
    fprintf(stderr,"\t-v : turns on verbose mode\n"
	    "\t    (nb: one of nlebv is required for any output)\n"
	    "\t-u <colx>:<coly> : defines x column & y column (default = 1:2)\n"
	    "\t   if no <coly> specifed, fit parametric to <colx>\n"
	    "\t-1 : single stats mode (using <colx>)\n"
	    "\t-r <recs> : maximum number of records (default 5000)\n"
	    "\t-p <x> : predicts y value with error estimate\n"
	    "\t-h : displays help page\n\n");
    exit(1);
}

