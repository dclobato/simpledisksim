/* file SSS.C */

/*

          SSS - Simulation Subroutine Set. Version 1.50
   Copyright (C) M. A. Pollatschek 1994.  All rights reserved.

 Copyright covers all alterations and renditions of this source
file as for example, but not limited to, OBJect, LIBrary or Quick
  LiBrary files derived this source. Copyright does not extend,
     however, to EXEecutable files derived from this source.

                         IMPORTANT NOTE

THIS  FILE,   ITS USE,   OPERATION  AND  SUPPORT  IS  PROVIDED  "AS  IS"
WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,BUT
NOT LIMITED TO,  THE IMPLIED WARRANTIES  OF MERCHANTABILITY AND  FITNESS
FOR A  PARTICULAR  PURPOSE.   THE  ENTIRE RISK  AS  TO THE  QUALITY  AND
PERFORMANCE  OF  THIS  FILE  IS  WITH  THE  USER.    IN NO  EVENT  SHALL
THE AUTHOR AND/OR THE PUBLISHER  BE  LIABLE  FOR ANY  DAMAGES INCLUDING,
WITHOUT LIMITATION,  ANY  LOST PROFITS, LOST SAVINGS OR OTHER INCIDENTAL
OR CONSEQUENTIAL  DAMAGES  ARISING  THE  USE  OR  INABILITY  TO USE THIS
FILE,  EVEN  IF  THE  AUTHOR  AND/OR  THE PUBLISHER  BEEN ADVISED OF THE
POSSIBILITY  OF SUCH  DAMAGES  OR  FOR  ANY  CLAIM  BY  ANY OTHER PARTY.

*/

#define EPSI 1.5258789e-5 /* 2 to the power of -16 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#ifdef __TURBOC__
#include <alloc.h>
#else
#include <malloc.h>
#endif

struct cell
        {
          double      upper_limit;
          int         number;
          struct cell *next;
        };
 
struct statistics
        {
          char   headline[20];
          char   time_persistent;
          char   called_already;
          double ini_time;
          double last_upd_sample_size;
          double last_upd_value;
          double min;
          double max;
          double sum;
          double sum_of_squares;
          struct cell *fst_cell;
        };
 
struct queue_member
        {
          struct queue_member *next_member;
          double       priority;
          int          code;
          int          id;
          void         *entity;
        };
 
struct queue
        {
          char          discipline[6];  /* LIFO,FIFO,BVF,SVF */
          int           number;
          struct statistics *q_stat;
          struct queue_member  *fst_member;
        };
 

int               ERR_CODE        = 3;

int               ERR_RET         = 0;
 
unsigned          current_rnd     = 1897;
 
int               ANTI            = 0;
 
struct queue      *calendar       = NULL;
 
double            tnow            = 0.0;
 
int               ID_CODE         = 0;

double            zero_entity     = 0.0;

struct queue      **q             = NULL;
 
int               NQUEUE          = 0;
 
int               NATTR           = 0;
 
double            *present_entity = NULL;
 
double            *prev_entity    = NULL;

struct statistics **stat_root     = NULL;

int               STATN           = 0;

char              math_pr_name[12]= "?";

/*********  Function prototypes *************/

#include "sss.h"
void exit(int);
void error_exit(int, char *);
struct statistics *initiate_statistics(char *,int,int,double,double);
void clear_statistics(struct statistics *);
int collect_statistics(struct statistics *,double);
void report_statistics(struct statistics *, FILE *);
struct queue_member *remfq(struct queue *,double *);
struct queue_member *find_by_rank(int,int);
int remfmid(struct queue *,struct queue_member *);
void list_calendar(void);
void list_queue(struct queue *);
void place_v_after_v1(struct queue_member *,struct queue_member *,struct queue *);
int enter_to_queue(struct queue *,void *,double,int,int);
struct queue *initialize_queue(char *,char *);
struct queue **set_up_queues(int);

int SIMERR(void)
{
  int i;
  i = ERR_RET;
  ERR_RET = 0;
  return(i);
}

void error_exit(int error_code, char *comes_from)
{
  if ((ERR_CODE & 2) == 2)
  {
    printf("In %s ", comes_from);
    switch (error_code)
    {
      case  1: printf("there is a current entity already in the system - use DISPOS"); break;
      case  2: printf("there is no more space in memory - reduce number of entities"); break;
      case  3: printf("attribute should be a pointer"); break;
      case  4: printf("queue number is too big or negative - consult INIQUE"); break;
      case  5: printf("no entity with specified rank and/or queue number"); break;
      case  6: printf("there is no current entity - use NEXTEV or REMVFx before"); break;
      case  7: printf("no initialization has been performed - use INIQUE"); break;
      case  8: printf("statistics index is too big or negative - consult INIQUE"); break;
      case  9: printf("statistics has already been set - do not use INISTA twice"); break;
      case 10: printf("summary file cannot be opened"); break;
      case 11: printf("basic parameters have already been set - do not INIQUE twice"); break;
      case 12: printf("too large rank for queue or calendar"); break;
      case 13: printf("parameters should be positive"); break;
      case 14: printf("parameters should be strictly ordered: min<mode<max"); break;
      case 15: printf("attribute number is too big or negative - consult INIQUE"); break;
      case 16: printf("event code should be greater than 1"); break;
      case 17: printf("no statistic is defined for indicated index - use INISTA"); break;
    }
    printf("\n");
  }
  if ((ERR_CODE & 1) == 1) exit(error_code);
  else ERR_RET = error_code;
}

int matherr(struct exception *a)
{
  a->retval = 0.0;
  printf("In %s parameters are invalid - check them\n",math_pr_name);
  return(0);
}

void  SETANT(int y)
{ if ((y == 0) || (y == 1)) ANTI = y; }

void  SETSEE(int i)
{ if (i > 0) current_rnd = (unsigned)i; }


double  RA(void)
{
  static unsigned long rnd_a = 293, rnd_c=57111;
  unsigned long current, aaa;
  double r;
  current = current_rnd;
  current = current*rnd_a + rnd_c;
  aaa = current>>16;
  if (aaa) current -= (aaa << 16);
  current_rnd = (unsigned)current;
  r = (double)current*EPSI;
  if (!((r > 0.0) && (r < 1.0)))
  {
    if (!(r > 0.0)) r = EPSI;
    if (!(r < 1.0)) r = 1.0 - EPSI;
  }
  if (ANTI) return(1.0 - r); else return(r);
}


double  UN(double minimum, double maximum)
{
  double x;
  strcpy(math_pr_name,"UN");
  x = minimum + RA()*(maximum - minimum);
  strcpy(math_pr_name,"?");
  return(x);
}

double  EX(double mean)
{
  double x;
  strcpy(math_pr_name,"EX/ER/GA/BE");
  if (mean < 0.0)
    { error_exit(13,"EX"); return(0.0); }
  x = - (mean) * log( RA() ) ;
  strcpy(math_pr_name,"?");
  return(x);
}
 
double  TR(double minimum, double mode, double maximum)
{
  double x,r,y;
  strcpy(math_pr_name,"TR");
  if ((maximum < mode) || (mode < minimum) || (minimum > maximum))
    { error_exit(14,"TR"); return(0.0); }
  if (maximum - minimum < 0.001)
  {
    strcpy(math_pr_name,"?");
    return(mode);
  }
  x=(mode - minimum)/(maximum - minimum);
  r=RA();
  if (r <= x) y = minimum + sqrt((mode - minimum)*(maximum - minimum)*r);
  else        y = maximum - sqrt((maximum - mode)*(maximum - minimum)*(1.0-r));
  strcpy(math_pr_name,"?");
  return(y);
}
 
int  NP(double mean)
{
  double r,a;   int n;
  strcpy(math_pr_name,"NP/BI");
  if (mean <= 0.0) { error_exit(13,"NP"); return(0.0); }
  a=exp(-(mean));   n=0;  r=RA();
  while (a<=r)  { ++n; r=r*RA(); }
  strcpy(math_pr_name,"?");
  return (n);
};
 
double  ER(double expon_mean, int k)
{
  double a;  int i;
  strcpy(math_pr_name,"ER/GA/BE");
  if ((expon_mean <= 0.0) || (k <= 0))
    { error_exit(13,"ER"); return(0.0); }
  a=0.0;  for(i=1; i<=k ; ++i) a += EX(expon_mean);
  strcpy(math_pr_name,"?");
  return(a);
}
 
double  GA(double beta, double alpha)
{
  double x,y,w,b,yy;  int a;
  strcpy(math_pr_name,"GA/BE");
  if ((beta <= 0.0) || (alpha <= 0.0))
    { error_exit(13,"GA"); return(0.0); }
  if ( alpha>0.0 && alpha<1.0)
  {
     x=y=1.0;
     while((x+y > 1.0) || (x+y == 0.0))
       { x=pow(RA(),(1./(alpha))); y=pow(RA(),(1./(1. - alpha))); }
     w=x/(x+y); yy = w*EX(1.0)*(beta);
  }
  else if(alpha>=1.0 && alpha<5.0)
  {
     a=(int)floor(alpha);  b=alpha-(double)a;
     x=((alpha/a)*(ER(1.0,a)));
     if (alpha == 1.)
     while(RA() > exp(-x)) x=ER(1.0,1); else
     while(RA()>(pow((x/(alpha)),b)*exp(-b*(x/alpha-1.))))
       x=((alpha/a)*(ER(1.0,a)));
     yy = x*(beta);
  }
  else if(alpha>=5.0)
  {
     a=(int)floor(alpha);
     if (RA()>=alpha-a) yy = ER(beta,a);
     else { a++;        yy = ER(beta,a); }
   }
  strcpy(math_pr_name,"?");
  return(yy);
}

double  BE(double theta, double phi)
{
  double g1,g2,y;
  strcpy(math_pr_name,"BE");
  if ((theta <= 0.0) || (phi <= 0.0))
    { error_exit(13,"BE"); return(0.0); }
  g1=GA(1.0,theta);  g2=GA(1.0,phi);
  if (g1 + g2 == 0.0) y = BE(theta, phi); else y = g1/(g1+g2);
  strcpy(math_pr_name,"?");
  return(y);
}
 
double pre_RN(void)
{
  double a,b,w,y;  static int f;  static double t;
  strcpy(math_pr_name,"RN/RL/BI");
  if (f == 0)
  {
    a=2.*RA()-1.;    b=2.*RA()-1.;   w=a*a+b*b;
    if ((w == 0.0) || ( w > 1.0 )) y = pre_RN(); else
    {
      t = a*sqrt(-2.*log(w)/w);  f=1;
      y = b*sqrt(-2.*log(w)/w);
    }
  }
  else
  {  f=0;     y = t;  }
  strcpy(math_pr_name,"?");
  return(y);
}
 
double  RN(double mean, double sd)
{ return(pre_RN()*sd + mean); }
 
double  RL(double mean, double sd)
{
  double x;
  strcpy(math_pr_name,"RL");
  x = exp(pre_RN()*sd + mean);
  strcpy(math_pr_name,"?");
  return(x);
}
 
int BI(int n, double p)
{
  int i, j;
  strcpy(math_pr_name,"BI");
  if ((n < 1) || (p < 0.0) || (p > 1.0))
    { error_exit(13,"BI"); return(0); }
  if ((n > 10) && ((p <= 0.1) || (p >= 0.9)))
    i = (p < 0.5)?NP(p*(double)n):n - NP((1.0 - p)*(double)n);
  else if ((n > 10) && ((p*(double)n > 10.0) || ((1.0 - p)*(double)n > 10.0)))
    i = (int)floor(RN(p*(double)n, sqrt(p*(1.0 - p)*(double)n)) + 0.5);
  else
    { i = 0; for (j=0; j<n; j++) if (RA() < p) i++; }
  if (i < 0) i = 0;
  if (i > n) i = n;
  strcpy(math_pr_name,"BI");
  return(i);
}
 
double  WE(double beta, double alpha)
{
  double x;
  strcpy(math_pr_name,"UN");
  if ((alpha <= 0.0) || (beta <= 0.0))
    { error_exit(13,"WE"); return(0.0); }
  x = beta*pow((-log(RA())),(1./(alpha)));
  strcpy(math_pr_name,"?");
  return(x);
}

double  DP(double *prob, int nval)
{
 int i ;  double c ;
 c = RA();
 i = 0;
 while ((i < nval-1) && (c > prob[i])) i++;
 return((double)(i));
}

double CD(double *x, double *p, int n)
{
  double c; int i;
  c = RA();
  if (c >= p[n-1]) return(x[n-1]);
  if (c <= p[0]) return(x[0]);
  i = 0;
  while ((i < n-2) && (c > p[i+1])) i++;
  return(x[i] + (x[i+1] - x[i])*
         ((c - p[i])/(p[i+1] - p[i])));
}
 
struct statistics *initiate_statistics(char *name, int if_time_p,
        int n_cells, double first_limit, double cell_width)
{
  struct statistics *v;
  struct cell *c, *c_old = NULL;
  int i;
  double a;
  v = (struct statistics *)malloc(sizeof(struct statistics));
  if (v != NULL)
  {
    strncpy(v->headline,name,20);
    v->headline[19] = 0;
    v->time_persistent = (char)if_time_p;
    if (if_time_p) n_cells = 0;
    v->called_already = 0;
    v->ini_time = (if_time_p)?tnow:0.0;
    v->last_upd_sample_size = v->ini_time;
    v->last_upd_value = 0.0;
    v->sum = 0.0;
    v->min = 0.0;
    v->max = 0.0;
    v->sum_of_squares = 0.0;
    v->fst_cell = NULL;
    a = first_limit;
    if (n_cells) for (i=0; i<=n_cells; i++)
    {
      c = (struct cell *)malloc(sizeof(struct cell));
      if (c == NULL) return(NULL);
      c->upper_limit = a;
      c->number = 0;
      a += cell_width;
      c->next = NULL;
      if (!i) v->fst_cell = c; else c_old->next = c;
      c_old = c;
    }
    else v->fst_cell = NULL;
  }
  return(v);
}
 
void  INISTA(int n, char *name, int if_time_p, int n_cells, double fl,
	double cw)
{
  struct statistics *v;
  char sname[20];
  if ((n < 1) || (STATN < n))
    { error_exit(8,"INISTA"); return; }
  v = (stat_root)[(n) - 1];
  if ( v != NULL) { error_exit(9,"INISTA"); return; }
  strncpy(sname,name,20);
  sname[19] = 0;
  if ((v = initiate_statistics(sname, if_time_p, n_cells, fl, cw)) == NULL)
    { error_exit(2,"INISTA"); return; }
  (stat_root)[(n) - 1] = v;
}
 
void clear_statistics(struct statistics *v)
{
  struct cell *c;
  if (v == NULL) return;
  v->called_already = 0;
  v->ini_time = (v->time_persistent)?tnow:0.0;
  v->last_upd_sample_size = v->ini_time;
  v->last_upd_value = 0.0;
  v->sum = 0.0;
  v->min = 0.0;
  v->max = 0.0;
  v->sum_of_squares = 0.0;
  c = v->fst_cell;
  while (c != NULL) { c->number = 0; c = c->next; }
}
 
int collect_statistics(struct statistics *x, double observation)
{
  struct cell *c;
  if (x == NULL) return(1);
 
  if (!x->called_already)
  {
    x->min = observation;
    x->max = observation;
    x->called_already = 1;
  }
  else
  {
    if (x->min > observation) x->min = observation;
    if (x->max < observation) x->max = observation;
    if (x->time_persistent)
    {
      x->sum += (tnow - x->last_upd_sample_size) * x->last_upd_value;
      x->sum_of_squares += (tnow - x->last_upd_sample_size) *
                x->last_upd_value * x->last_upd_value;
    }
  }
  if (x->time_persistent)
  {
    x->last_upd_value = observation;
    x->last_upd_sample_size = tnow;
  } else
  {
    (x->last_upd_sample_size)++;
    x->sum            += observation;
    x->sum_of_squares += observation*observation;
  }
  c = x->fst_cell;
  while (c != NULL)
  {
    if ((observation < c->upper_limit) || (c->next == NULL))
      { (c->number)++; break; }
    c = c->next;
  }
  return(0);
}

void  TALLY(int n, double observation)
{
  if ((n < 1) || (STATN < n)) { error_exit(8,"INISTA"); return; }
  if (collect_statistics( (stat_root)[n-1], observation) != 0)
     { error_exit(17,"TALLY"); return; }
}
 
void report_statistics(struct statistics *x, FILE *f)
{
  double a, s, n;
  struct cell *c;
  int i, j, maxc;
  if (x == NULL) { fprintf(f,"No statistics are defined\n"); return; }
  if (x->time_persistent) collect_statistics(x, x->last_upd_value);
  if (!(x->last_upd_sample_size))
  {
    fprintf(f,"%s - no statistics have been recorded\n",x->headline);
    return;
  }
  fprintf(f,"%s - summary statistics ",x->headline);
  n = x->last_upd_sample_size - x->ini_time;
  if (!(x->time_persistent))
    fprintf(f,"for sample size %6.0f\n",n); else
    fprintf(f,"for %9.2f time duration\n",n);
  if (n < 0.0001) return;
  a = x->sum/n;
  s = x->sum_of_squares/n - a*a;
  if (s >= 0.0) s = sqrt(s); else s = 0.0;
  fprintf(f,"  Average=%14.4f, standard deviation=%14.4f\n",a,s);
  fprintf(f,"  minimum=%14.4f, maximum           =%14.4f\n",x->min,x->max);
  i = 1;
  if ((c = x->fst_cell) != NULL)
  {
    maxc = 0;
    while (c != NULL)
      { if (maxc < c->number) maxc = c->number; c = c->next; }
    maxc = (maxc + 49)/50;
    if (maxc == 0) return;
    c = x->fst_cell;
    while (c != NULL)
    {
      if (c == x->fst_cell)
      fprintf(f,
        "cell        upper      frequ\n  #         limit       ency    one * = %d cases\n",
        maxc);
      if (c->next != NULL)  fprintf(f," %2d  %12.2f   %8d ",
                                 i,c->upper_limit,c->number); else
                           fprintf(f," %2d           inf.  %8d ",
                                                i,c->number);
      for (j=0; j<c->number/maxc; j++) fprintf(f, "*"); fprintf(f,"\n");
      i++; c = c->next;
    }
  }
}
 
void  CLEARS(int i)
{
  int j;
  if (stat_root == NULL) return;
  if (i == 0) for (j=0; j<STATN; j++) clear_statistics(stat_root[j]); else
  {
    if ((i > STATN) || (i < 1))
      { error_exit(8,"CLEAR"); return; }
    else clear_statistics(stat_root[i - 1]);
  }
}

double  SAVG(int i)
{
  double n;
  struct statistics *v;
  if (stat_root == NULL) { error_exit(7,"system"); return(0.0); }
  if ((i < 1) || (i > STATN)) { error_exit(8,"SAVG"); return(0.0); }
  v = stat_root[i - 1];
  if (v == NULL) { error_exit(17,"SAVG"); return(0.0); }
  if (v->time_persistent) collect_statistics(v, v->last_upd_value);
  /**3/1/94**/
  n = v->last_upd_sample_size - v->ini_time;
  if (n < 0.0001) return(0.0); else return(v->sum/n);
}

double  SSTD(int i)
{
  double n, a, s;
  struct statistics *v;
  if (stat_root == NULL) { error_exit(7,"system"); return(0.0); }
  if ((i < 1) || (i > STATN)) { error_exit(8,"SSTD"); return(0.0); }
  v = stat_root[i - 1];
  if (v == NULL) { error_exit(17,"SSTD"); return(0.0); }
  if (v->time_persistent) collect_statistics(v, v->last_upd_value);
  /**3/1/94**/
  n = v->last_upd_sample_size - v->ini_time;
  if (n < 0.0001) return(0.0);
  a = v->sum/n;  s = v->sum_of_squares/n - a*a;
  if (s >= 0.00001) return(sqrt(s)) ;  else return(0.0);
}
 
double  SMAX(int i)
{
  struct statistics *v;
  if (stat_root == NULL) { error_exit(7,"system"); return(0.0); }
  if ((i < 1) || (i > STATN)) { error_exit(8,"SMAX"); return(0.0); }
  v = stat_root[i - 1];
  if (v == NULL) { error_exit(17,"SMAX"); return(0.0); }
  return(v->max);
}
 
double  SMIN(int i)
{
  struct statistics *v;
  if (stat_root == NULL) { error_exit(7,"system"); return(0.0); }
  if ((i < 1) || (i > STATN)) { error_exit(8,"SMIN"); return(0.0); }
  v = stat_root[i - 1];
  if (v == NULL) { error_exit(17,"SMIN"); return(0.0); }
  return(v->min);
}

double  SNUM(int i)
{
  struct statistics *v;
  if (stat_root == NULL) { error_exit(7,"system"); return(0.0); }
  if ((i < 1) || (i > STATN)) { error_exit(8,"SNUM"); return(0.0); }
  v = stat_root[i - 1];
  if (v == NULL) { error_exit(17,"SNUM"); return(0.0); }
  return(v->last_upd_sample_size - v->ini_time);
}
 
void  CLEARQ(int i)
{
  int j;
  if (NQUEUE == 0) return;
  if (i == 0) for (j=0; j<NQUEUE; j++)
  {
    clear_statistics(q[j]->q_stat);
    collect_statistics(q[j]->q_stat, (double)(q[j]->number));
  }
  else
  {
    if ((i < 0) || (i > NQUEUE)) { error_exit(4,"CLEARQ"); return; }
    clear_statistics(q[i - 1]->q_stat);
    collect_statistics(q[i-1]->q_stat, (double)(q[i-1]->number));
  }
}

void  RESTOR(void)
{
  int j, i;
  CLEARS(0);
  CLEARQ(0);
  DISPOS();
  for (j=0; j<NQUEUE; j++)
    while(NQ(j)) { REMVFQ(j,1); DISPOS(); }
  while (NC()) { REMVFC(1); DISPOS(); }
  tnow = 0.0;
  for (i=0; i<NATTR; i++) prev_entity[i] = 0.0;
}
 
double  QAVG(int i)
{
  double n;
  struct statistics *v;
  if ((i > NQUEUE) || (i < 1)) { error_exit(4,"QAVG"); return(0.0); }
  v = q[i - 1]->q_stat;
  collect_statistics(v, (double)(q[i-1]->number));
  n = v->last_upd_sample_size - v->ini_time;
  if (n < 0.0001) return(0.0); else return(v->sum/n);
}

double  QSTD(int i)
{
  double n, a, s;
  struct statistics *v;
  if ((i > NQUEUE) || (i < 1)) { error_exit(4,"QSTD"); return(0.0); }
  v = q[i - 1]->q_stat;
  collect_statistics(v, (double)(q[i-1]->number));
  n = v->last_upd_sample_size - v->ini_time;
  if (n < 0.0001) return(0.0);
  a = v->sum/n;  s = v->sum_of_squares/n - a*a;
  if (s >= 0.00001) return(sqrt(s)) ;  else return(0.0);
}
 
double  QMAX(int i)
{
  struct statistics *v;
  if ((i > NQUEUE) || (i < 1)) { error_exit(4,"QMAX"); return(0.0); }
  v = q[i - 1]->q_stat;
  return(v->max);
}

double  QMIN(int i)
{
  struct statistics *v;
  if ((i > NQUEUE) || (i < 1)) { error_exit(4,"QMIN"); return(0.0); }
  v = q[i - 1]->q_stat;
  return(v->min);
}
 
double  QNUM(int i)
{
  struct statistics *v;
  if ((i > NQUEUE) || (i < 1)) { error_exit(4,"QNUM"); return(0.0); }
  v = q[i - 1]->q_stat;
  collect_statistics(v, (double)(q[i-1]->number));
  return(v->last_upd_sample_size - v->ini_time);
}

void  SUMRY(char *fn)
{
  int i;
  FILE *F;
  char p, buf[64];
  if (stat_root == NULL) { error_exit(7,"system"); return; }
  i = 0;  p = fn[i];
  while ((i<63) && (p != ' ') && (p != 0)) {buf[i] = p; i++; p = fn[i]; }
  buf[i] = 0;
  if (buf[0] == 0) F = stdout;
  else if ((F = fopen(buf,"a")) == NULL)
    { error_exit(10,"SUMRY"); return; }
  for (i=0; i<NQUEUE; i++) report_statistics((q)[i]->q_stat,F);
  for (i=0; i<STATN; i++) if (stat_root[i] != NULL)
     report_statistics(stat_root[i],F);
  if (buf[0] != 0) fclose(F);
}
 
void  SETDEB(int err)
{ if ((err > -1) && (err < 16)) ERR_CODE = err;}

void  SETIDE(int id) { ID_CODE  = id;}

void  SETT(double tm) { if(tm > tnow)tnow = tm;}

void  SETA(int i, double v)
{
  if (present_entity == NULL) { error_exit( 6,"SETA"); return; }
  if ((NATTR < i) || (i < 1)) { error_exit(15,"SETA"); return; }
  prev_entity[(i) - 1] = v;
}
 
void SETAP(double *a)
{
  if (present_entity == NULL) { error_exit( 6,"SETAP"); return; }
  if (NATTR != -1)            { error_exit( 3,"SETAP"); return; }
  if (a) prev_entity = a;
}

int    IDE(void)  { return(ID_CODE); }
double T  (void)  { return(tnow);    }
int    NCEN(void) { return((present_entity == NULL)?0:1); }

double *AP(void)
{
  if (present_entity == NULL) { error_exit( 6,"AP"); return(&zero_entity); }
  if (NATTR != -1)            { error_exit( 3,"AP"); return(&zero_entity); }
  return(prev_entity);
}

double  A(int i)
{
  if (present_entity == NULL) { error_exit( 6,"A"); return(0.0); }
  if ((NATTR < i) || (i < 1)) { error_exit(15,"A"); return(0.0); }
  return((prev_entity)[(i) - 1]);
}

void  DISPOS(void)
{
  if ((present_entity != NULL) && (present_entity != &zero_entity))
    free(present_entity);
  present_entity = NULL;
}

struct queue_member *remfq(struct queue *q, double *t)
{
  struct queue_member *v;
  if (q == NULL) return(NULL);
  v = q->fst_member;
  *t = v->priority;
  q->fst_member = v->next_member;
  (q->number)--;
  collect_statistics(q->q_stat, (double)q->number);
  return(v);
}
 
struct queue_member *find_by_rank(int nq, int ir)
{
  int i;
  struct queue_member *m;
  if ((nq > NQUEUE) || (nq < 1)) return(NULL);
  if ((q)[nq-1]->number < ir) return(NULL);
  m = (q)[nq-1]->fst_member;
  i = 1;
  while ((m != NULL) && (i < ir))
    { i++; m = m->next_member; }
  return(m);
}

void  SETQDC(int n, char *disc)
{
  if ((n > NQUEUE) || (n < 1)) { error_exit(4,"SETQDC"); return; }
  strncpy((q)[(n) - 1]->discipline,disc,6);
  ((q)[(n) - 1]->discipline)[5] = 0;
}

int QDC(int i)
{
  if ((i > NQUEUE) || (i < 1)) { error_exit(4,"QDC"); return('F'); }
  return((int)((q[i - 1]->discipline)[0]));
}
 
int remfmid(struct queue *q, struct queue_member *v)
{
  struct queue_member *v1, *v2;
  if (q == NULL) return(1);
  v2 = NULL;
  v1 = q->fst_member;
  while ((v1 != NULL) && (v1 != v)) { v2 = v1; v1 = v1->next_member; }
  if (v1 == NULL) return(2);
  if (v1 == q->fst_member) q->fst_member   = v1->next_member;
  else                     v2->next_member = v1->next_member;
  (q->number)--;
  collect_statistics(q->q_stat, (double)q->number);
  return(0);
}
 
double  AIQ(int nq, int ir, int i)
{
  struct queue_member *m;
   if ((NATTR<i) || (i<1)) { error_exit(15,"AIQ"); return(0.0); }
  m = find_by_rank(nq, ir);
  if (m == NULL) { error_exit(5,"AIQ"); return(0.0); }
  return(((double *)(m->entity))[(i) - 1]);
}

double *APIQ(int nq, int ir)
{
  struct queue_member *m;
   if (NATTR != -1) { error_exit(3,"APIQ"); return(&zero_entity); }
  m = find_by_rank(nq, ir);
  if (m == NULL) { error_exit(5,"APIQ"); return(&zero_entity); }
  return((double *)(m->entity));
}

double  PRIQ(int nq, int ir)
{
  struct queue_member *m;
  if ((nq < 1) || (nq > NQUEUE)) { error_exit(4,"PRIQ"); return(0.0); }
  m = find_by_rank(nq, ir);
  if (m == NULL) { error_exit(5,"PRIQ"); return(0.0); }
  return(m->priority);
}

int    IDIQ(int nq, int ir)
{
  struct queue_member *m;
  if ((nq < 1) || (nq > NQUEUE)) { error_exit(4,"IDIQ"); return(0); }
  m = find_by_rank(nq, ir);
  if (m == NULL) { error_exit(5,"IDIQ"); return(0); }
  return(m->id);
}
 
double AIC(int ir, int i)
{
  struct queue_member *m;
  int j;
  if (calendar == NULL) { error_exit(7,"system"); return(0.0); }
  if ((i > NATTR) || (i < 1)) { error_exit(15,"AIC"); return(0.0); }
  m = calendar->fst_member;
  j = 1;
  while ((m != NULL) && (j < ir))
    { j++; m = m->next_member; }
  if (m == NULL) { error_exit(5,"AIC"); return(0.0); }
  if (j != ir) { error_exit(12,"AIC"); return(0.0); }
  if (m->entity == NULL) return(0.0);
  else                   return(((double *)(m->entity))[(i) - 1]);
}

double *APIC(int ir)
{
  struct queue_member *m;
  int j;
  if (calendar == NULL) { error_exit(7,"system"); return(&zero_entity); }
  if (NATTR != -1) { error_exit(3,"APIC"); return(&zero_entity); }
  m = calendar->fst_member;
  j = 1;
  while ((m != NULL) && (j < ir))
    { j++; m = m->next_member; }
  if (m == NULL) { error_exit(5,"APIC"); return(&zero_entity); }
  if (j != ir) { error_exit(12,"APIC"); return(&zero_entity); }
  if (m->entity == NULL) return(&zero_entity);
  else                   return((double *)(m->entity));
}

int IDIC(int ir)
{
  int i; struct queue_member *m;
  if (calendar == NULL) { error_exit(7,"system"); return(0); }
  i = 1; m = calendar->fst_member;
  while ((m != NULL) && (i < ir))
    { i++; m = m->next_member; }
  if (i == ir) return(m->id); else { error_exit(12,"IDIC"); return(0); }
}
 
double TIC(int ir)
{
  int i; struct queue_member *m;
  if (calendar == NULL) { error_exit(7,"system"); return(0.0); }
  i = 1; m = calendar->fst_member;
  while ((m != NULL) && (i < ir))
    { i++; m = m->next_member; }
  if (i == ir) return(m->priority);
  else { error_exit(12,"TIC"); return(0.0); }
}

int NEIC(int ir)
{
  int i; struct queue_member *m;
  if (calendar == NULL) { error_exit(7,"system"); return(0); }
  i = 1; m = calendar->fst_member;
  while ((m != NULL) && (i < ir))
    { i++; m = m->next_member; }
  if (i == ir) return(m->code);
  else { error_exit(12,"NEIC"); return(0); }
}
 
int  NQ(int nq)
{
  if ((nq > NQUEUE) || (nq < 1)) { error_exit(4,"NQ"); return(0); }
  return((q)[(nq)-1]->number);
}

int  NC(void)
{
  if (calendar == NULL) { error_exit(7,"system"); return(0); }
  return(calendar->number);
}
 
void list_calendar(void)
{
  int n; struct queue_member *p;
  char qu[2];
  if (calendar == NULL) { printf("Calendar is not defined.\n"); return; }
  printf("Time: %6.2f. In the calendar %d entities.\n", tnow,
    calendar->number);
  p = calendar->fst_member;
  n = 1;
  while(p != NULL)
  {
    printf("#%4d. time:%6.2f  code:%3d  id:%3d\n",n, p->priority, p->code,
                p->id);
    n++; p = p->next_member;
  }
  printf("Press Q to abort, <RETURN> to continue\n");
  if (fgets(qu, 2, stdin) == NULL) exit(1);
  if ((qu[0] == 'Q') || (qu[0] == 'q')) exit(1);
}

void REMVFC(int ir)
{
  double PRIORITY;
  struct queue_member *m; int i, j, NEXT_CODE;
  if (calendar == NULL) { error_exit(7,"system"); return; }
  if (present_entity != NULL) { error_exit(1,"REMVFC"); return; }
  m = calendar->fst_member; j = 1;
  while ((m != NULL) && (j < ir))
    { j++; m = m->next_member; }
  if (m == NULL) { error_exit(5,"REMVFC"); return; }
  if ((ERR_CODE & 8) == 8)
  {
    printf("Just before removing %d. entity from calendar -\n",ir);
    list_calendar();
  }
  i = 0;
  if (ir == 1) m = remfq(calendar, &PRIORITY);
  else i = remfmid(calendar, m);
  if (i != 0) { error_exit(5,"REMVFC"); return; }
  NEXT_CODE = m->code;
  ID_CODE =  m->id;
  if (NEXT_CODE > 1)
  {
    present_entity = (double *)(m->entity);
    for (i=0; i<NATTR; i++) prev_entity[i] = present_entity[i];
    if (NATTR == -1) prev_entity = present_entity;
  }
  else
  {
    if (NATTR > 0)
    {
      present_entity = (double *)calloc(NATTR, sizeof(double));
      if (present_entity == NULL) { error_exit(2,"REMVFC"); return; }
      for (i=0; i<NATTR; i++) present_entity[i] = prev_entity[i];
    }
    else if ((NATTR == -1) && (prev_entity)) present_entity = prev_entity;
    else present_entity = &zero_entity;
  }
  free(m);
}
 
void list_queue(struct queue *q)
{
  struct queue_member *p;
  int i, n;
  char qu[2];
  if (q == NULL) printf("Queue is not defined\n");
  else
  {
    printf("Time: %6.2f. In the queue %d entities. Discipline: %s\n",
             tnow, q->number, q->discipline);
    p = q->fst_member;
    n = 1;
    while (p != NULL)
    {
      printf("#%4d. id:%3d  ",n,p->id);
      if ((*(q->discipline) == 'S') || (*(q->discipline) == 'B'))
        printf("disc.:%6.2f",p->priority);
      for (i=0; (i<3)&&(i<NATTR); i++)
        printf("  A(%d)=%6.2f", i+1,((double *)(p->entity))[i]);
      printf("\n");
      n++; p = p->next_member;
    }
  }
  printf("Press Q to abort, <RETURN> to continue\n");
  if (fgets(qu, 2, stdin) == NULL) exit(1);
  if ((qu[0] == 'Q') || (qu[0] == 'q')) exit(1);
}

void  REMVFQ(int nq, int ir)
{
  double PRIORITY;
  struct queue_member *m; int i;
  if (present_entity != NULL) { error_exit(1,"REMVFQ"); return; }
  m = find_by_rank(nq, ir);
  if (m == NULL) { error_exit(5,"REMVFQ"); return; }
  if ((ERR_CODE & 8) == 8)
  {
    printf("Just before removing %d. entity from queue #%d -\n",ir,nq);
    list_queue(q[(nq)-1]);
  }
  i = 0;
  if (ir == 1) m = remfq(q[(nq)-1], &PRIORITY);
  else i = remfmid(q[(nq)-1], m);
  if ((m == NULL) || (i != 0)) { error_exit(5,"REMVFQ"); return; }
  ID_CODE = m->id;
  present_entity = (double *)(m->entity);
  for (i=0; i<NATTR; i++) prev_entity[i] = present_entity[i];
  if (NATTR == -1) prev_entity = present_entity;
  free(m);
}

void place_v_after_v1(struct queue_member *v, struct queue_member *v1,
        struct queue *q)
{
  if (v1 == NULL)
  {
     v->next_member = q->fst_member;
     q->fst_member = v;
  }
  else if (v1->next_member == NULL) v1->next_member = v;
  else
  {
     v->next_member = v1->next_member;
     v1->next_member = v;
  }
}
 
int enter_to_queue(struct queue *q, void *new_member, double value,
        int code, int id)
{
  struct queue_member *v, *v1, *oldv1;
  if (q == NULL) return(1);
  v = (struct queue_member*)malloc(sizeof(struct queue_member));
  if (v == NULL) return(2);
  v->entity = new_member;
  v->priority = value;
  v->id = id;
  v->code = code;
  v->next_member = NULL;
  if (q->fst_member == NULL) q->fst_member = v; else
  {
    v1 = q->fst_member;
    switch (*(q->discipline))
    {
      case 'L': q->fst_member = v;
               v->next_member = v1;
               break;
      case 'B': if (v1->priority < value)
               {
                 place_v_after_v1(v, NULL, q);
               }
               else
               {
                 oldv1 = v1;
                 while ((v1->priority >= value) && (v1->next_member != NULL))
                 {
                   oldv1 = v1;
                   v1 = v1->next_member;
                 }
                 if (v1->next_member == NULL)
                 {
                   if (v1->priority < value) v1 = oldv1;
                 } else v1 = oldv1;
                 place_v_after_v1(v, v1, q);
               }
               break;
      case 'S': if (v1->priority >= value) place_v_after_v1(v, NULL, q);
               else
               {
                 oldv1 = v1;
                 while ((v1->priority < value) && (v1->next_member != NULL))
                 {
                   oldv1 = v1;
                   v1 = v1->next_member;
                 }
                 if (v1->next_member == NULL)
                 {
                   if (v1->priority >= value) v1 = oldv1;
                 } else v1 = oldv1;
                 place_v_after_v1(v, v1, q);
               }
               break;
      default: while(v1->next_member != NULL) v1 = v1->next_member;
               v1->next_member = v;
               break;
    }
  }
  (q->number)++;
  collect_statistics(q->q_stat, (double)q->number);
  return(0);
}
 
void  QUEUE(int nq, double PRIORITY)
{
  int i;
  if (present_entity == NULL) { error_exit(6,"QUEUE"); return; }
   if ((nq < 1) || (nq > NQUEUE)) { error_exit(4,"QUEUE"); return; }
  for (i=0; i<NATTR; i++) present_entity[i] = prev_entity[i];
  if (NATTR == -1) present_entity = prev_entity;
  if(enter_to_queue((q)[(nq) - 1], present_entity, PRIORITY,
                    0, ID_CODE))
    { error_exit(2,"QUEUE"); return; }
  present_entity = NULL;
}
 
void  SCHED(double time, int ne, int id)
{
  int i;
  if (calendar == NULL) { error_exit(7,"system"); return; }
  if (present_entity == NULL) { error_exit(6,"SCHED"); return; }
  if (ne <= 1) { error_exit(16,"SCHED"); return; }
  for (i=0; i<NATTR; i++) present_entity[i] = prev_entity[i];
  if (NATTR == -1) present_entity = prev_entity;
  if (time < 0.0) time = 0.0;
  if(enter_to_queue(calendar, present_entity, tnow+time, ne, id))
    { error_exit(2,"SCHED"); return; }
  present_entity = NULL;
}
 
void CREATE(double time, int id)
{
  if (calendar == NULL) { error_exit(7,"system"); return; }
  if (time < 0.0) time = 0.0;
  if(enter_to_queue(calendar,present_entity,tnow+time,1,id))
    { error_exit(2,"CREATE"); return; }
}

void  SIMEND(double time)
{
  if (calendar == NULL) { error_exit(7,"system"); return; }
  if (time < tnow) time = tnow;
  if(enter_to_queue(calendar,present_entity,time,0,0))
    { error_exit(2,"SIMEND"); return; }
}
 
struct queue *initialize_queue(char *dis, char *stat_n)
{
  struct queue *q;
  q = (struct queue *)malloc(sizeof(struct queue));
  if (q != NULL)
  {
    strncpy(q->discipline,dis,6);
    (q->discipline)[5] = 0;
    q->number   = 0;
    q->q_stat   = (*stat_n != 0)?initiate_statistics(stat_n,1,0,0.0,1.0):NULL;
    q->fst_member = NULL;
    collect_statistics(q->q_stat, (double)q->number);
  }
  return(q);
}
 
struct queue **set_up_queues(int n)
{
  struct queue *q;
  struct queue **qu;
  int i, j, k;/*0123456789*1*/
  char head[16];
  strcpy(head, "Queue No   1");
  if (n > 0)
  {
    qu = (struct queue **)calloc(n, sizeof(q));
    if (qu == NULL) return(qu);
  }
  calendar = initialize_queue("SVF","");
  if (calendar == NULL) return(NULL);
  if (n < 1) return(&calendar);
  for (i=0; i<n; i++)
  {
    j = i+1; k = 11;
    do { head[k--] = (char)(48 + j%10); j = j / 10; } while (j > 0);
    q = initialize_queue("FIFO",head);
    if (q == NULL) return(NULL); else qu[i] = q;
  }
  return(qu);
}

void  INIQUE(int max_q, int max_attr, int max_stat)
{
  int i;
  if (calendar) { error_exit(11,"INIQUE"); return; }
  if ((max_q < 0) || (max_attr < -1) || (max_stat < 0))
    { error_exit(13,"INIQUE"); return; }
  NQUEUE = max_q;
  q = set_up_queues(NQUEUE);
  if (q == NULL) { error_exit(2,"INIQUE"); return; }
  NATTR = max_attr;
  if (NATTR > 0)
  {
    prev_entity = (double *)calloc(NATTR, sizeof(double));
    if (prev_entity == NULL) { error_exit(2,"INIQUE"); return; }
    for (i=0; i<NATTR; i++) prev_entity[i] = 0.0;
  }
  STATN = max_stat; if (STATN < 1) return;
  stat_root = (struct statistics **)calloc(
    STATN, sizeof(struct statistics *));
  if (stat_root == NULL) { error_exit(2,"INIQUE"); return; }
  for (i = 0; i < STATN; i++) stat_root[i] = NULL;
}

void SHOWQ(int m)
{
  if (m == 0) { list_calendar(); return; }
  if ((m < 0) || (m > NQUEUE)) { error_exit(4,"SHOWQ"); return; }
  list_queue(q[m - 1]);
}
 
int  NEXTEV(void)
{
  struct queue_member *p;
  double t;
  int i, NEXT_CODE;
  if (ERR_RET) return(0);
  if (calendar == NULL) { error_exit(7,"system"); return(0); }
  if (present_entity != NULL) { error_exit(1,"NEXTEV"); return(0); }
  if ((ERR_CODE & 4) == 4)
    { printf("Just before NEXTEV: "); list_calendar(); }
  if (calendar->number == 0) { return(0); }
  p = remfq(calendar, &t);
  if (p == NULL) { error_exit(5,"NEXTEV"); return(0); }
  if (t > tnow) tnow = t;
  NEXT_CODE = p->code;
  ID_CODE =  p->id;
  if (NEXT_CODE > 1)
  {
    present_entity = (double *)(p->entity);
    for (i=0; i<NATTR; i++) prev_entity[i] = present_entity[i];
    if (NATTR == -1) prev_entity = present_entity;
  }
  else  if (NEXT_CODE == 1)
  {
    if (NATTR > 0)
    {
      present_entity = (double *)calloc(NATTR, sizeof(double));
      if (present_entity == NULL) { error_exit(2,"NEXTEV"); return(0); }
      for (i=0; i<NATTR; i++) present_entity[i] = prev_entity[i];
    }
    else if ((NATTR == -1) && (prev_entity)) present_entity = prev_entity;
    else present_entity = &zero_entity;
  }
  free(p);
  return(NEXT_CODE);
}
