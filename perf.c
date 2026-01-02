//SCU REVISION 8.0098 vr  2 jan 2026 13:41:25 CET
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

typedef struct timespec counter_t;

#define GET_COUNTER(P) clock_gettime(CLOCK_THREAD_CPUTIME_ID, P)

#define TICKS(TV) (TV.tv_sec * 1000000000 + TV.tv_nsec)

void update_mean_sigma(long long n, long long x,
  long long *xmax, double *mn, double *sn)
{
  double mnm1 = *mn;
  double snm1 = *sn;

  if (xmax != NULL)
  {
    if (x > *xmax) *xmax = x;
  }

  *mn = mnm1 + (x - mnm1) / n;
  
  *sn = snm1 + (x - mnm1) * (x - *mn);
}

#define NCALL 1000000LL

int main(int argc, char **argv)
{
  counter_t counter_dummy;
  counter_t *counter_pointer;

  counter_pointer = &counter_dummy;
  
  for (int j = 0; j < 10; j++)
  {
    double mn = 0.0;
    double sn = 0.0;
  
    for (long long n = 1; n <= NCALL; ++n)
    {
      counter_t counter_stamp;
    
      GET_COUNTER(&counter_stamp);
      GET_COUNTER(counter_pointer);
    
      update_mean_sigma(n, TICKS(counter_dummy) - TICKS(counter_stamp),
        NULL, &mn, &sn);
    }

    //note that the distribution is very skewed, so the distribution
    //is not normal:
    //deviations LESS than the mean (or the mean - sigma) hardly ever occur
    //but deviations LARGER than (mean + sigma) do occur more often
 
    //arbitrarily set the standard deviation to one-third of the mean
    //to check for large positive deviations instead of
    //round(sqrt(sigma / NCALL));

    long long sigma = round(mn / 3.0);
    long long mean = round(mn);

    long long nlarge = 0;
    long long largest = 0;

    for (long long n = 1; n <= NCALL; ++n)
    {
      counter_t counter_stamp;
    
      GET_COUNTER(&counter_stamp);
      GET_COUNTER(counter_pointer);
    
      long long delta = TICKS(counter_dummy) - TICKS(counter_stamp);

      if (delta > largest) largest = delta;
      if (delta > (mean + 3 * sigma)) nlarge++;
    }

    printf("NCALL=%lld mean=%lld nlarge=%lld largest=%lld\n",
      NCALL, mean, nlarge, largest);
  }
}
