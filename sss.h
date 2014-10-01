/* file sss.h */
 
/* SSS - Simulation Subroutine Set. Version 1.00
   Copyright (C) M. A. Pollatschek 1990.
   All rights reserved.
*/
 
double A(int i); /* attribute #i of current entity   */
double AIC(int r,int i); /* attribute #i of entity #r
                    in calendar                      */
double AIQ(int m,int r,int i); /* attribute #i of
                    entity #r in queue #m            */
double *AP(void); /* current attribute pointer       */
double *APIC(int r); /* attribute pointer of entity #r
                    in calendar                      */
double *APIQ(int m,int r); /* attribute pointer of
                    entity #r in queue #m            */
double BE(double U,double W); /* beta random variate */
int    BI(int N,double P); /* binomial random variate*/
double CD(double *X,double *P,int N); /* user defined
                    continuous random variate        */
void   CLEARQ(int m); /* clear statistic of queue #m */
void   CLEARS(int j); /* clear statistic #j          */
void   CREATE(double t,int id); /* put a new entity to
                    calendar in t time from present  */
void   DISPOS(void); /* dispose current entity       */
double DP(double *X,int N); /* user defined discrete
                    random variate                   */
double ER(double M,int K); /* Erlang random varaite  */
double EX(double M); /* exponential random variate   */
double GA(double M,double K); /* gamma random variate*/
int    IDE(void); /* identity of current entity      */
int    IDIC(int r); /* identity of entity #r in
                    calendar                         */
int    IDIQ(int m,int r); /* identity of entity #r in
                    queue #m                         */
void   INIQUE(int q,int a,int s); /* initiate q queues,
                    calendar and s statistics        */
void   INISTA(int j,char *h,int n,int N,double f,
                    double w); /* initiate statistic #j
                    with name h                      */
int    NC(void); /* number of entities in calendar   */
int    NCEN(void); /* number of current entities     */
int    NEIC(int r); /* event code of entity #r in
                    calendar                         */
int    NEXTEV(void); /* next event code              */
int    NP(double M); /* Poisson random variate       */
int    NQ(int m); /* number of entities in queue #m  */
double PRIQ(int m,int r); /* priority of entity #r in
                    queue #m                         */
double QAVG(int m); /* time-average of entities
                    simultaneously present in queue
                    #m                               */
int    QDC(int m); /* discipline of queue #m         */
double QMAX(int m); /* maximum of entities
                    simultaneously present in queue
                    #m                               */
double QMIN(int m); /* minimum of entities
                    simultaneously present in queue
                    #m                               */
double QNUM(int m); /* number of entities that has been
                    in queue #m                      */
double QSTD(int m); /* standard deviation of entities
                    simultaneously present in queue
                    #m                               */
void   QUEUE(int m,double p); /* put entity in queue #m
                    with priority p                  */
double RA(void); /* a random number between 0 and 1  */
void   REMVFC(int r); /* remove entity #r from
                    calendar                         */
void   REMVFQ(int m,int r); /* remove entity #r from
                    queue #m                         */
void   RESTOR(void); /* empties calendar and queues,
                    clears statistics and time       */
double RL(double M,double S); /* lognormal random
                    variate                          */
double RN(double M,double S); /* normal random
                    variate                          */
double SAVG(int j); /* average of variable #j        */
void   SCHED(double t,int e,int id); /* schedule event
                    e for current entity at time t from
                    now                              */
void   SETA(int i,double v); /* set attribute #i of
                    current entity to v              */
void   SETANT(int n); /* set antithetic stream       */
void   SETAP(double *a); /* sets attribute pointer   */
void   SETDEB(int n); /* set debugging/error report  */
void   SETIDE(int id); /* set identity of current
                    entity to id                     */
void   SETQDC(int m,char *c); /* set discipline for
                    queue #m to c                    */
void   SETSEE(int x); /* set random seed to x        */
void   SETT(double t); /* set current simulated time */
void   SIMEND(double t); /* finish simulation at t   */
int    SIMERR(void); /* returns last error & resets  */
void   SHOWQ(int m); /* display queue or calendar    */
double SMAX(int j); /* maximum of variable #j        */
double SMIN(int j); /* minimum of variable #j        */
double SNUM(int j); /* number of variables #j        */
double SSTD(int j); /* standard deviation of variable
                    #j                               */
void   SUMRY(char *u); /* append standard report to
                    file with name u                 */
double T(void); /* simulated time at present         */
void   TALLY(int j,double v); /* enter v as an
                    observation for variable #j      */
double TIC(int r); /* time scheduled for entity #m   */
double TR(double I,double B,double C); /* triangular
                    random variate                   */
double UN(double I,double C); /* uniform random
                    variate                          */
double WE(double M,double K); /* Weibull random
                    variate                          */
